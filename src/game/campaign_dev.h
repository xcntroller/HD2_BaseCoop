#ifndef HD2_CAMPAIGN_DEV_H
#define HD2_CAMPAIGN_DEV_H

#include <cstdint>
#include <functional>

class C_GameApp;
class I3D_scene;
namespace CampaignProtocol {
enum class E_WeaponEvent : std::uint8_t;
struct S_PlayerControls;
}

// Returns true when the explicit development launch switch was handled.
bool StartDevelopmentCampaign(C_GameApp &app);
void TickDevelopmentCampaign(C_GameApp &app);
void ReleaseDevelopmentCampaign();
bool RenderDevelopmentCampaignCutscene(I3D_scene &scene);
void PublishDevelopmentCampaignWeapon(int actor, CampaignProtocol::E_WeaponEvent action,
    int weaponUid, std::uint16_t rounds, std::uint16_t reloadMs, int consumedAmmo);
void SendDevelopmentCampaignControls(int actor, const CampaignProtocol::S_PlayerControls &controls);
void ApplyDevelopmentCampaignControls(int actor,
    const std::function<void(const CampaignProtocol::S_PlayerControls *)> &apply);
void LogDevelopmentCampaign(const char *event, std::uint64_t first = 0, std::uint64_t second = 0);

#endif
