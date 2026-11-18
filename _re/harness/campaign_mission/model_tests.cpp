#include "../../../src/game/campaign_mission_adapter.h"
#include "../../../src/game/campaign_control_adapter.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#ifdef _MSC_VER
static_assert(sizeof(void *) == 4 && _M_IX86, "intended x86 target");
#endif
namespace {
int g_Checks = 0;
#define CHECK(x) do { ++g_Checks; if (!(x)) throw std::runtime_error(#x); } while(false)
using R = E_CampaignReason;
using P = E_CampaignPhase;
struct Policy : I_CampaignPolicyView {
    S_CampaignPolicySnapshot GetSnapshot() const override {
        return {{{E_CampaignPolicy::SoldierSelection, E_CampaignPolicyState::Permitted, TextKey("fixture")},
            {E_CampaignPolicy::SwitchingAndSpectating, E_CampaignPolicyState::Permitted, TextKey("fixture")}}};
    }
};
S_CampaignSessionSetup Setup() {
    S_CampaignSessionSetup setup{SessionId(90),CampaignId(8),MissionKey("original/step/version"),{},true};
    setup.m_Rules.m_Difficulty=2;
    setup.m_Rules.m_Content={"sha256:"+std::string(64,'a'),"sha256:"+std::string(64,'b'),
        "sha256:"+std::string(64,'c'),"sha256:"+std::string(64,'d'),"sha256:"+std::string(64,'e')};
    return setup;
}
struct Fixture {
    Policy policy;
    C_CampaignRequestJournal journal{SessionId(90)};
    C_CampaignOwnership ownership{C_CampaignSession(Setup(), {17,0,9,4}), policy, journal};
    C_CampaignMissionAdapter adapter{ownership};
    PlayerId guest;
    S_CampaignMissionManifest frozen;
    explicit Fixture(bool claimHumans=false) {
        CHECK(ownership.Admit("guest", guest) == R::None);
        for (auto s : {SoldierId(1),SoldierId(2),SoldierId(3),SoldierId(4)}) CHECK(ownership.SetAvailability(s,true,true) == R::None);
        CHECK(ownership.StartPreparation() == R::None);
        if(claimHumans) for(auto peer:{PlayerId(1),guest}) {
            const auto state=ownership.GetSnapshot();const auto &soldier=state->m_Soldiers[peer==PlayerId(1)?0:1];
            S_CampaignOwnershipRequest r;r.m_Context={SessionId(90),peer,RequestId(1)};
            r.m_PreparationGeneration=ownership.GetSession().GetPreparationGeneration();r.m_ExpectedOwnershipRevision=state->m_Revision;
            r.m_Operation=S_CampaignClaim{{soldier.m_Soldier,soldier.m_ControlRevision}};
            CHECK(ownership.ApplyRequest(peer,r).m_Reason==R::None);
        }
        CHECK(ownership.Freeze() == R::None);
        auto state = ownership.GetSnapshot();
        frozen.m_Context = state->m_Context;
        frozen.m_PreparationGeneration = ownership.GetSession().GetPreparationGeneration();
        frozen.m_FrozenPreparationRevision = PreparationRevision(3);
        frozen.m_ViewRevision = ViewRevision(7); frozen.m_OwnershipRevision = state->m_Revision;
        frozen.m_RequiredParticipants = {PlayerId(1),guest};
        for (auto s : state->m_Soldiers) {
            S_CampaignSoldierPresentation p; p.m_Soldier = s.m_Soldier; p.m_Ownership = s;
            frozen.m_Soldiers.push_back(p);
            S_CampaignSoldierLoadout l; l.m_Soldier = s.m_Soldier;
            frozen.m_Loadouts.push_back(l);
        }
    }
    void BindAll(bool handoff = true) {
        // Actor identity deliberately differs from roster ID and player slots.
        for (int i = 0; i < 4; ++i) {
            const SoldierId s(static_cast<unsigned>(i+1));
            const auto c = adapter.GetContext();
            CHECK(adapter.Bind(s,{c.m_Session,c.m_MissionGeneration,ActorId(static_cast<unsigned>(20+i)),ActorSlotUse(0)}) == R::None);
            if (handoff) {
                const auto state = ownership.GetSnapshot();
                CHECK(ownership.CompleteControlHandoff({s,state->m_Soldiers[static_cast<unsigned>(i)].m_ControlRevision}) == R::None);
            }
        }
    }
    void Seal() { CHECK(adapter.Begin(frozen) == R::None); BindAll(); CHECK(adapter.SealInitialWorld() == R::None); }
    void AckAll() { for (auto p : frozen.m_RequiredParticipants) { const auto a = adapter.ExpectedAck(p); CHECK(a); CHECK(adapter.Acknowledge(p,*a)==R::None); } }
};
void TestDormancy() {
    CHECK(!IsCampaignMission()); CHECK(!IsCampaignSimulationAuthority()); CHECK(CanCampaignSimulate());
    Fixture f; InstallCampaignMissionView(&f.ownership); SetCampaignMissionAdapter(&f.adapter);
    CHECK(IsCampaignMission()); CHECK(IsCampaignSimulationAuthority()); CHECK(!CanCampaignSimulate());
    f.Seal(); f.AckAll(); S_CampaignStartSimulation start; CHECK(f.adapter.Start(start)==R::None);
    CHECK(CanCampaignSimulate()); CHECK(start.m_Generation==MissionGeneration(1));
    CHECK(f.adapter.End()==R::None); CHECK(!CanCampaignSimulate());
    SetCampaignMissionAdapter(nullptr); InstallCampaignMissionView(nullptr);
    CHECK(!IsCampaignMission()); CHECK(CanCampaignSimulate());
}
void TestFrozenRejection() {
    Fixture f; const auto before = f.ownership.GetSnapshot();
    auto bad = f.frozen; bad.m_Context.m_Rules.m_Difficulty=4;
    CHECK(f.adapter.Begin(bad)==R::ContentMismatch);
    bad=f.frozen; bad.m_RequiredParticipants.push_back(f.guest); CHECK(f.adapter.Begin(bad)==R::Malformed);
    bad=f.frozen; bad.m_Loadouts.pop_back(); CHECK(f.adapter.Begin(bad)==R::InvalidSoldier);
    bad=f.frozen; bad.m_Soldiers[0].m_Ownership.m_Alive=false; CHECK(f.adapter.Begin(bad)==R::StaleRevision);
    bad=f.frozen; bad.m_Context.m_Session=SessionId(1); CHECK(f.adapter.Begin(bad)==R::WrongSession);
    bad=f.frozen; bad.m_PreparationGeneration=PreparationGeneration(99); CHECK(f.adapter.Begin(bad)==R::WrongGeneration);
    CHECK(f.ownership.GetSnapshot()->m_Revision==before->m_Revision); CHECK(f.adapter.GetContext().m_Phase==P::Frozen);
}
void TestBarrier() {
    Fixture f; S_CampaignStartSimulation start;
    CHECK(f.adapter.Start(start)==R::SnapshotUnavailable); CHECK(f.adapter.Begin(f.frozen)==R::None);
    CHECK(f.adapter.SealInitialWorld()==R::ActorUnavailable); f.BindAll(false);
    CHECK(f.adapter.SealInitialWorld()==R::None); f.AckAll();
    // Even valid world acknowledgments cannot bypass controller handoff completion.
    CHECK(f.adapter.Start(start)==R::PendingTransaction);
    const auto pending = f.ownership.GetSnapshot();
    for (const auto &s : pending->m_Soldiers) CHECK(f.ownership.CompleteControlHandoff({s.m_Soldier,s.m_ControlRevision})==R::None);
    CHECK(f.adapter.Start(start)==R::None); CHECK(f.adapter.CanSimulate());
    CHECK(f.adapter.Start(start)==R::InvalidPhase);
}
void TestAckFences() {
    Fixture f; f.Seal(); const auto good=f.adapter.ExpectedAck(f.guest); CHECK(good);
    for (int field=0; field<8; ++field) {
        auto bad=*good;
        switch(field) {
        case 0: bad.m_Session=SessionId(1); break;
        case 1: bad.m_Generation=MissionGeneration(8); break;
        case 2: bad.m_Snapshot=SnapshotId(8); break;
        case 3: bad.m_PreparationGeneration=PreparationGeneration(8); break;
        case 4: bad.m_PreparationRevision=PreparationRevision(8); break;
        case 5: bad.m_ViewRevision=ViewRevision(8); break;
        case 6: bad.m_OwnershipRevision=OwnershipRevision(999); break;
        case 7: bad.m_PlayerEcho=PlayerId(99); break;
        }
        CHECK(f.adapter.Acknowledge(f.guest,bad)!=R::None);
        S_CampaignStartSimulation start; CHECK(f.adapter.Start(start)==R::SnapshotUnavailable);
    }
    CHECK(f.adapter.Acknowledge(PlayerId(99),*good)==R::NotAdmitted);
    CHECK(f.adapter.Acknowledge(f.guest,*good)==R::None); CHECK(f.adapter.Acknowledge(f.guest,*good)==R::None);
    S_CampaignStartSimulation start; CHECK(f.adapter.Start(start)==R::SnapshotUnavailable);
    auto host=f.adapter.ExpectedAck(PlayerId(1)); CHECK(host); CHECK(f.adapter.Acknowledge(PlayerId(1),*host)==R::None);
    CHECK(f.adapter.Start(start)==R::None);
}
void TestInvalidation() {
    for (int phase=0;phase<3;++phase) {
        Fixture f; CHECK(f.adapter.Begin(f.frozen)==R::None);
        if (phase>0) { f.BindAll(); CHECK(f.adapter.SealInitialWorld()==R::None); }
        if (phase>1) f.AckAll();
        CHECK(f.ownership.OnDisconnected(f.guest)==R::None);
        S_CampaignStartSimulation start; CHECK(f.adapter.Start(start)!=R::None);
        CHECK(f.adapter.SealInitialWorld()!=R::None); CHECK(!f.adapter.CanSimulate());
    }
    Fixture f; f.Seal(); f.AckAll(); f.adapter.Abort(R::ContentMismatch);
    S_CampaignStartSimulation start; CHECK(f.adapter.Start(start)==R::ContentMismatch); CHECK(!f.adapter.CanSimulate());
}
void TestNewGeneration() {
    Fixture f; f.Seal(); const auto old=f.adapter.GetManifest(); CHECK(old); auto stale=*f.adapter.ExpectedAck(f.guest);
    f.AckAll(); S_CampaignStartSimulation start; CHECK(f.adapter.Start(start)==R::None);
    CHECK(f.adapter.End()==R::None); CHECK(f.ownership.StartPreparation()==R::None); CHECK(f.ownership.Freeze()==R::None);
    auto state=f.ownership.GetSnapshot();
    f.frozen.m_Context=state->m_Context; f.frozen.m_PreparationGeneration=f.ownership.GetSession().GetPreparationGeneration();
    f.frozen.m_OwnershipRevision=state->m_Revision;
    for(auto &s:f.frozen.m_Soldiers) for(const auto &r:state->m_Soldiers) if(s.m_Soldier==r.m_Soldier) s.m_Ownership=r;
    CHECK(f.adapter.Begin(f.frozen)==R::SnapshotUnavailable);
    int retired=0;f.adapter.SetRetirementHook([&]{++retired;return R::PendingTransaction;});
    CHECK(f.adapter.Begin(f.frozen)==R::PendingTransaction);CHECK(f.adapter.GetContext().m_MissionGeneration==MissionGeneration(1));
    f.adapter.SetRetirementHook([&]{++retired;return R::None;});
    CHECK(f.adapter.Begin(f.frozen)==R::None);CHECK(retired==2);f.BindAll();CHECK(f.adapter.SealInitialWorld()==R::None);
    CHECK(f.adapter.Acknowledge(f.guest,stale)==R::WrongGeneration);
    CHECK(f.adapter.GetManifest()->m_Snapshot!=old->m_Snapshot);
    CHECK(old->m_Context.m_MissionGeneration==MissionGeneration(1)); CHECK(f.adapter.GetContext().m_MissionGeneration==MissionGeneration(2));
}
void TestStartupScripts() {
    Fixture f; CHECK(f.adapter.Begin(f.frozen)==R::None);
    const auto context=f.adapter.GetContext();
    CHECK(f.adapter.Bind(SoldierId(1),{context.m_Session,context.m_MissionGeneration,ActorId(20),ActorSlotUse(0)})==R::None);
    CHECK(f.ownership.SetScripted(SoldierId(1),true)==R::None);
    for(unsigned n=2;n<=4;++n) CHECK(f.adapter.Bind(SoldierId(n),{context.m_Session,context.m_MissionGeneration,ActorId(19+n),ActorSlotUse(0)})==R::None);
    CHECK(f.adapter.SealInitialWorld()==R::None);CHECK(f.adapter.GetManifest()->m_Soldiers[0].m_Ownership.m_Scripted);
    const auto state=f.ownership.GetSnapshot();
    for(const auto &s:state->m_Soldiers) CHECK(f.ownership.CompleteControlHandoff({s.m_Soldier,s.m_ControlRevision})==R::None);
    f.AckAll();S_CampaignStartSimulation start;CHECK(f.adapter.Start(start)==R::None);
    CHECK(f.adapter.CanSimulate());
    Fixture changed;CHECK(changed.adapter.Begin(changed.frozen)==R::None);
    CHECK(changed.ownership.OnDeath(SoldierId(4))==R::None);
    const auto c=changed.adapter.GetContext();
    CHECK(changed.adapter.Bind(SoldierId(1),{c.m_Session,c.m_MissionGeneration,ActorId(20),ActorSlotUse(0)})==R::StaleRevision);
}

void TestReconstruction() {
    struct View : I_CampaignOwnershipView {
        S_CampaignOwnershipSnapshot state;
        T_CampaignOwnershipSnapshot GetSnapshot() const override { return std::make_shared<const S_CampaignOwnershipSnapshot>(state); }
    } view;
    Fixture f; f.Seal(); auto manifest = *f.adapter.GetManifest(); view.state = *f.ownership.GetSnapshot();
    C_CampaignMissionReconstruction replica(manifest.m_Context.m_Session,f.guest,view);
    int calls=0;
    auto failed = [&](const auto &,const auto &) { ++calls;return R::ContentMismatch; };
    CHECK(replica.Reconstruct(manifest,{1,2,3},failed)==R::ContentMismatch);
    CHECK(calls==1);CHECK(!replica.GetAcknowledgment());CHECK(!replica.IsStarted());
    S_CampaignStartSimulation start{manifest.m_Context.m_Session,manifest.m_Context.m_MissionGeneration,manifest.m_Snapshot};
    CHECK(replica.AcceptStart(start)==R::SnapshotUnavailable);
    CHECK(replica.Reconstruct(manifest,{1,2,3},failed)==R::WrongGeneration);CHECK(calls==1);
    manifest.m_Snapshot=SnapshotId(2);
    auto success = [&](const auto &,const auto &) { ++calls;return R::None; };
    // Bytes arriving cannot acknowledge absent/incorrect native bindings.
    view.state.m_Soldiers[0].m_Actor.reset();
    CHECK(replica.Reconstruct(manifest,{1,2,3},success)==R::ActorUnavailable);CHECK(!replica.GetAcknowledgment());
    view.state=*f.ownership.GetSnapshot();manifest.m_Snapshot=SnapshotId(3);
    CHECK(replica.Reconstruct(manifest,{1,2,3},success)==R::None);CHECK(replica.GetAcknowledgment());
    start.m_Snapshot=SnapshotId(99);CHECK(replica.AcceptStart(start)==R::ContentMismatch);CHECK(!replica.IsStarted());
    start.m_Snapshot=manifest.m_Snapshot;CHECK(replica.AcceptStart(start)==R::None);CHECK(replica.IsStarted());
    view.state.m_Context.m_Phase=P::Running;
    CHECK(replica.AcceptStart(start)==R::None);CHECK(replica.IsStarted());
    view.state.m_Context.m_MissionGeneration=MissionGeneration(99);CHECK(!replica.IsStarted());
    view.state=*f.ownership.GetSnapshot();
    replica.Invalidate();CHECK(!replica.GetAcknowledgment());CHECK(!replica.IsStarted());
    manifest.m_Snapshot=SnapshotId(4);manifest.m_Bindings[0].m_Actor.m_Generation=MissionGeneration(99);
    CHECK(replica.Reconstruct(manifest,{1},success)==R::Malformed);CHECK(calls==3);
}

void TestGuestNativeGate() {
    struct View : I_CampaignOwnershipView {
        S_CampaignOwnershipSnapshot state;
        T_CampaignOwnershipSnapshot GetSnapshot() const override { return std::make_shared<const S_CampaignOwnershipSnapshot>(state); }
    } view;
    Fixture f;f.Seal();const auto manifest=*f.adapter.GetManifest();view.state=*f.ownership.GetSnapshot();
    view.state.m_Context.m_IsTransportServer=false;view.state.m_Context.m_IsTransportClient=true;
    view.state.m_Context.m_LocalPlayer=f.guest;
    C_CampaignMissionReconstruction replica(manifest.m_Context.m_Session,f.guest,view);
    InstallCampaignMissionView(&view);InstallCampaignMissionReconstruction(&replica);
    CHECK(!CanCampaignSimulate());
    CHECK(replica.Reconstruct(manifest,{1,2},[](const auto &,const auto &){return R::None;})==R::None);
    CHECK(!CanCampaignSimulate());
    // The Running publication can arrive before the authenticated Start message.
    view.state.m_Context.m_Phase=P::Running;CHECK(!CanCampaignSimulate());
    S_CampaignStartSimulation start{manifest.m_Context.m_Session,manifest.m_Context.m_MissionGeneration,SnapshotId(99)};
    CHECK(replica.AcceptStart(start)==R::ContentMismatch);CHECK(!CanCampaignSimulate());
    start.m_Snapshot=manifest.m_Snapshot;CHECK(replica.AcceptStart(start)==R::None);CHECK(CanCampaignSimulate());
    view.state.m_Context.m_MissionGeneration=MissionGeneration(99);CHECK(!CanCampaignSimulate());
    view.state=*f.ownership.GetSnapshot();view.state.m_Context.m_IsTransportServer=true;
    view.state.m_Context.m_Phase=P::Running;CHECK(!CanCampaignSimulate()); // host adapter must also be installed
    InstallCampaignMissionReconstruction(nullptr);InstallCampaignMissionView(nullptr);
}

void TestHostNativeGate() {
    Fixture f;CHECK(f.adapter.Begin(f.frozen)==R::None);f.BindAll();
    // A phase publication alone cannot impersonate lifecycle Start/barrier success.
    CHECK(f.ownership.StartSimulation()==R::None);
    SetCampaignMissionAdapter(&f.adapter);CHECK(IsCampaignSimulationAuthority());CHECK(!CanCampaignSimulate());
    CHECK(!f.adapter.CanSimulate());SetCampaignMissionAdapter(nullptr);
}

void TestMergedControllerComposition() {
    struct Sink:I_CampaignControlSink {
        int drains=0,executions=0;
        bool Drain(const ActorRef &) override{++drains;return true;}
        bool Apply(const S_CampaignControllerPlan &) override{return true;}
        bool Present(const std::optional<ActorRef> &) override{return true;}
        void Execute(const CampaignProtocol::S_Input &) override{++executions;}
    } hostSink,guestSink;
    Fixture f(true);C_CampaignControlAdapter control(f.ownership,hostSink);
    int retirements=0;
    f.adapter.SetRetirementHook([&]{++retirements;return control.RetireBeforeReconstruction();});
    CHECK(f.adapter.Begin(f.frozen)==R::None);CHECK(retirements==1);f.BindAll(false);
    CHECK(control.SynchronizeCommitted()==R::None);CHECK(f.adapter.SealInitialWorld()==R::None);
    C_CampaignReplicaControlAdapter guest(SessionId(90),f.guest,guestSink);
    C_CampaignMissionReconstruction reconstruction(SessionId(90),f.guest,guest);
    const auto manifest=*f.adapter.GetManifest();
    CHECK(reconstruction.Reconstruct(manifest,{1,2,3},[&](const auto &,const auto &){return guest.InstallCommitted(f.ownership.GetSnapshot());})==R::None);
    CHECK(reconstruction.GetAcknowledgment());
    CHECK(f.adapter.Acknowledge(f.guest,*reconstruction.GetAcknowledgment())==R::None);
    CHECK(f.adapter.Acknowledge(PlayerId(1),*f.adapter.ExpectedAck(PlayerId(1)))==R::None);
    S_CampaignStartSimulation start;CHECK(f.adapter.Start(start)==R::None);CHECK(control.SynchronizeCommitted()==R::None);
    CHECK(guest.InstallCommitted(f.ownership.GetSnapshot())==R::None); // phase-only publication may arrive first
    InstallCampaignMissionView(&guest);InstallCampaignMissionReconstruction(&reconstruction);
    CHECK(!CanCampaignSimulate());CHECK(reconstruction.AcceptStart(start)==R::None);CHECK(CanCampaignSimulate());
    const auto token=f.ownership.GetControlToken(f.guest);CHECK(token);CHECK(token->m_Actor.m_Actor==ActorId(21));
    CampaignProtocol::S_Input input{*token,1,CampaignProtocol::E_InputKind::ControlBits,{1}};
    CHECK(control.Enqueue(f.guest,input)==R::None);CHECK(control.ApplyNext()==R::None);CHECK(hostSink.executions==1);
    CHECK(control.OnDisconnected(f.guest)==R::None);CHECK(f.ownership.ResolveActor(token->m_Actor)==token->m_Soldier);
    CHECK(!f.ownership.FindOwner(token->m_Soldier));CHECK(control.Enqueue(f.guest,input)==R::NotAdmitted);
    reconstruction.Invalidate();CHECK(guest.RetireBeforeReconstruction()==R::None);CHECK(!CanCampaignSimulate());
    InstallCampaignMissionReconstruction(nullptr);InstallCampaignMissionView(nullptr);
    CHECK(f.adapter.End()==R::None);CHECK(control.SynchronizeCommitted()==R::None);
    CHECK(f.ownership.StartPreparation()==R::None);CHECK(f.ownership.Freeze()==R::None);
    const auto next=f.ownership.GetSnapshot();f.frozen.m_Context=next->m_Context;
    f.frozen.m_OwnershipRevision=next->m_Revision;f.frozen.m_PreparationGeneration=f.ownership.GetSession().GetPreparationGeneration();
    f.frozen.m_RequiredParticipants={PlayerId(1)}; // explicit fixture policy in a NEW preparation generation
    for(auto &s:f.frozen.m_Soldiers)for(const auto &r:next->m_Soldiers)if(s.m_Soldier==r.m_Soldier)s.m_Ownership=r;
    const int oldDrains=hostSink.drains;
    CHECK(f.adapter.Begin(f.frozen)==R::None);CHECK(retirements==2);CHECK(hostSink.drains>oldDrains);
    CHECK(f.adapter.GetContext().m_MissionGeneration==MissionGeneration(2));
}

}
int main() {
    try { TestDormancy(); TestFrozenRejection(); TestBarrier(); TestAckFences(); TestInvalidation(); TestNewGeneration(); TestStartupScripts(); TestReconstruction(); TestGuestNativeGate(); TestHostNativeGate(); TestMergedControllerComposition(); }
    catch(const std::exception &e) { std::cerr<<"FAIL after "<<g_Checks<<": "<<e.what()<<'\n'; return 1; }
    std::cout<<"PASS: 11 lifecycle suites / "<<g_Checks<<" checks\n";
}
