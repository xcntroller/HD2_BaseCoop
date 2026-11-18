#include "campaign_control_native.h"
#include "campaign_control_runtime.h"
#include "game/actors_module.h"
#include "game/human.h"
#include "game/human_controller.h"
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
    return static_cast<C_Human *>(g_actorsModule.GetHuman(static_cast<int>(actor.m_Actor.Value())));
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
    }
    if (auto *object = g_netReplication->FindObject(human->GetNetId(), false)) {
        object->ClearMessages(false, true, true);
        object->ClearMessages(false, false, true);
        object->SetStateHandlers(nullptr, nullptr, nullptr);
        object->SetMessageFactory(nullptr, nullptr);
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
    if (m_Dispatch) DispatchCampaignInput(input.m_Control, [&] { m_Dispatch(input); });
}
