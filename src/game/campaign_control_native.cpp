#include "campaign_control_native.h"
#include "campaign_control_runtime.h"

#include <algorithm>

#include "game/actors_module.h"
#include "game/human.h"
#include "game/human_controller.h"
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
}

C_Human *C_CampaignNativeControlSink::Resolve(const ActorRef &actor) const {
    const auto record = FindCampaignActorControl(static_cast<int>(actor.m_Actor.Value()));
    return record && record->m_Actor == actor ? ResolvePhysical(actor) : nullptr;
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
    human->m_body.SetPlayerControlled(plan.m_LocalView || plan.m_Controller == E_CampaignController::RemotePlayer);
    m_AppliedBindings[plan.m_Record.m_Soldier] = *plan.m_Record.m_Actor;
    return true;
}
bool C_CampaignNativeControlSink::Present(const std::optional<ActorRef> &actor) {
    auto *team = g_teamManager.GetPlayerTeam();
    C_Human *human = actor ? Resolve(*actor) : nullptr;
    if (actor && (!human || !IsCampaignLocalActor(human->GetNetId()))) return false;
    if (human && !team->IsMember(human)) return false;
    team->SelectCampaignMember(human);
    if (!human) {
        const auto state = GetCampaignControlSnapshot();
        if (state && state->m_Context.m_Phase == E_CampaignPhase::Running) {
            const auto owning = std::any_of(state->m_Soldiers.begin(), state->m_Soldiers.end(), [&](const auto &soldier) {
                return soldier.m_Alive && soldier.m_Deployed && soldier.m_Owner &&
                    soldier.m_Owner == state->m_Context.m_LocalPlayer;
            });
            if (!owning) {
                for (const auto &soldier : state->m_Soldiers) {
                    auto *watch = soldier.m_Alive && soldier.m_Actor ? Resolve(*soldier.m_Actor) : nullptr;
                    if (watch) {
                        team->SetWatchedHuman(watch);
                        break;
                    }
                }
            }
        }
    }
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
