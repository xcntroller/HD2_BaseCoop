#include "campaign_mission_adapter.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace {
using R = E_CampaignReason;
using P = E_CampaignPhase;
C_CampaignMissionAdapter *g_CampaignMission = nullptr;
const I_CampaignOwnershipView *g_CampaignView = nullptr;
bool SameRules(const S_CampaignRulesDescriptor &a, const S_CampaignRulesDescriptor &b) {
    const auto &x = a.m_Content; const auto &y = b.m_Content;
    return a.m_Difficulty == b.m_Difficulty && x.m_CampaignContent == y.m_CampaignContent &&
        x.m_MissionContent == y.m_MissionContent && x.m_SoldierContent == y.m_SoldierContent &&
        x.m_ItemContent == y.m_ItemContent && x.m_Rules == y.m_Rules;
}
bool SameRecord(const S_CampaignOwnershipRecord &a, const S_CampaignOwnershipRecord &b) {
    return a.m_Soldier == b.m_Soldier && a.m_Eligible == b.m_Eligible && a.m_Deployed == b.m_Deployed &&
        a.m_Alive == b.m_Alive && a.m_Owner == b.m_Owner && a.m_Actor == b.m_Actor &&
        a.m_Scripted == b.m_Scripted && a.m_ControlRevision == b.m_ControlRevision;
}
}
C_CampaignMissionAdapter::C_CampaignMissionAdapter(C_CampaignOwnership &ownership) : m_Ownership(ownership) {}
S_CampaignSessionContext C_CampaignMissionAdapter::GetContext() const { return m_Ownership.GetSnapshot()->m_Context; }
bool C_CampaignMissionAdapter::HasAuthority() const {
    const auto c = GetContext();
    return c.m_Kind == E_CampaignSessionKind::CampaignCoop && c.m_UsesCampaignRules && c.m_IsTransportServer;
}
E_CampaignReason C_CampaignMissionAdapter::Begin(const S_CampaignMissionManifest &frozen) {
    if (!HasAuthority()) return R::NotHost;
    const auto state = m_Ownership.GetSnapshot(); const auto &c = state->m_Context;
    if (c.m_Phase != P::Frozen || frozen.m_Context.m_Phase != P::Frozen) return R::InvalidPhase;
    if (frozen.m_Context.m_Session != c.m_Session || frozen.m_Context.m_Campaign != c.m_Campaign) return R::WrongSession;
    if (frozen.m_Context.m_Kind != c.m_Kind || !frozen.m_Context.m_UsesCampaignRules ||
        frozen.m_Context.m_HostPlayer != c.m_HostPlayer) return R::RulesMismatch;
    if (frozen.m_Context.m_Mission != c.m_Mission || !SameRules(frozen.m_Context.m_Rules, c.m_Rules)) return R::ContentMismatch;
    if (frozen.m_PreparationGeneration != m_Ownership.GetSession().GetPreparationGeneration() ||
        frozen.m_Context.m_MissionGeneration != c.m_MissionGeneration) return R::WrongGeneration;
    if (frozen.m_OwnershipRevision != state->m_Revision) return R::StaleRevision;
    if (!frozen.m_FrozenPreparationRevision.IsValid() || !frozen.m_ViewRevision.IsValid() ||
        frozen.m_Snapshot.IsValid() || !frozen.m_Bindings.empty() || frozen.m_Loadouts.empty() ||
        frozen.m_Loadouts.size() > 4 || frozen.m_Soldiers.size() > C_CampaignSession::kMaxRoster ||
        frozen.m_RequiredParticipants.empty() || frozen.m_RequiredParticipants.size() > 4) return R::Malformed;
    std::set<PlayerId> peers;
    for (auto p : frozen.m_RequiredParticipants) {
        if (!p.IsValid() || !peers.insert(p).second) return R::Malformed;
        const auto found = std::find_if(state->m_Participants.begin(), state->m_Participants.end(),
            [&](const auto &v) { return v.m_Player == p && v.m_Connected && v.m_Admitted; });
        if (found == state->m_Participants.end()) return R::NotAdmitted;
    }
    if (!c.m_HostPlayer || !peers.count(*c.m_HostPlayer)) return R::NotHost;
    std::set<SoldierId> presentations, loadouts, deployed;
    for (const auto &s : frozen.m_Soldiers) {
        if (!presentations.insert(s.m_Soldier).second || s.m_Soldier != s.m_Ownership.m_Soldier) return R::InvalidSoldier;
        const auto found = std::find_if(state->m_Soldiers.begin(), state->m_Soldiers.end(),
            [&](const auto &v) { return v.m_Soldier == s.m_Soldier; });
        if (found == state->m_Soldiers.end() || !SameRecord(*found, s.m_Ownership)) return R::StaleRevision;
    }
    for (const auto &s : state->m_Soldiers) if (s.m_Deployed) {
        if (!s.m_Eligible || !s.m_Alive || !presentations.count(s.m_Soldier)) return R::Ineligible;
        if (s.m_Owner && !peers.count(*s.m_Owner)) return R::NotAdmitted;
        deployed.insert(s.m_Soldier);
    }
    for (const auto &l : frozen.m_Loadouts) {
        if (!loadouts.insert(l.m_Soldier).second || !deployed.count(l.m_Soldier)) return R::InvalidSoldier;
        std::set<PreparationItemId> entries;
        for (const auto &i : l.m_Items) if (!i.m_Entry.IsValid() || !entries.insert(i.m_Entry).second ||
            !i.m_State.m_Item.IsValid() || !i.m_State.m_Quantity || !i.m_Placement.m_Slot.IsValid()) return R::InvalidItem;
        for (const auto &s : l.m_Skills) if (!s.m_Key.IsValid() || !std::isfinite(s.m_Value)) return R::Malformed;
    }
    if (deployed != loadouts) return R::InvalidSoldier;
    auto next = CampaignNextCounter(m_LastSnapshot); if (!next) return R::WrongGeneration;
    const auto reason = m_Ownership.BeginMissionGeneration(c.m_Mission);
    if (reason != R::None) return reason;
    m_LastSnapshot = *next; m_Manifest.reset(); m_Acknowledged.clear(); m_Failure = R::None;
    m_Loading = frozen; m_Loading->m_Snapshot = *next;
    m_Loading->m_Context = GetContext();
    m_LoadingRevision = m_Ownership.GetSnapshot()->m_Revision;
    return R::None;
}
E_CampaignReason C_CampaignMissionAdapter::Bind(SoldierId soldier, const ActorRef &actor) {
    if (!m_Loading || m_Manifest || GetContext().m_Phase != P::Loading || m_Failure != R::None) return R::InvalidPhase;
    if (std::none_of(m_Loading->m_Loadouts.begin(), m_Loading->m_Loadouts.end(),
        [&](const auto &l) { return l.m_Soldier == soldier; })) return R::InvalidSoldier;
    if (m_Ownership.GetSnapshot()->m_Revision != m_LoadingRevision) return R::StaleRevision;
    const auto reason = m_Ownership.BindActor(soldier, actor);
    if (reason == R::None) m_LoadingRevision = m_Ownership.GetSnapshot()->m_Revision;
    return reason;
}
E_CampaignReason C_CampaignMissionAdapter::SealInitialWorld() {
    if (!m_Loading || m_Manifest || GetContext().m_Phase != P::Loading || m_Failure != R::None) return R::InvalidPhase;
    auto candidate = *m_Loading; const auto state = m_Ownership.GetSnapshot();
    if (state->m_Revision != m_LoadingRevision) return R::StaleRevision;
    // Any membership/rights change invalidates the frozen cut. Binding alone is
    // allowed; BeginMissionGeneration and BindActor legitimately advance revisions.
    for (auto p : candidate.m_RequiredParticipants) {
        if (std::none_of(state->m_Participants.begin(), state->m_Participants.end(),
            [&](const auto &v) { return v.m_Player == p && v.m_Connected && v.m_Admitted; })) return R::Disconnected;
    }
    for (auto &s : candidate.m_Soldiers) {
        const auto found = std::find_if(state->m_Soldiers.begin(), state->m_Soldiers.end(),
            [&](const auto &v) { return v.m_Soldier == s.m_Soldier; });
        if (found == state->m_Soldiers.end()) return R::InvalidSoldier;
        const auto &old = s.m_Ownership;
        if (old.m_Owner != found->m_Owner || old.m_Alive != found->m_Alive || old.m_Eligible != found->m_Eligible ||
            old.m_Deployed != found->m_Deployed || found->m_Scripted) return R::StaleRevision;
        s.m_Ownership = *found;
        if (found->m_Deployed) {
            if (!found->m_Actor) return R::ActorUnavailable;
            candidate.m_Bindings.push_back({found->m_Soldier, *found->m_Actor, found->m_ControlRevision});
        }
    }
    candidate.m_Context = state->m_Context; candidate.m_OwnershipRevision = state->m_Revision;
    m_Manifest = std::make_shared<const S_CampaignMissionManifest>(std::move(candidate));
    return R::None;
}
E_CampaignReason C_CampaignMissionAdapter::CheckBaseline() const {
    if (m_Failure != R::None) return m_Failure;
    if (!m_Manifest) return R::SnapshotUnavailable;
    const auto state = m_Ownership.GetSnapshot();
    if (state->m_Context.m_Session != m_Manifest->m_Context.m_Session) return R::WrongSession;
    if (state->m_Context.m_MissionGeneration != m_Manifest->m_Context.m_MissionGeneration) return R::WrongGeneration;
    if (state->m_Revision != m_Manifest->m_OwnershipRevision) return R::StaleRevision;
    return R::None;
}
std::optional<S_CampaignSnapshotAck> C_CampaignMissionAdapter::ExpectedAck(PlayerId player) const {
    if (CheckBaseline() != R::None || std::find(m_Manifest->m_RequiredParticipants.begin(),
        m_Manifest->m_RequiredParticipants.end(), player) == m_Manifest->m_RequiredParticipants.end()) return std::nullopt;
    const auto &m = *m_Manifest;
    return S_CampaignSnapshotAck{m.m_Context.m_Session, player, m.m_Context.m_MissionGeneration,
        m.m_Snapshot, m.m_PreparationGeneration, m.m_FrozenPreparationRevision, m.m_ViewRevision, m.m_OwnershipRevision};
}
E_CampaignReason C_CampaignMissionAdapter::Acknowledge(PlayerId sender, const S_CampaignSnapshotAck &ack) {
    if (GetContext().m_Phase != P::Loading) return R::InvalidPhase;
    auto reason = CheckBaseline(); if (reason != R::None) return reason;
    const auto expected = ExpectedAck(sender); if (!expected || ack.m_PlayerEcho != sender) return R::NotAdmitted;
    if (ack.m_Session != expected->m_Session) return R::WrongSession;
    if (ack.m_Generation != expected->m_Generation || ack.m_PreparationGeneration != expected->m_PreparationGeneration) return R::WrongGeneration;
    if (ack.m_Snapshot != expected->m_Snapshot) return R::ContentMismatch;
    if (ack.m_PreparationRevision != expected->m_PreparationRevision || ack.m_ViewRevision != expected->m_ViewRevision ||
        ack.m_OwnershipRevision != expected->m_OwnershipRevision) return R::StaleRevision;
    m_Acknowledged.insert(sender); return R::None;
}
E_CampaignReason C_CampaignMissionAdapter::Start(S_CampaignStartSimulation &start) {
    auto reason = CheckBaseline(); if (reason != R::None) return reason;
    for (auto p : m_Manifest->m_RequiredParticipants) if (!m_Acknowledged.count(p)) return R::SnapshotUnavailable;
    reason = m_Ownership.StartSimulation(); if (reason != R::None) return reason;
    start = {m_Manifest->m_Context.m_Session, m_Manifest->m_Context.m_MissionGeneration, m_Manifest->m_Snapshot};
    return R::None;
}
E_CampaignReason C_CampaignMissionAdapter::End() { return m_Ownership.EndMission(); }
void C_CampaignMissionAdapter::Abort(E_CampaignReason reason) {
    m_Failure = reason == R::None ? R::SnapshotUnavailable : reason;
    m_Acknowledged.clear();
    if (GetContext().m_Phase == P::Running) m_Ownership.EndMission();
}
bool C_CampaignMissionAdapter::CanSimulate() const {
    return m_Failure == R::None && GetContext().m_Phase == P::Running;
}
C_CampaignMissionReconstruction::C_CampaignMissionReconstruction(SessionId session, PlayerId player,
    const I_CampaignOwnershipView &view) : m_Session(session), m_Player(player), m_View(view) {}
void C_CampaignMissionReconstruction::Invalidate() { m_Ack.reset(); m_Started = false; }
E_CampaignReason C_CampaignMissionReconstruction::Reconstruct(const S_CampaignMissionManifest &m,
    const std::vector<std::uint8_t> &world, const T_Reconstruct &reconstruct) {
    Invalidate();
    if (m.m_Context.m_Session != m_Session) return R::WrongSession;
    if (!m_Player.IsValid() || !m.m_Snapshot.IsValid() || !m.m_Context.m_MissionGeneration.IsValid()) return R::Malformed;
    if (m.m_Context.m_MissionGeneration < m_HighestGeneration ||
        !(m_HighestSnapshot < m.m_Snapshot)) return R::WrongGeneration;
    // Observe the floor even if reconstruction fails: delayed older transfers
    // cannot make a failed newer baseline current again.
    m_HighestGeneration = m.m_Context.m_MissionGeneration; m_HighestSnapshot = m.m_Snapshot;
    if (m.m_Context.m_Phase != P::Loading || m.m_Context.m_Kind != E_CampaignSessionKind::CampaignCoop ||
        !m.m_Context.m_UsesCampaignRules) return R::InvalidPhase;
    if (world.empty() || world.size() > 8 * 1024 * 1024 || !reconstruct ||
        m.m_Bindings.empty() || m.m_Bindings.size() > 4 || m.m_Soldiers.size() > 256 ||
        m.m_RequiredParticipants.empty() || m.m_RequiredParticipants.size() > 4 ||
        !m.m_PreparationGeneration.IsValid() || !m.m_FrozenPreparationRevision.IsValid() ||
        !m.m_ViewRevision.IsValid() || !m.m_OwnershipRevision.IsValid()) return R::Malformed;
    std::set<PlayerId> peers;
    for (auto p : m.m_RequiredParticipants) if (!p.IsValid() || !peers.insert(p).second) return R::Malformed;
    if (!peers.count(m_Player)) return R::NotAdmitted;
    std::set<SoldierId> soldiers; std::set<std::uint32_t> actors;
    for (const auto &b : m.m_Bindings) {
        if (!b.m_Soldier.IsValid() || !b.m_ControlRevision.IsValid() || !soldiers.insert(b.m_Soldier).second ||
            !actors.insert(b.m_Actor.m_Actor.Value()).second || b.m_Actor.m_Session != m_Session ||
            b.m_Actor.m_Generation != m.m_Context.m_MissionGeneration) return R::Malformed;
        const auto s = std::find_if(m.m_Soldiers.begin(), m.m_Soldiers.end(), [&](const auto &v) { return v.m_Soldier == b.m_Soldier; });
        if (s == m.m_Soldiers.end() || !s->m_Ownership.m_Deployed || !s->m_Ownership.m_Alive ||
            !s->m_Ownership.m_Eligible || s->m_Ownership.m_Actor != std::optional<ActorRef>(b.m_Actor) ||
            s->m_Ownership.m_ControlRevision != b.m_ControlRevision) return R::Malformed;
    }
    for (const auto &s : m.m_Soldiers) if (s.m_Ownership.m_Deployed && !soldiers.count(s.m_Soldier)) return R::ActorUnavailable;
    const auto reason = reconstruct(m, world); if (reason != R::None) return reason;
    const auto installed = m_View.GetSnapshot();
    if (!installed || installed->m_Context.m_Phase != P::Loading || installed->m_Context.m_Session != m_Session ||
        installed->m_Context.m_MissionGeneration != m.m_Context.m_MissionGeneration ||
        installed->m_Context.m_Campaign != m.m_Context.m_Campaign || installed->m_Context.m_Mission != m.m_Context.m_Mission ||
        !SameRules(installed->m_Context.m_Rules, m.m_Context.m_Rules) || installed->m_Revision != m.m_OwnershipRevision) return R::SnapshotUnavailable;
    for (const auto &s : m.m_Soldiers) {
        const auto r = std::find_if(installed->m_Soldiers.begin(), installed->m_Soldiers.end(), [&](const auto &v) { return v.m_Soldier == s.m_Soldier; });
        if (r == installed->m_Soldiers.end() || !SameRecord(*r,s.m_Ownership)) return R::ActorUnavailable;
    }
    m_Ack = S_CampaignSnapshotAck{m_Session,m_Player,m.m_Context.m_MissionGeneration,m.m_Snapshot,
        m.m_PreparationGeneration,m.m_FrozenPreparationRevision,m.m_ViewRevision,m.m_OwnershipRevision};
    return R::None;
}
std::optional<S_CampaignSnapshotAck> C_CampaignMissionReconstruction::GetAcknowledgment() const {
    if (!m_Ack) return std::nullopt;
    const auto state = m_View.GetSnapshot();
    if (!state || state->m_Context.m_Session != m_Session || state->m_Context.m_MissionGeneration != m_Ack->m_Generation ||
        state->m_Context.m_Phase != P::Loading || state->m_Revision != m_Ack->m_OwnershipRevision) return std::nullopt;
    return m_Ack;
}
E_CampaignReason C_CampaignMissionReconstruction::AcceptStart(const S_CampaignStartSimulation &start) {
    if (!GetAcknowledgment()) return R::SnapshotUnavailable;
    if (start.m_Session != m_Session) return R::WrongSession;
    if (start.m_Generation != m_Ack->m_Generation) return R::WrongGeneration;
    if (start.m_Snapshot != m_Ack->m_Snapshot) return R::ContentMismatch;
    m_Started = true; return R::None;
}
C_CampaignMissionAdapter *GetCampaignMissionAdapter() { return g_CampaignMission; }
void SetCampaignMissionAdapter(C_CampaignMissionAdapter *adapter) { g_CampaignMission = adapter; }
void InstallCampaignMissionView(const I_CampaignOwnershipView *view) { g_CampaignView = view; }
T_CampaignOwnershipSnapshot GetCampaignMissionSnapshot() {
    if (g_CampaignMission) return g_CampaignMission->GetOwnership().GetSnapshot();
    return g_CampaignView ? g_CampaignView->GetSnapshot() : nullptr;
}
bool IsCampaignMission() {
    const auto state = GetCampaignMissionSnapshot();
    return state && state->m_Context.m_Kind == E_CampaignSessionKind::CampaignCoop && state->m_Context.m_UsesCampaignRules;
}
bool IsCampaignSimulationAuthority() {
    const auto state = GetCampaignMissionSnapshot();
    return IsCampaignMission() && state->m_Context.m_IsTransportServer;
}
bool CanCampaignSimulate() {
    const auto state = GetCampaignMissionSnapshot();
    return !IsCampaignMission() || (state->m_Context.m_Phase == P::Running &&
        (!g_CampaignMission || g_CampaignMission->CanSimulate()));
}
