#include "../../../src/game/campaign_control_adapter.h"
#include "../../../src/game/campaign_control_runtime.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>

#ifdef _MSC_VER
#ifndef _M_IX86
#error Intended MSVC target is x86
#endif
static_assert(sizeof(void *) == 4, "intended target pointer width");
#endif

namespace {
int g_Checks = 0;
#define CHECK(value) do { ++g_Checks; if (!(value)) { std::cerr << "FAIL line " << __LINE__ << ": " #value "\n"; std::exit(1); } } while (false)
using R = E_CampaignReason;
using O = E_CampaignRequestOutcome;
struct C_Policy final : I_CampaignPolicyView {
    S_CampaignPolicySnapshot m_State;
    C_Policy() {
        m_State.m_Decisions = {{E_CampaignPolicy::SoldierSelection, E_CampaignPolicyState::Permitted, TextKey("fixture.selection")},
            {E_CampaignPolicy::SwitchingAndSpectating, E_CampaignPolicyState::Permitted, TextKey("fixture.switch")}};
    }
    S_CampaignPolicySnapshot GetSnapshot() const override { return m_State; }
};
S_CampaignSessionSetup Setup() {
    S_CampaignSessionSetup setup;
    setup.m_Session = SessionId(901);
    setup.m_Campaign = CampaignId(700);
    setup.m_Mission = MissionKey("base.sicily2.step1.v1");
    setup.m_Rules.m_Difficulty = 3;
    setup.m_Rules.m_Content.m_Rules = "fixture.campaign";
    return setup;
}
const S_CampaignOwnershipRecord &Record(const S_CampaignOwnershipSnapshot &state, SoldierId soldier) {
    for (const auto &record : state.m_Soldiers) if (record.m_Soldier == soldier) return record;
    throw std::logic_error("test soldier missing");
}
void Invariants(const C_CampaignOwnership &model, std::size_t squadLimit = 4) {
    auto state = model.GetSnapshot();
    CHECK(state->m_Participants.size() <= 4);
    std::set<PlayerId> owners;
    std::set<SoldierId> soldiers;
    std::set<std::uint32_t> actors;
    std::size_t deployed = 0;
    for (const auto &record : state->m_Soldiers) {
        CHECK(record.m_Soldier.IsValid());
        CHECK(soldiers.insert(record.m_Soldier).second);
        CHECK(record.m_ControlRevision.IsValid());
        if (record.m_Deployed) ++deployed;
        CHECK(!record.m_Deployed || record.m_Eligible);
        if (record.m_Owner) {
            CHECK(record.m_Alive && record.m_Eligible);
            CHECK(owners.insert(*record.m_Owner).second);
            CHECK(model.FindSoldier(*record.m_Owner) == record.m_Soldier);
            CHECK(model.FindOwner(record.m_Soldier) == record.m_Owner);
            bool admitted = false;
            for (const auto &p : state->m_Participants)
                if (p.m_Player == record.m_Owner) admitted = p.m_Connected && p.m_Admitted;
            CHECK(admitted);
        }
        if (record.m_Actor) {
            CHECK(actors.insert(record.m_Actor->m_Actor.Value()).second);
            CHECK(model.ResolveActor(*record.m_Actor) == record.m_Soldier);
        }
    }
    CHECK(deployed <= squadLimit);
    for (const auto &p : state->m_Participants)
        if (auto soldier = model.FindSoldier(p.m_Player)) CHECK(model.FindOwner(*soldier) == p.m_Player);
}
struct C_Fixture {
    C_Policy m_Policy;
    C_CampaignRequestJournal m_Journal{SessionId(901), 2};
    C_CampaignOwnership m_Model{C_CampaignSession(Setup(), {19, 0, 7, 42, 83}), m_Policy, m_Journal};
    std::map<PlayerId, std::uint64_t> m_RequestIds;
    PlayerId m_Host{1};
    PlayerId m_Guest;
    SoldierId m_A{1}, m_B{2}, m_C{3}, m_D{4}, m_E{5};
    C_Fixture() {
        CHECK(m_Model.Admit("Guest", m_Guest) == R::None);
        CHECK(m_Guest == PlayerId(2));
        for (auto soldier : {m_A, m_B, m_C, m_D}) CHECK(m_Model.SetAvailability(soldier, true, true) == R::None);
        CHECK(m_Model.SetAvailability(m_E, true, false) == R::None);
        CHECK(m_Model.StartPreparation() == R::None);
    }
    S_CampaignOwnershipTarget Target(SoldierId soldier) const {
        auto state = m_Model.GetSnapshot();
        return {soldier, Record(*state, soldier).m_ControlRevision};
    }
    S_CampaignOwnershipRequest Request(PlayerId player, T_CampaignOwnershipOperation operation) {
        auto state = m_Model.GetSnapshot();
        S_CampaignOwnershipRequest request;
        request.m_Context = {state->m_Context.m_Session, player, RequestId(++m_RequestIds[player])};
        if (state->m_Context.m_Phase == E_CampaignPhase::Preparation)
            request.m_PreparationGeneration = m_Model.GetSession().GetPreparationGeneration();
        if (state->m_Context.m_Phase == E_CampaignPhase::Running)
            request.m_MissionGeneration = state->m_Context.m_MissionGeneration;
        request.m_ExpectedOwnershipRevision = state->m_Revision;
        request.m_Operation = std::move(operation);
        return request;
    }
    S_CampaignRequestResult Claim(PlayerId player, SoldierId soldier) {
        auto result = m_Model.ApplyRequest(player, Request(player, S_CampaignClaim{Target(soldier)}));
        Invariants(m_Model);
        return result;
    }
    void BindAll() {
        const auto state = m_Model.GetSnapshot();
        std::uint32_t actorId = 0;
        for (const auto &record : state->m_Soldiers) {
            if (!record.m_Deployed) continue;
            ActorRef actor{state->m_Context.m_Session, state->m_Context.m_MissionGeneration,
                ActorId(actorId), ActorSlotUse(0)};
            actorId += 17; // intentionally unrelated to owner IDs and roster IDs
            CHECK(m_Model.BindActor(record.m_Soldier, actor) == R::None);
            CHECK(m_Model.CompleteControlHandoff(Target(record.m_Soldier)) == R::None);
        }
    }
    void Run() {
        auto frozenCut = m_Model.GetSnapshot();
        CHECK(m_Model.Freeze() == R::None);
        CHECK(m_Model.GetSnapshot()->m_Revision == frozenCut->m_Revision);
        for (const auto &record : frozenCut->m_Soldiers)
            CHECK(Target(record.m_Soldier).m_ExpectedControlRevision == record.m_ControlRevision);
        CHECK(m_Model.BeginMissionGeneration(MissionKey("base.sicily2.step1.v1")) == R::None);
        CHECK(m_Model.StartSimulation() == R::ActorUnavailable);
        BindAll();
        auto acknowledgedCut = m_Model.GetSnapshot();
        CHECK(m_Model.StartSimulation() == R::None);
        CHECK(m_Model.GetSnapshot()->m_Revision == acknowledgedCut->m_Revision);
        for (const auto &record : acknowledgedCut->m_Soldiers)
            CHECK(Target(record.m_Soldier).m_ExpectedControlRevision == record.m_ControlRevision);
        Invariants(m_Model);
    }
};

struct C_Sink : I_CampaignControlSink {
    std::vector<std::string> m_Events;
    std::map<SoldierId, E_CampaignController> m_Roles;
    std::optional<ActorRef> m_View;
    int m_Executed = 0;
    bool m_Fail = false;
    bool Drain(const ActorRef &) override { m_Events.push_back("drain"); return !m_Fail; }
    bool Apply(const S_CampaignControllerPlan &plan) override {
        m_Events.push_back("apply"); m_Roles[plan.m_Record.m_Soldier] = plan.m_Controller; return !m_Fail;
    }
    bool Present(const std::optional<ActorRef> &actor) override {
        m_Events.push_back("present"); m_View = actor; return !m_Fail;
    }
    void Execute(const CampaignProtocol::S_Input &) override { ++m_Executed; }
};
CampaignProtocol::S_Input Input(C_Fixture &f, PlayerId player, std::uint64_t seq,
    CampaignProtocol::E_InputKind kind = CampaignProtocol::E_InputKind::ControlBits) {
    auto token = f.m_Model.GetControlToken(player);
    CHECK(token);
    return {*token, seq, kind, {1, 2, 3}};
}
void TransfersAndQueues() {
    C_Fixture f;
    CHECK(f.Claim(f.m_Host, f.m_A).m_Reason == R::None);
    CHECK(f.Claim(f.m_Guest, f.m_B).m_Reason == R::None);
    f.Run();
    C_Sink sink;
    C_CampaignControlAdapter adapter(f.m_Model, sink);
    CHECK(adapter.Enqueue(f.m_Guest, Input(f, f.m_Guest, 1)) == R::PendingTransaction);
    CHECK(adapter.SynchronizeCommitted() == R::None);
    CHECK(sink.m_Roles[f.m_A] == E_CampaignController::LocalPlayer);
    CHECK(sink.m_Roles[f.m_B] == E_CampaignController::RemotePlayer);
    CHECK(sink.m_Roles[f.m_C] == E_CampaignController::SquadAI);
    CHECK(sink.m_View == Record(*f.m_Model.GetSnapshot(), f.m_A).m_Actor);
    bool applying = false;
    for (const auto &e : sink.m_Events) { if (e == "apply") applying = true; if (e == "drain") CHECK(!applying); }
    CHECK(sink.m_Events.back() == "present");
    std::uint64_t sequence = 0;
    for (auto kind : {CampaignProtocol::E_InputKind::ControlBits, CampaignProtocol::E_InputKind::ActorAction,
                     CampaignProtocol::E_InputKind::VehicleAction}) {
        auto input = Input(f, f.m_Guest, ++sequence, kind);
        CHECK(adapter.Enqueue(f.m_Guest, input) == R::None);
        CHECK(adapter.Enqueue(f.m_Guest, input) == R::StaleRevision);
        CHECK(adapter.ApplyNext() == R::None);
    }
    CHECK(sink.m_Executed == 3);
    auto old = Input(f, f.m_Guest, ++sequence);
    CHECK(adapter.Enqueue(f.m_Guest, old) == R::None);
    auto request = f.Request(f.m_Guest, S_CampaignSwitch{f.Target(f.m_B), f.Target(f.m_C)});
    CHECK(f.m_Model.ApplyRequest(f.m_Guest, request).m_Reason == R::None);
    CHECK(adapter.ApplyNext() == R::StaleRevision);
    CHECK(sink.m_Executed == 3);
    sink.m_Fail = true;
    CHECK(adapter.SynchronizeCommitted() == R::PendingTransaction);
    CHECK(!f.m_Model.GetControlToken(f.m_Guest));
    sink.m_Fail = false;
    CHECK(adapter.SynchronizeCommitted() == R::None);
    CHECK(sink.m_Roles[f.m_B] == E_CampaignController::SquadAI);
    CHECK(sink.m_Roles[f.m_C] == E_CampaignController::RemotePlayer);
    CHECK(adapter.Enqueue(f.m_Guest, old) == R::StaleRevision);
    auto current = Input(f, f.m_Guest, 1);
    CHECK(adapter.Enqueue(f.m_Guest, current) == R::None);
    CHECK(adapter.OnDisconnected(f.m_Guest) == R::None);
    CHECK(adapter.GetQueuedCount() == 0);
    CHECK(sink.m_Roles[f.m_C] == E_CampaignController::SquadAI);
    CHECK(adapter.OnDisconnected(f.m_Guest) == R::None);
    CHECK(adapter.Enqueue(f.m_Guest, current) == R::NotAdmitted);
    CHECK(Record(*f.m_Model.GetSnapshot(), f.m_C).m_Actor == current.m_Control.m_Actor);
}
void ScriptDeathCapacity() {
    C_Fixture f;
    CHECK(f.Claim(f.m_Guest, f.m_A).m_Reason == R::None);
    f.Run();
    C_Sink sink;
    C_CampaignControlAdapter adapter(f.m_Model, sink);
    CHECK(adapter.SynchronizeCommitted() == R::None);
    const auto old = Input(f, f.m_Guest, 1);
    CHECK(adapter.Enqueue(f.m_Guest, old) == R::None);
    CHECK(f.m_Model.SetScripted(f.m_A, true) == R::None);
    CHECK(adapter.ApplyNext() == R::StaleRevision);
    CHECK(adapter.SynchronizeCommitted() == R::None);
    CHECK(sink.m_Roles[f.m_A] == E_CampaignController::Scripted);
    CHECK(f.m_Model.SetScripted(f.m_A, false) == R::None);
    CHECK(adapter.SynchronizeCommitted() == R::None);
    for (std::size_t i = 1; i <= C_CampaignControlAdapter::kMaxQueuedInputs; ++i)
        CHECK(adapter.Enqueue(f.m_Guest, Input(f, f.m_Guest, i)) == R::None);
    const auto retry = Input(f, f.m_Guest, 257);
    CHECK(adapter.Enqueue(f.m_Guest, retry) == R::CapacityExceeded);
    CHECK(adapter.ApplyNext() == R::None);
    CHECK(adapter.Enqueue(f.m_Guest, retry) == R::None);
    CHECK(f.m_Model.OnDeath(f.m_A) == R::None);
    CHECK(adapter.ApplyNext() == R::StaleRevision);
    CHECK(adapter.SynchronizeCommitted() == R::None);
    CHECK(sink.m_Roles[f.m_A] == E_CampaignController::Suspended);
    CHECK(adapter.GetQueuedCount() == 0);
}
void RuntimeAndGuestPlan() {
    CHECK(!CampaignControlsActive());
    CHECK(!IsCampaignLocalActor(17));
    C_Fixture f;
    CHECK(f.Claim(f.m_Host, f.m_A).m_Reason == R::None);
    CHECK(f.Claim(f.m_Guest, f.m_B).m_Reason == R::None);
    f.Run();
    InstallCampaignControlView(&f.m_Model);
    CHECK(CampaignControlsActive());
    CHECK(BindCampaignTransportPlayer(6, f.m_Guest));
    CHECK(!BindCampaignTransportPlayer(7, f.m_Guest));
    CHECK(FindCampaignPlayerSlot(17) == 6);
    CHECK(!FindCampaignPlayerSlot(34)); // AI actor, no fake connection
    CHECK(IsCampaignLocalActor(0));
    CHECK(!IsCampaignLocalActor(17));
    auto snapshot = *f.m_Model.GetSnapshot();
    snapshot.m_Context.m_IsTransportServer = false;
    snapshot.m_Context.m_LocalPlayer = f.m_Guest;
    CHECK(C_CampaignControlAdapter::Plan(snapshot.m_Context, Record(snapshot, f.m_B)).m_LocalView);
    CHECK(C_CampaignControlAdapter::Plan(snapshot.m_Context, Record(snapshot, f.m_A)).m_Controller == E_CampaignController::Replica);
    CHECK(C_CampaignControlAdapter::Plan(snapshot.m_Context, Record(snapshot, f.m_C)).m_Controller == E_CampaignController::Replica);
    CHECK(f.m_Model.OnDisconnected(f.m_Guest) == R::None);
    CHECK(!FindCampaignPlayerSlot(17));
    ForgetCampaignTransportPlayer(f.m_Guest);
    CHECK(!FindCampaignActorControl(-1));
    InstallCampaignControlView(nullptr);
    CHECK(!CampaignControlsActive());
}
}
int main() {
    TransfersAndQueues(); ScriptDeathCapacity(); RuntimeAndGuestPlan();
    std::cout << "PASS: controller adapters: " << g_Checks << " checks\n";
}
