#ifndef HD2_CAMPAIGN_DEV_FIXTURE_H
#define HD2_CAMPAIGN_DEV_FIXTURE_H

#include "game/campaign_mission_native.h"

class C_CampaignScreen;
struct S_CampaignEquipmentRuleCatalog;

E_CampaignReason FreezeDevelopmentCampaign(C_CampaignOwnership &ownership, C_CampaignRequestJournal &journal,
    int missionNumber, PlayerId guest, S_CampaignNativeCatalog &native, S_CampaignMissionManifest &manifest);

E_CampaignReason ImportDevelopmentReplica(const C_CampaignSession &roster, int missionNumber,
    const S_CampaignMissionManifest &manifest, S_CampaignNativeCatalog &native,
    std::vector<S_CampaignNativeSpawn> &spawns);

E_CampaignReason ImportInitialCampaignPreparation(const C_CampaignSession &session, int missionNumber,
    const C_CampaignScreen &equipment, S_CampaignNativeCatalog &native,
    S_CampaignEquipmentRuleCatalog &catalog, S_CampaignPreparationSnapshot &snapshot);

#endif
