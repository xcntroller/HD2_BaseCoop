#include <winsock2.h>

#include "game/campaign_setup_session.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>

#include "game/campaign.h"
#include "game/campaign_content.h"
#include "game/controls.h"
#include "game/game_internal.h"
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
    const bool valid = BuildCampaignNetConfig(map->GetName(), host, "campaign-setup/original-v1",
                                              "original-campaign mission=1 difficulty=2 setup", config);
    if (!valid)
        LogSetup("campaign content fingerprint failed");
    return valid;
}

} // namespace

bool CampaignSetupMenuEnabled()
{
    const char *value = std::getenv("HD2_CAMPAIGN_COOP_MENU");
    return value && std::strcmp(value, "1") == 0;
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

C_CampaignSetupSession::~C_CampaignSetupSession()
{
    Close();
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
    C_HumanStore roster;
    if (!roster.Load(C_str("tables\\HumanStore.def")) || roster.GetCount() == 0 ||
        static_cast<std::size_t>(roster.GetCount()) > C_CampaignSession::kMaxRoster) {
        LogSetup("original campaign roster unavailable", roster.GetCount());
        return R::ContentMismatch;
    }
    std::vector<int> profileIndexes(roster.GetCount());
    std::iota(profileIndexes.begin(), profileIndexes.end(), 0);
    const S_CampaignSessionSetup setup{
        config.m_Session, CampaignId(config.m_Session.Value()), std::move(mission),
        {config.m_Hello.m_Content, kRegularDifficulty}, true
    };
    m_Journal = std::make_unique<C_CampaignRequestJournal>(config.m_Session);
    m_Ownership = std::make_unique<C_CampaignOwnership>(C_CampaignSession(setup, profileIndexes), m_Policy, *m_Journal);
    m_State = State::Hosting;
    config.m_Admitted = [this](const auto &agreement, int) {
        const auto player = agreement.m_Player;
        const auto reason = m_Ownership->AdmitTransportPlayer(player, "Guest " + std::to_string(player.Value()));
        if (reason == R::None)
            LogSetup("host admitted", agreement.m_Session.Value(), player.Value());
        return reason;
    };
    config.m_Departed = [this](PlayerId player, R reason) {
        if (m_State == State::Hosting) {
            m_Ownership->OnDisconnected(player);
            LogSetup("host observed guest departure", player.Value(), static_cast<std::uint64_t>(reason));
        }
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

    config.m_Admitted = [this](const auto &agreement, int) {
        m_Agreement = agreement;
        m_State = State::Guest;
        LogSetup("guest admitted", agreement.m_Session.Value(), agreement.m_Player.Value());
        return R::None;
    };
    config.m_Departed = [this](PlayerId player, R reason) {
        if ((m_State == State::Joining || m_State == State::Guest) && m_JoinFailure == R::None) {
            m_JoinFailure = reason;
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

void C_CampaignSetupSession::Update()
{
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
    if (m_Ownership) {
        m_Ownership->Close();
        m_Ownership.reset();
        m_Journal.reset();
    }
}
