#ifndef HD2_CAMPAIGN_OWNERSHIP_H
#define HD2_CAMPAIGN_OWNERSHIP_H

#include "campaign_session.h"

// Engine-independent host model. All methods run on the same nonreentrant
// authority queue as preparation and the shared request journal. Trusted events
// below are NOT remotely callable. Retain journal/policy for this model's lifetime.
class C_CampaignOwnership final : public I_CampaignOwnershipView {
public:
    C_CampaignOwnership(C_CampaignSession session, const I_CampaignPolicyView &policy,
        C_CampaignRequestJournal &journal, std::uint32_t squadLimit = 4);
    C_CampaignOwnership(const C_CampaignOwnership &) = delete;
    C_CampaignOwnership &operator=(const C_CampaignOwnership &) = delete;
    T_CampaignOwnershipSnapshot GetSnapshot() const override;
    std::size_t GetParticipantCount() const { return m_State.m_Participants.size(); }
    const C_CampaignSession &GetSession() const { return m_Session; }
    std::optional<PlayerId> FindOwner(SoldierId soldier) const;
    std::optional<SoldierId> FindSoldier(PlayerId player) const;
    std::optional<SoldierId> ResolveActor(const ActorRef &actor) const;
    bool IsLocallyOwned(SoldierId soldier) const;
    S_CampaignRequestResult ApplyRequest(PlayerId authenticatedSender,
        const S_CampaignOwnershipRequest &request);

    E_CampaignReason Admit(const std::string &displayName, PlayerId &admitted);
    // Commit the exact transport incarnation after successful negotiation.
    // The transport can consume identities while quarantining rejected peers.
    E_CampaignReason AdmitTransportPlayer(PlayerId player, const std::string &displayName);
    E_CampaignReason OnDisconnected(PlayerId player);
    E_CampaignReason OnDeath(SoldierId soldier);
    E_CampaignReason SetAvailability(SoldierId soldier, bool eligible, bool deployed);
    E_CampaignReason SetScripted(SoldierId soldier, bool scripted);
    E_CampaignReason BindActor(SoldierId soldier, const ActorRef &actor);
    E_CampaignReason UnbindActor(SoldierId soldier);
    // Queue/controller handoff barrier: acknowledge exactly the committed token
    // revision only after old queues/handlers have been drained/replaced.
    E_CampaignReason CompleteControlHandoff(const S_CampaignOwnershipTarget &target);
    std::optional<S_CampaignControlToken> GetControlToken(PlayerId player) const;
    E_CampaignReason ValidateInput(PlayerId authenticatedSender, const S_CampaignControlToken &token) const;

    E_CampaignReason StartPreparation();
    E_CampaignReason Freeze(); // caller has already checked equipment/deploy policy
    E_CampaignReason BeginMissionGeneration(MissionKey mission);
    E_CampaignReason StartSimulation(); // caller has verified full peer/world barrier
    E_CampaignReason EndMission();
    E_CampaignReason ReturnToSetup(MissionKey mission);
    E_CampaignReason RestoreRetryRoster(const std::vector<S_CampaignSoldierPresentation> &soldiers);
    E_CampaignReason Close();
private:
    E_CampaignReason Commit(C_CampaignSession session, S_CampaignOwnershipSnapshot candidate,
        bool invalidateAll = false);
    E_CampaignReason CheckPolicy(E_CampaignPolicy policy) const;
    E_CampaignReason CheckClaim(const S_CampaignOwnershipRecord &record) const;
    S_CampaignRequestResult MakeResult(const S_CampaignOwnershipRequest &request,
        E_CampaignReason reason) const;
    bool IsAdmitted(PlayerId player) const;
    bool IsLive() const;
    static S_CampaignOwnershipRecord *FindRecord(S_CampaignOwnershipSnapshot &state, SoldierId soldier);
    static const S_CampaignOwnershipRecord *FindRecord(const S_CampaignOwnershipSnapshot &state, SoldierId soldier);
    C_CampaignSession m_Session;
    const I_CampaignPolicyView &m_Policy;
    C_CampaignRequestJournal &m_Journal;
    std::uint32_t m_SquadLimit;
    PlayerId m_LastPlayer = PlayerId(1);
    S_CampaignOwnershipSnapshot m_State;
    std::map<PlayerId, SoldierId> m_ByPlayer;
    std::map<SoldierId, bool> m_HandoffComplete;
};

#endif
