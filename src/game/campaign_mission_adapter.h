#ifndef HD2_CAMPAIGN_MISSION_ADAPTER_H
#define HD2_CAMPAIGN_MISSION_ADAPTER_H

#include "campaign_ownership.h"
#include <functional>
#include <set>

// Trusted lifecycle on the same serialized authority queue as ownership/preparation.
// This boundary is dormant until explicitly installed by the live integration.
// Retain exactly one adapter for the whole SessionId lifetime: snapshot counters
// must never reset by recreating the adapter under an existing session.
class C_CampaignMissionAdapter {
public:
    explicit C_CampaignMissionAdapter(C_CampaignOwnership &ownership);
    C_CampaignMissionAdapter(const C_CampaignMissionAdapter &) = delete;
    C_CampaignMissionAdapter &operator=(const C_CampaignMissionAdapter &) = delete;
    E_CampaignReason Begin(const S_CampaignMissionManifest &frozen);
    void SetRetirementHook(std::function<E_CampaignReason()> retire) { m_Retire = std::move(retire); }
    E_CampaignReason Bind(SoldierId soldier, const ActorRef &actor);
    // After ALL world categories have been captured/reconstructed, native bindings
    // installed and controller handoffs completed. Manifest alone is insufficient.
    // Snapshot identity is allocated here, never from a peer or preparation revision.
    E_CampaignReason SealInitialWorld();
    E_CampaignReason Acknowledge(PlayerId authenticatedSender, const S_CampaignSnapshotAck &ack);
    E_CampaignReason Start(S_CampaignStartSimulation &start);
    E_CampaignReason End();
    void Abort(E_CampaignReason reason);
    E_CampaignReason CheckBaseline() const;
    bool CanSimulate() const;
    bool HasAuthority() const;
    S_CampaignSessionContext GetContext() const;
    C_CampaignOwnership &GetOwnership() { return m_Ownership; }
    std::shared_ptr<const S_CampaignMissionManifest> GetManifest() const { return m_Manifest; }
    std::optional<S_CampaignMissionManifest> GetLoadingManifest() const { return m_Loading; }
    std::optional<S_CampaignSnapshotAck> ExpectedAck(PlayerId player) const;
    E_CampaignReason GetFailure() const { return m_Failure; }
private:
    C_CampaignOwnership &m_Ownership;
    std::function<E_CampaignReason()> m_Retire;
    std::optional<S_CampaignMissionManifest> m_Loading;
    std::shared_ptr<const S_CampaignMissionManifest> m_Manifest;
    std::set<PlayerId> m_Acknowledged;
    SnapshotId m_LastSnapshot;
    T_CampaignOwnershipSnapshot m_LoadingState;
    E_CampaignReason RefreshStartupScripts();
    E_CampaignReason m_Failure = E_CampaignReason::None;
    bool m_Started = false;
};

// Guest/host-local reconstruction seam AFTER negotiated, integrity-checked world
// assembly. The engine callback reconstructs every world category atomically or
// leaves input disabled and returns failure; it must not send an ack itself.
class C_CampaignMissionReconstruction {
public:
    using T_Reconstruct = std::function<E_CampaignReason(const S_CampaignMissionManifest &,
        const std::vector<std::uint8_t> &)>;
    C_CampaignMissionReconstruction(SessionId session, PlayerId player,
        const I_CampaignOwnershipView &installedView);
    C_CampaignMissionReconstruction(const C_CampaignMissionReconstruction &) = delete;
    C_CampaignMissionReconstruction &operator=(const C_CampaignMissionReconstruction &) = delete;
    E_CampaignReason Reconstruct(const S_CampaignMissionManifest &manifest,
        const std::vector<std::uint8_t> &verifiedWorld, const T_Reconstruct &reconstruct);
    std::optional<S_CampaignSnapshotAck> GetAcknowledgment() const;
    E_CampaignReason AcceptStart(const S_CampaignStartSimulation &start);
    bool IsStarted() const;
    bool IsStartedFor(const S_CampaignSessionContext &context) const;
    void Invalidate();
private:
    SessionId m_Session;
    PlayerId m_Player;
    const I_CampaignOwnershipView &m_View;
    MissionGeneration m_HighestGeneration;
    SnapshotId m_HighestSnapshot;
    std::optional<S_CampaignSnapshotAck> m_Ack;
    bool m_Started = false;
};

// Process-local attachment; no serialized/engine layout changes. Caller retains
// lifetime and uninstalls before destroying the adapter. No production call installs
// this in COOP-009. Host and guest bridges must select the SAME explicit context.
C_CampaignMissionAdapter *GetCampaignMissionAdapter();
void SetCampaignMissionAdapter(C_CampaignMissionAdapter *adapter);
void InstallCampaignMissionView(const I_CampaignOwnershipView *view);
void InstallCampaignMissionReconstruction(const C_CampaignMissionReconstruction *reconstruction);
T_CampaignOwnershipSnapshot GetCampaignMissionSnapshot();
bool IsCampaignMission();
bool IsCampaignSimulationAuthority();
bool CanCampaignSimulate();

#endif
