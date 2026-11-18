#ifndef HD2_CAMPAIGN_DEV_H
#define HD2_CAMPAIGN_DEV_H

#include <cstdint>
#include <functional>

class C_GameApp;
class I3D_scene;
class I3D_frame;
struct S_HumanCommand;
namespace CampaignProtocol {
enum class E_WeaponEvent : std::uint8_t;
struct S_PlayerControls;
struct S_MissionPresentation;
}

// Returns true when the explicit development launch switch was handled.
bool StartDevelopmentCampaign(C_GameApp &app);
void TickDevelopmentCampaign(C_GameApp &app);
void ReleaseDevelopmentCampaign();
bool RenderDevelopmentCampaignCutscene(I3D_scene &scene);
bool IsDevelopmentCampaignCutscene();
void QueueDevelopmentCampaignPresentation(CampaignProtocol::S_MissionPresentation event);
void TrackDevelopmentCampaignFrame(const I3D_frame &frame);
void PublishDevelopmentCampaignWeapon(int actor, CampaignProtocol::E_WeaponEvent action,
    int weaponUid, std::uint16_t rounds, std::uint16_t reloadMs, int consumedAmmo);
void SendDevelopmentCampaignControls(int actor, const CampaignProtocol::S_PlayerControls &controls);
bool RequestDevelopmentCampaignInventoryMove(int actor, int itemUid, int fromActor,
                                             int fromContainer, int toContainer, int slot, int replacedUid = -1);
bool RequestDevelopmentCampaignDoorAction(int actor, const S_HumanCommand &command);
void ApplyDevelopmentCampaignControls(int actor,
    const std::function<void(const CampaignProtocol::S_PlayerControls *)> &apply);
void LogDevelopmentCampaign(const char *event, std::uint64_t first = 0, std::uint64_t second = 0);

#endif
