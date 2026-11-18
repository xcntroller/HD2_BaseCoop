#pragma once

struct S_CampaignNetConfig;

// Fingerprint the local campaign, mission, roster and equipment sources and the
// selected route's rules. A host also gets a fresh session identity.
bool BuildCampaignNetConfig(const char *missionName, bool host, const char *rulesKey,
                            const char *rules, S_CampaignNetConfig &config);
