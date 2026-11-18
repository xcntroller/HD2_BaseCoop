#include "campaign_control_native.h"
#include "campaign_control_runtime.h"

#include <algorithm>
#include "game/actors_module.h"
#include "game/human.h"
#include "game/human_body.h"
#include "game/human_controller.h"
#include "game/human_movement.h"
#include "game/controller_internal.h"
#include "game/controller_net.h"
#include "game/game.h"
#include "game/team.h"
#include "game/first_person_view.h"
#include "game/interface.h"
#include "game/pending.h"
#include "net/net_game.h"
#include "net/net_object.h"

namespace {
C_Human *ResolvePhysical(const ActorRef &actor) {
    const auto state = GetCampaignControlSnapshot();
    if (!state || state->m_Context.m_Session != actor.m_Session ||
        state->m_Context.m_MissionGeneration != actor.m_Generation) return nullptr;
    const int id = static_cast<int>(actor.m_Actor.Value());
    // The initial native spawn adapter reserves the 32 stable player-role actor
    // slots. Dynamic compacted actor IDs have no incarnation counter yet.
    if (id < 0 || id >= C_ActorsModule::PLAYER_SLOTS ||
        static_cast<std::uint8_t>(g_actorsModule.GetPlayerSlotUse(id)) != actor.m_SlotUse.Value()) return nullptr;
    return static_cast<C_Human *>(g_actorsModule.GetHuman(id));
}
C_Human *Resolve(const ActorRef &actor) {
    const auto record = FindCampaignActorControl(static_cast<int>(actor.m_Actor.Value()));
    return record && record->m_Actor == actor ? ResolvePhysical(actor) : nullptr;
}
}
bool C_CampaignNativeControlSink::Drain(const ActorRef &actor) {
    // A retired generation must never resolve a new occupant of the same slot.
    C_Human *human = Resolve(actor);
    if (!human) {
        // Unbind has already removed the catalog entry. Retire only an exact
        // binding this sink previously applied, in the same physical world.
        for (const auto &binding : m_AppliedBindings)
            if (binding.second == actor) { human = ResolvePhysical(actor); break; }
    }
    if (!human) return true;
    if (human->m_controller) {
        human->m_controller->ClearMessagesToServer();
        human->m_controller->ClearMessagesToClients();
        human->m_controller->SetControlBits(0);
        human->m_controller->m_axes = nullptr;
        human->m_controller->m_buttons = nullptr;
        if (human->m_controller->GetType() == HUMAN_CONTROLLER_PLAYER) {
            auto *player = static_cast<C_PlayerController *>(human->m_controller);
            for (auto &axis : player->m_controlAxes) axis = 0;
            for (auto &button : player->m_controlButtons) button = 0;
        } else if (human->m_controller->GetType() == HUMAN_CONTROLLER_REMOTE) {
            auto *remote = static_cast<C_RemoteController *>(human->m_controller);
            for (auto &axis : remote->m_controlAxes) axis = 0;
            for (auto &button : remote->m_controlButtons) button = 0;
        }
    }
    if (auto *object = g_netReplication->FindObject(human->GetNetId(), false)) {
        // Messages can be shared by incoming and forwarding queues. Pop drops
        // exactly this queue's reference; delete only after the final owner.
        for (bool outgoing : {true, false})
            while (auto *message = object->PopMessage(outgoing))
                if (message->refCount < 1) delete message;
        object->SetStateHandlers(NetReadControlBits, NetWriteControlBits, nullptr);
        object->SetMessageFactory(NetCreateMessage, nullptr);
    }
    return true;
}
bool C_CampaignNativeControlSink::Apply(const S_CampaignControllerPlan &plan) {
    if (!plan.m_Record.m_Actor) return false;
    C_Human *human = Resolve(*plan.m_Record.m_Actor);
    const auto current = FindCampaignActorControl(static_cast<int>(plan.m_Record.m_Actor->m_Actor.Value()));
    const auto state = GetCampaignControlSnapshot();
    if (!human || !current || !state || current->m_Soldier != plan.m_Record.m_Soldier ||
        current->m_ControlRevision != plan.m_Record.m_ControlRevision) return false;
    const auto expected = C_CampaignControlAdapter::Plan(state->m_Context, *current);
    if (expected.m_Controller != plan.m_Controller || expected.m_LocalView != plan.m_LocalView) return false;
    E_HumanController type = HUMAN_CONTROLLER_REMOTE;
    switch (plan.m_Controller) {
    case E_CampaignController::LocalPlayer:
    case E_CampaignController::RemotePlayer: type = HUMAN_CONTROLLER_PLAYER; break;
    case E_CampaignController::SquadAI: type = HUMAN_CONTROLLER_SQUAD; break;
    case E_CampaignController::Scripted: type = HUMAN_CONTROLLER_AI; break;
    default: break;
    }
    // A disconnect while scripted changes ownership, not script execution.
    const bool preserveScript = type == HUMAN_CONTROLLER_AI && human->m_controller &&
        human->m_controller->GetType() == HUMAN_CONTROLLER_AI;
    human->CreateController(type, !preserveScript);
    if (preserveScript) {
        if (auto *object = g_netReplication->FindObject(human->GetNetId(), false)) {
            object->SetMessageFactory(NetCreateMessage, human->m_controller);
            object->SetStateHandlers(NetReadControlBits, NetWriteControlBits, human->m_controller);
        }
    }
    human->m_body.SetPlayerControlled(plan.m_LocalView);
    m_AppliedBindings[plan.m_Record.m_Soldier] = *plan.m_Record.m_Actor;
    return true;
}
bool C_CampaignNativeControlSink::Present(const std::optional<ActorRef> &actor) {
    auto *team = g_teamManager.GetPlayerTeam();
    C_Human *human = actor ? Resolve(*actor) : nullptr;
    if (actor && (!human || !IsCampaignLocalActor(human->GetNetId()))) return false;
    if (human && !team->IsMember(human)) return false;
    team->SelectCampaignMember(human);
#ifndef HD2_DEDICATED
    GetGame()->GetInterface()->ShowInventory(false, nullptr);
    GetGame()->GetInterface()->RefreshInventory();
    GetGame()->GetFirstPersonView()->Refresh();
#endif
    team->UpdateCameraTarget();
    return true;
}
void C_CampaignNativeControlSink::Execute(const CampaignProtocol::S_Input &input) {
    if (m_Dispatch && Resolve(input.m_Control.m_Actor))
        DispatchCampaignInput(input.m_Control, [&] { m_Dispatch(input); });
}

namespace {
std::array<float, 3> VectorFields(const S_vector &value) {
    return {value.x, value.y, value.z};
}
S_vector4 NativeVector(const std::array<float, 3> &value, float w) {
    return {value[0], value[1], value[2], w};
}
}

bool C_CampaignNativeControlSink::CaptureSquadActor(CampaignProtocol::S_SquadActorState &state) const {
    auto *human = Resolve(state.m_Actor);
    if (!human || !human->GetFrame() || !human->GetController())
        return false;
    const auto *frame = human->GetFrame();
    const auto facing = frame->GetMatrix()(2).Normalized();
    auto view = human->GetViewDir().Normalized();
    if (view.Square() < 0.5f)
        view = facing;
    state.m_Position = VectorFields(frame->GetWorldPos());
    state.m_Facing = VectorFields(facing);
    state.m_ViewDirection = VectorFields(view);
    state.m_Velocity = VectorFields(human->GetVelocity());
    state.m_Health = static_cast<std::uint16_t>(std::clamp(human->GetStat(STAT_HEALTH), 0, 65535));
    state.m_Stance = static_cast<std::uint8_t>(human->GetWork()->GetStance());
    return true;
}

bool C_CampaignNativeControlSink::ApplySquadActors(
    const std::vector<CampaignProtocol::S_SquadActorState> &squad) const {
    std::vector<C_Human *> humans;
    humans.reserve(squad.size());
    for (const auto &actor : squad) {
        auto *human = Resolve(actor.m_Actor);
        if (!human || !human->GetFrame() || !human->GetMovement() || !human->GetController())
            return false;
        humans.push_back(human);
    }
    for (std::size_t i = 0; i < squad.size(); ++i) {
        auto &human = *humans[i];
        const auto &actor = squad[i];
        auto *frame = human.GetFrame();
        const auto pos = NativeVector(actor.m_Position, 1.0f);
        const auto facing = NativeVector(actor.m_Facing, 0.0f);
        const auto view = NativeVector(actor.m_ViewDirection, 0.0f);
        auto &visual = human.GetMovement()->m_visual;
        human.m_stats[STAT_HEALTH] = actor.m_Health;
        if (human.GetWork()->GetStance() != actor.m_Stance)
            human.m_body.SetStance(actor.m_Stance);
        // Only host-authored state can populate these native presentation tracks.
        if (auto *object = g_netReplication->FindObject(human.GetNetId(), false)) {
            if (auto *property = object->GetProperty(1)) {
                property->GetTrack().SetValue(pos);
                property->GetTrack().SetRate(NativeVector(actor.m_Velocity, 0.0f));
                property->GetDirTrack().SetDirection(view, 0.0f);
            }
        }
        if (IsCampaignLocalActor(human.GetNetId())) {
            // The local camera look remains responsive between host state cuts.
            frame->SetPos({pos.x, pos.y, pos.z});
            frame->Update();
            human.SetPosition4(pos);
        } else {
            visual.SetPosition(pos);
            visual.SetDirection(facing, 0.0f);
        }
    }
    return true;
}

bool C_CampaignNativeControlSink::ApplyWeaponEvent(const CampaignProtocol::S_WeaponEvent &event) const {
    auto *human = Resolve(event.m_Actor);
    if (!human)
        return false;
    auto *weapon = human->GetHandsWeapon();
    if (!weapon || weapon->GetUid() != static_cast<int>(event.m_ItemUid) ||
        static_cast<float>(event.m_Rounds) > weapon->GetMaxAmount())
        return false;

    const bool reload = event.m_Action == CampaignProtocol::E_WeaponEvent::Reload;
    if (event.m_ConsumedAmmo) {
        auto *ammo = human->FindItemByUid(static_cast<int>(*event.m_ConsumedAmmo));
        if (!ammo || ammo->GetKind() != ITEM_KIND_AMMO || ammo->GetTypeId() != weapon->GetDef()->GetAmmoItemId())
            return false;
        human->RemoveItem(ammo->GetUid(), true);
    }
    weapon->ApplyReplicaRounds(human, event.m_Rounds, event.m_ReloadTime, reload);
    if (!reload)
        weapon->PlayShotSound(human, ShotFrameOf(weapon), false);
    human->ExecuteBodyCommand(MakeBodyCommand(BODY_COMMAND_ACTION, reload ? BODY_ACTION_RELOAD : BODY_ACTION_FIRE));
#ifndef HD2_DEDICATED
    if (IsCampaignLocalActor(human->GetNetId())) {
        const int effect = reload ? FPV_EVENT_RELOAD : (event.m_Rounds ? FPV_EVENT_FIRE : FPV_EVENT_FIRE_LAST);
        GetGame()->GetFirstPersonView()->OnInventoryEvent(effect, 0);
    }
#endif
    return true;
}
