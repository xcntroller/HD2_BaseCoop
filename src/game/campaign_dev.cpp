#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
#include <wincrypt.h>

#include "game/campaign_dev.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include "game/actors_module.h"
#include "game/campaign.h"
#include "game/campaign_control_native.h"
#include "game/campaign_control_runtime.h"
#include "game/campaign_dev_fixture.h"
#include "game/campaign_ownership.h"
#include "game/console.h"
#include "game/controls.h"
#include "game/game.h"
#include "game/game_app.h"
#include "game/items_module.h"
#include "game/pending.h"
#include "game/profile.h"
#include "game/soldier.h"
#include "ls3d/igraph.h"
#include "ls3d/rw_data.h"
#include "net/campaign_net.h"
#include "net/dplay8.h"
#include "net/net_game.h"

void LogDevelopmentCampaign(const char *event, std::uint64_t first, std::uint64_t second)
{
    if (auto *log = std::fopen("campaign-dev.log", "a")) {
        std::fprintf(log, "%s %llu %llu\n", event, static_cast<unsigned long long>(first),
                     static_cast<unsigned long long>(second));
        std::fclose(log);
    }
}

namespace {

using R = E_CampaignReason;
using Clock = std::chrono::steady_clock;
using T_Digest = std::array<BYTE, 32>;
constexpr int kOriginalCampaign = 0;
constexpr int kMissionNumber = 1; // first operation mission after Tutorial
constexpr std::uint32_t kDifficulty = 2;
constexpr PlayerId kHostPlayer{1};
const std::vector<int> kRoster{0, 1, 2, 3};
constexpr char kFixtureRules[] =
    "original-campaign mission=1 difficulty=2 roster=0,1,2,3 host=0 guest=1 fixed-assignment slots=0,1,2,3 issue=mission-essentials";


enum class E_Launch { Ordinary, Invalid, Host, Join };
struct S_Launch {
    E_Launch m_Mode = E_Launch::Ordinary;
    std::uint32_t m_Address = 0;
};

S_Launch ParseLaunch()
{
    int count = 0;
    wchar_t **args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!args)
        return {};
    S_Launch launch;
    for (int i = 1; i < count; ++i) {
        if (std::wcscmp(args[i], L"-campaign-coop") != 0)
            continue;
        launch.m_Mode = E_Launch::Invalid;
        if (i + 1 < count && std::wcscmp(args[i + 1], L"host") == 0) {
            launch.m_Mode = E_Launch::Host;
        } else if (i + 2 < count && std::wcscmp(args[i + 1], L"join") == 0) {
            unsigned a, b, c, d;
            wchar_t extra;
            if (std::swscanf(args[i + 2], L"%3u.%3u.%3u.%3u%lc", &a, &b, &c, &d, &extra) == 4 &&
                a <= 255 && b <= 255 && c <= 255 && d <= 255) {
                launch.m_Mode = E_Launch::Join;
                launch.m_Address = a | (b << 8) | (c << 16) | (d << 24);
            }
        }
        break;
    }
    LocalFree(args);
    return launch;
}

class C_Hash {
public:
    explicit C_Hash(HCRYPTPROV provider) { CryptCreateHash(provider, CALG_SHA_256, 0, 0, &m_Hash); }
    ~C_Hash() { if (m_Hash) CryptDestroyHash(m_Hash); }
    bool Write(const void *data, DWORD size)
    {
        return m_Hash && CryptHashData(m_Hash, static_cast<const BYTE *>(data), size, 0);
    }
    bool WriteNumber(std::uint64_t value, DWORD size)
    {
        BYTE bytes[8]{};
        for (DWORD i = 0; i < size; ++i)
            bytes[i] = static_cast<BYTE>(value >> (i * 8));
        return Write(bytes, size);
    }
    bool Finish(T_Digest &digest)
    {
        DWORD size = static_cast<DWORD>(digest.size());
        return m_Hash && CryptGetHashParam(m_Hash, HP_HASHVAL, digest.data(), &size, 0) && size == digest.size();
    }

private:
    HCRYPTHASH m_Hash = 0;
};

struct S_Content {
    std::string m_Path;
    std::string m_InlineData;
};

bool DigestContent(HCRYPTPROV provider, const S_Content &entry, std::uint64_t &size, T_Digest &digest)
{
    C_Hash hash(provider);
    if (!entry.m_InlineData.empty()) {
        size = entry.m_InlineData.size();
        return hash.Write(entry.m_InlineData.data(), static_cast<DWORD>(size)) && hash.Finish(digest);
    }

    std::string path = entry.m_Path;
    std::replace(path.begin(), path.end(), '/', '\\');
    const int file = dtaOpen(path.c_str(), false);
    if (file < 0) {
        LogDevelopmentCampaign(path.c_str());
        return false;
    }
    const int length = dtaSeek(file, 0, 2);
    bool valid = length >= 0 && dtaSeek(file, 0, 0) == 0;
    std::array<BYTE, 32768> buffer;
    int remaining = length;
    while (valid && remaining > 0) {
        const int count = (std::min)(remaining, static_cast<int>(buffer.size()));
        valid = dtaRead(file, buffer.data(), count) == count && hash.Write(buffer.data(), static_cast<DWORD>(count));
        remaining -= count;
    }
    dtaClose(file);
    if (!valid) {
        LogDevelopmentCampaign(path.c_str());
        return false;
    }
    size = static_cast<std::uint64_t>(length);
    return hash.Finish(digest);
}

std::string DigestCategory(HCRYPTPROV provider, unsigned category, std::vector<S_Content> entries)
{
    std::sort(entries.begin(), entries.end(), [](const S_Content &a, const S_Content &b) {
        return a.m_Path < b.m_Path;
    });
    constexpr char prefix[] = "HD2COOP-CONTENT-v1";
    C_Hash hash(provider);
    if (!hash.Write(prefix, sizeof(prefix)) || !hash.WriteNumber(category, 1) || !hash.WriteNumber(entries.size(), 4))
        return {};
    for (const auto &entry : entries) {
        std::uint64_t size = 0;
        T_Digest digest;
        if (!DigestContent(provider, entry, size, digest) ||
            !hash.WriteNumber(entry.m_Path.size(), 2) ||
            !hash.Write(entry.m_Path.data(), static_cast<DWORD>(entry.m_Path.size())) ||
            !hash.WriteNumber(size, 8) || !hash.Write(digest.data(), static_cast<DWORD>(digest.size())))
            return {};
    }
    T_Digest digest;
    if (!hash.Finish(digest))
        return {};
    constexpr char hex[] = "0123456789abcdef";
    std::string encoded = "sha256:";
    for (const auto byte : digest) {
        encoded += hex[byte >> 4];
        encoded += hex[byte & 15];
    }
    return encoded;
}

bool BuildDevContent(const C_MissionDef &mission, bool host, S_CampaignNetConfig &config)
{
    HCRYPTPROV provider = 0;
    if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return false;

    const std::string path = std::string("missions/") + mission.GetName() + "/";
    auto &content = config.m_Hello.m_Content;
    content.m_CampaignContent = DigestCategory(provider, 1, {{"GameData/Gamedata00.gdt", {}}});
    content.m_MissionContent = DigestCategory(provider, 2, {
        {path + "scene.4ds", {}}, {path + "scene2.bin", {}}, {path + "tree.klz", {}}, {path + "Scripts.dta", {}},
        {"Scripts.dta", {}}
    });
    content.m_SoldierContent = DigestCategory(provider, 3, {{"tables/HumanStore.def", {}}});
    content.m_ItemContent = DigestCategory(provider, 4, {
        {"Tables/items.sav", {}}, {"GameData/presets.def", {}}, {"GameData/miss_itm.def", {}}
    });
    content.m_Rules = DigestCategory(provider, 5, {{"campaign-dev/arctic-fixture-v1", kFixtureRules}});

    std::uint64_t randomSession = 0;
    const bool session = !host ||
        (CryptGenRandom(provider, sizeof(randomSession), reinterpret_cast<BYTE *>(&randomSession)) && randomSession != 0);
    CryptReleaseContext(provider, 0);
    if (host) {
        config.m_Session = SessionId(randomSession);
        config.m_Host = kHostPlayer;
    }
    return session && !content.m_CampaignContent.empty() && !content.m_MissionContent.empty() &&
           !content.m_SoldierContent.empty() && !content.m_ItemContent.empty() && !content.m_Rules.empty();
}

C_CampaignNode *SelectOriginalMission()
{
    if (g_campaign->GetCount() <= kOriginalCampaign)
        return nullptr;
    g_mpSettings->sessionType = SESSION_NONE;
    g_mpSettings->dedicated = false;
    g_mpSettings->password = "";
    g_mpSettings->unknown174 = 0;
    g_playMode->mode = PLAY_MODE_CAMPAIGN;
    g_playMode->singleMission = 0;
    g_campaign->SetCurrentIndex(kOriginalCampaign);
    if (!g_campaign->AdvanceToMission())
        return nullptr;
    while (auto *step = g_campaign->NextStep()) {
        if (step->GetType() == CAMPAIGN_NODE_PLAY &&
            g_campaign->GetMissionNumber(kOriginalCampaign) == kMissionNumber) {
            auto *mission = g_campaign->GetCurrentMission();
            if (mission && !mission->IsTutorial())
                return step;
        }
    }
    return nullptr;
}

class C_DevPolicy final : public I_CampaignPolicyView {
public:
    S_CampaignPolicySnapshot GetSnapshot() const override
    {
        return {{{E_CampaignPolicy::SoldierSelection, E_CampaignPolicyState::Permitted, {}}}};
    }
};

C_DevPolicy g_policy;
std::unique_ptr<C_CampaignRequestJournal> g_journal;
std::unique_ptr<C_CampaignOwnership> g_ownership;
std::unique_ptr<C_CampaignMissionAdapter> g_mission;
C_CampaignNativeControlSink g_sink({});
std::unique_ptr<C_CampaignControlAdapter> g_control;
S_CampaignNativeCatalog g_catalog;
C_CampaignNode *g_step = nullptr;
std::optional<std::pair<PlayerId, int>> g_pendingGuest;
std::optional<Clock::time_point> g_loadingDeadline;

R PrepareHostMission(PlayerId guest, int slot)
{
    g_itemsModule.EnsureItemsLoaded();
    S_CampaignMissionManifest frozen;
    auto reason = FreezeDevelopmentCampaign(*g_ownership, *g_journal, kMissionNumber, guest, g_catalog, frozen);
    if (reason != R::None)
        return reason;

    g_mission = std::make_unique<C_CampaignMissionAdapter>(*g_ownership);
    g_control = std::make_unique<C_CampaignControlAdapter>(*g_ownership, g_sink);
    SetCampaignMissionAdapter(g_mission.get());
    InstallCampaignMissionView(g_ownership.get());
    InstallCampaignControlRuntime(g_ownership.get(), g_control.get());
    if (!BindCampaignTransportPlayer(g_localPlayerSlot, kHostPlayer) || !BindCampaignTransportPlayer(slot, guest))
        return R::NotAdmitted;

    S_CampaignMissionNativeHooks hooks;
    hooks.m_RetireBeforeReconstruction = [] {
        return GetGame() ? g_control->RetireBeforeReconstruction() : R::None;
    };
    hooks.m_SynchronizeControllers = [] { return g_control->SynchronizeCommitted(); };
    hooks.m_FindTransportPlayer = FindCampaignTransportPlayer;
    hooks.m_Disconnected = HandleCampaignDisconnect;
    InstallCampaignMissionNative(&g_catalog, std::move(hooks));

    reason = g_mission->Begin(frozen);
    if (reason != R::None)
        return reason;
    g_missionSquad->Clear();
    for (const auto &entry : g_catalog.m_Soldiers)
        *g_missionSquad->AddSoldier() = entry.second;
    return R::None;
}

R AcknowledgeHostLoad()
{
    const auto state = g_ownership->GetSnapshot();
    for (const auto &soldier : state->m_Soldiers) {
        if (!soldier.m_Actor)
            return R::ActorUnavailable;
        LogDevelopmentCampaign("host bound soldier to original mission", soldier.m_Soldier.Value(), soldier.m_Actor->m_Actor.Value());
    }
    if (CanCampaignSimulate() || GetGame()->GetMissionTime() != 0)
        return R::InvalidPhase;
    g_loadingDeadline = Clock::now() + std::chrono::milliseconds(CampaignProtocol::kTimeoutMs);
    LogDevelopmentCampaign("host waiting for initial-world reconstruction",
                           state->m_Context.m_MissionGeneration.Value(), GetGame()->GetMissionTime());
    return R::None;
}

bool Host(const C_MissionDef &mission, S_CampaignNetConfig config)
{
    const S_CampaignSessionSetup setup{
        config.m_Session, CampaignId(config.m_Session.Value()), MissionKey(mission.GetName()),
        {config.m_Hello.m_Content, kDifficulty}, true
    };
    g_journal = std::make_unique<C_CampaignRequestJournal>(setup.m_Session);
    g_ownership = std::make_unique<C_CampaignOwnership>(C_CampaignSession(setup, kRoster), g_policy, *g_journal);
    config.m_Admitted = [](const CampaignProtocol::S_Accept &agreement, int slot) {
        const auto state = g_ownership->GetSnapshot();
        if (state->m_Context.m_Phase == E_CampaignPhase::Running)
            return R::JoinNotSupported;
        if (state->m_Participants.size() > 1)
            return R::SessionFull;
        const auto reason = g_ownership->AdmitTransportPlayer(agreement.m_Player, "Guest");
        if (reason == R::None) {
            LogDevelopmentCampaign("host admitted campaign guest", agreement.m_Session.Value(), agreement.m_Player.Value());
            LogDevelopmentCampaign("provider slot", static_cast<std::uint64_t>(slot));
            g_pendingGuest = {agreement.m_Player, slot};
        }
        return reason;
    };
    config.m_Departed = [](PlayerId player, R reason) {
        g_pendingGuest.reset();
        if (g_control && GetGame() && GetGame()->IsMissionLoaded()) {
            const auto result = HandleCampaignDisconnect(player);
            if (result != R::None || g_mission->GetContext().m_Phase == E_CampaignPhase::Loading)
                g_mission->Abort(result == R::None ? R::Disconnected : result);
        } else {
            g_ownership->OnDisconnected(player);
        }
        LogDevelopmentCampaign("host observed guest departure", player.Value(), static_cast<std::uint64_t>(reason));
    };
    S_NetSessionDesc desc{};
    std::strcpy(desc.name, "Original Campaign Development");
    std::memcpy(desc.serviceProvider, &CLSID_NetDP8SP_TCPIP, sizeof(CLSID_NetDP8SP_TCPIP));
    return g_netGame.Host(desc, false, &config) >= 0;
}

bool Join(std::uint32_t address, S_CampaignNetConfig config)
{
    config.m_Admitted = [](const CampaignProtocol::S_Accept &agreement, int slot) {
        LogDevelopmentCampaign("guest negotiated campaign session", agreement.m_Session.Value(), agreement.m_Player.Value());
        LogDevelopmentCampaign("provider slot", static_cast<std::uint64_t>(slot));
        return R::None;
    };
    config.m_Departed = [](PlayerId player, R reason) {
        LogDevelopmentCampaign("guest session closed", player.Value(), static_cast<std::uint64_t>(reason));
        if (g_netGame.m_lastError < 0)
            LogDevelopmentCampaign("DirectPlay status", static_cast<std::uint32_t>(g_netGame.m_lastError));
        if (auto *app = GetGameApp(); app && app->m_state)
            app->m_state->SetPendingState(GAME_STATE_QUIT);
    };
    S_ServerEntry entry{};
    entry.Reset();
    entry.address = address;
    const auto result = g_netGame.Connect(&entry, &config);
    return result >= 0 || static_cast<std::uint32_t>(result) == 0x8000000a;
}

} // namespace

bool StartDevelopmentCampaign(C_GameApp &app)
{
    const auto launch = ParseLaunch();
    if (launch.m_Mode == E_Launch::Ordinary)
        return false;

    auto *console = new C_ConsoleState(nullptr);
    app.m_state = console;
    console->Start(nullptr);

    if (launch.m_Mode == E_Launch::Invalid) {
        LogDevelopmentCampaign("usage: -campaign-coop host or -campaign-coop join <IPv4>");
        console->SetState(GAME_STATE_QUIT);
        return true;
    }

    g_step = SelectOriginalMission();
    const auto *mission = g_step ? g_campaign->GetCurrentMission() : nullptr;
    if (!mission || !GetSoldierProfile(kRoster.back())) {
        LogDevelopmentCampaign("original campaign fixture unavailable");
        console->SetState(GAME_STATE_QUIT);
        return true;
    }
    LogDevelopmentCampaign(mission->GetName(), static_cast<std::uint64_t>(mission->GetObjectiveCount()));

    const bool host = launch.m_Mode == E_Launch::Host;
    S_CampaignNetConfig config;
    if (!BuildDevContent(*mission, host, config)) {
        LogDevelopmentCampaign("campaign content fingerprint failed");
        console->SetState(GAME_STATE_QUIT);
        return true;
    }

    const bool started = host ? Host(*mission, std::move(config)) : Join(launch.m_Address, std::move(config));
    if (!started) {
        LogDevelopmentCampaign("campaign connection setup failed", static_cast<std::uint32_t>(g_netGame.m_lastError));
        console->SetState(GAME_STATE_QUIT);
    }
    return true;
}

void TickDevelopmentCampaign(C_GameApp &app)
{
    if (!IsCampaignNet())
        return;
    if (app.m_state->GetTypeId() == GAME_STATE_TYPE_CONSOLE &&
        GetForegroundWindow() == GetIGraph()->GetHWND() && (GetAsyncKeyState(VK_END) & 0x8000)) {
        g_netGame.Close(true);
        app.m_state->SetState(GAME_STATE_QUIT);
        return;
    }
    if (g_loadingDeadline && g_mission->GetContext().m_Phase == E_CampaignPhase::Loading) {
        auto reason = g_mission->GetFailure();
        if (reason == R::None && Clock::now() >= *g_loadingDeadline) {
            reason = R::Timeout;
            g_mission->Abort(reason);
        }
        if (reason != R::None) {
            LogDevelopmentCampaign("host aborted initial-world wait", static_cast<std::uint64_t>(reason),
                                   GetGame()->GetMissionTime());
            app.m_state->SetState(GAME_STATE_QUIT);
        }
        return;
    }
    if (!g_pendingGuest)
        return;
    const auto [guest, slot] = *g_pendingGuest;
    g_pendingGuest.reset();
    auto result = PrepareHostMission(guest, slot);
    if (result == R::None) {
        if (app.StartStep(g_step, 0))
            result = AcknowledgeHostLoad();
        else
            result = g_mission->GetFailure() != R::None ? g_mission->GetFailure() : R::SnapshotUnavailable;
    }
    if (result != R::None) {
        LogDevelopmentCampaign("host campaign load failed", static_cast<std::uint64_t>(result));
        app.m_state->SetState(GAME_STATE_QUIT);
    }
}

void ReleaseDevelopmentCampaign()
{
    if (!g_ownership)
        return;
    InstallCampaignMissionNative(nullptr);
    SetCampaignMissionAdapter(nullptr);
    InstallCampaignMissionView(nullptr);
    InstallCampaignControlRuntime(nullptr, nullptr);
    g_loadingDeadline.reset();
    g_pendingGuest.reset();
    g_control.reset();
    g_mission.reset();
    g_ownership.reset();
    g_journal.reset();
    g_catalog = {};
    g_step = nullptr;
}
