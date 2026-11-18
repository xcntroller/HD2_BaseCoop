#include "campaign_ownership.h"
#include <algorithm>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace {
bool SameRecord(const S_CampaignOwnershipRecord &a, const S_CampaignOwnershipRecord &b) {
    return a.m_Soldier == b.m_Soldier && a.m_Eligible == b.m_Eligible && a.m_Deployed == b.m_Deployed &&
        a.m_Alive == b.m_Alive && a.m_Owner == b.m_Owner && a.m_Actor == b.m_Actor && a.m_Scripted == b.m_Scripted;
}
std::string CanonicalContent(const S_CampaignOwnershipRequest &request) {
    // Decimal unsigned fields + fixed delimiters/tags: injective for this DTO.
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << request.m_ExpectedOwnershipRevision.Value() << '|';
    auto generation = [&](const auto &value) { out << (value ? "v" : "x") << (value ? value->Value() : 0) << '|'; };
    generation(request.m_PreparationGeneration);
    generation(request.m_MissionGeneration);
    out << request.m_Operation.index() << '|';
    auto target = [&](const S_CampaignOwnershipTarget &value) {
        out << value.m_Soldier.Value() << ':' << value.m_ExpectedControlRevision.Value() << '|';
    };
    if (const auto *claim = std::get_if<S_CampaignClaim>(&request.m_Operation)) target(claim->m_Target);
    else if (const auto *release = std::get_if<S_CampaignRelease>(&request.m_Operation)) target(release->m_Target);
    else {
        const auto &change = std::get<S_CampaignSwitch>(request.m_Operation);
        target(change.m_From);
        target(change.m_To);
    }
    return out.str();
}
}

C_CampaignOwnership::C_CampaignOwnership(C_CampaignSession session, const I_CampaignPolicyView &policy,
    C_CampaignRequestJournal &journal, std::uint32_t squadLimit)
    : m_Session(std::move(session)), m_Policy(policy), m_Journal(journal), m_SquadLimit(squadLimit) {
    if (squadLimit == 0 || squadLimit > 4 || journal.GetSession() != m_Session.GetContext().m_Session ||
        m_Session.GetContext().m_Phase != E_CampaignPhase::Setup ||
        m_Session.GetContext().m_MissionGeneration.IsValid() || m_Session.GetPreparationGeneration().IsValid())
        throw std::invalid_argument("invalid ownership session/squad/journal");
    m_State.m_Context = m_Session.GetContext();
    m_State.m_Revision = OwnershipRevision(1);
    for (const auto &entry : m_Session.GetRoster()) {
        S_CampaignOwnershipRecord record;
        record.m_Soldier = entry.m_Soldier;
        record.m_Alive = true;
        record.m_ControlRevision = ControlRevision(1);
        m_State.m_Soldiers.push_back(record);
        m_HandoffComplete.emplace(entry.m_Soldier, false);
    }
    const auto host = *m_State.m_Context.m_HostPlayer;
    if (m_Journal.Admit(host) != E_CampaignReason::None)
        throw std::invalid_argument("journal cannot admit host");
    m_State.m_Participants.push_back({host, "Host", true, true, std::nullopt});
}
T_CampaignOwnershipSnapshot C_CampaignOwnership::GetSnapshot() const {
    return std::make_shared<const S_CampaignOwnershipSnapshot>(m_State);
}
S_CampaignOwnershipRecord *C_CampaignOwnership::FindRecord(S_CampaignOwnershipSnapshot &state, SoldierId soldier) {
    for (auto &record : state.m_Soldiers) if (record.m_Soldier == soldier) return &record;
    return nullptr;
}
const S_CampaignOwnershipRecord *C_CampaignOwnership::FindRecord(const S_CampaignOwnershipSnapshot &state, SoldierId soldier) {
    for (const auto &record : state.m_Soldiers) if (record.m_Soldier == soldier) return &record;
    return nullptr;
}
std::optional<PlayerId> C_CampaignOwnership::FindOwner(SoldierId soldier) const {
    const auto *record = FindRecord(m_State, soldier);
    return record ? record->m_Owner : std::nullopt;
}
std::optional<SoldierId> C_CampaignOwnership::FindSoldier(PlayerId player) const {
    auto found = m_ByPlayer.find(player);
    return found == m_ByPlayer.end() ? std::nullopt : std::optional<SoldierId>(found->second);
}
std::optional<SoldierId> C_CampaignOwnership::ResolveActor(const ActorRef &actor) const {
    if (actor.m_Session != m_State.m_Context.m_Session || actor.m_Generation != m_State.m_Context.m_MissionGeneration)
        return std::nullopt;
    for (const auto &record : m_State.m_Soldiers) if (record.m_Actor == actor) return record.m_Soldier;
    return std::nullopt;
}
bool C_CampaignOwnership::IsLocallyOwned(SoldierId soldier) const {
    auto owner = FindOwner(soldier);
    return owner && owner == m_State.m_Context.m_LocalPlayer;
}
bool C_CampaignOwnership::IsAdmitted(PlayerId player) const {
    for (const auto &participant : m_State.m_Participants)
        if (participant.m_Player == player) return participant.m_Connected && participant.m_Admitted;
    return false;
}
bool C_CampaignOwnership::IsLive() const { return m_State.m_Context.m_Phase != E_CampaignPhase::Closed; }

E_CampaignReason C_CampaignOwnership::Commit(C_CampaignSession session,
    S_CampaignOwnershipSnapshot candidate, bool invalidateAll) {
    auto next = CampaignNextCounter(m_State.m_Revision);
    if (!next) return E_CampaignReason::WrongGeneration;
    auto handoffs = m_HandoffComplete;
    std::map<PlayerId, SoldierId> reverse;
    for (auto &record : candidate.m_Soldiers) {
        const auto *old = FindRecord(m_State, record.m_Soldier);
        if (!old) throw std::logic_error("ownership catalog identity changed");
        if (invalidateAll || !SameRecord(*old, record)) {
            auto control = CampaignNextCounter(old->m_ControlRevision);
            if (!control) return E_CampaignReason::WrongGeneration;
            record.m_ControlRevision = *control;
            handoffs[record.m_Soldier] = false;
        }
        if (record.m_Owner && !reverse.emplace(*record.m_Owner, record.m_Soldier).second)
            throw std::logic_error("ownership reverse index collision");
    }
    candidate.m_Context = session.GetContext();
    candidate.m_Revision = *next;
    m_Session = std::move(session);
    m_State = std::move(candidate);
    m_ByPlayer = std::move(reverse);
    m_HandoffComplete = std::move(handoffs);
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignOwnership::CheckPolicy(E_CampaignPolicy policy) const {
    const auto snapshot = m_Policy.GetSnapshot();
    std::optional<E_CampaignPolicyState> state;
    for (const auto &decision : snapshot.m_Decisions) {
        if (decision.m_Policy != policy) continue;
        if (state) return E_CampaignReason::PolicyRequired; // ambiguous injected policy fails closed
        state = decision.m_State;
    }
    if (!state || *state == E_CampaignPolicyState::Unresolved) return E_CampaignReason::PolicyRequired;
    return *state == E_CampaignPolicyState::Permitted ? E_CampaignReason::None : E_CampaignReason::Ineligible;
}
E_CampaignReason C_CampaignOwnership::CheckClaim(const S_CampaignOwnershipRecord &record) const {
    if (!record.m_Eligible) return E_CampaignReason::Ineligible;
    if (!record.m_Alive) return E_CampaignReason::Dead;
    if (record.m_Owner) return E_CampaignReason::AlreadyOwned;
    if (record.m_Scripted) return E_CampaignReason::Scripted;
    if (m_State.m_Context.m_Phase == E_CampaignPhase::Running) {
        if (!record.m_Deployed) return E_CampaignReason::NotDeployed;
        if (!record.m_Actor) return E_CampaignReason::ActorUnavailable;
    }
    return E_CampaignReason::None;
}
S_CampaignRequestResult C_CampaignOwnership::MakeResult(const S_CampaignOwnershipRequest &request,
    E_CampaignReason reason) const {
    S_CampaignRequestResult result;
    result.m_Context = request.m_Context;
    result.m_Family = E_CampaignRequestFamily::Ownership;
    result.m_Outcome = reason == E_CampaignReason::None ? E_CampaignRequestOutcome::Accepted : E_CampaignRequestOutcome::Rejected;
    result.m_Reason = reason;
    auto prep = m_Session.GetPreparationGeneration();
    if (prep.IsValid()) result.m_PreparationGeneration = prep;
    if (m_State.m_Context.m_MissionGeneration.IsValid()) result.m_MissionGeneration = m_State.m_Context.m_MissionGeneration;
    result.m_OwnershipRevision = m_State.m_Revision;
    result.m_State = GetSnapshot();
    return result;
}
S_CampaignRequestResult C_CampaignOwnership::ApplyRequest(PlayerId authenticatedSender,
    const S_CampaignOwnershipRequest &request) {
    auto reject = [&](E_CampaignReason reason) { return MakeResult(request, reason); };
    if (request.m_Context.m_Session != m_State.m_Context.m_Session) return reject(E_CampaignReason::WrongSession);
    if (authenticatedSender != request.m_Context.m_PlayerEcho || !IsAdmitted(authenticatedSender))
        return reject(E_CampaignReason::NotAdmitted);
    const auto phase = m_State.m_Context.m_Phase;
    if (phase == E_CampaignPhase::Frozen) return reject(E_CampaignReason::Frozen);
    if (phase != E_CampaignPhase::Setup && phase != E_CampaignPhase::Preparation && phase != E_CampaignPhase::Running)
        return reject(E_CampaignReason::InvalidPhase);
    if ((phase == E_CampaignPhase::Preparation && request.m_PreparationGeneration != m_Session.GetPreparationGeneration()) ||
        (phase == E_CampaignPhase::Running && request.m_MissionGeneration != m_State.m_Context.m_MissionGeneration) ||
        (request.m_PreparationGeneration && request.m_PreparationGeneration != m_Session.GetPreparationGeneration()) ||
        (request.m_MissionGeneration && request.m_MissionGeneration != m_State.m_Context.m_MissionGeneration))
        return reject(E_CampaignReason::WrongGeneration);
    if (request.m_Operation.valueless_by_exception()) return reject(E_CampaignReason::Malformed);
    const auto content = CanonicalContent(request);
    if (auto early = m_Journal.Begin(request.m_Context, E_CampaignRequestFamily::Ownership, content,
        MakeResult(request, E_CampaignReason::Malformed))) return *early;

    auto candidate = m_State;
    auto reason = E_CampaignReason::None;
    auto getTarget = [&](const S_CampaignOwnershipTarget &target) -> S_CampaignOwnershipRecord * {
        auto *record = FindRecord(candidate, target.m_Soldier);
        if (!record) reason = E_CampaignReason::InvalidSoldier;
        else if (record->m_ControlRevision != target.m_ExpectedControlRevision) reason = E_CampaignReason::StaleRevision;
        return reason == E_CampaignReason::None ? record : nullptr;
    };
    if (request.m_ExpectedOwnershipRevision != m_State.m_Revision) reason = E_CampaignReason::StaleRevision;
    else if (const auto *claim = std::get_if<S_CampaignClaim>(&request.m_Operation)) {
        reason = CheckPolicy(E_CampaignPolicy::SoldierSelection);
        if (reason == E_CampaignReason::None) {
            if (auto *record = getTarget(claim->m_Target)) {
                reason = CheckClaim(*record);
                if (reason == E_CampaignReason::None && FindSoldier(authenticatedSender)) reason = E_CampaignReason::AlreadyControlsSoldier;
                if (reason == E_CampaignReason::None) record->m_Owner = authenticatedSender;
            }
        }
    } else if (const auto *release = std::get_if<S_CampaignRelease>(&request.m_Operation)) {
        reason = CheckPolicy(E_CampaignPolicy::SwitchingAndSpectating);
        if (reason == E_CampaignReason::None) {
            if (auto *record = getTarget(release->m_Target)) {
                if (record->m_Owner != authenticatedSender) reason = E_CampaignReason::NotOwner;
                else record->m_Owner.reset();
            }
        }
    } else {
        const auto &change = std::get<S_CampaignSwitch>(request.m_Operation);
        reason = CheckPolicy(E_CampaignPolicy::SwitchingAndSpectating);
        if (reason == E_CampaignReason::None) reason = CheckPolicy(E_CampaignPolicy::SoldierSelection);
        if (reason == E_CampaignReason::None) {
            auto *from = getTarget(change.m_From);
            auto *to = reason == E_CampaignReason::None ? getTarget(change.m_To) : nullptr;
            if (from && to) {
                if (from == to) reason = E_CampaignReason::Malformed;
                else if (from->m_Owner != authenticatedSender) reason = E_CampaignReason::NotOwner;
                else reason = CheckClaim(*to);
                if (reason == E_CampaignReason::None) {
                    from->m_Owner.reset();
                    to->m_Owner = authenticatedSender;
                }
            }
        }
    }
    if (reason == E_CampaignReason::None) reason = Commit(m_Session, std::move(candidate));
    auto result = MakeResult(request, reason);
    m_Journal.Finish(request.m_Context, E_CampaignRequestFamily::Ownership, content, result);
    return result;
}

E_CampaignReason C_CampaignOwnership::Admit(const std::string &displayName, PlayerId &admitted) {
    const auto phase = m_State.m_Context.m_Phase;
    if (phase == E_CampaignPhase::Running) return E_CampaignReason::JoinNotSupported;
    if (phase != E_CampaignPhase::Setup && phase != E_CampaignPhase::Preparation) return E_CampaignReason::InvalidPhase;
    if (m_State.m_Participants.size() == 4) return E_CampaignReason::SessionFull;
    if (displayName.size() > 128) return E_CampaignReason::Malformed;
    auto next = CampaignNextCounter(m_LastPlayer);
    if (!next) return E_CampaignReason::WrongGeneration;
    auto candidate = m_State;
    candidate.m_Participants.push_back({*next, displayName, true, true, std::nullopt});
    auto reason = m_Journal.Admit(*next);
    if (reason != E_CampaignReason::None) return reason;
    reason = Commit(m_Session, std::move(candidate));
    if (reason != E_CampaignReason::None) { m_Journal.Forget(*next); return reason; }
    m_LastPlayer = *next;
    admitted = *next;
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignOwnership::OnDisconnected(PlayerId player) {
    if (!IsAdmitted(player)) return E_CampaignReason::None;
    if (!IsLive()) return E_CampaignReason::InvalidPhase;
    auto candidate = m_State;
    for (auto &record : candidate.m_Soldiers) if (record.m_Owner == player) record.m_Owner.reset();
    auto &participants = candidate.m_Participants;
    participants.erase(std::remove_if(participants.begin(), participants.end(),
        [&](const S_CampaignParticipant &participant) { return participant.m_Player == player; }), participants.end());
    auto reason = Commit(m_Session, std::move(candidate));
    if (reason == E_CampaignReason::None) m_Journal.Forget(player);
    // Frozen/Loading participant-set changes require the caller to abort/re-evaluate
    // its external barrier; departure is never a snapshot acknowledgment.
    return reason;
}
E_CampaignReason C_CampaignOwnership::OnDeath(SoldierId soldier) {
    if (!IsLive()) return E_CampaignReason::InvalidPhase;
    auto candidate = m_State;
    auto *record = FindRecord(candidate, soldier);
    if (!record) return E_CampaignReason::InvalidSoldier;
    if (!record->m_Alive) return E_CampaignReason::None;
    record->m_Alive = false;
    record->m_Owner.reset();
    return Commit(m_Session, std::move(candidate));
}
E_CampaignReason C_CampaignOwnership::SetAvailability(SoldierId soldier, bool eligible, bool deployed) {
    const auto phase = m_State.m_Context.m_Phase;
    if (phase != E_CampaignPhase::Setup && phase != E_CampaignPhase::Preparation && phase != E_CampaignPhase::Running)
        return E_CampaignReason::InvalidPhase;
    auto candidate = m_State;
    auto *record = FindRecord(candidate, soldier);
    if (!record) return E_CampaignReason::InvalidSoldier;
    if (deployed && !eligible) return E_CampaignReason::Ineligible;
    // Running cannot create another deployment/life through a catalog flag.
    if (phase == E_CampaignPhase::Running && deployed && !record->m_Deployed) return E_CampaignReason::InvalidPhase;
    if (record->m_Eligible == eligible && record->m_Deployed == deployed) return E_CampaignReason::None;
    record->m_Eligible = eligible;
    record->m_Deployed = deployed;
    if (!eligible || (phase == E_CampaignPhase::Running && !deployed)) record->m_Owner.reset();
    if (!deployed) record->m_Actor.reset();
    const auto count = std::count_if(candidate.m_Soldiers.begin(), candidate.m_Soldiers.end(),
        [](const S_CampaignOwnershipRecord &value) { return value.m_Deployed; });
    if (static_cast<std::size_t>(count) > m_SquadLimit) return E_CampaignReason::MissionRestriction;
    return Commit(m_Session, std::move(candidate));
}
E_CampaignReason C_CampaignOwnership::SetScripted(SoldierId soldier, bool scripted) {
    const auto phase = m_State.m_Context.m_Phase;
    if (phase != E_CampaignPhase::Loading && phase != E_CampaignPhase::Running) return E_CampaignReason::InvalidPhase;
    auto candidate = m_State;
    auto *record = FindRecord(candidate, soldier);
    if (!record) return E_CampaignReason::InvalidSoldier;
    if (record->m_Scripted == scripted) return E_CampaignReason::None;
    record->m_Scripted = scripted;
    return Commit(m_Session, std::move(candidate));
}
E_CampaignReason C_CampaignOwnership::BindActor(SoldierId soldier, const ActorRef &actor) {
    const auto phase = m_State.m_Context.m_Phase;
    if (phase != E_CampaignPhase::Loading && phase != E_CampaignPhase::Running) return E_CampaignReason::InvalidPhase;
    if (actor.m_Session != m_State.m_Context.m_Session) return E_CampaignReason::WrongSession;
    if (actor.m_Generation != m_State.m_Context.m_MissionGeneration) return E_CampaignReason::WrongGeneration;
    auto candidate = m_State;
    auto *record = FindRecord(candidate, soldier);
    if (!record) return E_CampaignReason::InvalidSoldier;
    if (!record->m_Deployed) return E_CampaignReason::NotDeployed;
    for (const auto &other : candidate.m_Soldiers)
        if (other.m_Soldier != soldier && other.m_Actor && other.m_Actor->m_Actor == actor.m_Actor)
            return E_CampaignReason::AlreadyOwned;
    if (record->m_Actor == actor) return E_CampaignReason::None;
    record->m_Actor = actor;
    return Commit(m_Session, std::move(candidate));
}
E_CampaignReason C_CampaignOwnership::UnbindActor(SoldierId soldier) {
    if (!IsLive()) return E_CampaignReason::InvalidPhase;
    auto candidate = m_State;
    auto *record = FindRecord(candidate, soldier);
    if (!record) return E_CampaignReason::InvalidSoldier;
    if (!record->m_Actor) return E_CampaignReason::None;
    record->m_Actor.reset();
    return Commit(m_Session, std::move(candidate));
}
E_CampaignReason C_CampaignOwnership::CompleteControlHandoff(const S_CampaignOwnershipTarget &target) {
    const auto phase = m_State.m_Context.m_Phase;
    if (phase != E_CampaignPhase::Loading && phase != E_CampaignPhase::Running) return E_CampaignReason::InvalidPhase;
    const auto *record = FindRecord(m_State, target.m_Soldier);
    if (!record) return E_CampaignReason::InvalidSoldier;
    if (record->m_ControlRevision != target.m_ExpectedControlRevision) return E_CampaignReason::StaleRevision;
    if (!record->m_Actor) return E_CampaignReason::ActorUnavailable;
    m_HandoffComplete.at(target.m_Soldier) = true;
    return E_CampaignReason::None;
}
std::optional<S_CampaignControlToken> C_CampaignOwnership::GetControlToken(PlayerId player) const {
    auto soldier = FindSoldier(player);
    if (!soldier) return std::nullopt;
    const auto *record = FindRecord(m_State, *soldier);
    if (!record || !record->m_Actor) return std::nullopt;
    S_CampaignControlToken token{m_State.m_Context.m_Session, m_State.m_Context.m_MissionGeneration,
        player, *soldier, *record->m_Actor, record->m_ControlRevision};
    return ValidateInput(player, token) == E_CampaignReason::None ? std::optional<S_CampaignControlToken>(token) : std::nullopt;
}
E_CampaignReason C_CampaignOwnership::ValidateInput(PlayerId authenticatedSender, const S_CampaignControlToken &token) const {
    if (token.m_Session != m_State.m_Context.m_Session) return E_CampaignReason::WrongSession;
    if (token.m_Generation != m_State.m_Context.m_MissionGeneration) return E_CampaignReason::WrongGeneration;
    if (!IsAdmitted(authenticatedSender) || token.m_Player != authenticatedSender) return E_CampaignReason::NotAdmitted;
    if (m_State.m_Context.m_Phase != E_CampaignPhase::Running) return E_CampaignReason::InvalidPhase;
    const auto *record = FindRecord(m_State, token.m_Soldier);
    if (!record) return E_CampaignReason::InvalidSoldier;
    if (record->m_ControlRevision != token.m_ControlRevision) return E_CampaignReason::StaleRevision;
    if (record->m_Owner != authenticatedSender) return E_CampaignReason::NotOwner;
    if (!record->m_Alive) return E_CampaignReason::Dead;
    if (!record->m_Eligible) return E_CampaignReason::Ineligible;
    if (!record->m_Deployed) return E_CampaignReason::NotDeployed;
    if (record->m_Scripted) return E_CampaignReason::Scripted;
    if (record->m_Actor != token.m_Actor) return E_CampaignReason::ActorUnavailable;
    if (!m_HandoffComplete.at(record->m_Soldier)) return E_CampaignReason::PendingTransaction;
    return E_CampaignReason::None;
}

E_CampaignReason C_CampaignOwnership::StartPreparation() {
    auto session = m_Session;
    auto reason = session.StartPreparation();
    if (reason != E_CampaignReason::None) return reason;
    auto candidate = m_State;
    for (auto &record : candidate.m_Soldiers) { record.m_Actor.reset(); record.m_Scripted = false; }
    return Commit(std::move(session), std::move(candidate), true);
}
E_CampaignReason C_CampaignOwnership::Freeze() {
    auto session = m_Session;
    auto reason = session.Freeze();
    return reason == E_CampaignReason::None ? Commit(std::move(session), m_State, true) : reason;
}
E_CampaignReason C_CampaignOwnership::BeginMissionGeneration(MissionKey mission) {
    auto session = m_Session;
    auto reason = session.BeginMissionGeneration(std::move(mission));
    if (reason != E_CampaignReason::None) return reason;
    auto candidate = m_State;
    for (auto &record : candidate.m_Soldiers) {
        record.m_Actor.reset();
        record.m_Scripted = false;
        if (!record.m_Eligible || !record.m_Alive || !record.m_Deployed ||
            (record.m_Owner && !IsAdmitted(*record.m_Owner))) record.m_Owner.reset();
    }
    return Commit(std::move(session), std::move(candidate), true);
}
E_CampaignReason C_CampaignOwnership::StartSimulation() {
    if (m_State.m_Context.m_Phase != E_CampaignPhase::Loading) return E_CampaignReason::InvalidPhase;
    for (const auto &record : m_State.m_Soldiers) {
        if (!record.m_Alive || !record.m_Eligible || !record.m_Deployed) continue;
        if (!record.m_Actor) return E_CampaignReason::ActorUnavailable;
        if (!m_HandoffComplete.at(record.m_Soldier)) return E_CampaignReason::PendingTransaction;
    }
    auto session = m_Session;
    auto reason = session.StartSimulation();
    // Loading already fenced controls; completion opens that exact revision.
    return reason == E_CampaignReason::None ? Commit(std::move(session), m_State) : reason;
}
E_CampaignReason C_CampaignOwnership::EndMission() {
    auto session = m_Session;
    auto reason = session.EndMission();
    return reason == E_CampaignReason::None ? Commit(std::move(session), m_State, true) : reason;
}
E_CampaignReason C_CampaignOwnership::ReturnToSetup(MissionKey mission) {
    auto session = m_Session;
    auto reason = session.ReturnToSetup(std::move(mission));
    if (reason != E_CampaignReason::None) return reason;
    auto candidate = m_State;
    for (auto &record : candidate.m_Soldiers) { record.m_Actor.reset(); record.m_Scripted = false; }
    return Commit(std::move(session), std::move(candidate), true);
}
E_CampaignReason C_CampaignOwnership::Close() {
    if (!IsLive()) return E_CampaignReason::None;
    auto session = m_Session;
    session.Close();
    auto candidate = m_State;
    for (auto &record : candidate.m_Soldiers) { record.m_Owner.reset(); record.m_Actor.reset(); record.m_Scripted = false; }
    const auto participants = candidate.m_Participants;
    candidate.m_Participants.clear();
    auto reason = Commit(std::move(session), std::move(candidate), true);
    if (reason == E_CampaignReason::None)
        for (const auto &participant : participants) m_Journal.Forget(participant.m_Player);
    return reason;
}
