#ifndef HD2_CAMPAIGN_DEV_H
#define HD2_CAMPAIGN_DEV_H

#include <cstdint>

class C_GameApp;
class I3D_scene;

// Returns true when the explicit development launch switch was handled.
bool StartDevelopmentCampaign(C_GameApp &app);
void TickDevelopmentCampaign(C_GameApp &app);
void ReleaseDevelopmentCampaign();
bool RenderDevelopmentCampaignCutscene(I3D_scene &scene);
void LogDevelopmentCampaign(const char *event, std::uint64_t first = 0, std::uint64_t second = 0);

#endif
