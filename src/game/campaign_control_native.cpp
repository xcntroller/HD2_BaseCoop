#include "campaign_control_native.h"
#include "campaign_control_runtime.h"
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
C_Human *Resolve(const ActorRef &actor) {
    const auto record = FindCampaignActorControl(static_cast<int>(actor.m_Actor.Value()));
    if (!record || record->m_Actor != actor) return nullptr;
    const int id = static_cast<int>(actor.m_Actor.Value());
    // The initial native spawn adapter reserves the 32 stable player-role actor
    // slots. Dynamic compacted actor IDs have no incarnation counter yet.
    if (id < 0 || id >= C_ActorsModule::PLAYER_SLOTS ||
        static_cast<std::uint8_t>(g_actorsModule.GetPlayerSlotUse(id)) != actor.m_SlotUse.Value()) return nullptr;
    return static_cast<C_Human *>(g_actorsModule.GetHuman(id));
}
}
bool C_CampaignNativeControlSink::Drain(const ActorRef &actor) {
    // A retired generation must never resolve a new occupant of the same slot.
    C_Human *human = Resolve(actor);
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
    if (!human) return false;
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
    human->m_body.SetPlayerControlled(plan.m_LocalView);
    return true;
}
bool C_CampaignNativeControlSink::Present(const std::optional<ActorRef> &actor) {
    auto *team = g_teamManager.GetPlayerTeam();
    C_Human *human = actor ? Resolve(*actor) : nullptr;
    if (actor && !human) return false;
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
