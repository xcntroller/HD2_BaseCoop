#include "../../../src/game/campaign_control_native.h"
#include "../../../src/game/campaign_control_runtime.h"
#include "stub/engine.h"
#include <iostream>
#include <cstdlib>
#define CHECK(x) do { ++checks; if (!(x)) { std::cerr << "FAIL native line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while(false)
struct C_View : I_CampaignOwnershipView {
    S_CampaignOwnershipSnapshot m_State;
    T_CampaignOwnershipSnapshot GetSnapshot() const override { return std::make_shared<const S_CampaignOwnershipSnapshot>(m_State); }
};
struct C_Policy : I_CampaignPolicyView {
    S_CampaignPolicySnapshot GetSnapshot() const override {
        return {{{E_CampaignPolicy::SoldierSelection,E_CampaignPolicyState::Permitted,TextKey("fixture")}}};
    }
};
int main() {
    int checks = 0;
    C_View view;
    view.m_State.m_Context.m_Kind = E_CampaignSessionKind::CampaignCoop;
    view.m_State.m_Context.m_Session = SessionId(1);
    view.m_State.m_Context.m_MissionGeneration = MissionGeneration(7);
    view.m_State.m_Context.m_Phase = E_CampaignPhase::Running;
    view.m_State.m_Context.m_IsTransportServer = true;
    view.m_State.m_Context.m_LocalPlayer = PlayerId(4);
    ActorRef actor{SessionId(1), MissionGeneration(7), ActorId(17), ActorSlotUse(3)};
    view.m_State.m_Soldiers.push_back({SoldierId(6), true, true, true, PlayerId(4), actor, false, ControlRevision(9)});
    InstallCampaignControlView(&view);
    C_PlayerController controller;
    controller.m_Type = HUMAN_CONTROLLER_PLAYER;
    controller.m_controlAxes.fill(1); controller.m_controlButtons.fill(1);
    controller.m_axes = controller.m_controlAxes.data(); controller.m_buttons = controller.m_controlButtons.data();
    controller.m_Server.push_back(1); controller.m_Clients.push_back(2);
    C_Human human;
    human.m_Id = 17; human.m_controller = &controller;
    g_actorsModule.m_Human = &human; g_actorsModule.m_Use = 3;
    C_NetObject object;
    g_Replication.m_Object = &object;
    auto *shared = new C_NetObjectMessage;
    shared->refCount = 2; object.m_In.push_back(shared); object.m_Out.push_back(shared);
    int executed = 0;
    C_CampaignNativeControlSink sink([&](const CampaignProtocol::S_Input &) { ++executed; });
    CHECK(sink.Drain(actor));
    CHECK(C_NetObjectMessage::m_Destroyed == 1);
    CHECK(controller.m_Server.empty() && controller.m_Clients.empty());
    CHECK(!controller.m_axes && !controller.m_buttons && controller.m_Bits == 0);
    for (auto axis : controller.m_controlAxes) CHECK(axis == 0);
    for (auto button : controller.m_controlButtons) CHECK(button == 0);
    S_CampaignControllerPlan plan{view.m_State.m_Soldiers[0], E_CampaignController::LocalPlayer, true};
    CHECK(sink.Apply(plan)); CHECK(human.m_Forced); CHECK(human.m_body.m_Local);
    CHECK(human.m_Health == 711 && human.m_Equipment == 42 && human.m_Skills == 83);
    // Retirement is after the model commit: the old binding is no longer in view.
    controller.m_controlAxes.fill(1); controller.m_Server.push_back(3);
    view.m_State.m_Soldiers[0].m_Actor.reset();
    CHECK(sink.Drain(actor));
    CHECK(controller.m_Server.empty());
    for (auto axis : controller.m_controlAxes) CHECK(axis == 0);
    view.m_State.m_Soldiers[0].m_Actor = actor;
    CHECK(sink.Apply(plan));
    g_teamManager.m_Team.m_Member = &human;
    CHECK(sink.Present(actor)); CHECK(g_teamManager.m_Team.m_Selected == &human);
    CHECK(!g_Game.m_Interface.m_Inventory && g_Game.m_Interface.m_Refreshes == 1 && g_Game.m_View.m_Refreshes == 1);
    CHECK(sink.Present(std::nullopt)); CHECK(!g_teamManager.m_Team.m_Selected);
    auto stale = plan; stale.m_Record.m_Actor->m_SlotUse = ActorSlotUse(2);
    CHECK(!sink.Apply(stale));
    auto input = CampaignProtocol::S_Input{{SessionId(1), MissionGeneration(7), PlayerId(4), SoldierId(6), actor, ControlRevision(9)}, 1, CampaignProtocol::E_InputKind::ActorAction, {1}};
    sink.Execute(input); CHECK(executed == 1);
    g_actorsModule.m_Use = 4;
    sink.Execute(input); CHECK(executed == 1);
    g_actorsModule.m_Use = 3;
    view.m_State.m_Soldiers[0].m_ControlRevision = ControlRevision(10);
    sink.Execute(input); CHECK(executed == 1);
    CHECK(!sink.Apply(plan)); // old control revision cannot reinstall old handlers
    view.m_State.m_Soldiers[0].m_Scripted = true;
    plan = C_CampaignControlAdapter::Plan(view.m_State.m_Context, view.m_State.m_Soldiers[0]);
    controller.m_Type = HUMAN_CONTROLLER_AI;
    CHECK(sink.Drain(actor));
    CHECK(sink.Apply(plan)); CHECK(!human.m_Forced); // preserve an executing script
    CHECK(object.m_StateContext == human.m_controller && object.m_FactoryContext == human.m_controller);
    controller.m_controlAxes.fill(1);
    view.m_State.m_Context.m_MissionGeneration = MissionGeneration(8);
    CHECK(sink.Drain(actor)); // never clear an occupant in a later physical world
    CHECK(controller.m_controlAxes[0] == 1);
    InstallCampaignControlView(nullptr);
    // Compose the real model, actual native sink and post-commit reconciliation.
    C_Policy policy;
    S_CampaignSessionSetup setup;
    setup.m_Session=SessionId(81);setup.m_Campaign=CampaignId(1);setup.m_Mission=MissionKey("fixture");
    C_CampaignRequestJournal journal(setup.m_Session);
    C_CampaignOwnership model(C_CampaignSession(setup,{7}),policy,journal);
    CHECK(model.SetAvailability(SoldierId(1),true,true)==E_CampaignReason::None);
    CHECK(model.StartPreparation()==E_CampaignReason::None);
    auto state=model.GetSnapshot();
    S_CampaignOwnershipRequest request;
    request.m_Context={setup.m_Session,PlayerId(1),RequestId(1)};
    request.m_PreparationGeneration=model.GetSession().GetPreparationGeneration();
    request.m_ExpectedOwnershipRevision=state->m_Revision;
    request.m_Operation=S_CampaignClaim{{SoldierId(1),state->m_Soldiers[0].m_ControlRevision}};
    CHECK(model.ApplyRequest(PlayerId(1),request).m_Reason==E_CampaignReason::None);
    CHECK(model.Freeze()==E_CampaignReason::None);
    CHECK(model.BeginMissionGeneration(setup.m_Mission)==E_CampaignReason::None);
    actor={setup.m_Session,model.GetSnapshot()->m_Context.m_MissionGeneration,ActorId(17),ActorSlotUse(3)};
    CHECK(model.BindActor(SoldierId(1),actor)==E_CampaignReason::None);
    controller.m_Type=HUMAN_CONTROLLER_PLAYER;
    C_CampaignNativeControlSink realSink([](const CampaignProtocol::S_Input &) {});
    C_CampaignControlAdapter adapter(model,realSink);
    InstallCampaignControlRuntime(&model,&adapter);
    CHECK(adapter.SynchronizeCommitted()==E_CampaignReason::None);
    CHECK(model.StartSimulation()==E_CampaignReason::None);
    CHECK(adapter.SynchronizeCommitted()==E_CampaignReason::None);
    controller.m_controlAxes.fill(1);controller.m_Server.push_back(9);
    CHECK(model.UnbindActor(SoldierId(1))==E_CampaignReason::None);
    CHECK(adapter.SynchronizeCommitted()==E_CampaignReason::None);
    CHECK(controller.m_Server.empty()&&controller.m_controlAxes[0]==0);
    CHECK(!g_teamManager.m_Team.m_Selected);
    InstallCampaignControlView(nullptr);
    std::cout << "PASS: native sink with instrumented engine: " << checks << " checks\n";
}
