#include "../../../src/game/campaign_ownership.h"
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
        CHECK(m_Model.Freeze() == R::None);
        CHECK(m_Model.BeginMissionGeneration(MissionKey("base.sicily2.step1.v1")) == R::None);
        CHECK(m_Model.StartSimulation() == R::ActorUnavailable);
        BindAll();
        CHECK(m_Model.StartSimulation() == R::None);
        Invariants(m_Model);
    }
};

void CatalogAndContext() {
    for (const auto &indexes : std::vector<std::vector<int>>{{}, {1, 1}, {-1}, {7, -1}, std::vector<int>(257, 8)}) {
        bool failed = false;
        try { C_CampaignSession session(Setup(), indexes); } catch (const std::invalid_argument &) { failed = true; }
        CHECK(failed);
    }
    C_CampaignSession session(Setup(), {19, 0, 7});
    CHECK(session.FindSoldier(0) == SoldierId(2));
    CHECK(session.FindProfile(SoldierId(1)) == 19);
    CHECK(!session.FindSoldier(-1));
    CHECK(!session.FindProfile(SoldierId(0)));
    CHECK(!session.FindProfile(SoldierId(100)));
    CHECK(session.GetContext().m_IsTransportServer && session.GetContext().m_IsTransportClient);
    CHECK(session.GetContext().m_Rules.m_Difficulty == 3);
    CHECK(C_CampaignSession::UsesCampaignRules(session.GetContext()));
    CHECK(C_CampaignSession::HasSimulationAuthority(session.GetContext()));
    auto client = session.GetContext();
    client.m_IsTransportServer = false;
    CHECK(!C_CampaignSession::HasSimulationAuthority(client));
    auto solo = S_CampaignSessionContext{};
    CHECK(C_CampaignSession::HasSimulationAuthority(solo));
    solo.m_UsesCampaignRules = true;
    CHECK(C_CampaignSession::UsesCampaignRules(solo));
    solo.m_Kind = E_CampaignSessionKind::LegacyMultiplayer;
    CHECK(!C_CampaignSession::UsesCampaignRules(solo));
    CHECK(!C_CampaignSession::HasSimulationAuthority(solo));
    CHECK(session.StartSimulation() == R::InvalidPhase);
    CHECK(session.Freeze() == R::InvalidPhase);
    const auto maximum = (std::numeric_limits<std::uint64_t>::max)();
    CHECK(!CampaignNextCounter(ControlRevision(maximum)));
    CHECK(CampaignNextCounter(ControlRevision(maximum - 1)) == ControlRevision(maximum));
    CHECK(CampaignNextCounter(PlayerId{}) == PlayerId(1));
}

void ClaimsAndAtomicSwitch() {
    C_Fixture f;
    auto first = f.Request(f.m_Host, S_CampaignClaim{f.Target(f.m_A)});
    auto race = f.Request(f.m_Guest, S_CampaignClaim{f.Target(f.m_A)});
    const auto original = f.m_Model.GetSnapshot();
    auto accepted = f.m_Model.ApplyRequest(f.m_Host, first);
    CHECK(accepted.m_Outcome == O::Accepted);
    CHECK(f.m_Model.ApplyRequest(f.m_Guest, race).m_Reason == R::StaleRevision);
    CHECK(f.Claim(f.m_Guest, f.m_A).m_Reason == R::AlreadyOwned);
    CHECK(f.Claim(f.m_Host, f.m_B).m_Reason == R::AlreadyControlsSoldier);
    CHECK(f.Claim(f.m_Guest, f.m_B).m_Outcome == O::Accepted);
    CHECK(!Record(*original, f.m_A).m_Owner); // immutable retained publication
    auto before = f.m_Model.GetSnapshot();
    auto fail = f.Request(f.m_Host, S_CampaignSwitch{f.Target(f.m_A), f.Target(f.m_B)});
    CHECK(f.m_Model.ApplyRequest(f.m_Host, fail).m_Reason == R::AlreadyOwned);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    CHECK(f.m_Model.FindSoldier(f.m_Host) == f.m_A);
    auto stale = f.Request(f.m_Host, S_CampaignSwitch{f.Target(f.m_A), f.Target(f.m_C)});
    std::get<S_CampaignSwitch>(stale.m_Operation).m_To.m_ExpectedControlRevision = ControlRevision(999);
    CHECK(f.m_Model.ApplyRequest(f.m_Host, stale).m_Reason == R::StaleRevision);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    auto change = f.Request(f.m_Host, S_CampaignSwitch{f.Target(f.m_A), f.Target(f.m_C)});
    auto result = f.m_Model.ApplyRequest(f.m_Host, change);
    CHECK(result.m_Outcome == O::Accepted);
    CHECK(result.m_OwnershipRevision.Value() == before->m_Revision.Value() + 1);
    CHECK(!f.m_Model.FindOwner(f.m_A));
    CHECK(f.m_Model.FindSoldier(f.m_Host) == f.m_C);
    CHECK(f.Target(f.m_A).m_ExpectedControlRevision.Value() == Record(*before, f.m_A).m_ControlRevision.Value() + 1);
    CHECK(f.Target(f.m_C).m_ExpectedControlRevision.Value() == Record(*before, f.m_C).m_ControlRevision.Value() + 1);
    CHECK(f.m_Model.IsLocallyOwned(f.m_C));
    Invariants(f.m_Model);
    // Unknown and missing targets never enter a legacy table or mutate state.
    auto invalid = f.Request(f.m_Guest, S_CampaignClaim{{SoldierId(99), ControlRevision(1)}});
    before = f.m_Model.GetSnapshot();
    CHECK(f.m_Model.ApplyRequest(f.m_Guest, invalid).m_Reason == R::InvalidSoldier);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    CHECK(!f.m_Model.FindOwner(SoldierId{}));
}

void AdmissionAndRestrictions() {
    C_Fixture f;
    PlayerId third, fourth, rejected(111);
    CHECK(f.m_Model.Admit("third", third) == R::None);
    CHECK(f.m_Model.Admit("fourth", fourth) == R::None);
    auto before = f.m_Model.GetSnapshot();
    CHECK(f.m_Model.Admit("fifth", rejected) == R::SessionFull);
    CHECK(rejected == PlayerId(111));
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    CHECK(f.m_Model.SetAvailability(f.m_E, true, true) == R::MissionRestriction);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    CHECK(f.m_Model.SetAvailability(f.m_C, false, false) == R::None);
    CHECK(f.Claim(f.m_Guest, f.m_C).m_Reason == R::Ineligible);
    CHECK(f.m_Model.SetAvailability(f.m_C, false, true) == R::Ineligible);
    CHECK(f.Claim(f.m_Guest, f.m_B).m_Outcome == O::Accepted);
    CHECK(f.m_Model.SetAvailability(f.m_B, false, false) == R::None);
    CHECK(!f.m_Model.FindSoldier(f.m_Guest));
    CHECK(f.m_Model.OnDisconnected(f.m_Guest) == R::None);
    const auto revision = f.m_Model.GetSnapshot()->m_Revision;
    CHECK(f.m_Model.OnDisconnected(f.m_Guest) == R::None);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == revision);
    PlayerId reconnect;
    CHECK(f.m_Model.Admit("Guest", reconnect) == R::None);
    CHECK(reconnect != f.m_Guest && reconnect.Value() > fourth.Value());
    auto forged = f.Request(reconnect, S_CampaignClaim{f.Target(f.m_A)});
    CHECK(f.m_Model.ApplyRequest(f.m_Host, forged).m_Reason == R::NotAdmitted);
    forged.m_Context.m_PlayerEcho = f.m_Guest;
    CHECK(f.m_Model.ApplyRequest(f.m_Guest, forged).m_Reason == R::NotAdmitted);
    Invariants(f.m_Model);

    C_Policy policy;
    C_CampaignRequestJournal journal(SessionId(901));
    C_CampaignOwnership restricted(C_CampaignSession(Setup(), {1, 2, 3}), policy, journal, 1);
    CHECK(restricted.SetAvailability(SoldierId(1), true, true) == R::None);
    before = restricted.GetSnapshot();
    CHECK(restricted.SetAvailability(SoldierId(2), true, true) == R::MissionRestriction);
    CHECK(restricted.GetSnapshot()->m_Revision == before->m_Revision);
    Invariants(restricted, 1);
}

void PoliciesAndDuplicateResults() {
    C_Fixture f;
    f.m_Policy.m_State.m_Decisions.clear();
    auto request = f.Request(f.m_Host, S_CampaignClaim{f.Target(f.m_A)});
    auto before = f.m_Model.GetSnapshot();
    auto result = f.m_Model.ApplyRequest(f.m_Host, request);
    CHECK(result.m_Reason == R::PolicyRequired);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    f.m_Policy = C_Policy{};
    auto replay = f.m_Model.ApplyRequest(f.m_Host, request);
    CHECK(replay.m_Outcome == O::Duplicate && replay.m_OriginalOutcome == O::Rejected);
    CHECK(replay.m_Reason == R::PolicyRequired); // rejections consume sequence too
    CHECK(f.Claim(f.m_Host, f.m_A).m_Outcome == O::Accepted);
    auto release = f.Request(f.m_Host, S_CampaignRelease{f.Target(f.m_A)});
    result = f.m_Model.ApplyRequest(f.m_Host, release);
    CHECK(result.m_Outcome == O::Accepted);
    auto after = f.m_Model.GetSnapshot();
    CHECK(f.m_Model.ApplyRequest(f.m_Host, release).m_Outcome == O::Duplicate);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == after->m_Revision);
    auto conflict = release;
    conflict.m_ExpectedOwnershipRevision = after->m_Revision;
    CHECK(f.m_Model.ApplyRequest(f.m_Host, conflict).m_Reason == R::RequestConflict);
    CHECK(f.m_Model.ApplyRequest(f.m_Host, request).m_Outcome == O::ResyncRequired); // 2-entry cache evicted it
    CHECK(f.m_Model.ApplyRequest(f.m_Host, request).m_Reason == R::RequestExpired);
    auto stale = f.Request(f.m_Host, S_CampaignClaim{f.Target(f.m_A)});
    stale.m_ExpectedOwnershipRevision = OwnershipRevision(1);
    CHECK(f.m_Model.ApplyRequest(f.m_Host, stale).m_Reason == R::StaleRevision);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == after->m_Revision);
    auto wrongSession = f.Request(f.m_Host, S_CampaignClaim{f.Target(f.m_A)});
    wrongSession.m_Context.m_Session = SessionId(2);
    CHECK(f.m_Model.ApplyRequest(f.m_Host, wrongSession).m_Reason == R::WrongSession);
    CHECK(f.m_Model.Freeze() == R::None);
    CHECK(f.m_Model.ApplyRequest(f.m_Host, release).m_Reason == R::Frozen);
    CHECK(f.m_Model.StartPreparation() == R::None);
    CHECK(f.m_Model.ApplyRequest(f.m_Host, release).m_Reason == R::WrongGeneration);
    Invariants(f.m_Model);
}

void PhaseAndScopeFences() {
    C_Fixture f;
    f.m_Policy.m_State.m_Decisions[0].m_State = E_CampaignPolicyState::Denied;
    CHECK(f.Claim(f.m_Host, f.m_A).m_Reason == R::Ineligible);
    f.m_Policy.m_State.m_Decisions[0].m_State = E_CampaignPolicyState::Permitted;
    auto claim = f.Request(f.m_Host, S_CampaignClaim{f.Target(f.m_A)});
    CHECK(f.m_Model.ApplyRequest(f.m_Host, claim).m_Outcome == O::Accepted);
    auto release = f.Request(f.m_Host, S_CampaignRelease{f.Target(f.m_A)});
    CHECK(f.m_Model.ApplyRequest(f.m_Host, release).m_Outcome == O::Accepted);
    auto duplicate = f.m_Model.ApplyRequest(f.m_Host, claim);
    CHECK(duplicate.m_Outcome == O::Duplicate && duplicate.m_OriginalOutcome == O::Accepted);
    CHECK(Record(*std::get<T_CampaignOwnershipSnapshot>(duplicate.m_State), f.m_A).m_Owner == f.m_Host);
    CHECK(!f.m_Model.FindOwner(f.m_A)); // replay does not roll current authority back
    auto notOwner = f.Request(f.m_Host, S_CampaignRelease{f.Target(f.m_A)});
    CHECK(f.m_Model.ApplyRequest(f.m_Host, notOwner).m_Reason == R::NotOwner);
    auto missing = f.Request(f.m_Host, S_CampaignClaim{f.Target(f.m_A)});
    missing.m_PreparationGeneration.reset();
    CHECK(f.m_Model.ApplyRequest(f.m_Host, missing).m_Reason == R::WrongGeneration);
    auto before = f.m_Model.GetSnapshot();
    CHECK(f.m_Model.SetAvailability(SoldierId{}, true, false) == R::InvalidSoldier);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    CHECK(f.m_Model.Freeze() == R::None);
    PlayerId unassigned;
    CHECK(f.m_Model.Admit("frozen", unassigned) == R::InvalidPhase);
    CHECK(f.m_Model.SetAvailability(f.m_A, false, false) == R::InvalidPhase);
    CHECK(f.m_Model.BeginMissionGeneration(MissionKey("base.sicily2.step1.v1")) == R::None);
    const auto state = f.m_Model.GetSnapshot();
    auto actor = ActorRef{state->m_Context.m_Session, state->m_Context.m_MissionGeneration, ActorId(0), ActorSlotUse(0)};
    actor.m_Session = SessionId(90);
    CHECK(f.m_Model.BindActor(f.m_A, actor) == R::WrongSession);
    actor.m_Session = state->m_Context.m_Session;
    CHECK(f.m_Model.BindActor(f.m_E, actor) == R::NotDeployed);
    CHECK(f.m_Model.BindActor(SoldierId(99), actor) == R::InvalidSoldier);
    CHECK(f.m_Model.BindActor(f.m_A, actor) == R::None);
    CHECK(f.m_Model.CompleteControlHandoff(f.Target(f.m_E)) == R::ActorUnavailable);
    CHECK(f.m_Model.StartSimulation() == R::PendingTransaction);
    CHECK(f.m_Model.ReturnToSetup(MissionKey("base.sicily2.step1.v1")) == R::None);
    CHECK(!f.m_Model.ResolveActor(actor));
    CHECK(f.m_Model.StartPreparation() == R::None);
    CHECK(f.Claim(f.m_Host, f.m_A).m_Outcome == O::Accepted);
    f.Run();
    auto token = *f.m_Model.GetControlToken(f.m_Host);
    CHECK(f.m_Model.SetScripted(f.m_A, true) == R::None);
    CHECK(f.m_Model.SetScripted(f.m_A, false) == R::None);
    CHECK(f.m_Model.FindOwner(f.m_A) == f.m_Host);
    CHECK(!f.m_Model.GetControlToken(f.m_Host));
    CHECK(f.m_Model.CompleteControlHandoff(f.Target(f.m_A)) == R::None);
    CHECK(f.m_Model.GetControlToken(f.m_Host));
    CHECK(f.m_Model.ValidateInput(f.m_Host, token) == R::StaleRevision);
    CHECK(f.m_Model.SetAvailability(f.m_E, true, true) == R::InvalidPhase);
    CHECK(f.m_Model.SetAvailability(f.m_A, false, false) == R::None);
    CHECK(!f.m_Model.FindOwner(f.m_A));
    CHECK(!f.m_Model.ResolveActor(token.m_Actor));
    Invariants(f.m_Model);
}

void ScriptDeathAndInput() {
    C_Fixture f;
    CHECK(f.Claim(f.m_Host, f.m_A).m_Outcome == O::Accepted);
    CHECK(f.Claim(f.m_Guest, f.m_B).m_Outcome == O::Accepted);
    f.Run();
    auto hostToken = f.m_Model.GetControlToken(f.m_Host);
    auto guestToken = f.m_Model.GetControlToken(f.m_Guest);
    CHECK(hostToken && guestToken);
    CHECK(hostToken->m_Actor.m_Actor == ActorId(0)); // zero is a legitimate actor
    CHECK(f.m_Model.ValidateInput(f.m_Host, *hostToken) == R::None);
    CHECK(f.m_Model.ValidateInput(f.m_Guest, *hostToken) == R::NotAdmitted);
    PlayerId late;
    CHECK(f.m_Model.Admit("late", late) == R::JoinNotSupported);
    CHECK(f.Claim(f.m_Guest, f.m_E).m_Reason == R::NotDeployed);
    CHECK(f.m_Model.SetScripted(f.m_B, true) == R::None);
    CHECK(f.m_Model.FindOwner(f.m_B) == f.m_Guest);
    CHECK(!f.m_Model.GetControlToken(f.m_Guest));
    CHECK(f.m_Model.ValidateInput(f.m_Guest, *guestToken) == R::StaleRevision);
    CHECK(f.m_Model.OnDisconnected(f.m_Guest) == R::None);
    CHECK(Record(*f.m_Model.GetSnapshot(), f.m_B).m_Scripted);
    CHECK(!f.m_Model.FindOwner(f.m_B));
    CHECK(f.m_Model.SetScripted(f.m_B, false) == R::None);
    CHECK(!f.m_Model.FindOwner(f.m_B)); // script exit restores current AI, never cached player
    CHECK(Record(*f.m_Model.GetSnapshot(), f.m_B).m_Actor == guestToken->m_Actor);
    CHECK(f.m_Model.ValidateInput(f.m_Guest, *guestToken) == R::NotAdmitted);
    auto before = f.m_Model.GetSnapshot();
    CHECK(f.m_Model.SetScripted(f.m_B, false) == R::None);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    CHECK(f.m_Model.OnDeath(f.m_A) == R::None);
    CHECK(!f.m_Model.FindSoldier(f.m_Host));
    CHECK(!Record(*f.m_Model.GetSnapshot(), f.m_A).m_Alive);
    CHECK(f.Claim(f.m_Host, f.m_A).m_Reason == R::Dead);
    before = f.m_Model.GetSnapshot();
    CHECK(f.m_Model.OnDeath(f.m_A) == R::None);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
    CHECK(f.m_Model.ValidateInput(f.m_Host, *hostToken) == R::StaleRevision);
    CHECK(f.m_Model.BeginMissionGeneration(MissionKey("base.sicily2.step1.v1")) == R::None);
    CHECK(!Record(*f.m_Model.GetSnapshot(), f.m_A).m_Alive); // no life from reset
    CHECK(!Record(*f.m_Model.GetSnapshot(), f.m_A).m_Actor);
    CHECK(f.m_Model.GetSession().FindProfile(f.m_A) == 19);
    CHECK(f.m_Model.ValidateInput(f.m_Host, *hostToken) == R::WrongGeneration);
    Invariants(f.m_Model);
}

void HandoffAndReuse() {
    C_Fixture f;
    CHECK(f.Claim(f.m_Host, f.m_A).m_Outcome == O::Accepted);
    f.Run();
    auto original = *f.m_Model.GetControlToken(f.m_Host);
    auto change = f.Request(f.m_Host, S_CampaignSwitch{f.Target(f.m_A), f.Target(f.m_B)});
    CHECK(f.m_Model.ApplyRequest(f.m_Host, change).m_Outcome == O::Accepted);
    CHECK(!f.m_Model.GetControlToken(f.m_Host));
    CHECK(f.m_Model.CompleteControlHandoff({f.m_B, ControlRevision(1)}) == R::StaleRevision);
    CHECK(f.m_Model.CompleteControlHandoff(f.Target(f.m_B)) == R::None);
    CHECK(f.m_Model.GetControlToken(f.m_Host));
    change = f.Request(f.m_Host, S_CampaignSwitch{f.Target(f.m_B), f.Target(f.m_A)});
    CHECK(f.m_Model.ApplyRequest(f.m_Host, change).m_Outcome == O::Accepted);
    CHECK(f.m_Model.CompleteControlHandoff(f.Target(f.m_A)) == R::None);
    auto back = *f.m_Model.GetControlToken(f.m_Host);
    CHECK(back.m_Actor == original.m_Actor);
    CHECK(back.m_ControlRevision != original.m_ControlRevision);
    CHECK(f.m_Model.ValidateInput(f.m_Host, original) == R::StaleRevision);
    CHECK(f.m_Model.BindActor(f.m_B, back.m_Actor) == R::AlreadyOwned);
    CHECK(f.m_Model.UnbindActor(f.m_A) == R::None);
    CHECK(!f.m_Model.ResolveActor(back.m_Actor));
    CHECK(!f.m_Model.GetControlToken(f.m_Host));
    CHECK(f.m_Model.BindActor(f.m_A, back.m_Actor) == R::None); // same bounded slot/use wrapped
    CHECK(f.m_Model.CompleteControlHandoff(f.Target(f.m_A)) == R::None);
    CHECK(f.m_Model.ValidateInput(f.m_Host, back) == R::StaleRevision);
    auto wrongActor = *f.m_Model.GetControlToken(f.m_Host);
    wrongActor.m_Actor.m_SlotUse = ActorSlotUse(1);
    CHECK(f.m_Model.ValidateInput(f.m_Host, wrongActor) == R::ActorUnavailable);
    auto delayed = f.Request(f.m_Host, S_CampaignRelease{f.Target(f.m_A)});
    CHECK(f.m_Model.BeginMissionGeneration(MissionKey("base.sicily2.step1.v1")) == R::None);
    CHECK(!f.m_Model.ResolveActor(original.m_Actor));
    CHECK(f.m_Model.BindActor(f.m_A, original.m_Actor) == R::WrongGeneration);
    CHECK(f.m_Model.StartSimulation() == R::ActorUnavailable);
    f.BindAll();
    CHECK(f.m_Model.StartSimulation() == R::None);
    CHECK(f.m_Model.ApplyRequest(f.m_Host, delayed).m_Reason == R::WrongGeneration);
    CHECK(f.m_Model.ValidateInput(f.m_Host, original) == R::WrongGeneration);
    CHECK(f.m_Model.EndMission() == R::None);
    CHECK(!f.m_Model.GetControlToken(f.m_Host));
    CHECK(f.m_Model.ReturnToSetup(MissionKey("base.next.step2.v1")) == R::None);
    CHECK(f.m_Model.GetSession().FindProfile(f.m_A) == 19);
    CHECK(f.m_Model.Close() == R::None);
    const auto revision = f.m_Model.GetSnapshot()->m_Revision;
    CHECK(f.m_Model.Close() == R::None);
    CHECK(f.m_Model.GetSnapshot()->m_Revision == revision);
    CHECK(f.m_Model.GetSnapshot()->m_Participants.empty());
    CHECK(!f.m_Model.FindOwner(f.m_A));
    CHECK(f.m_Model.OnDeath(f.m_B) == R::InvalidPhase);
    CHECK(f.m_Model.ApplyRequest(f.m_Host, delayed).m_Reason == R::NotAdmitted);
    // Journal participant storage is freed on termination.
    for (std::uint64_t i = 20; i < 24; ++i) CHECK(f.m_Journal.Admit(PlayerId(i)) == R::None);
    Invariants(f.m_Model);
}

void CrossFamilyJournal() {
    C_CampaignRequestJournal journal(SessionId(10), 1);
    CHECK(journal.Admit(PlayerId(4)) == R::None);
    auto current = S_CampaignRequestResult{};
    current.m_Context = {SessionId(10), PlayerId(4), RequestId(2)};
    current.m_Family = E_CampaignRequestFamily::Edit;
    current.m_PreparationGeneration = PreparationGeneration(1);
    current.m_Outcome = O::Rejected;
    current.m_Reason = R::StockUnavailable;
    CHECK(!journal.Begin(current.m_Context, current.m_Family, "edit|item=7|count=1", current));
    CHECK(journal.Begin(current.m_Context, current.m_Family, "edit|item=7|count=1", current)->m_Reason == R::PendingTransaction);
    journal.Finish(current.m_Context, current.m_Family, "edit|item=7|count=1", current);
    auto replay = journal.Begin(current.m_Context, current.m_Family, "edit|item=7|count=1", current);
    CHECK(replay->m_Outcome == O::Duplicate && replay->m_OriginalOutcome == O::Rejected);
    CHECK(replay->m_Reason == R::StockUnavailable);
    CHECK(journal.Begin(current.m_Context, E_CampaignRequestFamily::Ready, "edit|item=7|count=1", current)->m_Reason == R::RequestConflict);
    CHECK(journal.Begin(current.m_Context, current.m_Family, "edit|item=7|count=2", current)->m_Reason == R::RequestConflict);
    current.m_Context.m_Request = RequestId(1);
    CHECK(journal.Begin(current.m_Context, E_CampaignRequestFamily::Ownership, "claim|1", current)->m_Reason == R::RequestExpired);
    current.m_Context.m_Request = RequestId(3);
    current.m_Family = E_CampaignRequestFamily::Ready;
    current.m_Outcome = O::Accepted;
    CHECK(!journal.Begin(current.m_Context, current.m_Family, "ready|true", current));
    journal.Finish(current.m_Context, current.m_Family, "ready|true", current);
    current.m_Context.m_Request = RequestId(2);
    CHECK(journal.Begin(current.m_Context, E_CampaignRequestFamily::Edit, "edit|item=7|count=1", current)->m_Reason == R::RequestExpired);
    current.m_Context.m_Request = RequestId(3);
    current.m_PreparationGeneration = PreparationGeneration(2);
    CHECK(journal.Begin(current.m_Context, current.m_Family, "ready|true", current)->m_Reason == R::WrongGeneration);
    current.m_Context.m_Request = RequestId(4);
    CHECK(journal.Begin(current.m_Context, current.m_Family, std::string(4097, 'x'), current)->m_Reason == R::Malformed);
    current.m_Context.m_Request = RequestId{};
    CHECK(journal.Begin(current.m_Context, current.m_Family, "ready|true", current)->m_Reason == R::Malformed);
    current.m_Context.m_Request = RequestId(4);
    current.m_Context.m_PlayerEcho = PlayerId(77);
    CHECK(journal.Begin(current.m_Context, current.m_Family, "ready|true", current)->m_Reason == R::NotAdmitted);
    current.m_Context.m_PlayerEcho = PlayerId(4);
    current.m_Context.m_Session = SessionId(11);
    CHECK(journal.Begin(current.m_Context, current.m_Family, "ready|true", current)->m_Reason == R::WrongSession);
}

// Deterministic interleavings repeatedly check both indexes, immutable old cuts,
// request races and no partial catalog commits; not a mirror of implementation.
void Interleavings() {
    C_Fixture f;
    std::uint32_t random = 0x4004;
    for (int step = 0; step < 300; ++step) {
        random = random * 1664525U + 1013904223U;
        PlayerId player = (random & 1U) ? f.m_Host : f.m_Guest;
        SoldierId target(1 + ((random >> 5) % 4));
        T_CampaignOwnershipOperation operation = S_CampaignClaim{f.Target(target)};
        if (auto owned = f.m_Model.FindSoldier(player)) {
            if ((random >> 9) & 1U) operation = S_CampaignRelease{f.Target(*owned)};
            else operation = S_CampaignSwitch{f.Target(*owned), f.Target(target)};
        }
        auto request = f.Request(player, operation);
        auto before = f.m_Model.GetSnapshot();
        if ((random >> 13) % 3 == 0) request.m_ExpectedOwnershipRevision = OwnershipRevision(1);
        auto result = f.m_Model.ApplyRequest(player, request);
        Invariants(f.m_Model);
        if (result.m_Outcome == O::Rejected) CHECK(f.m_Model.GetSnapshot()->m_Revision == before->m_Revision);
        auto revision = f.m_Model.GetSnapshot()->m_Revision;
        CHECK(f.m_Model.ApplyRequest(player, request).m_Outcome == O::Duplicate);
        CHECK(f.m_Model.GetSnapshot()->m_Revision == revision);
    }
}
}

int main() {
    CatalogAndContext();
    ClaimsAndAtomicSwitch();
    AdmissionAndRestrictions();
    PoliciesAndDuplicateResults();
    PhaseAndScopeFences();
    ScriptDeathAndInput();
    HandoffAndReuse();
    CrossFamilyJournal();
    Interleavings();
    std::cout << "campaign ownership: 9 suites, " << g_Checks << " checks passed\n";
}
