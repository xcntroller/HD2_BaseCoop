#ifndef HD2_CAMPAIGN_CONTROL_ADAPTER_H
#define HD2_CAMPAIGN_CONTROL_ADAPTER_H

#include "campaign_ownership.h"
#include "../net/campaign_protocol.h"
#include <deque>
#include <map>

// Roles describe effective control, independently of actor/player slot numbers.
enum class E_CampaignController { Suspended, LocalPlayer, RemotePlayer, SquadAI, Scripted, Replica };
struct S_CampaignControllerPlan {
    S_CampaignOwnershipRecord m_Record;
    E_CampaignController m_Controller = E_CampaignController::Suspended;
    bool m_LocalView = false;
};
// Synchronous authority-thread port. Drain MUST discard handler/network/control
// bits/action/reliable queues, never execute them. Apply MUST replace handlers even
// if controller type is unchanged. Neither operation may respawn or reset state.
class I_CampaignControlSink {
public:
    virtual ~I_CampaignControlSink() = default;
    virtual bool Drain(const ActorRef &actor) = 0;
    virtual bool Apply(const S_CampaignControllerPlan &plan) = 0;
    virtual bool Present(const std::optional<ActorRef> &localActor) = 0;
    // Payload is owned and revalidated immediately before this call. Target actor
    // IDs inside actions remain actor IDs and require separate gameplay checks.
    virtual void Execute(const CampaignProtocol::S_Input &input) = 0;
};

// Dormant component: caller owns sink/model and invokes on ONE nonreentrant queue.
// No global installation, native opcode or production session is enabled here.
class C_CampaignControlAdapter {
public:
    static constexpr std::size_t kMaxQueuedInputs = 256;
    C_CampaignControlAdapter(C_CampaignOwnership &ownership, I_CampaignControlSink &sink);
    static S_CampaignControllerPlan Plan(const S_CampaignSessionContext &context,
        const S_CampaignOwnershipRecord &record);
    // Reconcile a committed ownership cut, drain every old binding before applying
    // any new binding, then presentation, then the exact-revision ownership barrier.
    E_CampaignReason SynchronizeCommitted();
    E_CampaignReason OnDisconnected(PlayerId exactIncarnation);
    E_CampaignReason OnScripted(SoldierId soldier, bool scripted);
    E_CampaignReason OnDeath(SoldierId soldier);
    E_CampaignReason Enqueue(PlayerId authenticatedSender, const CampaignProtocol::S_Input &input);
    E_CampaignReason ApplyNext();
    std::size_t GetQueuedCount() const { return m_Inputs.size(); }
    bool IsSynchronized() const;
private:
    struct S_Queued { PlayerId m_Sender; CampaignProtocol::S_Input m_Input; };
    C_CampaignOwnership &m_Ownership;
    I_CampaignControlSink &m_Sink;
    T_CampaignOwnershipSnapshot m_Applied;
    std::map<PlayerId, CampaignProtocol::C_InputGate> m_Gates;
    std::deque<S_Queued> m_Inputs;
    bool m_Ready = false;
};
// Guest consumes complete committed catalogs; owns no authority model and cannot
// acknowledge the host's private handoff barrier. Install it as the native view.
class C_CampaignReplicaControlAdapter final : public I_CampaignOwnershipView {
public:
    C_CampaignReplicaControlAdapter(SessionId session, PlayerId localPlayer, I_CampaignControlSink &sink)
        : m_Session(session), m_LocalPlayer(localPlayer), m_Sink(sink) {}
    T_CampaignOwnershipSnapshot GetSnapshot() const override { return m_View; }
    E_CampaignReason InstallCommitted(T_CampaignOwnershipSnapshot snapshot);
    bool IsSynchronized() const { return m_Ready; }
private:
    SessionId m_Session;
    PlayerId m_LocalPlayer;
    I_CampaignControlSink &m_Sink;
    T_CampaignOwnershipSnapshot m_View, m_Applied;
    CampaignProtocol::T_Bytes m_LastWire;
    bool m_Ready = false;
};
#endif
