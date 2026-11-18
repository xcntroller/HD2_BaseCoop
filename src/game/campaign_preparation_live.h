#ifndef HD2_CAMPAIGN_PREPARATION_LIVE_H
#define HD2_CAMPAIGN_PREPARATION_LIVE_H

#include "campaign_ownership.h"
#include "campaign_preparation.h"
#include "net/campaign_protocol.h"

#include <chrono>
#include <deque>

// One adapter per admitted menu session. Host callbacks and Tick run on the
// game thread. UI proposals are queued until Tick so results cannot reenter
// the presenter while it is recording a submission.
class C_CampaignPreparationLive final : public I_CampaignPreparation {
public:
    using T_Send = std::function<bool(PlayerId, const CampaignProtocol::T_Message &)>;

    C_CampaignPreparationLive(SessionId session, PlayerId local, T_Send send);
    E_CampaignReason OpenHost(C_CampaignOwnership &ownership, C_CampaignRequestJournal &journal,
        S_CampaignPreparationSnapshot initial, S_CampaignEquipmentRuleCatalog catalog);
    bool Receive(PlayerId peer, const CampaignProtocol::T_Message &message);
    E_CampaignReason Tick();
    void Disconnect(E_CampaignReason reason);
    RequestId NextRequestId();
    std::shared_ptr<const S_CampaignMissionManifest> GetFrozenManifest() const;
    bool IsFrozenPublished() const;
    E_CampaignReason ReopenHost();

    S_CampaignPreparationView GetSnapshot() const override;
    T_CampaignSubmission SubmitEdit(const S_CampaignEditRequest &request) override;
    T_CampaignSubmission SubmitOwnership(const S_CampaignOwnershipRequest &request) override;
    T_CampaignSubmission SetReady(const S_CampaignReadyRequest &request) override;
    T_CampaignSubmission RequestDeploy(const S_CampaignDeployRequest &request) override;
    void RequestResync() override;
    std::optional<T_CampaignPreparationEvent> PollEvent() override;

private:
    class C_DeploymentPolicy final : public I_CampaignDeploymentPolicy {
    public:
        S_CampaignDeployEvaluation Evaluate(const S_CampaignPreparationSnapshot &state) const override;
    };

    T_CampaignSubmission Submit(const S_CampaignRequestContext &context, E_CampaignRequestFamily family,
        CampaignProtocol::T_Message message);
    bool Dispatch(PlayerId sender, const CampaignProtocol::T_Message &message);
    T_CampaignPreparationSnapshot Publication(PlayerId viewer) const;
    void Complete(const S_CampaignRequestResult &result);
    void Install(E_CampaignReason reason);

    SessionId m_Session;
    PlayerId m_Local;
    T_Send m_Send;
    CampaignProtocol::C_PreparationReplica m_Replica;
    C_DeploymentPolicy m_DeploymentPolicy;
    C_CampaignOwnership *m_Ownership = nullptr;
    C_CampaignRequestJournal *m_Journal = nullptr;
    std::unique_ptr<C_CampaignEquipmentRules> m_Rules;
    std::unique_ptr<C_CampaignPreparation> m_Authority;
    std::map<PlayerId, std::pair<PreparationGeneration, ViewRevision>> m_Published;
    std::optional<CampaignProtocol::T_Message> m_LocalRequest;
    RequestId m_LastRequest;
    struct S_Pending {
        S_CampaignRequestContext m_Context;
        E_CampaignRequestFamily m_Family;
        PreparationGeneration m_Generation;
        std::chrono::steady_clock::time_point m_Deadline;
    };
    std::optional<S_Pending> m_Pending;
    std::optional<std::chrono::steady_clock::time_point> m_FreezeDeadline;
    std::deque<T_CampaignPreparationEvent> m_Events;
    bool m_HasSnapshot = false;
    bool m_Resync = false;
    E_CampaignReason m_Failure = E_CampaignReason::None;
    std::chrono::steady_clock::time_point m_NextResync{};
};

#endif
