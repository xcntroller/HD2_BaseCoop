#ifndef HD2_CAMPAIGN_CONTROL_NATIVE_H
#define HD2_CAMPAIGN_CONTROL_NATIVE_H
#include "campaign_control_adapter.h"
#include <functional>

class C_Human;

// Concrete engine handoff port. COOP-010 supplies negotiated, bounded action
// decoding; the native untagged net-object/reliable routes stay quarantined.
class C_CampaignNativeControlSink final : public I_CampaignControlSink {
public:
    using T_Dispatch = std::function<void(const CampaignProtocol::S_Input &)>;
    explicit C_CampaignNativeControlSink(T_Dispatch dispatch) : m_Dispatch(std::move(dispatch)) {}
    bool Drain(const ActorRef &actor) override;
    bool Apply(const S_CampaignControllerPlan &plan) override;
    bool Present(const std::optional<ActorRef> &actor) override;
    void Execute(const CampaignProtocol::S_Input &input) override;
    bool CaptureSquadActor(CampaignProtocol::S_SquadActorState &state) const;
    bool ApplySquadActors(const std::vector<CampaignProtocol::S_SquadActorState> &squad) const;
    bool CaptureAmbientHumans(SessionId session, MissionGeneration generation,
                              std::vector<CampaignProtocol::S_AmbientHumanState> &out) const;
    bool ApplyAmbientHumans(const std::vector<CampaignProtocol::S_AmbientHumanState> &actors) const;
    bool ApplyWeaponEvent(const CampaignProtocol::S_WeaponEvent &event) const;
    E_CampaignReason MoveInventory(const ActorRef &actor, CampaignProtocol::S_ItemMove &move) const;
    bool ApplyInventoryEvent(const CampaignProtocol::S_InventoryEvent &event) const;
private:
    C_Human *Resolve(const ActorRef &actor) const;
    void ApplyHumanPresentation(C_Human &human, const CampaignProtocol::S_SquadActorState &actor, bool local) const;
    T_Dispatch m_Dispatch;
    std::map<SoldierId, ActorRef> m_AppliedBindings;
};
#endif
