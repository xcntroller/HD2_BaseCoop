#ifndef HD2_CAMPAIGN_DEV_FIXTURE_H
#define HD2_CAMPAIGN_DEV_FIXTURE_H

#include "game/campaign_mission_native.h"

E_CampaignReason FreezeDevelopmentCampaign(C_CampaignOwnership &ownership, C_CampaignRequestJournal &journal,
    int missionNumber, PlayerId guest, S_CampaignNativeCatalog &native, S_CampaignMissionManifest &manifest);

#endif
