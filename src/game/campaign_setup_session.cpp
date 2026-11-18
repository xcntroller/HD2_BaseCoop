#include <winsock2.h>

#include "game/campaign_setup_session.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game/campaign.h"
#include "game/campaign_content.h"
#include "game/campaign_dev_fixture.h"
#include "game/campaign_mission_adapter.h"
#include "game/controls.h"
#include "game/game_internal.h"
#include "game/items_module.h"
#include "game/pending.h"
#include "game/profile.h"
#include "net/dplay8.h"
#include "net/net_game.h"

namespace {

using R = E_CampaignReason;
using State = E_CampaignSetupState;
constexpr int kOriginalCampaign = 0;
constexpr int kFirstOperationMission = 1;
constexpr std::uint32_t kRegularDifficulty = 2;

class C_ScopedPreparationRoster {
public:
    explicit C_ScopedPreparationRoster(C_HumanStore &roster) : m_Previous(g_gameProfile.m_humanStore)
    {
        g_gameProfile.m_humanStore = &roster;
    }
    ~C_ScopedPreparationRoster() { g_gameProfile.m_humanStore = m_Previous; }
private:
    C_HumanStore *m_Previous;
};

void LogSetup(const char *event, std::uint64_t first = 0, std::uint64_t second = 0)
{
    if (auto *log = std::fopen("campaign-setup.log", "a")) {
        std::fprintf(log, "%s %llu %llu\n", event, static_cast<unsigned long long>(first),
                     static_cast<unsigned long long>(second));
        std::fclose(log);
    }
}

bool MakeSetupConfig(bool host, S_CampaignNetConfig &config, MissionKey &mission)
{
    const auto *map = g_campaign->GetMissionDefinition(kOriginalCampaign, kFirstOperationMission);
    if (map)
        mission = MissionKey(map->GetName());
    if (!map || map->IsTutorial() || !mission.IsValid()) {
        LogSetup("original campaign mission unavailable");
        return false;
    }
    config.m_Route = E_CampaignLanRoute::CampaignSession;
    const bool valid = BuildCampaignNetConfig(map->GetName(), host, "campaign-setup/original-v2",
                                              "original-campaign mission=1 difficulty=2 preparation", config);
    constexpr auto features = CampaignProtocol::kLivePreparation | CampaignProtocol::kMissionUpdates |
        CampaignProtocol::kWeaponEvents | CampaignProtocol::kInventoryMoves |
        CampaignProtocol::kCampaignProgression;
    config.m_Hello.m_Required |= features;
    config.m_Hello.m_Offered |= features;
    if (!valid)
        LogSetup("campaign content fingerprint failed");
    return valid;
}

} // namespace

bool CampaignSetupMenuEnabled()
{
#ifdef HD2_DEDICATED
    return false;
#else
    const char *value = std::getenv("HD2_CAMPAIGN_COOP_MENU");
    return !value || std::strcmp(value, "0") != 0;
#endif
}

std::optional<std::uint32_t> ParseCampaignLanAddress(const char *text)
{
    std::uint32_t address = 0;
    for (unsigned octet = 0; octet < 4; ++octet) {
        unsigned value = 0;
        unsigned digits = 0;
        while (*text >= '0' && *text <= '9' && digits < 3) {
            value = value * 10 + (*text++ - '0');
            ++digits;
        }
        if (!digits || value > 255 || *text != (octet == 3 ? '\0' : '.'))
            return std::nullopt;
        address |= value << (8 * octet);
        if (octet != 3)
            ++text;
    }
    if (address == 0 || address == 0xffffffff)
        return std::nullopt;
    return address;
}

C_CampaignSetupSession::C_CampaignSetupSession() = default;

C_CampaignSetupSession::~C_CampaignSetupSession()
{
    Close();
}

std::vector<int> GetInitialCampaignRoster()
{
    C_HumanStore roster;
    if (!roster.Load(C_str("tables\\HumanStore.def")) || roster.GetCount() == 0 ||
        static_cast<std::size_t>(roster.GetCount()) > C_CampaignSession::kMaxRoster)
        return {};
    std::vector<int> profiles;
    for (int i = 0; i < roster.GetCount() && profiles.size() < CampaignProtocol::kMaxParticipants; ++i)
        if (roster.Get(i))
            profiles.push_back(i);
    return profiles;
}

R C_CampaignSetupSession::Host()
{
    if (m_State != State::Idle || IsCampaignNet() || !IsLocalGame())
        return R::InvalidPhase;
    m_JoinFailure = R::None;
    MissionKey mission;
    S_CampaignNetConfig config;
    if (!MakeSetupConfig(true, config, mission))
        return R::ContentMismatch;

    // Use the original catalog for a fresh session without altering solo roster progress.
    const auto profileIndexes = GetInitialCampaignRoster();
    if (profileIndexes.size() != CampaignProtocol::kMaxParticipants)
        return R::ContentMismatch;
    const S_CampaignSessionSetup setup{
        config.m_Session, CampaignId(config.m_Session.Value()), std::move(mission),
        {config.m_Hello.m_Content, kRegularDifficulty}, true
    };
    m_Journal = std::make_unique<C_CampaignRequestJournal>(config.m_Session);
    m_Ownership = std::make_unique<C_CampaignOwnership>(C_CampaignSession(setup, profileIndexes), m_Policy, *m_Journal);
    m_State = State::Hosting;
    config.m_Admitted = [this](const auto &agreement, int slot) {
        const auto player = agreement.m_Player;
        const auto reason = m_Ownership->AdmitTransportPlayer(player, "Guest " + std::to_string(player.Value()));
        if (reason == R::None) {
            m_PeerSlots.emplace(player, slot);
            LogSetup("host admitted", agreement.m_Session.Value(), player.Value());
        }
        return reason;
    };
    config.m_Departed = [this](PlayerId player, R reason) {
        if (m_State == State::Hosting) {
            m_PeerSlots.erase(player);
            if (m_MissionDeparture)
                m_MissionDeparture(player, reason);
            else
                m_Ownership->OnDisconnected(player);
            LogSetup("host observed guest departure", player.Value(), static_cast<std::uint64_t>(reason));
        }
    };
    config.m_Received = [this](PlayerId player, auto message) {
        Receive(player, std::move(message));
    };

    S_NetSessionDesc desc{};
    std::strcpy(desc.name, "Co-op Campaign");
    std::memcpy(desc.serviceProvider, &CLSID_NetDP8SP_TCPIP, sizeof(CLSID_NetDP8SP_TCPIP));
    if (g_netGame.Host(desc, false, &config) < 0) {
        Close();
        return R::Disconnected;
    }
    LogSetup("host opened setup", config.m_Session.Value(), config.m_Host.Value());
    return R::None;
}

R C_CampaignSetupSession::Join(std::uint32_t address)
{
    if (m_State != State::Idle || IsCampaignNet() || !IsLocalGame())
        return R::InvalidPhase;
    m_JoinFailure = R::None;
    MissionKey mission;
    S_CampaignNetConfig config;
    if (!MakeSetupConfig(false, config, mission))
        return R::ContentMismatch;

    config.m_Admitted = [this](const auto &agreement, int slot) {
        m_Agreement = agreement;
        m_PeerSlots.emplace(agreement.m_Player, slot);
        m_Preparation = std::make_unique<C_CampaignPreparationLive>(agreement.m_Session, agreement.m_Player,
            [](PlayerId player, const auto &message) { return SendCampaignMessage(g_netGame, player, message); });
        m_State = State::Guest;
        LogSetup("guest admitted", agreement.m_Session.Value(), agreement.m_Player.Value());
        return R::None;
    };
    config.m_Received = [this](PlayerId player, auto message) {
        Receive(player, std::move(message));
    };
    config.m_Departed = [this](PlayerId player, R reason) {
        if ((m_State == State::Joining || m_State == State::Guest) && m_JoinFailure == R::None) {
            m_JoinFailure = reason;
            if (m_Preparation)
                m_Preparation->Disconnect(reason);
            if (m_MissionDeparture)
                m_MissionDeparture(player, reason);
            LogSetup("guest connection ended", player.Value(), static_cast<std::uint64_t>(reason));
        }
    };
    S_ServerEntry entry{};
    entry.Reset();
    entry.address = address;
    m_State = State::Joining;
    const auto result = g_netGame.Connect(&entry, &config);
    if (result < 0 && result != E_PENDING) {
        Close();
        return R::Disconnected;
    }
    LogSetup("guest joining setup", address);
    return R::None;
}

R C_CampaignSetupSession::BeginPreparation()
{
    if (m_State != State::Hosting || m_Preparation)
        return R::InvalidPhase;
    const auto *equipment = g_campaign->GetInitialEquipmentScreen(kOriginalCampaign, kFirstOperationMission);
    const auto *mission = g_campaign->GetMissionDefinition(kOriginalCampaign, kFirstOperationMission);
    if (!equipment || !mission)
        return R::SnapshotUnavailable;
    g_itemsModule.EnsureItemsLoaded();
    const auto &session = m_Ownership->GetSession();
    auto native = std::make_unique<S_CampaignNativeCatalog>();
    S_CampaignEquipmentRuleCatalog rules;
    S_CampaignPreparationSnapshot initial;
    C_HumanStore roster;
    if (!roster.Load(C_str("tables\\HumanStore.def")))
        return R::ContentMismatch;
    R reason;
    {
        C_ScopedPreparationRoster profile(roster);
        reason = ImportInitialCampaignPreparation(session, kFirstOperationMission, *equipment, *native, rules, initial);
    }
    if (reason != R::None) {
        LogSetup("campaign equipment import failed", static_cast<std::uint64_t>(reason));
        return reason;
    }

    for (const auto &soldier : initial.m_Soldiers) {
        reason = m_Ownership->SetAvailability(soldier.m_Soldier, true, true);
        if (reason != R::None)
            return reason;
    }
    reason = m_Ownership->StartPreparation();
    if (reason != R::None)
        return reason;
    const auto ownership = m_Ownership->GetSnapshot();
    initial.m_Context = ownership->m_Context;
    initial.m_Generation = session.GetPreparationGeneration();
    initial.m_PreparationRevision = PreparationRevision(1);
    initial.m_ViewRevision = ViewRevision(1);
    initial.m_OwnershipRevision = ownership->m_Revision;
    initial.m_Briefing = {TextKey("original-arctic1/operation"), TextKey("original-arctic1/mission"),
        TextKey("original-arctic1/briefing"), AssetKey(mission->GetImage().c_str())};
    const auto host = *ownership->m_Context.m_HostPlayer;
    auto preparation = std::make_unique<C_CampaignPreparationLive>(initial.m_Context.m_Session, host,
        [](PlayerId player, const auto &message) { return SendCampaignMessage(g_netGame, player, message); });
    reason = preparation->OpenHost(*m_Ownership, *m_Journal, std::move(initial), std::move(rules));
    if (reason != R::None) {
        LogSetup("campaign preparation authority unavailable", static_cast<std::uint64_t>(reason));
        Close();
        return reason;
    }
    m_Preparation = std::move(preparation);
    m_Catalog = std::move(native);
    LogSetup("host opened campaign preparation", ownership->m_Context.m_Session.Value(),
             session.GetPreparationGeneration().Value());
    return R::None;
}

R C_CampaignSetupSession::BeginContinuedPreparation(const C_CampaignScreen &equipment)
{
    if (m_State != State::Hosting || !m_Preparation)
        return R::InvalidPhase;
    const auto *mission = g_campaign->GetCurrentMission();
    if (!mission)
        return R::SnapshotUnavailable;
    const int index = g_campaign->GetCurrentIndex();
    const int number = g_campaign->GetMissionNumber(index);
    const auto previous = m_Ownership->GetSnapshot();
    auto reason = m_Ownership->ReturnToSetup(MissionKey(mission->GetName()));
    if (reason != R::None)
        return reason;

    const auto &session = m_Ownership->GetSession();
    const auto host = *previous->m_Context.m_HostPlayer;
    const bool single = index == 0 && (number == 2 || number == 15);
    const auto roster = session.GetRoster();
    std::vector<S_CampaignRosterEntry> candidates;
    for (const auto &entry : roster)
        if (const auto *profile = GetSoldierProfile(entry.m_ProfileIndex))
            if (!profile->dead && g_gameProfile.FindSoldierState(entry.m_ProfileIndex))
                candidates.push_back(entry);
    if (candidates.empty())
        return R::InvalidSoldier;
    if (single) {
        // The authored mission permits one deployed soldier. Keep the host's
        // surviving assignment when possible, otherwise use the first survivor.
        // The existing owner of that survivor retains their claim.
        const auto selected = std::find_if(candidates.begin(), candidates.end(), [&](const auto &entry) {
            return std::any_of(previous->m_Soldiers.begin(), previous->m_Soldiers.end(), [&](const auto &soldier) {
                return soldier.m_Soldier == entry.m_Soldier && soldier.m_Owner == host;
            });
        });
        const auto lead = selected == candidates.end() ? candidates.front() : *selected;
        candidates.assign(1, lead);
    }
    for (const auto &entry : roster) {
        const bool deploy = std::any_of(candidates.begin(), candidates.end(),
            [&](const auto &candidate) { return candidate.m_Soldier == entry.m_Soldier; });
        reason = m_Ownership->SetAvailability(entry.m_Soldier, deploy, deploy);
        if (reason != R::None)
            return reason;
    }
    reason = m_Ownership->StartPreparation();
    if (reason != R::None)
        return reason;

    const auto ownership = m_Ownership->GetSnapshot();
    S_CampaignPreparationSnapshot initial;
    initial.m_Context = ownership->m_Context;
    initial.m_Generation = session.GetPreparationGeneration();
    initial.m_PreparationRevision = PreparationRevision(1);
    initial.m_ViewRevision = ViewRevision(1);
    initial.m_OwnershipRevision = ownership->m_Revision;
    initial.m_Briefing = {TextKey("original-campaign/operation"), TextKey("original-campaign/mission"),
        TextKey("original-campaign/briefing"), AssetKey(mission->GetImage().c_str())};
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto &entry = candidates[i];
        auto &presentation = initial.m_Soldiers.emplace_back();
        presentation.m_Soldier = entry.m_Soldier;
        presentation.m_Profile = SoldierProfileKey("original-roster/" + std::to_string(entry.m_ProfileIndex));
        presentation.m_DisplayName = GetSoldierProfile(entry.m_ProfileIndex)->fullName.c_str();
        presentation.m_Preview = AssetKey("original-portrait/" + std::to_string(entry.m_ProfileIndex));
        const auto record = std::find_if(ownership->m_Soldiers.begin(), ownership->m_Soldiers.end(),
            [&](const auto &soldier) { return soldier.m_Soldier == entry.m_Soldier; });
        presentation.m_Ownership = *record;
        initial.m_Slots[i].m_Soldier = entry.m_Soldier;
    }
    auto catalog = std::make_unique<S_CampaignNativeCatalog>();
    S_CampaignEquipmentRuleCatalog rules;
    reason = ImportContinuedCampaignPreparation(session, number, equipment, *catalog, rules, initial);
    if (reason != R::None) {
        LogSetup("campaign carryover equipment import failed", static_cast<std::uint64_t>(reason), number);
        return reason;
    }
    reason = m_Preparation->OpenNextHost(std::move(initial), std::move(rules));
    if (reason != R::None)
        return reason;
    m_Catalog = std::move(catalog);
    m_ContinuedPreparation = true;
    LogSetup("host opened continued campaign preparation", number, session.GetPreparationGeneration().Value());
    return R::None;
}

void C_CampaignSetupSession::Receive(PlayerId peer, CampaignProtocol::T_Message message)
{
    // Requests belong to the preparation generation until the runtime takes
    // over. The guest still accepts a new preparation generation after abort.
    if (m_Preparation && (!m_MissionReceive || m_ContinuedPreparation || m_State == State::Guest) && m_Preparation->Receive(peer, message)) {
        if (!m_MissionQueue.empty()) {
            const auto state = m_Preparation->GetSnapshot().m_Snapshot;
            if (state && state->m_Context.m_Phase == E_CampaignPhase::Preparation)
                m_MissionQueue.clear();
        }
        return;
    }
    if (m_MissionReceive) {
        m_MissionReceive(peer, std::move(message));
        return;
    }
    if (m_State != State::Guest || !m_Preparation)
        return;
    const auto state = m_Preparation->GetSnapshot().m_Snapshot;
    if (!state || state->m_Context.m_Phase != E_CampaignPhase::Frozen)
        return;
    if ((m_MissionQueue.empty() && std::holds_alternative<S_CampaignMissionManifest>(message)) ||
        (!m_MissionQueue.empty() && std::holds_alternative<CampaignProtocol::S_WorldChunk>(message) &&
         m_MissionQueue.size() <= CampaignProtocol::kMaxChunks))
        m_MissionQueue.emplace_back(peer, std::move(message));
}

bool C_CampaignSetupSession::DeploymentPending() const
{
    return m_Preparation && (m_State == State::Hosting ? m_Preparation->IsFrozenPublished() : !m_MissionQueue.empty());
}

std::optional<int> C_CampaignSetupSession::GetPeerSlot(PlayerId player) const
{
    const auto slot = m_PeerSlots.find(player);
    return slot != m_PeerSlots.end() ? std::make_optional(slot->second) : std::nullopt;
}

void C_CampaignSetupSession::SetMissionCallbacks(std::function<void(PlayerId, CampaignProtocol::T_Message)> receive,
    std::function<void(PlayerId, R)> departure)
{
    m_MissionReceive = std::move(receive);
    m_MissionDeparture = std::move(departure);
    auto queued = std::move(m_MissionQueue);
    m_MissionQueue.clear();
    if (m_MissionReceive)
        for (auto &entry : queued)
            m_MissionReceive(entry.first, std::move(entry.second));
}

C_CampaignMissionAdapter &C_CampaignSetupSession::GetMissionAdapter()
{
    if (!m_MissionAdapter)
        m_MissionAdapter = std::make_unique<C_CampaignMissionAdapter>(*m_Ownership);
    return *m_MissionAdapter;
}

R C_CampaignSetupSession::ReopenPreparation(const S_CampaignMissionManifest *retry)
{
    if (m_State != State::Hosting || !m_Preparation)
        return R::InvalidPhase;
    const auto mission = m_Ownership->GetSession().GetContext().m_Mission;
    auto reason = m_Ownership->ReturnToSetup(mission);
    if (reason == R::None && retry)
        reason = m_Ownership->RestoreRetryRoster(retry->m_Soldiers);
    if (reason == R::None)
        reason = m_Ownership->StartPreparation();
    if (reason == R::None)
        reason = m_Preparation->ReopenHost();
    if (reason == R::None)
        LogSetup("host reopened campaign preparation",
            m_Ownership->GetSession().GetPreparationGeneration().Value(),
            m_Ownership->GetSession().GetContext().m_MissionGeneration.Value());
    return reason;
}

void C_CampaignSetupSession::Update()
{
    if (m_Preparation && m_JoinFailure == R::None && (!m_MissionReceive || m_ContinuedPreparation || m_State == State::Guest)) {
        m_JoinFailure = m_Preparation->Tick();
        if (m_JoinFailure != R::None && m_State == State::Hosting &&
            m_Ownership->GetSession().GetContext().m_Phase == E_CampaignPhase::Frozen)
            m_JoinFailure = ReopenPreparation();
    }
    if (m_JoinFailure != R::None && m_State != State::Idle) {
        const auto reason = m_JoinFailure;
        Close();
        m_JoinFailure = reason;
    }
}

void C_CampaignSetupSession::Close()
{
    const auto state = m_State;
    m_State = State::Idle;
    m_JoinFailure = R::None;
    if (state != State::Idle) {
        g_netGame.Close(true);
        LogSetup("local setup closed", static_cast<unsigned>(state));
    }
    m_Agreement.reset();
    m_ContinuedPreparation = false;
    SetMissionCallbacks({}, {});
    m_PeerSlots.clear();
    m_MissionAdapter.reset();
    m_Preparation.reset();
    m_Catalog.reset();
    if (m_Ownership) {
        m_Ownership->Close();
        m_Ownership.reset();
        m_Journal.reset();
    }
}
