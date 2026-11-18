#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>

#include "game/campaign_dev.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include "game/actors_module.h"
#include "game/campaign.h"
#include "game/campaign_content.h"
#include "game/campaign_control_native.h"
#include "game/campaign_control_runtime.h"
#include "game/campaign_dev_fixture.h"
#include "game/campaign_dev_world.h"
#include "game/campaign_ownership.h"
#include "game/console.h"
#include "game/controls.h"
#include "game/game.h"
#include "game/game_app.h"
#include "game/human.h"
#include "game/human_controller.h"
#include "game/items_module.h"
#include "game/pending.h"
#include "game/profile.h"
#include "game/soldier.h"
#include "game/spawn_point.h"
#include "ls3d/i3d.h"
#include "ls3d/igraph.h"
#include "net/campaign_net.h"
#include "net/dplay8.h"
#include "net/net_game.h"
#include "script/script_module.h"

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
using Phase = E_CampaignPhase;
namespace Wire = CampaignProtocol;
using Clock = std::chrono::steady_clock;
constexpr int kOriginalCampaign = 0;
constexpr int kMissionNumber = 1; // first operation mission after Tutorial
constexpr std::uint32_t kDifficulty = 2;
constexpr PlayerId kHostPlayer{1};
const std::vector<int> kRoster{0, 1, 2, 3};
constexpr char kFixtureRules[] =
    "original-campaign mission=1 difficulty=2 roster=0,1,2,3 host=0 guest=1 fixed-assignment slots=0,1,2,3 issue=mission-essentials initial-world=CIW1";


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
void StoreDevelopmentControls(const Wire::S_Input &input);
C_CampaignNativeControlSink g_sink(StoreDevelopmentControls);
std::unique_ptr<C_CampaignControlAdapter> g_control;
S_CampaignNativeCatalog g_catalog;
C_CampaignNode *g_step = nullptr;
std::optional<std::pair<PlayerId, int>> g_pendingGuest;
std::optional<Clock::time_point> g_loadingDeadline;
Wire::C_WorldSender g_worldSender;
PlayerId g_transferPlayer;
bool g_guestAcknowledged = false;

struct C_InitialGuestView final : I_CampaignOwnershipView {
    T_CampaignOwnershipSnapshot GetSnapshot() const override { return m_Snapshot; }
    T_CampaignOwnershipSnapshot m_Snapshot;
};

struct S_GuestSession {
    S_GuestSession(const Wire::S_Accept &agreement, int slot) : m_Agreement(agreement), m_Slot(slot) {}
    ~S_GuestSession() { if (m_Camera) m_Camera->Release(); }
    Wire::S_Accept m_Agreement;
    int m_Slot;
    std::optional<S_CampaignMissionManifest> m_Manifest;
    Wire::C_WorldAssembler m_Assembler;
    std::optional<Wire::T_Bytes> m_World;
    C_InitialGuestView m_InitialView;
    bool m_Aborted = false;
    std::uint64_t m_LastUpdate = 0;
    std::uint64_t m_LastWeaponEvent = 0;
    std::uint64_t m_LastInventoryEvent = 0;
    std::uint32_t m_HostMissionTime = 0;
    std::uint64_t m_InputSequence = 0;
    std::uint16_t m_PendingButtons = 0;
    ControlRevision m_InputRevision;
    Clock::time_point m_NextInput{};
    std::optional<Wire::S_MissionCamera> m_CutsceneCamera;
    I3D_camera *m_Camera = nullptr;
    std::unique_ptr<C_CampaignReplicaControlAdapter> m_Control;
    std::unique_ptr<C_CampaignMissionReconstruction> m_Reconstruction;
};

std::unique_ptr<S_GuestSession> g_guest;
std::optional<Clock::time_point> g_nextMissionUpdate;
std::uint64_t g_missionUpdateSequence = 0;
std::uint64_t g_weaponSequence = 0, g_inventorySequence = 0;
bool g_liveSendFailed = false;
OwnershipRevision g_publishedOwnership;
std::vector<std::uint8_t> g_publishedObjectives;

struct S_DevelopmentMotion {
    std::optional<S_vector> m_Origin;
    unsigned m_RecordedCentimeters = 0;
};
std::array<S_DevelopmentMotion, 4> g_squadMotion;

void ObserveSquadMotion(const Wire::S_MissionUpdate &update, const char *event)
{
    if (!update.m_PlayerInputEnabled)
        return;
    for (const auto &actor : update.m_Squad) {
        const auto id = actor.m_Actor.m_Actor.Value();
        if (id >= g_squadMotion.size())
            continue;
        auto &motion = g_squadMotion[id];
        const auto &p = actor.m_Position;
        const S_vector current(p[0], p[1], p[2]);
        if (!motion.m_Origin) {
            motion.m_Origin = current;
            continue;
        }
        const auto delta = current - *motion.m_Origin;
        const float distance = S_vector(delta.x, 0.0f, delta.z).Magnitude();
        if (!(distance >= 0.25f) || distance >= 10000.0f)
            continue;
        const auto cm = static_cast<unsigned>(distance * 100.0f);
        if (cm >= motion.m_RecordedCentimeters + 25) {
            motion.m_RecordedCentimeters = cm;
            LogDevelopmentCampaign(event, id, cm);
        }
    }
}

struct S_ReceivedControls {
    S_CampaignControlToken m_Token;
    Wire::S_PlayerControls m_Controls;
    Clock::time_point m_Received;
};
std::optional<S_ReceivedControls> g_receivedControls;
bool g_loggedInput = false;
std::uint16_t g_lastGuestMovement = 0;

bool ApplyInventoryMove(const S_CampaignControlToken &token, Wire::S_ItemMove move);

void StoreDevelopmentControls(const Wire::S_Input &input)
{
    if (input.m_Kind == Wire::E_InputKind::ActorAction) {
        Wire::S_ItemMove move;
        if (Wire::DecodeItemMove(input.m_Payload, move) == R::None)
            ApplyInventoryMove(input.m_Control, move);
        return;
    }
    Wire::S_PlayerControls controls;
    if (Wire::DecodePlayerControls(input.m_Payload, controls) != R::None)
        return;
    if (g_receivedControls && g_receivedControls->m_Token.m_Actor == input.m_Control.m_Actor &&
        g_receivedControls->m_Token.m_ControlRevision == input.m_Control.m_ControlRevision &&
        g_receivedControls->m_Token.m_Player == input.m_Control.m_Player)
        controls.m_Pressed |= g_receivedControls->m_Controls.m_Pressed;
    g_receivedControls = S_ReceivedControls{input.m_Control, controls, Clock::now()};
    constexpr auto movement = Wire::kMoveForward | Wire::kMoveBack | Wire::kMoveLeft | Wire::kMoveRight;
    const auto heldMovement = static_cast<std::uint16_t>(controls.m_Held & movement);
    if (heldMovement != g_lastGuestMovement) {
        g_lastGuestMovement = heldMovement;
        LogDevelopmentCampaign("host accepted guest movement controls", input.m_Control.m_Actor.m_Actor.Value(), heldMovement);
    }
    if (!g_loggedInput && (controls.m_Held || controls.m_Pressed)) {
        LogDevelopmentCampaign("host accepted tagged guest controls", input.m_Control.m_Player.Value(),
                               input.m_Control.m_Actor.m_Actor.Value());
        g_loggedInput = true;
    }
}


std::uint64_t WorldTimeMs()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count());
}

void FailGuest(R reason)
{
    if (g_guest->m_Aborted)
        return;
    g_guest->m_Aborted = true;
    LogDevelopmentCampaign("guest initial world rejected", static_cast<std::uint64_t>(reason));
    GetGameApp()->m_state->SetState(GAME_STATE_QUIT);
}

bool MatchesContent(const S_CampaignContentDescriptor &a, const S_CampaignContentDescriptor &b)
{
    return a.m_CampaignContent == b.m_CampaignContent && a.m_MissionContent == b.m_MissionContent &&
        a.m_ItemContent == b.m_ItemContent && a.m_SoldierContent == b.m_SoldierContent && a.m_Rules == b.m_Rules;
}

R ValidateManifest(const S_GuestSession &guest, const S_CampaignMissionManifest &manifest)
{
    const auto &context = manifest.m_Context;
    const auto *mission = g_campaign->GetCurrentMission();
    if (!mission || context.m_Mission != MissionKey(mission->GetName()) ||
        context.m_Campaign != CampaignId(guest.m_Agreement.m_Session.Value()) ||
        context.m_Rules.m_Difficulty != kDifficulty ||
        !MatchesContent(context.m_Rules.m_Content, guest.m_Agreement.m_Content))
        return R::ContentMismatch;
    if (context.m_Phase != Phase::Loading || context.m_Kind != E_CampaignSessionKind::CampaignCoop ||
        !context.m_UsesCampaignRules || !context.m_IsTransportServer || !context.m_IsTransportClient ||
        context.m_HostPlayer != kHostPlayer || context.m_LocalPlayer != kHostPlayer ||
        manifest.m_RequiredParticipants != std::vector<PlayerId>{kHostPlayer, guest.m_Agreement.m_Player})
        return R::Malformed;
    if (manifest.m_Soldiers.size() != kRoster.size() || manifest.m_Bindings.size() != kRoster.size())
        return R::InvalidSoldier;
    for (std::size_t i = 0; i < manifest.m_Soldiers.size(); ++i) {
        const auto &record = manifest.m_Soldiers[i].m_Ownership;
        const std::optional<PlayerId> owner = i == 0 ? std::optional<PlayerId>(kHostPlayer) :
            i == 1 ? std::optional<PlayerId>(guest.m_Agreement.m_Player) : std::nullopt;
        if (record.m_Soldier != SoldierId(i + 1) || record.m_Owner != owner || !record.m_Actor ||
            record.m_Actor->m_Actor.Value() != i)
            return R::InvalidSoldier;
    }
    return R::None;
}

S_CampaignSnapshotAck GuestBaseline(const S_GuestSession &guest)
{
    const auto &m = *guest.m_Manifest;
    return {m.m_Context.m_Session, guest.m_Agreement.m_Player, m.m_Context.m_MissionGeneration,
            m.m_Snapshot, m.m_PreparationGeneration, m.m_FrozenPreparationRevision,
            m.m_ViewRevision, m.m_OwnershipRevision};
}

R ApplyMissionUpdate(S_GuestSession &guest, const Wire::S_MissionUpdate &update)
{
    const auto state = GetCampaignMissionSnapshot();
    auto *game = GetGame();
    if (!state || !CanCampaignSimulate() || !guest.m_Manifest || !game || !game->GetMission())
        return R::InvalidPhase;
    if (update.m_Session != state->m_Context.m_Session ||
        update.m_Generation != state->m_Context.m_MissionGeneration ||
        update.m_Snapshot != guest.m_Manifest->m_Snapshot)
        return R::WrongGeneration;
    if (update.m_Sequence <= guest.m_LastUpdate || update.m_MissionTime < guest.m_HostMissionTime)
        return R::StaleRevision;
    if (update.m_OwnershipRevision != state->m_Revision)
        return R::SnapshotGap;
    auto *mission = game->GetMission();
    const auto bound = std::count_if(state->m_Soldiers.begin(), state->m_Soldiers.end(),
        [](const auto &soldier) { return soldier.m_Actor.has_value(); });
    if (update.m_Squad.size() != static_cast<std::size_t>(bound) ||
        update.m_Objectives.size() != static_cast<std::size_t>(mission->GetObjectiveCount()))
        return R::ContentMismatch;
    if (!g_sink.ApplySquadActors(update.m_Squad))
        return R::ActorUnavailable;
    ObserveSquadMotion(update, "guest applied host squad movement cm");

    for (std::size_t index = 0; index < update.m_Objectives.size(); ++index) {
        auto *objective = mission->GetObjective(static_cast<int>(index));
        if (objective->status != update.m_Objectives[index]) {
            objective->SetStatus(update.m_Objectives[index]);
            LogDevelopmentCampaign("guest applied objective state", index + 1, objective->status);
        }
    }
    if (guest.m_CutsceneCamera.has_value() != update.m_CutsceneCamera.has_value())
        LogDevelopmentCampaign(update.m_CutsceneCamera ? "guest began host cutscene view" :
                               "guest ended host cutscene view", update.m_MissionTime);
    guest.m_CutsceneCamera = update.m_CutsceneCamera;
    game->SetPlayerInputEnabled(update.m_PlayerInputEnabled);
    if (!guest.m_LastUpdate)
        LogDevelopmentCampaign("guest applied first live mission state", update.m_MissionTime, update.m_Squad.size());
    guest.m_LastUpdate = update.m_Sequence;
    guest.m_HostMissionTime = update.m_MissionTime;
    return R::None;
}

void ReceiveWeaponEvent(S_GuestSession &guest, const Wire::S_WeaponEvent &event)
{
    const auto state = GetCampaignMissionSnapshot();
    if (!state || !CanCampaignSimulate() || !guest.m_Manifest ||
        event.m_Actor.m_Generation != state->m_Context.m_MissionGeneration ||
        event.m_Snapshot != guest.m_Manifest->m_Snapshot ||
        event.m_OwnershipRevision != state->m_Revision)
        return FailGuest(R::SnapshotGap);
    if (event.m_Sequence <= guest.m_LastWeaponEvent)
        return;
    if (event.m_Sequence != guest.m_LastWeaponEvent + 1 || !g_sink.ApplyWeaponEvent(event))
        return FailGuest(R::ActorUnavailable);

    guest.m_LastWeaponEvent = event.m_Sequence;
    LogDevelopmentCampaign(event.m_Action == Wire::E_WeaponEvent::Fire ? "guest presented host shot" :
                           "guest presented host reload", event.m_Actor.m_Actor.Value(), event.m_Rounds);
}

void ReceiveInventoryEvent(S_GuestSession &guest, const Wire::S_InventoryEvent &event)
{
    const auto state = GetCampaignMissionSnapshot();
    if (!state || !CanCampaignSimulate() || !guest.m_Manifest ||
        event.m_Actor.m_Generation != state->m_Context.m_MissionGeneration ||
        event.m_Snapshot != guest.m_Manifest->m_Snapshot || event.m_OwnershipRevision != state->m_Revision)
        return FailGuest(R::SnapshotGap);
    if (event.m_Sequence <= guest.m_LastInventoryEvent)
        return;
    if (event.m_Sequence != guest.m_LastInventoryEvent + 1 || !g_sink.ApplyInventoryEvent(event))
        return FailGuest(R::InvalidPlacement);
    guest.m_LastInventoryEvent = event.m_Sequence;
    LogDevelopmentCampaign("guest applied host inventory move", event.m_Actor.m_Actor.Value(), event.m_Move.m_ItemUid);
}

void ReceiveGuestWorld(PlayerId, Wire::T_Message message)
{
    if (!g_guest || g_guest->m_Aborted)
        return;
    auto &guest = *g_guest;
    if (auto *manifest = std::get_if<S_CampaignMissionManifest>(&message)) {
        const auto reason = !guest.m_Manifest ? ValidateManifest(guest, *manifest) : R::InvalidPhase;
        if (reason != R::None)
            return FailGuest(reason);
        guest.m_Manifest = std::move(*manifest);
        LogDevelopmentCampaign("guest received initial manifest", guest.m_Manifest->m_Snapshot.Value());
    } else if (const auto *chunk = std::get_if<Wire::S_WorldChunk>(&message)) {
        if (!guest.m_Manifest || guest.m_World || guest.m_Reconstruction)
            return FailGuest(R::InvalidPhase);
        const auto reason = guest.m_Assembler.Add(*chunk, GuestBaseline(guest), WorldTimeMs());
        if (reason != R::None)
            return FailGuest(reason);
        guest.m_World = guest.m_Assembler.Take();
    } else if (const auto *start = std::get_if<S_CampaignStartSimulation>(&message)) {
        if (!guest.m_Reconstruction)
            return FailGuest(R::InvalidPhase);
        auto reason = guest.m_Reconstruction->AcceptStart(*start);
        if (reason != R::None)
            return FailGuest(reason);
        auto running = std::make_shared<S_CampaignOwnershipSnapshot>(*guest.m_Control->GetSnapshot());
        running->m_Context.m_Phase = Phase::Running;
        reason = guest.m_Control->InstallCommitted(running);
        if (reason != R::None)
            return FailGuest(reason);
        GetGame()->SetPlayerInputEnabled(false);
        LogDevelopmentCampaign("guest started campaign simulation", start->m_Generation.Value(), start->m_Snapshot.Value());
    } else if (const auto *ownership = std::get_if<S_CampaignOwnershipSnapshot>(&message)) {
        const auto state = GetCampaignMissionSnapshot();
        if (!state || !CanCampaignSimulate() || ownership->m_Context.m_Phase != Phase::Running ||
            ownership->m_Context.m_MissionGeneration != state->m_Context.m_MissionGeneration ||
            ownership->m_Context.m_HostPlayer != state->m_Context.m_HostPlayer ||
            ownership->m_Context.m_Mission != state->m_Context.m_Mission)
            return FailGuest(R::WrongGeneration);
        const auto reason = guest.m_Control->InstallCommitted(std::make_shared<S_CampaignOwnershipSnapshot>(*ownership));
        if (reason != R::None)
            FailGuest(reason);
    } else if (const auto *update = std::get_if<Wire::S_MissionUpdate>(&message)) {
        const auto reason = ApplyMissionUpdate(guest, *update);
        if (reason != R::None && reason != R::StaleRevision)
            FailGuest(reason);
    } else if (const auto *weapon = std::get_if<Wire::S_WeaponEvent>(&message)) {
        ReceiveWeaponEvent(guest, *weapon);
    } else if (const auto *inventory = std::get_if<Wire::S_InventoryEvent>(&message)) {
        ReceiveInventoryEvent(guest, *inventory);
    }
}

R ReconstructGuestWorld(S_GuestSession &guest, const std::function<bool()> &startMission)
{
    const auto &manifest = *guest.m_Manifest;
    auto loading = std::make_shared<S_CampaignOwnershipSnapshot>();
    loading->m_Context = manifest.m_Context;
    loading->m_Context.m_IsTransportServer = false;
    loading->m_Context.m_LocalPlayer = guest.m_Agreement.m_Player;
    loading->m_Revision = manifest.m_OwnershipRevision;
    for (const auto &peer : manifest.m_RequiredParticipants)
        loading->m_Participants.push_back({peer, peer == kHostPlayer ? "Host" : "Guest", true, true, std::nullopt});
    for (const auto &soldier : manifest.m_Soldiers)
        loading->m_Soldiers.push_back(soldier.m_Ownership);

    guest.m_InitialView.m_Snapshot = loading;
    InstallCampaignMissionView(&guest.m_InitialView);
    InstallCampaignControlView(&guest.m_InitialView);
    guest.m_Control = std::make_unique<C_CampaignReplicaControlAdapter>(guest.m_Agreement.m_Session,
                                                                      guest.m_Agreement.m_Player, g_sink);
    guest.m_Reconstruction = std::make_unique<C_CampaignMissionReconstruction>(
        guest.m_Agreement.m_Session, guest.m_Agreement.m_Player, *guest.m_Control);
    InstallCampaignMissionReconstruction(guest.m_Reconstruction.get());

    const auto reason = guest.m_Reconstruction->Reconstruct(manifest, *guest.m_World,
        [&](const S_CampaignMissionManifest &frozen, const Wire::T_Bytes &world) -> R {
            const S_CampaignSessionSetup setup{frozen.m_Context.m_Session, frozen.m_Context.m_Campaign,
                                               frozen.m_Context.m_Mission, frozen.m_Context.m_Rules, true};
            const C_CampaignSession roster(setup, kRoster);
            g_itemsModule.EnsureItemsLoaded();
            std::vector<S_CampaignNativeSpawn> spawns;
            auto result = ImportDevelopmentReplica(roster, kMissionNumber, frozen, g_catalog, spawns);
            if (result != R::None)
                return result;
            g_netGame.MakeReplication();
            if (!startMission())
                return R::SnapshotUnavailable;

            for (auto &spawn : spawns) {
                auto *point = static_cast<C_SpawnPoint *>(g_actorsModule.FindSpawnPoint(0));
                auto *human = point ? point->SpawnCampaign(spawn.m_State, spawn.m_Stats.data()) : nullptr;
                if (!human)
                    return R::ActorUnavailable;
                const auto binding = std::find_if(frozen.m_Bindings.begin(), frozen.m_Bindings.end(),
                    [&](const auto &candidate) { return candidate.m_Soldier == spawn.m_Soldier; });
                const auto actor = human->GetNetId();
                if (binding == frozen.m_Bindings.end() || binding->m_Actor.m_Actor.Value() != static_cast<unsigned>(actor) ||
                    binding->m_Actor.m_SlotUse.Value() != static_cast<std::uint8_t>(g_actorsModule.GetPlayerSlotUse(actor)))
                    return R::ActorUnavailable;
                point->NotifyCampaignSpawned();
            }

            InstallCampaignControlView(guest.m_Control.get());
            result = guest.m_Control->InstallCommitted(loading);
            if (result != R::None || !BindCampaignTransportPlayer(guest.m_Slot, guest.m_Agreement.m_Player))
                return result == R::None ? R::NotAdmitted : result;
            InstallCampaignMissionView(guest.m_Control.get());

            result = InstallDevelopmentInitialWorld(world);
            if (result != R::None)
                return result;
            LogDevelopmentCampaign("guest reconstructed original initial world", world.size(), Wire::Crc32(world));
            return R::None;
        });
    guest.m_World.reset();
    if (reason != R::None)
        return reason;
    const auto ack = guest.m_Reconstruction->GetAcknowledgment();
    if (!ack || !SendCampaignMessage(g_netGame, guest.m_Agreement.m_Player, *ack))
        return R::Disconnected;
    LogDevelopmentCampaign("guest acknowledged campaign snapshot", ack->m_Generation.Value(), ack->m_Snapshot.Value());
    return R::None;
}

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

R SendHostInitialWorld(PlayerId guest)
{
    const auto state = g_ownership->GetSnapshot();
    for (const auto &soldier : state->m_Soldiers) {
        if (!soldier.m_Actor)
            return R::ActorUnavailable;
        LogDevelopmentCampaign("host bound soldier to original mission", soldier.m_Soldier.Value(), soldier.m_Actor->m_Actor.Value());
    }
    if (CanCampaignSimulate() || GetGame()->GetMissionTime() != 0)
        return R::InvalidPhase;

    Wire::T_Bytes world;
    auto reason = CaptureDevelopmentInitialWorld(world);
    if (reason != R::None || (reason = g_mission->SealInitialWorld()) != R::None)
        return reason;
    const auto manifest = g_mission->GetManifest();
    const auto hostAck = g_mission->ExpectedAck(kHostPlayer);
    const auto guestAck = g_mission->ExpectedAck(guest);
    if (!hostAck || !guestAck)
        return R::SnapshotUnavailable;
    reason = g_mission->Acknowledge(kHostPlayer, *hostAck);
    if (reason != R::None)
        return reason;
    if (!SendCampaignMessage(g_netGame, guest, *manifest))
        return R::Disconnected;
    reason = g_worldSender.Begin(*guestAck, std::move(world), WorldTimeMs());
    if (reason != R::None)
        return reason;

    g_transferPlayer = guest;
    g_publishedOwnership = manifest->m_OwnershipRevision;
    g_loadingDeadline = Clock::now() + std::chrono::milliseconds(Wire::kTimeoutMs);
    LogDevelopmentCampaign("host waiting for initial-world reconstruction",
                           state->m_Context.m_MissionGeneration.Value(), GetGame()->GetMissionTime());
    return R::None;
}

void ReceiveHostWorld(PlayerId player, Wire::T_Message message)
{
    if (const auto *input = std::get_if<Wire::S_Input>(&message)) {
        if (!g_control)
            return;
        Wire::S_PlayerControls controls;
        Wire::S_ItemMove move;
        const bool valid = input->m_Kind == Wire::E_InputKind::ControlBits
            ? Wire::DecodePlayerControls(input->m_Payload, controls) == R::None
            : input->m_Kind == Wire::E_InputKind::ActorAction && Wire::DecodeItemMove(input->m_Payload, move) == R::None;
        if (!valid)
            return;
        const auto result = g_control->Enqueue(player, *input);
        if (result != R::None && result != R::PendingTransaction)
            LogDevelopmentCampaign("host rejected tagged guest controls", player.Value(), static_cast<std::uint64_t>(result));
        return;
    }

    const auto *ack = std::get_if<S_CampaignSnapshotAck>(&message);
    if (!ack || !g_mission || player != g_transferPlayer || !g_worldSender.IsComplete())
        return;
    if (g_mission->GetContext().m_Phase != Phase::Loading || !g_loadingDeadline ||
        Clock::now() >= *g_loadingDeadline)
        return;
    const auto result = g_mission->Acknowledge(player, *ack);
    if (result != R::None) {
        LogDevelopmentCampaign("host rejected snapshot acknowledgment", player.Value(), static_cast<std::uint64_t>(result));
        return;
    }
    g_guestAcknowledged = true;
}

std::array<float, 3> VectorFields(const S_vector &value)
{
    return {value.x, value.y, value.z};
}

bool PublishLiveOwnership(const S_CampaignOwnershipSnapshot &state)
{
    if (g_publishedOwnership == state.m_Revision)
        return true;
    if (!SendCampaignMessage(g_netGame, g_transferPlayer, state))
        return false;
    g_publishedOwnership = state.m_Revision;
    return true;
}

bool ApplyInventoryMove(const S_CampaignControlToken &token, Wire::S_ItemMove move)
{
    auto *game = GetGame();
    if (!game || !game->IsPlayerInputEnabled() || !CanCampaignSimulate())
        return false;
    const auto reason = g_sink.MoveInventory(token.m_Actor, move);
    if (reason != R::None) {
        LogDevelopmentCampaign("host rejected inventory move", token.m_Actor.m_Actor.Value(), static_cast<std::uint64_t>(reason));
        if (reason == R::SnapshotGap)
            g_liveSendFailed = true;
        return false;
    }
    LogDevelopmentCampaign("host committed inventory move", token.m_Actor.m_Actor.Value(), move.m_ItemUid);
    if (!g_transferPlayer.Value() || g_liveSendFailed)
        return true;
    const auto state = g_ownership->GetSnapshot();
    const auto manifest = g_mission->GetManifest();
    if (!state || !manifest) {
        g_liveSendFailed = true;
        return false;
    }
    const Wire::S_InventoryEvent event{token.m_Actor, manifest->m_Snapshot, state->m_Revision, ++g_inventorySequence, move};
    g_liveSendFailed = !PublishLiveOwnership(*state) || !SendCampaignMessage(g_netGame, g_transferPlayer, event);
    return !g_liveSendFailed;
}

bool PublishMissionUpdate(Clock::time_point now)
{
    if (!g_transferPlayer.Value() || !CanCampaignSimulate() ||
        !g_control->IsSynchronized() || (g_nextMissionUpdate && now < *g_nextMissionUpdate))
        return true;
    auto *game = GetGame();
    const auto state = g_ownership->GetSnapshot();
    const auto manifest = g_mission->GetManifest();
    if (!game || !game->GetMission() || !manifest)
        return false;
    if (!PublishLiveOwnership(*state))
        return false;
    Wire::S_MissionUpdate update;
    update.m_Session = state->m_Context.m_Session;
    update.m_Generation = state->m_Context.m_MissionGeneration;
    update.m_Snapshot = manifest->m_Snapshot;
    update.m_OwnershipRevision = state->m_Revision;
    update.m_Sequence = ++g_missionUpdateSequence;
    update.m_MissionTime = game->GetMissionTime();
    update.m_PlayerInputEnabled = game->IsPlayerInputEnabled();
    if (g_scriptModule.IsCutsceneRunning()) {
        if (auto *camera = game->GetScene()->GetActiveCamera()) {
            const auto &matrix = camera->GetMatrix();
            const auto direction = matrix(2).Normalized();
            const auto up = (matrix(1) - direction * matrix(1).Dot(direction)).Normalized();
            update.m_CutsceneCamera = Wire::S_MissionCamera{
                VectorFields(matrix(3)), VectorFields(direction), VectorFields(up), camera->GetFOV()};
            update.m_PlayerInputEnabled = false;
        }
    }
    for (const auto &soldier : state->m_Soldiers) {
        if (!soldier.m_Actor)
            continue;
        Wire::S_SquadActorState actor;
        actor.m_Actor = *soldier.m_Actor;
        if (!g_sink.CaptureSquadActor(actor))
            return false;
        update.m_Squad.push_back(actor);
    }
    auto *mission = game->GetMission();
    for (int index = 0; index < mission->GetObjectiveCount(); ++index)
        update.m_Objectives.push_back(static_cast<std::uint8_t>(mission->GetObjective(index)->status));
    g_nextMissionUpdate = now + std::chrono::milliseconds(66);
    if (!SendCampaignMessage(g_netGame, g_transferPlayer, update))
        return false;
    ObserveSquadMotion(update, "host measured authoritative squad movement cm");
    for (std::size_t index = 0; index < update.m_Objectives.size(); ++index) {
        if (index >= g_publishedObjectives.size() || g_publishedObjectives[index] != update.m_Objectives[index])
            LogDevelopmentCampaign("host published objective state", index + 1, update.m_Objectives[index]);
    }
    g_publishedObjectives = update.m_Objectives;
    if (update.m_Sequence == 1)
        LogDevelopmentCampaign("host published first live mission state", update.m_MissionTime, update.m_Squad.size());
    return true;
}

R ReconcileGuestDeparture(PlayerId player)
{
    const auto state = g_ownership->GetSnapshot();
    const auto soldier = std::find_if(state->m_Soldiers.begin(), state->m_Soldiers.end(),
        [&](const auto &record) { return record.m_Owner == player && record.m_Actor; });
    std::optional<S_CampaignControlToken> token;
    if (soldier != state->m_Soldiers.end())
        token = S_CampaignControlToken{state->m_Context.m_Session, state->m_Context.m_MissionGeneration,
                                      player, soldier->m_Soldier, *soldier->m_Actor, soldier->m_ControlRevision};
    const auto before = token ? CaptureDevelopmentSoldier(token->m_Actor) : std::nullopt;
    const auto reason = HandleCampaignDisconnect(player);
    if (reason != R::None)
        return reason;
    if (!token) {
        LogDevelopmentCampaign("host departure had no bound owner", player.Value());
        return R::None;
    }
    const auto stale = g_ownership->ValidateInput(player, *token);
    if (stale == R::None)
        return R::NotOwner;
    LogDevelopmentCampaign("host refused retired input token", player.Value(), static_cast<std::uint64_t>(stale));
    if (!before)
        return R::ActorUnavailable;

    const auto after = CaptureDevelopmentSoldier(token->m_Actor);
    const auto actor = static_cast<int>(token->m_Actor.m_Actor.Value());
    const auto record = FindCampaignActorControl(actor);
    const auto *human = static_cast<const C_Human *>(after ? after->m_Actor : nullptr);
    if (!after || before->m_Actor != after->m_Actor || !record || record->m_Owner ||
        human->GetControllerType() != HUMAN_CONTROLLER_SQUAD) {
        LogDevelopmentCampaign("host guest soldier binding differed after handoff", actor);
        return R::ActorUnavailable;
    }
    if (before->m_State != after->m_State) {
        const auto mismatch = std::mismatch(before->m_State.begin(), before->m_State.end(),
                                            after->m_State.begin(), after->m_State.end());
        LogDevelopmentCampaign("host handoff state differed at byte", mismatch.first - before->m_State.begin(), actor);
        return R::ContentMismatch;
    }
    LogDevelopmentCampaign("host preserved same guest soldier for squad AI", actor, Wire::Crc32(after->m_State));
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
    config.m_Received = ReceiveHostWorld;
    config.m_Departed = [](PlayerId player, R reason) {
        if (g_pendingGuest && g_pendingGuest->first == player)
            g_pendingGuest.reset();
        if (g_control && GetGame() && GetGame()->IsMissionLoaded()) {
            const auto result = ReconcileGuestDeparture(player);
            if (result != R::None || g_mission->GetContext().m_Phase == E_CampaignPhase::Loading)
                g_mission->Abort(result == R::None ? R::Disconnected : result);
        } else {
            g_ownership->OnDisconnected(player);
        }
        if (g_transferPlayer == player) {
            g_transferPlayer = {};
            g_liveSendFailed = false;
        }
        if (g_receivedControls && g_receivedControls->m_Token.m_Player == player)
            g_receivedControls.reset();
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
        g_guest = std::make_unique<S_GuestSession>(agreement, slot);
        LogDevelopmentCampaign("guest negotiated campaign session", agreement.m_Session.Value(), agreement.m_Player.Value());
        LogDevelopmentCampaign("provider slot", static_cast<std::uint64_t>(slot));
        return R::None;
    };
    config.m_Received = ReceiveGuestWorld;
    config.m_Departed = [](PlayerId player, R reason) {
        if (g_guest)
            g_guest->m_Aborted = true;
        LogDevelopmentCampaign("guest session closed", player.Value(), static_cast<std::uint64_t>(reason));
        if (g_netGame.m_lastError < 0)
            LogDevelopmentCampaign("DirectPlay status", static_cast<std::uint32_t>(g_netGame.m_lastError));
        // The transport may retire replication objects before the next state update.
        if (auto *app = GetGameApp(); app && app->m_state)
            app->m_state->SetState(GAME_STATE_QUIT);
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
    if (!BuildCampaignNetConfig(mission->GetName(), host, "campaign-dev/arctic-fixture-v1",
                                kFixtureRules, config)) {
        LogDevelopmentCampaign("campaign content fingerprint failed");
        console->SetState(GAME_STATE_QUIT);
        return true;
    }

    config.m_Hello.m_Required |= Wire::kMissionUpdates | Wire::kWeaponEvents | Wire::kInventoryMoves;
    config.m_Hello.m_Offered |= Wire::kMissionUpdates | Wire::kWeaponEvents | Wire::kInventoryMoves;
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
    if (g_guest) {
        if (g_guest->m_Aborted)
            return;
        if (g_guest->m_World) {
            const auto reason = ReconstructGuestWorld(*g_guest, [&] { return app.StartStep(g_step, 0); });
            if (reason != R::None)
                FailGuest(reason);
        } else if (g_guest->m_Assembler.Tick(WorldTimeMs()) == R::Timeout) {
            FailGuest(R::Timeout);
        }
        return;
    }
    if (g_loadingDeadline && g_mission->GetContext().m_Phase == Phase::Loading) {
        auto reason = g_mission->GetFailure();
        if (reason == R::None && Clock::now() >= *g_loadingDeadline) {
            reason = R::Timeout;
            g_mission->Abort(reason);
        }
        if (reason == R::None && !g_worldSender.IsComplete()) {
            reason = g_worldSender.Pump(WorldTimeMs(), [](const auto &chunk) {
                return SendCampaignMessage(g_netGame, g_transferPlayer, chunk);
            });
        }
        if (reason == R::None && g_guestAcknowledged) {
            S_CampaignStartSimulation start;
            reason = g_mission->Start(start);
            if (reason == R::None && !SendCampaignMessage(g_netGame, g_transferPlayer, start))
                reason = R::Disconnected;
            if (reason == R::None)
                LogDevelopmentCampaign("host started campaign simulation", start.m_Generation.Value(), start.m_Snapshot.Value());
        }
        if (reason != R::None) {
            g_mission->Abort(reason);
            LogDevelopmentCampaign("host aborted initial-world wait", static_cast<std::uint64_t>(reason),
                                   GetGame()->GetMissionTime());
            app.m_state->SetState(GAME_STATE_QUIT);
        }
        return;
    }
    if (g_mission && g_mission->GetContext().m_Phase == Phase::Running) {
        for (unsigned count = 0; count < 32 && g_control->GetQueuedCount() && g_control->IsSynchronized(); ++count) {
            const auto reason = g_control->ApplyNext();
            if (reason != R::None)
                LogDevelopmentCampaign("host discarded retired guest controls", static_cast<std::uint64_t>(reason));
        }
        if (g_liveSendFailed || !PublishMissionUpdate(Clock::now())) {
            LogDevelopmentCampaign("host failed to publish mission state", g_missionUpdateSequence);
            g_netGame.Close(true);
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
            result = SendHostInitialWorld(guest);
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
    if (!g_ownership && !g_guest)
        return;
    InstallCampaignMissionReconstruction(nullptr);
    InstallCampaignMissionNative(nullptr);
    SetCampaignMissionAdapter(nullptr);
    InstallCampaignMissionView(nullptr);
    InstallCampaignControlRuntime(nullptr, nullptr);
    g_loadingDeadline.reset();
    g_worldSender.Reset();
    g_transferPlayer = {};
    g_guestAcknowledged = false;
    g_nextMissionUpdate.reset();
    g_missionUpdateSequence = 0;
    g_weaponSequence = 0;
    g_inventorySequence = 0;
    g_liveSendFailed = false;
    g_publishedOwnership = {};
    g_publishedObjectives.clear();
    g_squadMotion = {};
    g_receivedControls.reset();
    g_loggedInput = false;
    g_lastGuestMovement = 0;
    g_guest.reset();
    g_pendingGuest.reset();
    g_control.reset();
    g_mission.reset();
    g_ownership.reset();
    g_journal.reset();
    g_catalog = {};
    g_step = nullptr;
}

bool RenderDevelopmentCampaignCutscene(I3D_scene &scene)
{
    if (!g_guest || !g_guest->m_CutsceneCamera || g_guest->m_Aborted)
        return false;
    auto *savedCamera = scene.GetActiveCamera();
    if (!savedCamera)
        return false;
    auto &guest = *g_guest;
    if (!guest.m_Camera)
        guest.m_Camera = static_cast<I3D_camera *>(I3DGetDriver()->CreateFrame(FRAME_CAMERA));
    if (!guest.m_Camera)
        return false;

    const auto &state = *guest.m_CutsceneCamera;
    const auto vector = [](const auto &v) { return S_vector(v[0], v[1], v[2]); };
    auto &camera = *guest.m_Camera;
    camera.SetWorldPosDir3(vector(state.m_Position), vector(state.m_Direction), vector(state.m_Up));
    camera.SetFOV(state.m_Fov);
    camera.SetAspectRatio(16.0f / 9.0f);
    float nearRange, farRange;
    savedCamera->GetRange(nearRange, farRange);
    camera.SetRange(nearRange, farRange);
    camera.Update();
    savedCamera->AddRef();
    scene.SetActiveCamera(&camera);
    const auto *rect = scene.GetViewportRect();
    const std::array<int, 4> savedRect{rect[0], rect[1], rect[2], rect[3]};
    const int width = GetIGraph()->ScrnX();
    const int height = GetIGraph()->ScrnY();
    const int band = static_cast<int>(width * (9.0f / 16.0f));
    const int top = std::max(0, (height - band) / 2);
    scene.SetViewport(0, top, width, height - top);
    scene.Render(0);
    scene.SetViewport(savedRect[0], savedRect[1], savedRect[2], savedRect[3]);
    scene.SetActiveCamera(savedCamera);
    savedCamera->Release();
    return true;
}

void SendDevelopmentCampaignControls(int actor, const Wire::S_PlayerControls &controls)
{
    if (!g_guest || g_guest->m_Aborted || !g_guest->m_LastUpdate || !CanCampaignSimulate())
        return;
    const auto state = GetCampaignControlSnapshot();
    const auto record = FindCampaignActorControl(actor);
    if (!state || !record || !record->m_Actor || !IsCampaignLocalActor(actor))
        return;

    auto &guest = *g_guest;
    if (guest.m_InputRevision != record->m_ControlRevision) {
        guest.m_InputRevision = record->m_ControlRevision;
        guest.m_PendingButtons = 0;
    }
    guest.m_PendingButtons |= controls.m_Pressed;
    const auto now = Clock::now();
    if (now < guest.m_NextInput)
        return;

    auto packet = controls;
    packet.m_Pressed = guest.m_PendingButtons;
    Wire::S_Input input;
    input.m_Control = {state->m_Context.m_Session, state->m_Context.m_MissionGeneration,
                       guest.m_Agreement.m_Player, record->m_Soldier, *record->m_Actor, record->m_ControlRevision};
    input.m_Sequence = ++guest.m_InputSequence;
    if (Wire::EncodePlayerControls(packet, input.m_Payload) != R::None)
        return;
    if (!SendCampaignMessage(g_netGame, guest.m_Agreement.m_Player, input))
        return;
    guest.m_PendingButtons = 0;
    guest.m_NextInput = now + std::chrono::milliseconds(40);
}

void ApplyDevelopmentCampaignControls(int actor,
    const std::function<void(const Wire::S_PlayerControls *)> &apply)
{
    if (!g_receivedControls || !g_ownership || !CanCampaignSimulate() ||
        !GetGame() || !GetGame()->IsPlayerInputEnabled()) {
        apply(nullptr);
        return;
    }
    auto &input = *g_receivedControls;
    if (input.m_Token.m_Actor.m_Actor.Value() != static_cast<unsigned>(actor) ||
        Clock::now() - input.m_Received > std::chrono::milliseconds(350) ||
        g_ownership->ValidateInput(input.m_Token.m_Player, input.m_Token) != R::None ||
        !g_control->IsSynchronized()) {
        apply(nullptr);
        return;
    }
    DispatchCampaignInput(input.m_Token, [&] { apply(&input.m_Controls); });
    input.m_Controls.m_Pressed = 0;
}

void PublishDevelopmentCampaignWeapon(int actor, Wire::E_WeaponEvent action, int weaponUid,
                                      std::uint16_t rounds, std::uint16_t reloadMs, int consumedAmmo)
{
    if (!g_ownership || !CanCampaignSimulate() || weaponUid < 0)
        return;
    const auto state = g_ownership->GetSnapshot();
    const auto record = FindCampaignActorControl(actor);
    const auto manifest = g_mission->GetManifest();
    if (!state || !record || !record->m_Actor || !manifest)
        return;
    LogDevelopmentCampaign(action == Wire::E_WeaponEvent::Fire ? "host fired weapon" : "host reloaded weapon",
                           record->m_Actor->m_Actor.Value(), rounds);
    if (!g_transferPlayer.Value() || g_liveSendFailed)
        return;

    Wire::S_WeaponEvent event{*record->m_Actor, manifest->m_Snapshot, state->m_Revision, ++g_weaponSequence,
                              action, static_cast<std::uint32_t>(weaponUid), rounds, reloadMs, std::nullopt};
    if (consumedAmmo >= 0)
        event.m_ConsumedAmmo = static_cast<std::uint32_t>(consumedAmmo);
    g_liveSendFailed = !PublishLiveOwnership(*state) || !SendCampaignMessage(g_netGame, g_transferPlayer, event);
}

bool RequestDevelopmentCampaignInventoryMove(int actor, int itemUid, int fromActor,
                                             int fromContainer, int toContainer, int slot, int replacedUid)
{
    if (!CanCampaignSimulate() || !GetGame() || !GetGame()->IsPlayerInputEnabled() ||
        (fromActor != -1 && fromActor != actor) || itemUid < 0 || fromContainer < 0 ||
        fromContainer >= ITEM_CONTAINER_COUNT || toContainer < 0 || toContainer > ITEM_CONTAINER_COUNT ||
        slot < 0 || slot >= 256 || !IsCampaignLocalActor(actor))
        return false;
    if (replacedUid >= 0 && (toContainer != ITEM_CONTAINER_HANDS || fromContainer == toContainer || itemUid == replacedUid))
        return false;
    Wire::S_ItemMove move{static_cast<std::uint32_t>(itemUid), static_cast<std::uint8_t>(fromContainer),
                         static_cast<std::uint8_t>(toContainer), static_cast<std::uint16_t>(slot), std::nullopt};
    if (replacedUid >= 0)
        move.m_ReplacedUid = static_cast<std::uint32_t>(replacedUid);
    if (g_ownership) {
        const auto token = g_ownership->GetControlToken(kHostPlayer);
        if (!token || static_cast<int>(token->m_Actor.m_Actor.Value()) != actor || !g_control->IsSynchronized())
            return false;
        bool moved = false;
        DispatchCampaignInput(*token, [&] { moved = ApplyInventoryMove(*token, move); });
        return moved;
    }
    if (!g_guest || g_guest->m_Aborted || !g_guest->m_LastUpdate)
        return false;
    const auto state = GetCampaignControlSnapshot();
    const auto record = FindCampaignActorControl(actor);
    if (!state || !record || !record->m_Actor)
        return false;
    auto &guest = *g_guest;
    Wire::S_Input input;
    input.m_Control = {state->m_Context.m_Session, state->m_Context.m_MissionGeneration, guest.m_Agreement.m_Player,
                       record->m_Soldier, *record->m_Actor, record->m_ControlRevision};
    input.m_Sequence = ++guest.m_InputSequence;
    input.m_Kind = Wire::E_InputKind::ActorAction;
    return Wire::EncodeItemMove(move, input.m_Payload) == R::None && SendCampaignMessage(g_netGame, guest.m_Agreement.m_Player, input);
}
