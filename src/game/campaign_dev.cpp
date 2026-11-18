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
#include <deque>

#include "game/actors_module.h"
#include "game/campaign.h"
#include "game/campaign_content.h"
#include "game/campaign_control_native.h"
#include "game/campaign_control_runtime.h"
#include "game/campaign_dev_fixture.h"
#include "game/campaign_dev_world.h"
#include "game/campaign_ownership.h"
#include "game/campaign_setup_session.h"
#include "game/campaign_preparation_native_ui.h"
#include "game/console.h"
#include "game/controls.h"
#include "game/debriefing.h"
#include "game/door.h"
#include "game/game.h"
#include "game/game_app.h"
#include "game/human.h"
#include "game/human_controller.h"
#include "game/items_module.h"
#include "game/main_menu.h"
#include "game/pending.h"
#include "game/profile.h"
#include "game/soldier.h"
#include "game/spawn_point.h"
#include "game/sound_module.h"
#include "game/wire_cut.h"
#include "game/world.h"
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

C_CampaignNode *SelectOriginalMission(bool configureDevelopmentSettings = true)
{
    if (g_campaign->GetCount() <= kOriginalCampaign)
        return nullptr;
    if (configureDevelopmentSettings) {
        g_mpSettings->sessionType = SESSION_NONE;
        g_mpSettings->dedicated = false;
        g_mpSettings->password = "";
        g_mpSettings->unknown174 = 0;
    }
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
std::unique_ptr<C_CampaignOwnership> g_developmentOwnership;
std::unique_ptr<C_CampaignMissionAdapter> g_developmentMission;
C_CampaignOwnership *g_ownership = nullptr;
C_CampaignMissionAdapter *g_mission = nullptr;
std::unique_ptr<C_CampaignSetupSession> g_menuSession;
std::optional<R> g_menuFailure;
std::unique_ptr<C_Campaign> g_menuCampaign;
std::unique_ptr<S_MissionSquad> g_menuSquad;
std::unique_ptr<C_HumanStore> g_menuRoster;
C_Campaign *g_previousCampaign = nullptr;
S_MissionSquad *g_previousSquad = nullptr;
C_HumanStore *g_previousRoster = nullptr;
std::optional<S_PlayMode> g_previousPlayMode;
C_Diary g_campaignDiary;
C_ItemStock g_campaignStock;
C_ItemStock g_campaignFieldStock;
C_ItemStock *g_previousStock = nullptr;
C_ItemStock *g_previousFieldStock = nullptr;
std::array<int, SAVE_GAME_VALUES> g_previousScriptValues{};
std::array<int, SAVE_GAME_VALUES> g_missionStartScriptValues{};
std::vector<S_SquadSoldier> g_missionStartSquad;
std::optional<Wire::S_CampaignReport> g_report;
bool g_reportVisible = false;
bool g_reportDirty = false;
bool g_advanceReport = false;
bool g_reportReadySent = false;
std::uint32_t g_stepOrdinal = 0;
std::deque<Wire::S_CampaignStep> g_pendingSteps;
std::deque<Wire::T_Message> g_pendingWorldMessages;
bool g_continuedPreparation = false;
const C_CampaignScreen *g_continuedEquipment = nullptr;

class C_ContinuedPreparationMenu final : public C_MenuState {
public:
    bool BuildScreens() override { return true; }
    void OnMessage(S_ControlMessage *) override {}

    bool HandleInput(float *, float *pressed) override
    {
        if (m_Preparation && !m_keyboardOff)
            m_Preparation->KeyInput(pressed);
        return true;
    }

    void Update(int timeDelta) override
    {
        auto *model = g_menuSession->GetPreparation();
        if (!m_Preparation && model->GetSnapshot().m_Status == E_CampaignSyncStatus::Current) {
            const auto state = model->GetSnapshot().m_Snapshot;
            if (state && state->m_Context.m_Phase == Phase::Preparation)
                m_Preparation = std::make_unique<C_CampaignPreparationNativeUI>(*model,
                    g_campaign->GetCurrentIndex(), g_campaign->GetMissionNumber(g_campaign->GetCurrentIndex()), true);
        }
        if (m_Preparation) {
            if (!m_Preparation->IsUsable() || m_Preparation->LeaveRequested()) {
                SetState(GAME_STATE_MAIN_MENU);
                return;
            }
            m_Preparation->Update(timeDelta);
            if (!m_keyboardOff && m_cursor)
                m_Preparation->MouseInput(*m_cursor, timeDelta);
        }
        GetSoundModule()->Update(timeDelta);
    }

    bool OnRender() override
    {
        if (m_Preparation && m_Preparation->IsUsable())
            m_Preparation->Render();
        return true;
    }

private:
    std::unique_ptr<C_CampaignPreparationNativeUI> m_Preparation;
};
void StoreDevelopmentControls(const Wire::S_Input &input);
C_CampaignNativeControlSink g_sink(StoreDevelopmentControls);
std::unique_ptr<C_CampaignControlAdapter> g_control;
S_CampaignNativeCatalog g_catalog;
C_CampaignNode *g_step = nullptr;
std::optional<std::pair<PlayerId, int>> g_pendingGuest;
std::optional<Clock::time_point> g_loadingDeadline;
struct S_PeerWorld {
    Wire::C_WorldSender m_Sender;
    bool m_Acknowledged = false;
};
std::map<PlayerId, S_PeerWorld> g_peers;

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
    std::optional<Clock::time_point> m_AbortDeadline;
    std::uint64_t m_LastUpdate = 0;
    std::uint64_t m_LastWeaponEvent = 0;
    std::uint64_t m_LastInventoryEvent = 0;
    std::uint32_t m_HostMissionTime = 0;
    Wire::E_MissionOutcome m_Outcome = Wire::E_MissionOutcome::Running;
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
std::vector<Wire::S_MissionPresentation> g_presentation;
std::vector<std::string> g_trackedFrames;

struct S_DevelopmentMotion {
    std::optional<S_vector> m_Origin;
    std::optional<std::uint16_t> m_Health;
    unsigned m_RecordedCentimeters = 0;
};
std::array<S_DevelopmentMotion, 4> g_squadMotion;

void ObserveSquadState(const Wire::S_MissionUpdate &update, const char *movement, const char *health)
{
    for (const auto &actor : update.m_Squad) {
        const auto id = actor.m_Actor.m_Actor.Value();
        if (id >= g_squadMotion.size())
            continue;
        auto &motion = g_squadMotion[id];
        if (motion.m_Health != actor.m_Health) {
            motion.m_Health = actor.m_Health;
            LogDevelopmentCampaign(health, id, actor.m_Health);
        }
        if (!update.m_PlayerInputEnabled)
            continue;
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
            LogDevelopmentCampaign(movement, id, cm);
        }
    }
}

struct S_ReceivedControls {
    S_CampaignControlToken m_Token;
    Wire::S_PlayerControls m_Controls;
    Clock::time_point m_Received;
    std::uint16_t m_LastMovement = 0;
};
std::map<std::uint32_t, S_ReceivedControls> g_receivedControls;
bool g_loggedInput = false;

bool ApplyInventoryMove(const S_CampaignControlToken &token, Wire::S_ItemMove move);
bool ApplyWorldAction(const S_CampaignControlToken &token, Wire::E_InputKind kind, const Wire::T_Bytes &payload);

void StoreDevelopmentControls(const Wire::S_Input &input)
{
    if (input.m_Kind == Wire::E_InputKind::DoorAction || input.m_Kind == Wire::E_InputKind::WireCut) {
        if (!ApplyWorldAction(input.m_Control, input.m_Kind, input.m_Payload))
            LogDevelopmentCampaign("host rejected guest world action", input.m_Control.m_Actor.m_Actor.Value(),
                                   static_cast<unsigned>(input.m_Kind));
        return;
    }
    if (input.m_Kind == Wire::E_InputKind::ActorAction) {
        Wire::S_ItemMove move;
        if (Wire::DecodeItemMove(input.m_Payload, move) == R::None)
            ApplyInventoryMove(input.m_Control, move);
        return;
    }
    Wire::S_PlayerControls controls;
    if (Wire::DecodePlayerControls(input.m_Payload, controls) != R::None)
        return;
    auto &received = g_receivedControls[input.m_Control.m_Actor.m_Actor.Value()];
    if (received.m_Token.m_Actor == input.m_Control.m_Actor &&
        received.m_Token.m_ControlRevision == input.m_Control.m_ControlRevision &&
        received.m_Token.m_Player == input.m_Control.m_Player)
        controls.m_Pressed |= received.m_Controls.m_Pressed;
    constexpr auto movement = Wire::kMoveForward | Wire::kMoveBack | Wire::kMoveLeft | Wire::kMoveRight;
    const auto heldMovement = static_cast<std::uint16_t>(controls.m_Held & movement);
    if (heldMovement != received.m_LastMovement) {
        LogDevelopmentCampaign("host accepted guest movement controls", input.m_Control.m_Actor.m_Actor.Value(), heldMovement);
    }
    received = {input.m_Control, controls, Clock::now(), heldMovement};
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
    if (!g_menuSession) {
        GetGameApp()->m_state->SetState(GAME_STATE_QUIT);
    } else if (CanCampaignSimulate()) {
        g_menuFailure = reason;
    } else {
        g_guest->m_AbortDeadline = Clock::now() + std::chrono::milliseconds(Wire::kTimeoutMs + 1000);
    }
}

bool MatchesContent(const S_CampaignContentDescriptor &a, const S_CampaignContentDescriptor &b)
{
    return a.m_CampaignContent == b.m_CampaignContent && a.m_MissionContent == b.m_MissionContent &&
        a.m_ItemContent == b.m_ItemContent && a.m_SoldierContent == b.m_SoldierContent && a.m_Rules == b.m_Rules;
}

R ValidateMenuManifest(const S_GuestSession &guest, const S_CampaignMissionManifest &manifest)
{
    const auto frozen = g_menuSession->GetPreparation()->GetSnapshot().m_Snapshot;
    if (!frozen || frozen->m_Context.m_Phase != Phase::Frozen ||
        manifest.m_PreparationGeneration != frozen->m_Generation ||
        manifest.m_FrozenPreparationRevision != frozen->m_PreparationRevision || manifest.m_ViewRevision != frozen->m_ViewRevision)
        return R::StaleRevision;
    if (!(frozen->m_Context.m_MissionGeneration < manifest.m_Context.m_MissionGeneration) ||
        !(frozen->m_OwnershipRevision < manifest.m_OwnershipRevision))
        return R::WrongGeneration;
    std::vector<PlayerId> participants;
    for (const auto &peer : frozen->m_Participants)
        if (peer.m_Connected && peer.m_Admitted)
            participants.push_back(peer.m_Player);
    if (manifest.m_RequiredParticipants != participants ||
        std::find(participants.begin(), participants.end(), guest.m_Agreement.m_Player) == participants.end())
        return R::NotAdmitted;
    if (manifest.m_Soldiers.size() != frozen->m_Soldiers.size() ||
        manifest.m_Bindings.size() != static_cast<std::size_t>(std::count_if(frozen->m_Soldiers.begin(),
            frozen->m_Soldiers.end(), [](const auto &soldier) { return soldier.m_Ownership.m_Deployed; })))
        return R::InvalidSoldier;
    for (std::size_t i = 0; i < manifest.m_Soldiers.size(); ++i) {
        const auto &committed = frozen->m_Soldiers[i];
        const auto &deployed = manifest.m_Soldiers[i];
        if (deployed.m_Soldier != committed.m_Soldier || deployed.m_Profile != committed.m_Profile ||
            deployed.m_Ownership.m_Owner != committed.m_Ownership.m_Owner ||
            deployed.m_Ownership.m_Actor.has_value() != committed.m_Ownership.m_Deployed)
            return R::InvalidSoldier;
    }
    Wire::T_Bytes original, deployed;
    auto comparison = *frozen;
    comparison.m_Loadouts = manifest.m_Loadouts;
    if (Wire::Encode(*frozen, original) != R::None || Wire::Encode(comparison, deployed) != R::None)
        return R::Malformed;
    return original == deployed ? R::None : R::ContentMismatch;
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
        context.m_HostPlayer != kHostPlayer || context.m_LocalPlayer != kHostPlayer)
        return R::Malformed;
    if (g_menuSession)
        return ValidateMenuManifest(guest, manifest);
    if (manifest.m_RequiredParticipants != std::vector<PlayerId>{kHostPlayer, guest.m_Agreement.m_Player})
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
    if (!state || !guest.m_Manifest || !game || !game->GetMission())
        return R::InvalidPhase;
    if (update.m_Session != state->m_Context.m_Session ||
        update.m_Generation != state->m_Context.m_MissionGeneration ||
        update.m_Snapshot != guest.m_Manifest->m_Snapshot)
        return R::WrongGeneration;
    if (update.m_Sequence <= guest.m_LastUpdate || update.m_MissionTime < guest.m_HostMissionTime)
        return R::StaleRevision;
    if (!CanCampaignSimulate())
        return R::InvalidPhase;
    if (update.m_OwnershipRevision != state->m_Revision)
        return R::SnapshotGap;
    auto *mission = game->GetMission();
    const auto bound = std::count_if(state->m_Soldiers.begin(), state->m_Soldiers.end(),
        [](const auto &soldier) { return soldier.m_Actor.has_value(); });
    if (update.m_Squad.size() != static_cast<std::size_t>(bound) ||
        update.m_Objectives.size() != static_cast<std::size_t>(mission->GetObjectiveCount()))
        return R::ContentMismatch;
    if (!g_sink.ApplySquadActors(update.m_Squad) || !g_sink.ApplyAmbientHumans(update.m_Ambient))
        return R::ActorUnavailable;
    ObserveSquadState(update, "guest applied host squad movement cm",
                      "guest applied host squad health");
    for (const auto &event : update.m_Presentation)
        g_scriptModule.ApplyCampaignPresentation(event);
    for (const auto &pose : update.m_FramePoses) {
        auto *frame = game->GetScene()->FindFrame(pose.m_Frame.c_str(), FRAME_MASK_ALL);
        if (frame && !GetFrameActor(frame)) {
            const auto vec = [](const auto &v) { return S_vector(v[0], v[1], v[2]); };
            frame->SetWorldPosDir3(vec(pose.m_Position), vec(pose.m_Direction), vec(pose.m_Up));
            frame->Update();
        }
    }
    if (guest.m_CutsceneCamera && !update.m_CutsceneCamera)
        g_scriptModule.ClearCampaignCutsceneCopies();

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
    if (!guest.m_LastUpdate) {
        LogDevelopmentCampaign("guest applied first live mission state", update.m_MissionTime, update.m_Squad.size());
        LogDevelopmentCampaign("guest applied ambient humans", update.m_Ambient.size());
    }
    guest.m_LastUpdate = update.m_Sequence;
    guest.m_HostMissionTime = update.m_MissionTime;
    if (update.m_Outcome != Wire::E_MissionOutcome::Running) {
        auto ending = std::make_shared<S_CampaignOwnershipSnapshot>(*state);
        ending->m_Context.m_Phase = Phase::Ending;
        const auto reason = guest.m_Control->InstallCommitted(ending);
        if (reason != R::None)
            return reason;
        guest.m_Outcome = update.m_Outcome;
        const bool success = update.m_Outcome == Wire::E_MissionOutcome::Success;
        game->PresentCampaignMissionResult(success);
        LogDevelopmentCampaign("guest applied terminal mission outcome", success, update.m_MissionTime);
    }
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
    if (auto *report = std::get_if<Wire::S_CampaignReport>(&message)) {
        if (!g_menuSession || !guest.m_Manifest || g_stepOrdinal != 0)
            return;
        const auto &manifest = *guest.m_Manifest;
        const Wire::S_ReportKey key{manifest.m_Context.m_Session, manifest.m_Context.m_MissionGeneration,
                                   manifest.m_Snapshot};
        if (!(key == report->m_Report))
            return;
        if (guest.m_Outcome == Wire::E_MissionOutcome::Running ||
            report->m_Success != (guest.m_Outcome == Wire::E_MissionOutcome::Success))
            return FailGuest(R::ContentMismatch);
        g_report = std::move(*report);
        return;
    }
    if (auto *step = std::get_if<Wire::S_CampaignStep>(&message)) {
        if (g_report && step->m_Report == g_report->m_Report &&
            step->m_Ordinal == g_stepOrdinal + g_pendingSteps.size() + 1 && g_pendingSteps.size() < 8)
            g_pendingSteps.push_back(std::move(*step));
        return;
    }
    if (!g_pendingSteps.empty() && (std::holds_alternative<S_CampaignMissionManifest>(message) ||
        std::holds_alternative<Wire::S_WorldChunk>(message))) {
        if (g_pendingWorldMessages.size() <= Wire::kMaxChunks)
            g_pendingWorldMessages.push_back(std::move(message));
        return;
    }
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
        g_report.reset();
        g_reportReadySent = false;
        g_stepOrdinal = 0;
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
        if (reason != R::None && reason != R::StaleRevision && reason != R::WrongGeneration)
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
            g_itemsModule.EnsureItemsLoaded();
            std::vector<S_CampaignNativeSpawn> spawns;
            R result;
            if (g_menuSession) {
                const auto profiles = GetInitialCampaignRoster();
                if (profiles.empty())
                    return R::InvalidSoldier;
                const C_CampaignSession roster(setup, profiles);
                result = ImportCampaignReplicaCatalog(roster, frozen, g_catalog);
                if (result == R::None)
                    result = BuildCampaignNativeSpawns(frozen, roster, g_catalog, spawns);
            } else {
                const C_CampaignSession roster(setup, kRoster);
                result = ImportDevelopmentReplica(roster, kMissionNumber, frozen, g_catalog, spawns);
            }
            if (result != R::None)
                return result;
            g_netGame.MakeReplication();
            if (!startMission())
                return R::SnapshotUnavailable;

            if (g_menuSession) {
                for (const auto &spawn : spawns) {
                    auto *soldier = g_gameProfile.FindSoldierState(spawn.m_State.profileIndex);
                    if (!soldier)
                        soldier = g_missionSquad->AddSoldier();
                    *soldier = spawn.m_State;
                }
            }

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

void PublishTerminalMissionUpdate(const S_CampaignMissionResult &result, const S_CampaignOwnershipSnapshot &state);

R PrepareFrozenMission(const S_CampaignMissionManifest &frozen, const std::map<PlayerId, int> &slots)
{
    g_control = std::make_unique<C_CampaignControlAdapter>(*g_ownership, g_sink);
    InstallCampaignControlRuntime(g_ownership, g_control.get());
    for (const auto &slot : slots)
        if (!BindCampaignTransportPlayer(slot.second, slot.first))
            return R::NotAdmitted;
    SetCampaignMissionAdapter(g_mission);
    InstallCampaignMissionView(g_ownership);

    S_CampaignMissionNativeHooks hooks;
    hooks.m_RetireBeforeReconstruction = [] {
        return GetGame() ? g_control->RetireBeforeReconstruction() : R::None;
    };
    hooks.m_SynchronizeControllers = [] { return g_control->SynchronizeCommitted(); };
    hooks.m_FindTransportPlayer = FindCampaignTransportPlayer;
    hooks.m_Disconnected = HandleCampaignDisconnect;
    hooks.m_CampaignResult = PublishTerminalMissionUpdate;
    InstallCampaignMissionNative(&g_catalog, std::move(hooks));

    const auto reason = g_mission->Begin(frozen);
    if (reason != R::None)
        return reason;
    if (!g_menuSession) {
        g_missionSquad->Clear();
        for (const auto &entry : g_catalog.m_Soldiers)
            *g_missionSquad->AddSoldier() = entry.second;
        return R::None;
    }
    std::vector<S_CampaignNativeSpawn> spawns;
    const auto spawnResult = BuildCampaignNativeSpawns(frozen, g_ownership->GetSession(), g_catalog, spawns);
    if (spawnResult != R::None)
        return spawnResult;
    for (auto *soldier : g_missionSquad->states)
        soldier->deployed = false;
    for (const auto &spawn : spawns) {
        auto *soldier = g_gameProfile.FindSoldierState(spawn.m_State.profileIndex);
        if (!soldier)
            soldier = g_missionSquad->AddSoldier();
        *soldier = spawn.m_State;
        soldier->MergeItems();
        soldier->autoEquip = false;
    }
    g_missionStartSquad.clear();
    for (const auto *soldier : g_missionSquad->states)
        g_missionStartSquad.push_back(*soldier);
    std::copy_n(g_scriptModule.GetSaveGameValues(), g_missionStartScriptValues.size(), g_missionStartScriptValues.begin());
    g_campaignFieldStock.Reset();
    return R::None;
}

R PrepareHostMission(PlayerId guest, int slot)
{
    g_itemsModule.EnsureItemsLoaded();
    S_CampaignMissionManifest frozen;
    const auto reason = FreezeDevelopmentCampaign(*g_ownership, *g_journal, kMissionNumber, guest, g_catalog, frozen);
    if (reason != R::None)
        return reason;
    g_developmentMission = std::make_unique<C_CampaignMissionAdapter>(*g_ownership);
    g_mission = g_developmentMission.get();
    return PrepareFrozenMission(frozen, {{kHostPlayer, g_localPlayerSlot}, {guest, slot}});
}

R SendHostInitialWorld()
{
    const auto state = g_ownership->GetSnapshot();
    for (const auto &soldier : state->m_Soldiers) {
        if (!soldier.m_Deployed)
            continue;
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
    LogDevelopmentCampaign("host captured original initial world", world.size(), Wire::Crc32(world));
    LogDevelopmentCampaign("host sealed campaign preparation", manifest->m_PreparationGeneration.Value(),
                           manifest->m_FrozenPreparationRevision.Value());
    const auto hostAck = g_mission->ExpectedAck(kHostPlayer);
    if (!hostAck)
        return R::SnapshotUnavailable;
    reason = g_mission->Acknowledge(kHostPlayer, *hostAck);
    if (reason != R::None)
        return reason;
    g_peers.clear();
    for (const auto peer : manifest->m_RequiredParticipants) {
        if (peer == kHostPlayer)
            continue;
        const auto ack = g_mission->ExpectedAck(peer);
        if (!ack)
            return R::SnapshotUnavailable;
        if (!SendCampaignMessage(g_netGame, peer, *manifest))
            return R::Disconnected;
        auto &transfer = g_peers[peer];
        reason = transfer.m_Sender.Begin(*ack, world, WorldTimeMs());
        if (reason != R::None)
            return reason;
    }

    g_publishedOwnership = manifest->m_OwnershipRevision;
    g_loadingDeadline = Clock::now() + std::chrono::milliseconds(Wire::kTimeoutMs);
    LogDevelopmentCampaign("host waiting for initial-world reconstruction",
                           state->m_Context.m_MissionGeneration.Value(), GetGame()->GetMissionTime());
    return R::None;
}

bool SetReportReady(PlayerId player, const Wire::S_ReportKey &key)
{
    if (!g_reportVisible || g_advanceReport || !(g_report->m_Report == key))
        return false;
    for (auto &peer : g_report->m_Participants) {
        if (peer.m_Player != player)
            continue;
        if (!peer.m_Ready) {
            peer.m_Ready = true;
            g_reportDirty = true;
            LogDevelopmentCampaign("campaign report player ready", player.Value(), key.m_Generation.Value());
        }
        return true;
    }
    return false;
}

void ReceiveHostWorld(PlayerId player, Wire::T_Message message)
{
    if (const auto *ready = std::get_if<Wire::S_ReportReady>(&message)) {
        SetReportReady(player, ready->m_Report);
        return;
    }
    if (const auto *input = std::get_if<Wire::S_Input>(&message)) {
        if (!g_control)
            return;
        Wire::S_PlayerControls controls;
        Wire::S_ItemMove move;
        Wire::S_DoorAction door;
        std::string fence;
        const bool valid =
            (input->m_Kind == Wire::E_InputKind::ControlBits && Wire::DecodePlayerControls(input->m_Payload, controls) == R::None) ||
            (input->m_Kind == Wire::E_InputKind::ActorAction && Wire::DecodeItemMove(input->m_Payload, move) == R::None) ||
            (input->m_Kind == Wire::E_InputKind::DoorAction && Wire::DecodeDoorAction(input->m_Payload, door) == R::None) ||
            (input->m_Kind == Wire::E_InputKind::WireCut && Wire::DecodeWireCut(input->m_Payload, fence) == R::None);
        if (!valid)
            return;
        const auto result = g_control->Enqueue(player, *input);
        if (result != R::None && result != R::PendingTransaction)
            LogDevelopmentCampaign("host rejected tagged guest controls", player.Value(), static_cast<std::uint64_t>(result));
        return;
    }

    const auto *ack = std::get_if<S_CampaignSnapshotAck>(&message);
    const auto peer = g_peers.find(player);
    if (!ack || !g_mission || peer == g_peers.end() || !peer->second.m_Sender.IsComplete())
        return;
    if (g_mission->GetContext().m_Phase != Phase::Loading || !g_loadingDeadline ||
        Clock::now() >= *g_loadingDeadline)
        return;
    const auto result = g_mission->Acknowledge(player, *ack);
    if (result != R::None) {
        LogDevelopmentCampaign("host rejected snapshot acknowledgment", player.Value(), static_cast<std::uint64_t>(result));
        return;
    }
    peer->second.m_Acknowledged = true;
    LogDevelopmentCampaign("host accepted initial-world acknowledgment", player.Value(), ack->m_Snapshot.Value());
}

std::array<float, 3> VectorFields(const S_vector &value)
{
    return {value.x, value.y, value.z};
}

bool SendHostMissionMessage(const Wire::T_Message &message)
{
    bool sent = true;
    for (const auto &peer : g_peers)
        if (!SendCampaignMessage(g_netGame, peer.first, message))
            sent = false;
    return sent;
}

bool PublishLiveOwnership(const S_CampaignOwnershipSnapshot &state)
{
    if (g_publishedOwnership == state.m_Revision)
        return true;
    if (!SendHostMissionMessage(state))
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
    if (g_peers.empty() || g_liveSendFailed)
        return true;
    const auto state = g_ownership->GetSnapshot();
    const auto manifest = g_mission->GetManifest();
    if (!state || !manifest) {
        g_liveSendFailed = true;
        return false;
    }
    const Wire::S_InventoryEvent event{token.m_Actor, manifest->m_Snapshot, state->m_Revision, ++g_inventorySequence, move};
    g_liveSendFailed = !PublishLiveOwnership(*state) || !SendHostMissionMessage(event);
    return !g_liveSendFailed;
}

constexpr std::array<int, 5> kNativeDoorCommands{
    HUMAN_COMMAND_OPEN_DOOR, HUMAN_COMMAND_CLOSE_DOOR, HUMAN_COMMAND_LOCK_DOOR,
    HUMAN_COMMAND_UNLOCK_DOOR, HUMAN_COMMAND_PICK_LOCK
};

C_Human *FindWorldActionHuman(const S_CampaignControlToken &token)
{
    const auto *game = GetGame();
    if (!game || !game->IsPlayerInputEnabled() || !CanCampaignSimulate())
        return nullptr;
    const auto record = FindCampaignActorControl(static_cast<int>(token.m_Actor.m_Actor.Value()));
    auto *human = record ? ResolveCampaignHuman(*record) : nullptr;
    if (!human || !human->IsAlive() || human->IsUnconscious() || human->GetVehicle() || human->HasOrder() ||
        !human->GetBodyFrame() || !human->GetController())
        return nullptr;
    return human;
}

bool InWorldActionReach(const C_Human &human, const C_Actor &target, float reach)
{
    auto *frame = target.GetFrame();
    if (!target.IsActive() || !frame || !frame->IsOn())
        return false;
    // The local roll menu tests the picked surface. On the host, check its
    // reach against the nearest point in the authored object's local bound.
    I3D_bbox hierarchy;
    const auto *box = I3DGetDriver()->GetHierarchyBBox(hierarchy, frame);
    if (!box || !(box->min.x <= box->max.x && box->min.y <= box->max.y && box->min.z <= box->max.z))
        return false;
    const auto position = human.GetBodyFrame()->GetWorldPos();
    const auto &matrix = frame->GetMatrix();
    S_matrix inverse;
    if (!inverse.Inverse(matrix))
        return false;
    const S_vector local = position * inverse;
    const S_vector nearest(std::clamp(local.x, box->min.x, box->max.x),
                           std::clamp(local.y, box->min.y, box->max.y),
                           std::clamp(local.z, box->min.z, box->max.z));
    return (nearest * matrix - position).Square() <= reach * reach;
}

bool ApplyDoorAction(C_Human &human, const Wire::S_DoorAction &action)
{
    auto *target = g_actorsModule.GetActorById(static_cast<int>(action.m_Door));
    if (!target || target->GetType() != ACTOR_DOOR)
        return false;
    auto &door = *static_cast<C_Door *>(target);
    if (!InWorldActionReach(human, door, door.GetUnknown118() ? 0.65f : 1.8f))
        return false;

    const int native = kNativeDoorCommands[static_cast<std::size_t>(action.m_Action)];
    if ((native == HUMAN_COMMAND_OPEN_DOOR && (door.IsLocked() || door.GetMotion() != DOOR_CLOSED)) ||
        (native == HUMAN_COMMAND_CLOSE_DOOR && (door.IsLocked() || door.GetMotion() != DOOR_OPEN)) ||
        (native == HUMAN_COMMAND_LOCK_DOOR && (door.IsLocked() || door.GetMotion() != DOOR_CLOSED ||
            !human.FindItem(door.GetLockKey()))) ||
        (native == HUMAN_COMMAND_UNLOCK_DOOR && (!door.IsLocked() || !human.FindItem(door.GetLockKey()))) ||
        (native == HUMAN_COMMAND_PICK_LOCK && (!door.IsLocked() || !door.CanPickLock())))
        return false;

    S_HumanCommand command;
    command.category = HUMAN_COMMAND_VEHICLE;
    command.type = native;
    command.param1 = static_cast<int>(action.m_Door);
    if (!human.GetController()->ExecuteCommand(command))
        return false;
    LogDevelopmentCampaign("host accepted door action", action.m_Door, static_cast<unsigned>(action.m_Action));
    return true;
}

bool ApplyWorldAction(const S_CampaignControlToken &token, Wire::E_InputKind kind, const Wire::T_Bytes &payload)
{
    auto *human = FindWorldActionHuman(token);
    if (!human)
        return false;
    if (kind == Wire::E_InputKind::DoorAction) {
        Wire::S_DoorAction action;
        return Wire::DecodeDoorAction(payload, action) == R::None && ApplyDoorAction(*human, action);
    }

    std::string name;
    if (kind != Wire::E_InputKind::WireCut || Wire::DecodeWireCut(payload, name) != R::None || !human->FindWireCutter())
        return false;
    auto *frame = GetGame()->GetScene()->FindFrame(name.c_str(), FRAME_MASK_ALL);
    auto *fence = GetFrameActorAs<C_WireCut>(frame, ACTOR_WIRE_CUT);
    if (!fence || fence->IsCut() || !InWorldActionReach(*human, *fence, 1.8f))
        return false;
    S_HumanCommand command;
    command.category = HUMAN_COMMAND_VEHICLE;
    command.type = HUMAN_COMMAND_CUT_WIRE;
    command.param1 = fence->GetNetId();
    if (!human->GetController()->ExecuteCommand(command))
        return false;
    LogDevelopmentCampaign("host accepted wire cut", fence->GetNetId());
    return true;
}

bool PublishMissionUpdate(Clock::time_point now, const S_CampaignOwnershipSnapshot &state,
                          const S_CampaignMissionResult *terminal = nullptr)
{
    if (g_peers.empty() || (!terminal && (!CanCampaignSimulate() ||
        !g_control->IsSynchronized() || (g_nextMissionUpdate && now < *g_nextMissionUpdate))))
        return true;
    auto *game = GetGame();
    const auto manifest = g_mission->GetManifest();
    if (!game || !game->GetMission() || !manifest)
        return false;
    if (!PublishLiveOwnership(state))
        return false;
    Wire::S_MissionUpdate update;
    update.m_Session = state.m_Context.m_Session;
    update.m_Generation = state.m_Context.m_MissionGeneration;
    update.m_Snapshot = manifest->m_Snapshot;
    update.m_OwnershipRevision = state.m_Revision;
    update.m_Sequence = ++g_missionUpdateSequence;
    update.m_MissionTime = game->GetMissionTime();
    update.m_PlayerInputEnabled = !terminal && game->IsPlayerInputEnabled();
    if (terminal)
        update.m_Outcome = terminal->m_Success ? Wire::E_MissionOutcome::Success : Wire::E_MissionOutcome::Failure;
    if (!terminal && g_scriptModule.IsCutsceneRunning()) {
        if (auto *camera = game->GetScene()->GetActiveCamera()) {
            const auto &matrix = camera->GetMatrix();
            const auto direction = matrix(2).Normalized();
            const auto up = (matrix(1) - direction * matrix(1).Dot(direction)).Normalized();
            update.m_CutsceneCamera = Wire::S_MissionCamera{
                VectorFields(matrix(3)), VectorFields(direction), VectorFields(up), camera->GetFOV()};
            update.m_PlayerInputEnabled = false;
        }
    }
    for (const auto &soldier : state.m_Soldiers) {
        if (!soldier.m_Actor)
            continue;
        Wire::S_SquadActorState actor;
        actor.m_Actor = *soldier.m_Actor;
        if (!g_sink.CaptureSquadActor(actor))
            return false;
        update.m_Squad.push_back(actor);
    }
    if (!g_sink.CaptureAmbientHumans(update.m_Session, update.m_Generation, update.m_Ambient))
        return false;
    if (terminal) {
        update.m_Objectives = terminal->m_Objectives;
    } else {
        const auto *mission = game->GetMission();
        for (int index = 0; index < mission->GetObjectiveCount(); ++index)
            update.m_Objectives.push_back(static_cast<std::uint8_t>(mission->GetObjective(index)->status));
    }
    for (const auto &name : g_trackedFrames) {
        const auto *frame = game->GetScene()->FindFrame(name.c_str(), FRAME_MASK_ALL);
        if (!frame)
            continue;
        const auto &matrix = frame->GetMatrix();
        const auto direction = matrix(2).Normalized();
        const auto up = (matrix(1) - direction * matrix(1).Dot(direction)).Normalized();
        update.m_FramePoses.push_back({name, VectorFields(matrix(3)), VectorFields(direction), VectorFields(up)});
    }
    update.m_Presentation.swap(g_presentation);
    g_nextMissionUpdate = now + std::chrono::milliseconds(66);
    if (!SendHostMissionMessage(update)) {
        Wire::T_Bytes bytes;
        const auto reason = Wire::Encode(update, bytes);
        LogDevelopmentCampaign("host mission update send or encoding failed", update.m_Sequence,
                               static_cast<std::uint64_t>(reason));
        if (reason != R::None) {
            for (const auto &event : update.m_Presentation)
                LogDevelopmentCampaign(event.m_Frame.c_str(), static_cast<unsigned>(event.m_Action), event.m_Args[0]);
        }
        return false;
    }
    ObserveSquadState(update, "host measured authoritative squad movement cm",
                      "host measured authoritative squad health");
    for (std::size_t index = 0; index < update.m_Objectives.size(); ++index) {
        if (index >= g_publishedObjectives.size() || g_publishedObjectives[index] != update.m_Objectives[index])
            LogDevelopmentCampaign("host published objective state", index + 1, update.m_Objectives[index]);
    }
    g_publishedObjectives = update.m_Objectives;
    if (update.m_Sequence == 1) {
        LogDevelopmentCampaign("host published first live mission state", update.m_MissionTime, update.m_Squad.size());
        LogDevelopmentCampaign("host published ambient humans", update.m_Ambient.size());
    }
    return true;
}

void PublishTerminalMissionUpdate(const S_CampaignMissionResult &result, const S_CampaignOwnershipSnapshot &state)
{
    LogDevelopmentCampaign("host captured terminal mission outcome", result.m_Success, result.m_Soldiers.size());
    if (auto *log = std::fopen("campaign-dev.log", "a")) {
        for (const auto &[id, soldier] : result.m_Soldiers) {
            std::fprintf(log, "host captured terminal soldier %u health %d\n",
                         static_cast<unsigned>(id.Value()), soldier.health);
            const auto write = [log](const char *slot, const S_SoldierItem &item) {
                if (item.typeId >= 0 && item.present > 0)
                    std::fprintf(log, "  %s item %d quantity %d\n", slot, item.typeId, item.present);
            };
            write("head", soldier.head);
            write("uniform", soldier.uniform);
            write("hands", soldier.hands);
            write("backpack", soldier.backpack);
            write("long gun", soldier.longGun);
            for (const auto &item : soldier.belt)
                write("belt", item);
            for (const auto &item : soldier.backpackContents)
                write("backpack contents", item);
        }
        std::fclose(log);
    }
    if (!PublishMissionUpdate(Clock::now(), state, &result)) {
        LogDevelopmentCampaign("host could not send terminal mission outcome", result.m_Generation.Value());
        g_netGame.Close(true);
    }
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

void HostDeparture(PlayerId player, R reason)
{
    if (g_pendingGuest && g_pendingGuest->first == player)
        g_pendingGuest.reset();
    if (g_reportVisible && !g_advanceReport) {
        auto &peers = g_report->m_Participants;
        peers.erase(std::remove_if(peers.begin(), peers.end(), [&](const auto &peer) {
            return peer.m_Player == player;
        }), peers.end());
        g_reportDirty = true;
    }
    if (g_control && GetGame() && GetGame()->IsMissionLoaded() &&
        g_mission->GetContext().m_Phase != Phase::Ending) {
        const auto result = ReconcileGuestDeparture(player);
        if (result != R::None || g_mission->GetContext().m_Phase == Phase::Loading)
            g_mission->Abort(result == R::None ? R::Disconnected : result);
    } else {
        g_ownership->OnDisconnected(player);
    }
    if (g_peers.erase(player))
        g_liveSendFailed = false;
    for (auto input = g_receivedControls.begin(); input != g_receivedControls.end();) {
        if (input->second.m_Token.m_Player == player)
            input = g_receivedControls.erase(input);
        else
            ++input;
    }
    LogDevelopmentCampaign("host observed guest departure", player.Value(), static_cast<std::uint64_t>(reason));
}

bool Host(const C_MissionDef &mission, S_CampaignNetConfig config)
{
    const S_CampaignSessionSetup setup{
        config.m_Session, CampaignId(config.m_Session.Value()), MissionKey(mission.GetName()),
        {config.m_Hello.m_Content, kDifficulty}, true
    };
    g_journal = std::make_unique<C_CampaignRequestJournal>(setup.m_Session);
    g_developmentOwnership = std::make_unique<C_CampaignOwnership>(
        C_CampaignSession(setup, kRoster), g_policy, *g_journal);
    g_ownership = g_developmentOwnership.get();
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
    config.m_Departed = HostDeparture;
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

R DeployPreparedMenuMission(const std::function<bool(C_CampaignNode *)> &startMission)
{
    const auto frozen = g_menuSession->GetPreparation()->GetFrozenManifest();
    const auto *catalog = g_menuSession->GetPreparationCatalog();
    if (!frozen || !catalog)
        return R::SnapshotUnavailable;
    g_ownership = g_menuSession->GetHostOwnership();
    g_mission = &g_menuSession->GetMissionAdapter();
    g_catalog = *catalog;
    std::map<PlayerId, int> slots{{kHostPlayer, g_localPlayerSlot}};
    for (const auto player : frozen->m_RequiredParticipants) {
        if (player == kHostPlayer)
            continue;
        const auto slot = g_menuSession->GetPeerSlot(player);
        if (!slot)
            return R::NotAdmitted;
        slots.emplace(player, *slot);
    }
    g_menuSession->SetContinuedPreparationActive(false);
    g_netGame.MakeReplication();
    auto reason = PrepareFrozenMission(*frozen, slots);
    if (reason != R::None)
        return reason;
    if (!startMission(g_step))
        return g_mission->GetFailure() == R::None ? R::SnapshotUnavailable : g_mission->GetFailure();
    return SendHostInitialWorld();
}

R StartMenuDeployment(const std::function<bool(C_CampaignNode *)> &startMission)
{
    auto campaign = std::make_unique<C_Campaign>();
    auto roster = std::make_unique<C_HumanStore>();
    if (!campaign->LoadFiles() || !roster->Load(C_str("tables\\HumanStore.def")))
        return R::ContentMismatch;
    g_menuCampaign = std::move(campaign);
    g_menuSquad = std::make_unique<S_MissionSquad>();
    g_menuRoster = std::move(roster);
    g_previousCampaign = g_campaign;
    g_previousSquad = g_missionSquad;
    g_previousRoster = g_gameProfile.m_humanStore;
    g_previousPlayMode = *g_playMode;
    g_previousStock = g_gameProfile.m_itemStock18;
    g_previousFieldStock = g_gameProfile.m_itemStock1C;
    g_gameProfile.m_itemStock18 = &g_campaignStock;
    g_gameProfile.m_itemStock1C = &g_campaignFieldStock;
    std::copy_n(g_scriptModule.GetSaveGameValues(), g_previousScriptValues.size(), g_previousScriptValues.begin());
    std::fill_n(g_scriptModule.GetSaveGameValues(), g_previousScriptValues.size(), 0);
    g_campaign = g_menuCampaign.get();
    g_missionSquad = g_menuSquad.get();
    g_gameProfile.m_humanStore = g_menuRoster.get();
    g_continuedEquipment = g_campaign->GetInitialEquipmentScreen(kOriginalCampaign, kMissionNumber);
    g_step = SelectOriginalMission(false);
    if (!g_step)
        return R::ContentMismatch;

    if (const auto &guest = g_menuSession->GetGuestAgreement()) {
        const auto slot = g_menuSession->GetPeerSlot(guest->m_Player);
        if (!slot)
            return R::NotAdmitted;
        g_guest = std::make_unique<S_GuestSession>(*guest, *slot);
        g_menuSession->SetMissionCallbacks(ReceiveGuestWorld, [](PlayerId, R reason) {
            if (g_guest)
                g_guest->m_Aborted = true;
            g_menuFailure = reason;
        });
        return R::None;
    }

    g_menuSession->SetMissionCallbacks(ReceiveHostWorld, HostDeparture);
    return DeployPreparedMenuMission(startMission);
}

bool SendProgression(const Wire::T_Message &message)
{
    bool sent = true;
    const auto state = g_ownership->GetSnapshot();
    for (const auto &peer : state->m_Participants)
        if (peer.m_Admitted && peer.m_Connected && peer.m_Player != kHostPlayer)
            sent = SendCampaignMessage(g_netGame, peer.m_Player, message) && sent;
    return sent;
}

R CaptureCampaignReport(const S_CampaignMissionResult &result)
{
    const auto state = g_ownership->GetSnapshot();
    Wire::S_CampaignReport report;
    report.m_Report = {result.m_Session, result.m_Generation, result.m_Snapshot};
    report.m_Success = result.m_Success;
    const auto &scriptValues = result.m_Success ? result.m_ScriptValues : g_missionStartScriptValues;
    std::copy(scriptValues.begin(), scriptValues.end(), report.m_ScriptValues.begin());

    for (const auto &[soldier, terminal] : result.m_Soldiers) {
        const auto profileIndex = g_ownership->GetSession().FindProfile(soldier);
        if (!profileIndex || *profileIndex != terminal.profileIndex ||
            !GetSoldierProfile(*profileIndex) || !g_gameProfile.FindSoldierState(*profileIndex))
            return R::InvalidSoldier;
    }
    for (const auto &[soldier, terminal] : result.m_Soldiers) {
        *g_gameProfile.FindSoldierState(terminal.profileIndex) = terminal;
        GetSoldierProfile(terminal.profileIndex)->dead = terminal.health <= 0;
    }
    if (result.m_Success) {
        C_DebriefingMenu::ApplyCampaignReportResults();
        g_campaignDiary.CommitCurrent();
        std::copy(result.m_ScriptValues.begin(), result.m_ScriptValues.end(), g_scriptModule.GetSaveGameValues());
    } else {
        g_campaignDiary.ClearCurrent();
    }

    for (const auto &participant : state->m_Participants) {
        if (!participant.m_Admitted || !participant.m_Connected)
            continue;
        const bool controls = std::any_of(state->m_Soldiers.begin(), state->m_Soldiers.end(), [&](const auto &soldier) {
            return soldier.m_Alive && soldier.m_Deployed && soldier.m_Owner == participant.m_Player;
        });
        report.m_Participants.push_back({participant.m_Player, participant.m_DisplayName, false, !controls});
    }
    for (const auto &entry : g_ownership->GetSession().GetRoster()) {
        const auto *native = g_gameProfile.FindSoldierState(entry.m_ProfileIndex);
        if (!native)
            continue;
        const auto *profile = GetSoldierProfile(entry.m_ProfileIndex);
        const auto &gains = native->progress;
        auto &soldier = report.m_Soldiers.emplace_back();
        soldier.m_Soldier = entry.m_Soldier;
        soldier.m_Health = native->health;
        soldier.m_Experience = gains.experience;
        soldier.m_Gains = {gains.unknown00, gains.unknown04, gains.endurance, gains.morale,
                           gains.shooting, gains.stealth, gains.firstAid, gains.lockpick};
        soldier.m_Rank = static_cast<std::uint8_t>(profile->unknown24);
        if (native->unknown34 >= 0)
            soldier.m_Promotion = static_cast<std::uint8_t>(native->unknown34);
        if (native->unknown38 >= 0)
            soldier.m_Award = static_cast<std::uint8_t>(native->unknown38);
        for (const int medal : profile->unknown28)
            soldier.m_Medals.push_back(static_cast<std::uint8_t>(medal));
    }
    const auto &diary = g_campaignDiary.GetTexts();
    const auto kept = (std::min)(diary.size(), Wire::kMaxCollection);
    report.m_Diary.assign(diary.end() - kept, diary.end());
    g_report = std::move(report);
    g_reportDirty = true;
    return R::None;
}

R InstallCampaignReport(const Wire::S_CampaignReport &report)
{
    if (!g_guest || !g_guest->m_Manifest)
        return R::InvalidPhase;
    const auto &manifest = *g_guest->m_Manifest;
    const S_CampaignSessionSetup setup{manifest.m_Context.m_Session, manifest.m_Context.m_Campaign,
        manifest.m_Context.m_Mission, manifest.m_Context.m_Rules, true};
    const C_CampaignSession roster(setup, GetInitialCampaignRoster());
    std::vector<std::pair<int, const Wire::S_ReportSoldier *>> rows;
    for (const auto &soldier : report.m_Soldiers) {
        const auto profileIndex = roster.FindProfile(soldier.m_Soldier);
        if (!profileIndex || !GetSoldierProfile(*profileIndex))
            return R::InvalidSoldier;
        rows.emplace_back(*profileIndex, &soldier);
    }
    g_missionSquad->Clear();
    for (const auto &[index, row] : rows) {
        auto *profile = GetSoldierProfile(index);
        auto *soldier = g_missionSquad->AddSoldier();
        soldier->SetProfile(index, 0);
        soldier->health = row->m_Health;
        const auto &p = row->m_Gains;
        soldier->progress = {p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], row->m_Experience};
        soldier->unknown34 = row->m_Promotion ? *row->m_Promotion : -1;
        soldier->unknown38 = row->m_Award ? *row->m_Award : -1;
        profile->dead = row->m_Health <= 0;
        profile->unknown24 = row->m_Rank;
        profile->unknown28.assign(row->m_Medals.begin(), row->m_Medals.end());
    }
    g_campaignDiary.Clear();
    for (const auto id : report.m_Diary)
        g_campaignDiary.AddText(id);
    g_campaignDiary.CommitCurrent();
    std::copy(report.m_ScriptValues.begin(), report.m_ScriptValues.end(), g_scriptModule.GetSaveGameValues());
    return R::None;
}

R RetireMenuWorld()
{
    auto *game = GetGame();
    if (!game || !game->IsMissionLoaded())
        return R::None;
    const auto reason = g_control ? g_control->RetireBeforeReconstruction() :
        g_guest->m_Control->RetireBeforeReconstruction();
    if (reason != R::None)
        return reason;
    // Stop while the campaign view still owns the old world. Native LAN
    // teardown would clear the squad data used by the original report.
    auto *app = GetGameApp();
    app->m_state->Stop();
    delete app->m_state;
    app->m_state = new C_GameState();
    InstallCampaignMissionReconstruction(nullptr);
    SetCampaignMissionAdapter(nullptr);
    InstallCampaignMissionView(nullptr);
    InstallCampaignControlRuntime(nullptr, nullptr);
    g_netGame.DeleteReplication();
    g_control.reset();
    if (g_guest) {
        g_guest->m_Control.reset();
        g_guest->m_Reconstruction.reset();
        g_guest->m_InitialView.m_Snapshot.reset();
        if (g_guest->m_Camera) {
            g_guest->m_Camera->Release();
            g_guest->m_Camera = nullptr;
        }
    }
    g_loadingDeadline.reset();
    g_nextMissionUpdate.reset();
    g_receivedControls.clear();
    g_presentation.clear();
    g_trackedFrames.clear();
    g_publishedObjectives.clear();
    g_squadMotion = {};
    g_loggedInput = false;
    return R::None;
}

R OpenCampaignReport(const std::function<bool(C_CampaignNode *)> &startStep)
{
    const auto *result = GetCampaignMissionResult();
    if (g_guest ? !g_report : !result)
        return R::None;
    auto *node = g_campaign->NextStep();
    if (!node || node->GetType() != CAMPAIGN_NODE_SCREEN || static_cast<C_CampaignScreen *>(node)->GetKind() != 5)
        return R::ContentMismatch;
    auto reason = g_guest ? InstallCampaignReport(*g_report) : CaptureCampaignReport(*result);
    if (reason != R::None)
        return reason;
    reason = RetireMenuWorld();
    if (reason != R::None)
        return reason;

    g_step = node;
    g_reportVisible = true;
    if (!startStep(node))
        return R::SnapshotUnavailable;
    LogDevelopmentCampaign("opened original campaign report", g_report->m_Report.m_Generation.Value(),
                           g_report->m_Success);
    return R::None;
}

Wire::S_CampaignStep DescribeCampaignStep(const C_CampaignNode *node)
{
    using Kind = Wire::E_CampaignStep;
    Wire::S_CampaignStep step;
    step.m_Report = g_report->m_Report;
    step.m_Ordinal = g_stepOrdinal + 1;
    const auto *mission = g_campaign->GetCurrentMission();
    step.m_Mission = mission ? MissionKey(mission->GetName()) : MissionKey("campaign-end");
    if (!node) {
        step.m_Kind = Kind::End;
    } else if (node->GetType() == CAMPAIGN_NODE_VIDEO) {
        step.m_Kind = Kind::Video;
        step.m_Video = static_cast<const C_CampaignVideo *>(node)->GetFileName();
    } else if (node->GetType() == CAMPAIGN_NODE_SCREEN) {
        step.m_Kind = Kind::Screen;
        step.m_Screen = static_cast<const C_CampaignScreen *>(node)->GetKind();
    } else {
        step.m_Kind = Kind::Play;
    }
    return step;
}

void ResetGuestMission()
{
    const auto agreement = g_guest->m_Agreement;
    const int slot = g_guest->m_Slot;
    g_guest = std::make_unique<S_GuestSession>(agreement, slot);
}

bool OpenContinuedPreparation(C_GameApp &app)
{
    app.m_state->Stop();
    delete app.m_state;
    app.m_state = new C_ContinuedPreparationMenu();
    return app.m_state->Start(const_cast<C_CampaignScreen *>(g_continuedEquipment));
}

R StartProgressionStep(C_GameApp &app, C_CampaignNode *node,
                       const std::function<bool(C_CampaignNode *)> &startStep)
{
    g_reportVisible = false;
    g_step = node;
    if (!node) {
        app.m_state->SetState(GAME_STATE_MAIN_MENU);
        return R::None;
    }
    if (node->GetType() == CAMPAIGN_NODE_SCREEN) {
        const auto *screen = static_cast<C_CampaignScreen *>(node);
        if (screen->GetKind() == C_CampaignScreen::KIND_EQUIPMENT || screen->GetKind() == 4) {
            g_continuedEquipment = screen;
            if (!g_guest) {
                const auto reason = g_menuSession->BeginContinuedPreparation(*screen);
                if (reason != R::None)
                    return reason;
            }
            g_continuedPreparation = true;
            return OpenContinuedPreparation(app) ? R::None : R::SnapshotUnavailable;
        }
    }
    if (node->GetType() == CAMPAIGN_NODE_PLAY) {
        g_continuedPreparation = false;
        if (g_guest) {
            ResetGuestMission();
            return R::None;
        }
        return DeployPreparedMenuMission(startStep);
    }
    return startStep(node) ? R::None : R::SnapshotUnavailable;
}

R RetryCampaignMission(C_GameApp &app)
{
    auto *mission = g_campaign->GetCurrentMission();
    if (!mission || !g_continuedEquipment)
        return R::SnapshotUnavailable;
    mission->Reset();
    C_CampaignNode *node = nullptr;
    while ((node = g_campaign->NextStep()) && node->GetType() != CAMPAIGN_NODE_PLAY) {}
    if (!node || g_campaign->GetCurrentMission() != mission)
        return R::ContentMismatch;
    g_reportVisible = false;
    g_continuedPreparation = true;
    g_step = node;
    if (!g_guest) {
        g_missionSquad->Clear();
        for (const auto &saved : g_missionStartSquad) {
            *g_missionSquad->AddSoldier() = saved;
            GetSoldierProfile(saved.profileIndex)->dead = false;
        }
        std::copy(g_missionStartScriptValues.begin(), g_missionStartScriptValues.end(),
                  g_scriptModule.GetSaveGameValues());
        const auto saved = g_mission->GetManifest();
        if (!saved)
            return R::SnapshotUnavailable;
        auto reason = g_menuSession->ReopenPreparation(saved.get());
        if (reason != R::None)
            return reason;
        g_menuSession->SetContinuedPreparationActive(true);
    } else {
        ResetGuestMission();
    }
    return OpenContinuedPreparation(app) ? R::None : R::SnapshotUnavailable;
}

R TickCampaignProgression(C_GameApp &app, const std::function<bool(C_CampaignNode *)> &startStep)
{
    if (GetGame() && !g_reportVisible && !g_continuedPreparation &&
        (g_guest ? g_report && g_stepOrdinal == 0 : GetCampaignMissionResult() != nullptr)) {
        const auto reason = OpenCampaignReport(startStep);
        if (reason != R::None)
            return reason;
    }

    if (g_guest) {
        while (!g_pendingSteps.empty()) {
            auto step = std::move(g_pendingSteps.front());
            g_pendingSteps.pop_front();
            if (g_reportVisible && !std::all_of(g_report->m_Participants.begin(), g_report->m_Participants.end(),
                                              [](const auto &peer) { return peer.m_Ready; }))
                return R::PendingTransaction;
            R reason;
            if (step.m_Kind == Wire::E_CampaignStep::Retry) {
                if (!g_reportVisible || g_report->m_Success)
                    return R::InvalidPhase;
                reason = RetryCampaignMission(app);
            } else {
                auto *node = g_campaign->NextStep();
                const auto local = DescribeCampaignStep(node);
                if (local.m_Kind != step.m_Kind || local.m_Mission != step.m_Mission ||
                    local.m_Screen != step.m_Screen || local.m_Video != step.m_Video)
                    return R::ContentMismatch;
                reason = StartProgressionStep(app, node, startStep);
            }
            if (reason != R::None)
                return reason;
            ++g_stepOrdinal;
        }
        auto pending = std::move(g_pendingWorldMessages);
        g_pendingWorldMessages.clear();
        for (auto &message : pending)
            ReceiveGuestWorld({}, std::move(message));
        return R::None;
    }

    if (g_reportDirty) {
        if (!SendProgression(*g_report))
            return R::Disconnected;
        g_reportDirty = false;
    }
    if (g_reportVisible && g_advanceReport) {
        g_advanceReport = false;
        if (!CanAdvanceLiveCampaignReport())
            return R::None;
        if (!g_report->m_Success) {
            auto step = DescribeCampaignStep(g_step);
            step.m_Kind = Wire::E_CampaignStep::Retry;
            step.m_Screen = 0;
            if (!SendProgression(step))
                return R::Disconnected;
            ++g_stepOrdinal;
            return RetryCampaignMission(app);
        }
        C_DebriefingMenu::CommitCampaignReportProgress();
        LogDevelopmentCampaign("host committed campaign report", g_report->m_Report.m_Generation.Value(),
                               g_missionSquad->states.size());
    } else if (!g_step || (g_step->GetType() != CAMPAIGN_NODE_VIDEO &&
        g_step->GetType() != CAMPAIGN_NODE_SCREEN) || app.m_state->GetState() != GAME_STATE_ENDED) {
        if (!g_continuedPreparation)
            return R::None;
        g_menuSession->Update();
        if (g_menuSession->GetState() == E_CampaignSetupState::Idle)
            return g_menuSession->GetJoinFailure();
        if (!g_menuSession->DeploymentPending())
            return R::None;
        if (g_step->GetType() == CAMPAIGN_NODE_PLAY) {
            g_continuedPreparation = false;
            return DeployPreparedMenuMission(startStep);
        }
    }

    auto *node = g_campaign->NextStep();
    const auto step = DescribeCampaignStep(node);
    if (!SendProgression(step))
        return R::Disconnected;
    ++g_stepOrdinal;
    LogDevelopmentCampaign("host advanced original campaign step", step.m_Ordinal, static_cast<unsigned>(step.m_Kind));
    return StartProgressionStep(app, node, startStep);
}

void RestoreMenuProfile()
{
    if (!g_previousCampaign)
        return;
    g_campaign = g_previousCampaign;
    g_missionSquad = g_previousSquad;
    g_gameProfile.m_humanStore = g_previousRoster;
    g_gameProfile.m_itemStock18 = g_previousStock;
    g_gameProfile.m_itemStock1C = g_previousFieldStock;
    g_campaignStock.Reset();
    g_campaignFieldStock.Reset();
    g_campaignDiary.Clear();
    std::copy(g_previousScriptValues.begin(), g_previousScriptValues.end(), g_scriptModule.GetSaveGameValues());
    *g_playMode = *g_previousPlayMode;
    g_previousCampaign = nullptr;
    g_previousSquad = nullptr;
    g_previousRoster = nullptr;
    g_previousPlayMode.reset();
    g_menuSquad.reset();
    g_menuRoster.reset();
    g_menuCampaign.reset();
}

void ReturnFromMenuDeployment(C_GameApp &app, R reason, bool keepSession)
{
    if (GetGame() && GetGame()->IsMissionLoaded()) {
        if (g_netReplication) {
            if (g_control)
                g_control->RetireBeforeReconstruction();
            else if (g_guest && g_guest->m_Control)
                g_guest->m_Control->RetireBeforeReconstruction();
        }
        app.m_state->Stop();
    } else if (app.m_state->GetTypeId() != GAME_STATE_TYPE_GAME) {
        app.m_state->Stop();
    }
    delete app.m_state;
    app.m_state = nullptr;

    auto session = std::move(g_menuSession);
    session->SetMissionCallbacks({}, {});
    ReleaseDevelopmentCampaign();
    g_netGame.DeleteReplication();
    if (keepSession && session->GetHostOwnership())
        keepSession = session->ReopenPreparation() == R::None;
    if (!keepSession)
        session.reset();
    auto *menu = new C_MainMenu();
    app.m_state = menu;
    menu->RestoreCampaignPreparation(std::move(session), reason);
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
    if (!app.m_state)
        return;
    const auto returnToMenu = [&](R reason, bool keepSession) {
        if (app.m_state->GetTypeId() == GAME_STATE_TYPE_GAME_MENU)
            app.CloseGameMenu();
        ReturnFromMenuDeployment(app, reason, keepSession);
    };
    if (!g_menuSession && IsCampaignNet(E_CampaignLanRoute::CampaignSession) &&
        app.m_state->GetTypeId() == GAME_STATE_TYPE_MAIN_MENU) {
        g_menuSession = static_cast<C_MainMenu *>(app.m_state)->TakeCampaignDeployment();
        if (g_menuSession) {
            const auto reason = StartMenuDeployment([&](auto *step) { return app.StartStep(step, 0); });
            if (reason != R::None) {
                LogDevelopmentCampaign("menu campaign load failed", static_cast<std::uint64_t>(reason));
                returnToMenu(reason, g_menuSession->GetHostOwnership() != nullptr);
            }
            return;
        }
    }
    if (g_menuSession) {
        const int state = app.m_state->GetState();
        if (g_menuFailure || !IsCampaignNet(E_CampaignLanRoute::CampaignSession) ||
            state == GAME_STATE_MAIN_MENU || state == GAME_STATE_LOBBY) {
            returnToMenu(g_menuFailure.value_or(R::None), false);
            return;
        }
        const auto progression = TickCampaignProgression(app, [&](auto *step) { return app.StartStep(step, 0, true); });
        if (progression != R::None) {
            LogDevelopmentCampaign("campaign progression failed", static_cast<unsigned>(progression));
            returnToMenu(progression, false);
            return;
        }
        if (app.m_state->GetState() == GAME_STATE_MAIN_MENU) {
            returnToMenu(R::None, false);
            return;
        }
        if (g_guest && g_continuedPreparation) {
            if (!g_guest->m_World)
                return;
            g_continuedPreparation = false;
        }
        if (g_guest) {
            const auto preparation = g_menuSession->GetPreparation()->GetSnapshot().m_Snapshot;
            if (preparation && preparation->m_Context.m_Phase == Phase::Preparation &&
                g_step && g_step->GetType() == CAMPAIGN_NODE_PLAY) {
                if (g_stepOrdinal == 0) {
                    returnToMenu(R::SnapshotUnavailable, true);
                    return;
                }
                const auto reason = RetireMenuWorld();
                if (reason != R::None || !g_continuedEquipment) {
                    returnToMenu(reason == R::None ? R::InvalidPhase : reason, false);
                    return;
                }
                ResetGuestMission();
                g_continuedPreparation = true;
                if (!OpenContinuedPreparation(app))
                    returnToMenu(R::SnapshotUnavailable, false);
                return;
            }
            if (g_guest->m_AbortDeadline && Clock::now() >= *g_guest->m_AbortDeadline) {
                returnToMenu(R::Timeout, false);
                return;
            }
        }
    } else if (!IsCampaignNet(E_CampaignLanRoute::DevelopmentMission)) {
        return;
    }
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
        for (auto &peer : g_peers) {
            if (reason != R::None)
                break;
            if (!peer.second.m_Sender.IsComplete())
                reason = peer.second.m_Sender.Pump(WorldTimeMs(), [&](const auto &chunk) {
                    return SendCampaignMessage(g_netGame, peer.first, chunk);
                });
        }
        const bool acknowledged = std::all_of(g_peers.begin(), g_peers.end(),
            [](const auto &peer) { return peer.second.m_Acknowledged; });
        if (reason == R::None && acknowledged) {
            S_CampaignStartSimulation start;
            reason = g_mission->Start(start);
            if (reason == R::None && !SendHostMissionMessage(start))
                reason = R::Disconnected;
            if (reason == R::None)
                LogDevelopmentCampaign("host started campaign simulation", start.m_Generation.Value(), start.m_Snapshot.Value());
            if (reason == R::None)
                g_loadingDeadline.reset();
            if (reason == R::None) {
                g_report.reset();
                g_stepOrdinal = 0;
            }
        }
        if (reason != R::None) {
            g_mission->Abort(reason);
            LogDevelopmentCampaign("host aborted initial-world wait", static_cast<std::uint64_t>(reason),
                                   GetGame()->GetMissionTime());
            if (g_menuSession && g_stepOrdinal != 0) {
                const auto retired = RetireMenuWorld();
                const auto reopened = retired == R::None ? g_menuSession->ReopenPreparation() : retired;
                if (reopened != R::None) {
                    returnToMenu(reopened, false);
                } else {
                    g_menuSession->SetContinuedPreparationActive(true);
                    g_continuedPreparation = true;
                    if (!OpenContinuedPreparation(app))
                        returnToMenu(R::SnapshotUnavailable, false);
                }
            } else if (g_menuSession) {
                returnToMenu(reason, true);
            } else {
                app.m_state->SetState(GAME_STATE_QUIT);
            }
        }
        return;
    }
    if (g_mission && g_mission->GetContext().m_Phase == Phase::Running) {
        for (unsigned count = 0; count < 32 && g_control->GetQueuedCount() && g_control->IsSynchronized(); ++count) {
            const auto reason = g_control->ApplyNext();
            if (reason != R::None)
                LogDevelopmentCampaign("host discarded retired guest controls", static_cast<std::uint64_t>(reason));
        }
        if (g_liveSendFailed || !PublishMissionUpdate(Clock::now(), *g_ownership->GetSnapshot())) {
            LogDevelopmentCampaign("host failed to publish mission state", g_missionUpdateSequence);
            if (g_menuSession)
                returnToMenu(R::Disconnected, false);
            else {
                g_netGame.Close(true);
                app.m_state->SetState(GAME_STATE_QUIT);
            }
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
            result = SendHostInitialWorld();
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
    if (!g_ownership && !g_guest && !g_menuSession && !g_menuCampaign)
        return;
    InstallCampaignMissionReconstruction(nullptr);
    InstallCampaignMissionNative(nullptr);
    SetCampaignMissionAdapter(nullptr);
    InstallCampaignMissionView(nullptr);
    InstallCampaignControlRuntime(nullptr, nullptr);
    g_loadingDeadline.reset();
    g_peers.clear();
    g_nextMissionUpdate.reset();
    g_missionUpdateSequence = 0;
    g_weaponSequence = 0;
    g_inventorySequence = 0;
    g_liveSendFailed = false;
    g_publishedOwnership = {};
    g_publishedObjectives.clear();
    g_presentation.clear();
    g_trackedFrames.clear();
    g_squadMotion = {};
    g_receivedControls.clear();
    g_loggedInput = false;
    g_guest.reset();
    g_pendingGuest.reset();
    g_control.reset();
    g_mission = nullptr;
    g_ownership = nullptr;
    g_developmentMission.reset();
    g_developmentOwnership.reset();
    g_journal.reset();
    g_menuFailure.reset();
    g_menuSession.reset();
    RestoreMenuProfile();
    g_catalog = {};
    g_step = nullptr;
    g_report.reset();
    g_reportVisible = g_reportDirty = g_advanceReport = g_reportReadySent = false;
    g_stepOrdinal = 0;
    g_pendingSteps.clear();
    g_pendingWorldMessages.clear();
    g_continuedPreparation = false;
    g_continuedEquipment = nullptr;
    g_missionStartSquad.clear();
}

bool IsLiveCampaignProgression()
{
    return g_menuSession != nullptr;
}

C_Diary *GetLiveCampaignDiary()
{
    return g_menuCampaign ? &g_campaignDiary : nullptr;
}

const Wire::S_CampaignReport *GetLiveCampaignReport()
{
    return g_reportVisible ? &*g_report : nullptr;
}

bool IsLiveCampaignReportHost()
{
    return g_reportVisible && !g_guest;
}

bool IsLocalLiveCampaignReportReady()
{
    if (!g_reportVisible)
        return false;
    const auto local = g_guest ? g_guest->m_Agreement.m_Player : kHostPlayer;
    return g_reportReadySent || std::any_of(g_report->m_Participants.begin(), g_report->m_Participants.end(),
        [&](const auto &peer) { return peer.m_Player == local && peer.m_Ready; });
}

bool CanAdvanceLiveCampaignReport()
{
    return IsLiveCampaignReportHost() && !g_report->m_Participants.empty() &&
        std::all_of(g_report->m_Participants.begin(), g_report->m_Participants.end(),
                    [](const auto &peer) { return peer.m_Ready; });
}

void ReadyLiveCampaignReport()
{
    if (!g_reportVisible || IsLocalLiveCampaignReportReady())
        return;
    if (g_guest) {
        g_reportReadySent = SendCampaignMessage(g_netGame, g_guest->m_Agreement.m_Player,
                                                Wire::S_ReportReady{g_report->m_Report});
    } else {
        SetReportReady(kHostPlayer, g_report->m_Report);
    }
}

void AdvanceLiveCampaignReport()
{
    if (CanAdvanceLiveCampaignReport())
        g_advanceReport = true;
}

void QueueDevelopmentCampaignPresentation(Wire::S_MissionPresentation event)
{
    if (g_peers.empty() || !IsCampaignSimulationAuthority() || !CanCampaignSimulate())
        return;
    if (g_presentation.size() == 64) {
        g_liveSendFailed = true;
        return;
    }
    g_presentation.push_back(std::move(event));
}

void TrackDevelopmentCampaignFrame(const I3D_frame &frame)
{
    if (g_peers.empty() || !IsCampaignSimulationAuthority() || !CanCampaignSimulate() ||
        frame.GetType() == FRAME_CAMERA || GetFrameActor(&frame))
        return;
    const char *name = frame.GetName();
    if (*name && g_trackedFrames.size() < 64 &&
        std::find(g_trackedFrames.begin(), g_trackedFrames.end(), name) == g_trackedFrames.end())
        g_trackedFrames.emplace_back(name);
}

bool IsDevelopmentCampaignCutscene()
{
    return g_guest && g_guest->m_CutsceneCamera && !g_guest->m_Aborted;
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
    const auto received = g_receivedControls.find(static_cast<unsigned>(actor));
    if (received == g_receivedControls.end() || !g_ownership || !CanCampaignSimulate() ||
        !GetGame() || !GetGame()->IsPlayerInputEnabled()) {
        apply(nullptr);
        return;
    }
    auto &input = received->second;
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
    if (g_peers.empty() || g_liveSendFailed)
        return;

    Wire::S_WeaponEvent event{*record->m_Actor, manifest->m_Snapshot, state->m_Revision, ++g_weaponSequence,
                              action, static_cast<std::uint32_t>(weaponUid), rounds, reloadMs, std::nullopt};
    if (consumedAmmo >= 0)
        event.m_ConsumedAmmo = static_cast<std::uint32_t>(consumedAmmo);
    g_liveSendFailed = !PublishLiveOwnership(*state) || !SendHostMissionMessage(event);
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

bool RequestDevelopmentCampaignWorldAction(int actor, const S_HumanCommand &command)
{
    if (command.param1 < 0 || !CanCampaignSimulate() || !IsCampaignLocalActor(actor))
        return false;

    Wire::S_Input input;
    if (command.type == HUMAN_COMMAND_CUT_WIRE) {
        const auto *fence = g_temporaryActors.FindActor(command.param1);
        if (!fence || fence->GetType() != ACTOR_WIRE_CUT || !fence->GetFrame())
            return false;
        input.m_Kind = Wire::E_InputKind::WireCut;
        if (Wire::EncodeWireCut(fence->GetFrame()->GetName(), input.m_Payload) != R::None)
            return false;
    } else {
        const auto requested = std::find(kNativeDoorCommands.begin(), kNativeDoorCommands.end(), command.type);
        if (requested == kNativeDoorCommands.end())
            return false;
        const Wire::S_DoorAction action{static_cast<std::uint32_t>(command.param1),
            static_cast<Wire::E_DoorAction>(requested - kNativeDoorCommands.begin())};
        input.m_Kind = Wire::E_InputKind::DoorAction;
        if (Wire::EncodeDoorAction(action, input.m_Payload) != R::None)
            return false;
    }
    if (g_ownership) {
        const auto token = g_ownership->GetControlToken(kHostPlayer);
        if (!token || token->m_Actor.m_Actor.Value() != static_cast<std::uint32_t>(actor) || !g_control->IsSynchronized())
            return false;
        bool accepted = false;
        DispatchCampaignInput(*token, [&] { accepted = ApplyWorldAction(*token, input.m_Kind, input.m_Payload); });
        return accepted;
    }
    const auto state = GetCampaignControlSnapshot();
    const auto record = FindCampaignActorControl(actor);
    if (!state || !record || !record->m_Actor || !g_guest || g_guest->m_Aborted || !g_guest->m_LastUpdate)
        return false;
    auto &guest = *g_guest;
    input.m_Control = {state->m_Context.m_Session, state->m_Context.m_MissionGeneration, guest.m_Agreement.m_Player,
                       record->m_Soldier, *record->m_Actor, record->m_ControlRevision};
    input.m_Sequence = ++guest.m_InputSequence;
    return SendCampaignMessage(g_netGame, guest.m_Agreement.m_Player, input);
}
