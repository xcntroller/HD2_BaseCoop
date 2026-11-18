#include "game/campaign_preparation_live.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace {

using R = E_CampaignReason;
using Phase = E_CampaignPhase;
using Family = E_CampaignRequestFamily;
using Clock = std::chrono::steady_clock;
namespace Wire = CampaignProtocol;

constexpr auto kRequestTimeout = std::chrono::milliseconds(Wire::kTimeoutMs);
constexpr auto kResyncInterval = std::chrono::seconds(1);

S_CampaignPreparationJournal BindJournal(C_CampaignRequestJournal &journal)
{
    return {
        [&journal](const auto &context, auto family, const auto &content, const auto &state) {
            return journal.Begin(context, family, content, state);
        },
        [&journal](const auto &context, auto family, const auto &content, const auto &state) {
            journal.Finish(context, family, content, state);
        }
    };
}

S_CampaignPreparationLimits PreparationLimits()
{
    S_CampaignPreparationLimits limits;
    limits.m_MaxEntries = Wire::kMaxCollection;
    return limits;
}

} // namespace

C_CampaignPreparationLive::C_CampaignPreparationLive(SessionId session, PlayerId local, T_Send send)
    : m_Session(session), m_Local(local), m_Send(std::move(send)), m_Replica(session)
{
}

S_CampaignDeployEvaluation C_CampaignPreparationLive::C_DeploymentPolicy::Evaluate(
    const S_CampaignPreparationSnapshot &state) const
{
    S_CampaignDeployEvaluation evaluation{R::None, {}, {}};
    for (const auto &participant : state.m_Participants) {
        if (!participant.m_Admitted || !participant.m_Connected)
            continue;
        evaluation.m_RequiredParticipants.push_back(participant.m_Player);
        const auto ownsSoldier = [&](const auto &soldier) {
            return soldier.m_Ownership.m_Owner == participant.m_Player && soldier.m_Ownership.m_Alive &&
                soldier.m_Ownership.m_Eligible && soldier.m_Ownership.m_Deployed && !soldier.m_Ownership.m_Scripted;
        };
        if (std::none_of(state.m_Soldiers.begin(), state.m_Soldiers.end(), ownsSoldier))
            evaluation.m_Reason = R::NotDeployed;
        else if (evaluation.m_Reason == R::None && participant.m_ReadyAt != state.m_PreparationRevision)
            evaluation.m_Reason = R::PendingTransaction;
    }
    if (evaluation.m_RequiredParticipants.empty())
        evaluation.m_Reason = R::NotAdmitted;
    return evaluation;
}

R C_CampaignPreparationLive::OpenHost(C_CampaignOwnership &ownership, C_CampaignRequestJournal &journal,
    S_CampaignPreparationSnapshot initial, S_CampaignEquipmentRuleCatalog catalog)
{
    if (m_Authority || m_Resync || m_HasSnapshot || journal.GetSession() != m_Session ||
        initial.m_Context.m_Session != m_Session || initial.m_Context.m_HostPlayer != m_Local)
        return R::InvalidPhase;

    try {
        auto rules = std::make_unique<C_CampaignEquipmentRules>(std::move(catalog));
        auto authority = std::make_unique<C_CampaignPreparation>(std::move(initial), ownership, *rules,
            m_DeploymentPolicy, BindJournal(journal), PreparationLimits(), [&ownership] { return ownership.Freeze(); });
        Wire::T_Bytes encoded;
        if (Wire::Encode(*authority->GetSnapshot(m_Local), encoded) != R::None)
            return R::SnapshotUnavailable;
        m_Ownership = &ownership;
        m_Journal = &journal;
        m_Rules = std::move(rules);
        m_Authority = std::move(authority);
    } catch (const std::invalid_argument &) {
        return R::InvalidItem;
    }
    return R::None;
}

R C_CampaignPreparationLive::ReopenHost()
{
    if (!m_Authority || !m_Journal || m_Ownership->GetSession().GetContext().m_Phase != Phase::Preparation)
        return R::InvalidPhase;
    auto initial = *m_Authority->GetSnapshot(m_Local);
    const auto &session = m_Ownership->GetSession();
    initial.m_Context = session.GetContext();
    initial.m_Generation = session.GetPreparationGeneration();
    initial.m_PreparationRevision = PreparationRevision(1);
    initial.m_ViewRevision = ViewRevision(1);
    m_LocalRequest.reset();
    m_Pending.reset();
    m_FreezeDeadline.reset();
    m_Events.clear();
    m_Published.clear();
    try {
        m_Authority = std::make_unique<C_CampaignPreparation>(std::move(initial), *m_Ownership, *m_Rules,
            m_DeploymentPolicy, BindJournal(*m_Journal), PreparationLimits(), [this] { return m_Ownership->Freeze(); });
    } catch (const std::invalid_argument &) {
        return R::InvalidItem;
    }
    return R::None;
}

T_CampaignPreparationSnapshot C_CampaignPreparationLive::Publication(PlayerId viewer) const
{
    auto snapshot = *m_Authority->GetSnapshot(viewer);
    const bool host = snapshot.m_Context.m_HostPlayer == viewer;
    snapshot.m_Context.m_IsTransportServer = host;
    snapshot.m_Context.m_IsTransportClient = true;
    const bool preparing = snapshot.m_Context.m_Phase == Phase::Preparation &&
        snapshot.m_Sync == E_CampaignSyncStatus::Current;
    const bool hasSoldier = m_Ownership->FindSoldier(viewer).has_value();
    for (auto &soldier : snapshot.m_Soldiers) {
        const auto &record = soldier.m_Ownership;
        const auto claim = !preparing ? R::Frozen : record.m_Owner ? R::AlreadyOwned :
            hasSoldier ? R::AlreadyControlsSoldier : !record.m_Deployed ? R::NotDeployed :
            !record.m_Eligible ? R::Ineligible : !record.m_Alive ? R::Dead :
            record.m_Scripted ? R::Scripted : R::None;
        const auto release = !preparing ? R::Frozen : record.m_Owner == viewer ? R::None : R::NotOwner;
        soldier.m_CanClaim = {claim == R::None, claim};
        soldier.m_CanRelease = {release == R::None, release};
    }
    return std::make_shared<const S_CampaignPreparationSnapshot>(std::move(snapshot));
}

R C_CampaignPreparationLive::Tick()
{
    if (m_Failure != R::None)
        return m_Failure;
    if (m_Authority) {
        auto reason = m_Authority->SynchronizeOwnership();
        if (reason != R::None)
            return reason;
        if (m_LocalRequest) {
            auto request = std::move(*m_LocalRequest);
            m_LocalRequest.reset();
            Dispatch(m_Local, request);
        }

        auto state = Publication(m_Local);
        for (auto peer = m_Published.begin(); peer != m_Published.end();) {
            const bool admitted = std::any_of(state->m_Participants.begin(), state->m_Participants.end(),
                [&](const auto &entry) { return entry.m_Player == peer->first && entry.m_Admitted && entry.m_Connected; });
            peer = admitted ? std::next(peer) : m_Published.erase(peer);
        }
        for (const auto &peer : state->m_Participants) {
            if (peer.m_Player == m_Local || !peer.m_Admitted || !peer.m_Connected)
                continue;
            auto &last = m_Published[peer.m_Player];
            const auto current = std::make_pair(state->m_Generation, state->m_ViewRevision);
            if (last != current) {
                const auto snapshot = Publication(peer.m_Player);
                if (m_Send(peer.m_Player, *snapshot))
                    last = current;
            }
        }
        if (GetFrozenManifest() && !IsFrozenPublished()) {
            const auto now = Clock::now();
            if (!m_FreezeDeadline)
                m_FreezeDeadline = now + kRequestTimeout;
            else if (now >= *m_FreezeDeadline)
                return R::Timeout;
        }
    } else if (m_Resync && Clock::now() >= m_NextResync) {
        m_NextResync = Clock::now() + kResyncInterval;
        m_Send(m_Local, Wire::S_PreparationResync{m_Session, m_Local});
    }
    if (m_Pending && Clock::now() >= m_Pending->m_Deadline) {
        S_CampaignRequestResult result;
        result.m_Context = m_Pending->m_Context;
        result.m_Family = m_Pending->m_Family;
        result.m_Reason = R::Timeout;
        result.m_PreparationGeneration = m_Pending->m_Generation;
        Complete(result);
        RequestResync();
    }
    return R::None;
}

bool C_CampaignPreparationLive::Dispatch(PlayerId sender, const Wire::T_Message &message)
{
    S_CampaignRequestResult result;
    if (const auto *request = std::get_if<S_CampaignEditRequest>(&message))
        result = m_Authority->ApplyEdit(sender, *request);
    else if (const auto *request = std::get_if<S_CampaignReadyRequest>(&message))
        result = m_Authority->SetReady(sender, *request);
    else if (const auto *request = std::get_if<S_CampaignDeployRequest>(&message))
        result = m_Authority->RequestDeploy(sender, *request);
    else if (const auto *request = std::get_if<S_CampaignOwnershipRequest>(&message)) {
        result = m_Ownership->ApplyRequest(sender, *request);
        m_Authority->SynchronizeOwnership();
    } else if (const auto *resync = std::get_if<Wire::S_PreparationResync>(&message)) {
        if (resync->m_Session == m_Session && resync->m_PlayerEcho == sender)
            m_Published.erase(sender);
        return true;
    } else {
        return false;
    }
    if (sender == m_Local)
        Complete(result);
    else
        m_Send(sender, result);
    return true;
}

bool C_CampaignPreparationLive::Receive(PlayerId peer, const Wire::T_Message &message)
{
    if (m_Authority)
        return Dispatch(peer, message);
    if (const auto *snapshot = std::get_if<S_CampaignPreparationSnapshot>(&message)) {
        if (snapshot->m_Context.m_LocalPlayer == m_Local && !snapshot->m_Context.m_IsTransportServer)
            Install(m_Replica.Install(*snapshot));
    } else if (const auto *delta = std::get_if<Wire::S_PreparationDelta>(&message)) {
        if (delta->m_Replacement.m_Context.m_LocalPlayer == m_Local && !delta->m_Replacement.m_Context.m_IsTransportServer) {
            const auto reason = m_Replica.Apply(*delta);
            if (reason == R::SnapshotGap)
                RequestResync();
            else if (!m_Resync)
                Install(reason);
        }
    } else if (const auto *result = std::get_if<S_CampaignRequestResult>(&message)) {
        Complete(*result);
    } else {
        return false;
    }
    return true;
}

void C_CampaignPreparationLive::Install(R reason)
{
    if (reason == R::None) {
        m_HasSnapshot = true;
        m_Resync = false;
    } else if (reason == R::SnapshotGap || reason == R::SnapshotUnavailable) {
        RequestResync();
    }
}

void C_CampaignPreparationLive::Complete(const S_CampaignRequestResult &result)
{
    if (!m_Pending || result.m_Context.m_Session != m_Pending->m_Context.m_Session ||
        result.m_Context.m_PlayerEcho != m_Pending->m_Context.m_PlayerEcho ||
        result.m_Context.m_Request != m_Pending->m_Context.m_Request || result.m_Family != m_Pending->m_Family)
        return;
    const LocalSubmissionId token(result.m_Context.m_Request.Value());
    if (result.m_PreparationGeneration != m_Pending->m_Generation)
        RequestResync();
    m_Pending.reset();
    m_Events.push_back(S_CampaignRequestResultEvent{token, result});
}

void C_CampaignPreparationLive::Disconnect(R reason)
{
    m_Failure = reason == R::None ? R::Disconnected : reason;
    m_Pending.reset();
    m_LocalRequest.reset();
    m_Events.clear();
}

RequestId C_CampaignPreparationLive::NextRequestId()
{
    const auto next = CampaignNextCounter(m_LastRequest);
    if (next)
        m_LastRequest = *next;
    return next.value_or(RequestId{});
}

std::shared_ptr<const S_CampaignMissionManifest> C_CampaignPreparationLive::GetFrozenManifest() const
{
    return m_Authority ? m_Authority->GetFrozenManifest() : nullptr;
}

bool C_CampaignPreparationLive::IsFrozenPublished() const
{
    const auto manifest = GetFrozenManifest();
    if (!manifest)
        return false;
    return std::all_of(manifest->m_RequiredParticipants.begin(), manifest->m_RequiredParticipants.end(),
        [&](PlayerId peer) {
            if (peer == m_Local)
                return true;
            const auto sent = m_Published.find(peer);
            return sent != m_Published.end() &&
                sent->second == std::make_pair(manifest->m_PreparationGeneration, manifest->m_ViewRevision);
        });
}

S_CampaignPreparationView C_CampaignPreparationLive::GetSnapshot() const
{
    if (m_Failure != R::None)
        return {E_CampaignSyncStatus::Disconnected, m_Failure, nullptr};
    if (m_Authority) {
        auto state = Publication(m_Local);
        const auto sync = state->m_Sync;
        return {sync, sync == E_CampaignSyncStatus::Current ? R::None : R::SnapshotUnavailable, std::move(state)};
    }
    if (!m_HasSnapshot && !m_Resync)
        return {};
    if (m_Resync)
        return {E_CampaignSyncStatus::Resyncing, R::SnapshotGap, nullptr};
    return m_Replica.GetView();
}

T_CampaignSubmission C_CampaignPreparationLive::Submit(const S_CampaignRequestContext &context,
    Family family, Wire::T_Message message)
{
    if (m_Pending)
        return S_CampaignLocalFailure{R::PendingTransaction};
    const auto view = GetSnapshot();
    if (!view.m_Snapshot || view.m_Status != E_CampaignSyncStatus::Current)
        return S_CampaignLocalFailure{view.m_Reason};
    if (view.m_Snapshot->m_Context.m_Phase != Phase::Preparation)
        return S_CampaignLocalFailure{R::Frozen};
    if (context.m_Session != m_Session || context.m_PlayerEcho != m_Local || !context.m_Request.IsValid())
        return S_CampaignLocalFailure{R::Malformed};

    if (m_Authority)
        m_LocalRequest = std::move(message);
    else if (!m_Send(m_Local, message))
        return S_CampaignLocalFailure{R::Disconnected};
    m_Pending = S_Pending{context, family, view.m_Snapshot->m_Generation, Clock::now() + kRequestTimeout};
    return S_CampaignQueuedSubmission{LocalSubmissionId(context.m_Request.Value())};
}

T_CampaignSubmission C_CampaignPreparationLive::SubmitEdit(const S_CampaignEditRequest &request)
{
    return Submit(request.m_Context.m_Request, Family::Edit, request);
}

T_CampaignSubmission C_CampaignPreparationLive::SubmitOwnership(const S_CampaignOwnershipRequest &request)
{
    return Submit(request.m_Context, Family::Ownership, request);
}

T_CampaignSubmission C_CampaignPreparationLive::SetReady(const S_CampaignReadyRequest &request)
{
    return Submit(request.m_Context.m_Request, Family::Ready, request);
}

T_CampaignSubmission C_CampaignPreparationLive::RequestDeploy(const S_CampaignDeployRequest &request)
{
    return Submit(request.m_Context.m_Request, Family::Deploy, request);
}

void C_CampaignPreparationLive::RequestResync()
{
    if (!m_Authority && !m_Resync && m_Failure == R::None) {
        m_Resync = true;
        m_NextResync = {};
    }
}

std::optional<T_CampaignPreparationEvent> C_CampaignPreparationLive::PollEvent()
{
    if (m_Events.empty())
        return std::nullopt;
    auto event = std::move(m_Events.front());
    m_Events.pop_front();
    return event;
}
