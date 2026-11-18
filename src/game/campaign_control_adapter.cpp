#include "campaign_control_adapter.h"
#include <algorithm>

namespace {
bool SameBinding(const S_CampaignOwnershipRecord &a, const S_CampaignOwnershipRecord &b) {
    return a.m_Soldier == b.m_Soldier && a.m_Actor == b.m_Actor && a.m_ControlRevision == b.m_ControlRevision;
}
bool SameCut(const S_CampaignOwnershipSnapshot &a, const S_CampaignOwnershipSnapshot &b) {
    return a.m_Context.m_Session == b.m_Context.m_Session &&
        a.m_Context.m_MissionGeneration == b.m_Context.m_MissionGeneration &&
        a.m_Context.m_Phase == b.m_Context.m_Phase && a.m_Revision == b.m_Revision;
}
}
C_CampaignControlAdapter::C_CampaignControlAdapter(C_CampaignOwnership &ownership, I_CampaignControlSink &sink)
    : m_Ownership(ownership), m_Sink(sink) {}

S_CampaignControllerPlan C_CampaignControlAdapter::Plan(const S_CampaignSessionContext &context,
    const S_CampaignOwnershipRecord &record) {
    S_CampaignControllerPlan plan;
    plan.m_Record = record;
    if (context.m_Kind != E_CampaignSessionKind::CampaignCoop || !record.m_Actor ||
        !record.m_Alive || !record.m_Eligible || !record.m_Deployed ||
        (context.m_Phase != E_CampaignPhase::Loading && context.m_Phase != E_CampaignPhase::Running)) return plan;
    const bool server = context.m_IsTransportServer;
    if (record.m_Scripted) plan.m_Controller = server ? E_CampaignController::Scripted : E_CampaignController::Replica;
    else if (record.m_Owner && record.m_Owner == context.m_LocalPlayer) {
        plan.m_Controller = E_CampaignController::LocalPlayer;
        plan.m_LocalView = true;
    } else if (record.m_Owner) plan.m_Controller = server ? E_CampaignController::RemotePlayer : E_CampaignController::Replica;
    else plan.m_Controller = server ? E_CampaignController::SquadAI : E_CampaignController::Replica;
    return plan;
}
bool C_CampaignControlAdapter::IsSynchronized() const {
    const auto current = m_Ownership.GetSnapshot();
    return m_Ready && m_Applied && SameCut(*m_Applied, *current);
}
E_CampaignReason C_CampaignControlAdapter::SynchronizeCommitted() {
    const auto state = m_Ownership.GetSnapshot();
    if (state->m_Context.m_Kind != E_CampaignSessionKind::CampaignCoop) return E_CampaignReason::WrongSession;
    if (IsSynchronized()) return E_CampaignReason::None;
    m_Ready = false;
    m_Inputs.clear();
    const auto changed = [&](const S_CampaignOwnershipRecord &record) {
        if (!m_Applied) return true;
        const auto oldPlan = Plan(m_Applied->m_Context, record);
        const auto newPlan = Plan(state->m_Context, record);
        if (oldPlan.m_Controller != newPlan.m_Controller) return true;
        for (const auto &old : m_Applied->m_Soldiers) if (SameBinding(old, record)) return false;
        return true;
    };
    // Failed ports are retried from the beginning, while input stays disabled.
    if (m_Applied) for (const auto &record : m_Applied->m_Soldiers)
        if (record.m_Actor) {
            bool retained = false;
            for (const auto &next : state->m_Soldiers) if (SameBinding(record, next)) retained = true;
            if (!retained && !m_Sink.Drain(*record.m_Actor)) return E_CampaignReason::PendingTransaction;
        }
    for (const auto &record : state->m_Soldiers)
        if (record.m_Actor && changed(record) && !m_Sink.Drain(*record.m_Actor)) return E_CampaignReason::PendingTransaction;
    std::optional<ActorRef> local;
    for (const auto &record : state->m_Soldiers) {
        if (!record.m_Actor) continue;
        const auto plan = Plan(state->m_Context, record);
        if (changed(record) && !m_Sink.Apply(plan)) return E_CampaignReason::PendingTransaction;
        if (plan.m_LocalView) local = record.m_Actor;
    }
    if (!m_Sink.Present(local)) return E_CampaignReason::PendingTransaction;
    if (!SameCut(*state, *m_Ownership.GetSnapshot())) return E_CampaignReason::StaleRevision;
    const auto phase = state->m_Context.m_Phase;
    if (phase == E_CampaignPhase::Loading || phase == E_CampaignPhase::Running)
        for (const auto &record : state->m_Soldiers) if (record.m_Actor) {
            const auto reason = m_Ownership.CompleteControlHandoff({record.m_Soldier, record.m_ControlRevision});
            if (reason != E_CampaignReason::None) return reason;
        }
    // Retain sequence fences across same-token reconciliations; remove disconnected
    // incarnations only. A new token naturally replaces the protocol gate baseline.
    for (auto it = m_Gates.begin(); it != m_Gates.end();) {
        const bool live = std::any_of(state->m_Participants.begin(), state->m_Participants.end(),
            [&](const S_CampaignParticipant &p) { return p.m_Player == it->first && p.m_Connected && p.m_Admitted; });
        if (!live) it = m_Gates.erase(it); else ++it;
    }
    m_Applied = state;
    m_Ready = true;
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignControlAdapter::OnDisconnected(PlayerId exactIncarnation) {
    const auto reason = m_Ownership.OnDisconnected(exactIncarnation);
    if (reason != E_CampaignReason::None) return reason;
    return SynchronizeCommitted();
}
E_CampaignReason C_CampaignControlAdapter::Enqueue(PlayerId sender, const CampaignProtocol::S_Input &input) {
    auto reason = m_Ownership.ValidateInput(sender, input.m_Control);
    if (reason != E_CampaignReason::None) return reason;
    if (!IsSynchronized()) return E_CampaignReason::PendingTransaction;
    if (m_Inputs.size() >= kMaxQueuedInputs) return E_CampaignReason::CapacityExceeded;
    const auto state = m_Ownership.GetSnapshot();
    const auto record = std::find_if(state->m_Soldiers.begin(), state->m_Soldiers.end(),
        [&](const S_CampaignOwnershipRecord &r) { return r.m_Soldier == input.m_Control.m_Soldier; });
    if (record == state->m_Soldiers.end()) return E_CampaignReason::InvalidSoldier;
    reason = m_Gates[sender].Validate(input, sender, state->m_Context, *record, true, true);
    if (reason != E_CampaignReason::None) return reason;
    m_Inputs.push_back({sender, input});
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignControlAdapter::ApplyNext() {
    if (m_Inputs.empty()) return E_CampaignReason::None;
    const auto queued = std::move(m_Inputs.front());
    m_Inputs.pop_front();
    const auto reason = m_Ownership.ValidateInput(queued.m_Sender, queued.m_Input.m_Control);
    if (reason != E_CampaignReason::None) return reason;
    if (!IsSynchronized()) return E_CampaignReason::PendingTransaction;
    m_Sink.Execute(queued.m_Input);
    return E_CampaignReason::None;
}

E_CampaignReason C_CampaignControlAdapter::OnScripted(SoldierId soldier, bool scripted) {
    // Script VM callbacks may be executing on the controller being replaced.
    // Commit now; the authority queue reconciles once that stack has returned.
    return m_Ownership.SetScripted(soldier, scripted);
}
E_CampaignReason C_CampaignControlAdapter::OnDeath(SoldierId soldier) {
    // Do not replace/delete a controller from its own OnDeath call stack. The
    // authority queue reconciles after that event returns, before further input.
    return m_Ownership.OnDeath(soldier);
}

E_CampaignReason C_CampaignReplicaControlAdapter::InstallCommitted(T_CampaignOwnershipSnapshot snapshot) {
    if (!snapshot) return E_CampaignReason::SnapshotUnavailable;
    if (snapshot->m_Context.m_Session != m_Session ||
        snapshot->m_Context.m_Kind != E_CampaignSessionKind::CampaignCoop) return E_CampaignReason::WrongSession;
    CampaignProtocol::T_Bytes encoded;
    if (CampaignProtocol::Encode(CampaignProtocol::T_Message(*snapshot), encoded) != E_CampaignReason::None)
        return E_CampaignReason::Malformed;
    if (m_View) {
        if (snapshot->m_Context.m_MissionGeneration < m_View->m_Context.m_MissionGeneration)
            return E_CampaignReason::WrongGeneration;
        if (snapshot->m_Revision < m_View->m_Revision) return E_CampaignReason::StaleRevision;
        if (snapshot->m_Revision == m_View->m_Revision && encoded == m_LastWire && m_Ready)
            return E_CampaignReason::None;
        // Phase can advance without catalog revision (Loading -> Running), but
        // a same-revision different catalog must not overwrite committed state.
        if (snapshot->m_Revision == m_View->m_Revision && encoded != m_LastWire) {
            auto normalized = *snapshot;
            normalized.m_Context.m_Phase = m_View->m_Context.m_Phase;
            normalized.m_Context.m_LocalPlayer = m_View->m_Context.m_LocalPlayer;
            normalized.m_Context.m_IsTransportServer = m_View->m_Context.m_IsTransportServer;
            normalized.m_Context.m_IsTransportClient = m_View->m_Context.m_IsTransportClient;
            CampaignProtocol::T_Bytes normalizedWire, oldWire;
            if (CampaignProtocol::Encode(CampaignProtocol::T_Message(normalized), normalizedWire) != E_CampaignReason::None ||
                CampaignProtocol::Encode(CampaignProtocol::T_Message(*m_View), oldWire) != E_CampaignReason::None ||
                normalizedWire != oldWire) return E_CampaignReason::RequestConflict;
            const auto from = m_View->m_Context.m_Phase;
            const auto to = snapshot->m_Context.m_Phase;
            if (to != from && !(from == E_CampaignPhase::Loading && to == E_CampaignPhase::Running) &&
                !(from == E_CampaignPhase::Preparation && to == E_CampaignPhase::Frozen)) return E_CampaignReason::InvalidPhase;
        }
    }
    m_Ready = false;
    auto localView = *snapshot;
    localView.m_Context.m_IsTransportServer = false;
    localView.m_Context.m_IsTransportClient = true;
    localView.m_Context.m_LocalPlayer = m_LocalPlayer;
    // Native Resolve uses this new immutable catalog while installing handlers;
    // caller must withhold input/world acknowledgment until this method succeeds.
    m_View = std::make_shared<const S_CampaignOwnershipSnapshot>(std::move(localView));
    m_LastWire = std::move(encoded);
    const auto changed = [&](const S_CampaignOwnershipRecord &record) {
        if (!m_Applied) return true;
        for (const auto &old : m_Applied->m_Soldiers)
            if (SameBinding(record, old) &&
                C_CampaignControlAdapter::Plan(m_Applied->m_Context, old).m_Controller ==
                C_CampaignControlAdapter::Plan(m_View->m_Context, record).m_Controller) return false;
        return true;
    };
    if (m_Applied) for (const auto &record : m_Applied->m_Soldiers) if (record.m_Actor) {
        bool retained = false;
        for (const auto &next : m_View->m_Soldiers) if (SameBinding(record, next)) retained = true;
        if (!retained && !m_Sink.Drain(*record.m_Actor)) return E_CampaignReason::PendingTransaction;
    }
    for (const auto &record : m_View->m_Soldiers)
        if (record.m_Actor && changed(record) && !m_Sink.Drain(*record.m_Actor)) return E_CampaignReason::PendingTransaction;
    std::optional<ActorRef> local;
    for (const auto &record : m_View->m_Soldiers) if (record.m_Actor) {
        const auto plan = C_CampaignControlAdapter::Plan(m_View->m_Context, record);
        if (changed(record) && !m_Sink.Apply(plan)) return E_CampaignReason::PendingTransaction;
        if (plan.m_LocalView) local = record.m_Actor;
    }
    if (!m_Sink.Present(local)) return E_CampaignReason::PendingTransaction;
    m_Applied = m_View;
    m_Ready = true;
    return E_CampaignReason::None;
}

E_CampaignReason C_CampaignControlAdapter::RetireBeforeReconstruction() {
    m_Ready = false;
    m_Inputs.clear();
    if (m_Applied) for (const auto &record : m_Applied->m_Soldiers)
        if (record.m_Actor && !m_Sink.Drain(*record.m_Actor)) return E_CampaignReason::PendingTransaction;
    if (!m_Sink.Present(std::nullopt)) return E_CampaignReason::PendingTransaction;
    m_Applied.reset(); // retries in the same world must reinstall cleared handlers
    return E_CampaignReason::None;
}

E_CampaignReason C_CampaignReplicaControlAdapter::RetireBeforeReconstruction() {
    m_Ready = false;
    if (m_Applied) for (const auto &record : m_Applied->m_Soldiers)
        if (record.m_Actor && !m_Sink.Drain(*record.m_Actor)) return E_CampaignReason::PendingTransaction;
    if (!m_Sink.Present(std::nullopt)) return E_CampaignReason::PendingTransaction;
    m_Applied.reset();
    return E_CampaignReason::None;
}
