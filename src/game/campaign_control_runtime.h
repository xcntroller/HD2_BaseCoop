#ifndef HD2_CAMPAIGN_CONTROL_RUNTIME_H
#define HD2_CAMPAIGN_CONTROL_RUNTIME_H
#include "campaign_contracts.h"
#include <functional>

// Explicit non-owning integration seam; default is uninstalled. The owner must
// uninstall before destroying the view. Production attachment belongs to COOP-010.
class C_CampaignControlAdapter;
void InstallCampaignControlRuntime(const I_CampaignOwnershipView *view, C_CampaignControlAdapter *adapter);
std::optional<PlayerId> FindCampaignTransportPlayer(int slot);
E_CampaignReason HandleCampaignDisconnect(PlayerId exactIncarnation);
void InstallCampaignControlView(const I_CampaignOwnershipView *view);
bool CampaignControlsActive();
bool CampaignInputAuthorized(int actorId);
void DispatchCampaignInput(const S_CampaignControlToken &token, const std::function<void()> &execute);
T_CampaignOwnershipSnapshot GetCampaignControlSnapshot();
std::optional<S_CampaignOwnershipRecord> FindCampaignActorControl(int actorId);
bool IsCampaignLocalActor(int actorId);
// Authenticated transport adapter supplies these; actor IDs are never slots.
bool BindCampaignTransportPlayer(int slot, PlayerId incarnation);
void ForgetCampaignTransportPlayer(PlayerId incarnation);
std::optional<int> FindCampaignPlayerSlot(int actorId);
#endif
