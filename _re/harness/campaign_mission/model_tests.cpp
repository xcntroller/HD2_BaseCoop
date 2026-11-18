#include "../../../src/game/campaign_mission_adapter.h"
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
struct Fixture {
    Policy policy;
    C_CampaignRequestJournal journal{SessionId(90)};
    C_CampaignOwnership ownership{C_CampaignSession({SessionId(90), CampaignId(8), MissionKey("original/step/version"), {}, true}, {17,0,9,4}), policy, journal};
    C_CampaignMissionAdapter adapter{ownership};
    PlayerId guest;
    S_CampaignMissionManifest frozen;
    Fixture() {
        CHECK(ownership.Admit("guest", guest) == R::None);
        for (auto s : {SoldierId(1),SoldierId(2),SoldierId(3),SoldierId(4)}) CHECK(ownership.SetAvailability(s,true,true) == R::None);
        CHECK(ownership.StartPreparation() == R::None);
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
    CHECK(f.adapter.Begin(f.frozen)==R::None); f.BindAll(); CHECK(f.adapter.SealInitialWorld()==R::None);
    CHECK(f.adapter.Acknowledge(f.guest,stale)==R::WrongGeneration);
    CHECK(f.adapter.GetManifest()->m_Snapshot!=old->m_Snapshot);
    CHECK(old->m_Context.m_MissionGeneration==MissionGeneration(1)); CHECK(f.adapter.GetContext().m_MissionGeneration==MissionGeneration(2));
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
    replica.Invalidate();CHECK(!replica.GetAcknowledgment());CHECK(!replica.IsStarted());
    manifest.m_Snapshot=SnapshotId(4);manifest.m_Bindings[0].m_Actor.m_Generation=MissionGeneration(99);
    CHECK(replica.Reconstruct(manifest,{1},success)==R::Malformed);CHECK(calls==3);
}

}
int main() {
    try { TestDormancy(); TestFrozenRejection(); TestBarrier(); TestAckFences(); TestInvalidation(); TestNewGeneration(); TestReconstruction(); }
    catch(const std::exception &e) { std::cerr<<"FAIL after "<<g_Checks<<": "<<e.what()<<'\n'; return 1; }
    std::cout<<"PASS: 7 lifecycle suites / "<<g_Checks<<" checks\n";
}
