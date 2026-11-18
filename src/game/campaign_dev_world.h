#ifndef HD2_CAMPAIGN_DEV_WORLD_H
#define HD2_CAMPAIGN_DEV_WORLD_H

#include "net/campaign_protocol.h"

// Freeze the original fixture's native world at mission time zero. The replica
// checks its reconstructed actor, item, objective and script records before
// applying the peer's validated authoritative actor poses.
E_CampaignReason CaptureDevelopmentInitialWorld(CampaignProtocol::T_Bytes &world);
E_CampaignReason InstallDevelopmentInitialWorld(const CampaignProtocol::T_Bytes &world);

#endif
