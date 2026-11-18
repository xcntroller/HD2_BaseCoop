#ifndef HD2_CAMPAIGN_DEV_H
#define HD2_CAMPAIGN_DEV_H

#include <cstdint>

class C_GameApp;

// Returns true when the explicit development launch switch was handled.
bool StartDevelopmentCampaign(C_GameApp &app);
void TickDevelopmentCampaign(C_GameApp &app);
void ReleaseDevelopmentCampaign();
void LogDevelopmentCampaign(const char *event, std::uint64_t first = 0, std::uint64_t second = 0);

#endif
