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
int main() {
    int checks = 0;
    C_View view;
    view.m_State.m_Context.m_Kind = E_CampaignSessionKind::CampaignCoop;
    view.m_State.m_Context.m_Session = SessionId(1);
    view.m_State.m_Context.m_MissionGeneration = MissionGeneration(7);
    view.m_State.m_Context.m_Phase = E_CampaignPhase::Running;
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
    plan.m_Controller = E_CampaignController::Scripted; controller.m_Type = HUMAN_CONTROLLER_AI;
    CHECK(sink.Apply(plan)); CHECK(!human.m_Forced); // preserve an executing script
    InstallCampaignControlView(nullptr);
    std::cout << "PASS: native sink with instrumented engine: " << checks << " checks\n";
}
