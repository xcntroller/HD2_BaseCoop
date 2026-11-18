#ifndef HD2_CAMPAIGN_MISSION_NATIVE_H
#define HD2_CAMPAIGN_MISSION_NATIVE_H
#include "campaign_mission_adapter.h"
#include "profile.h"
#include <array>
#include <map>

class C_Human;
// Immutable native import catalog for one campaign content descriptor. Bootstrap
// imports original profile metadata once; committed equipment/skills always replace
// the seed's values. No network keys are interpreted as numeric legacy indices.
struct S_CampaignNativeCatalog {
    std::map<ItemCatalogKey, int> m_Items;
    std::map<PlacementKey, uint8_t> m_Placements;
    std::map<SkillKey, int> m_Skills;
    std::map<SoldierId, S_SquadSoldier> m_Soldiers;
};
struct S_CampaignNativeSpawn {
    SoldierId m_Soldier;
    S_SquadSoldier m_State;
    std::array<int, STAT_COUNT> m_Stats;
};
E_CampaignReason BuildCampaignNativeSpawns(const S_CampaignMissionManifest &manifest,
    const C_CampaignSession &session, const S_CampaignNativeCatalog &catalog,
    std::vector<S_CampaignNativeSpawn> &spawns);

// All attachment functions are trusted integration seams. COOP-010 must bind the
// control adapter's exact-incarnation disconnect helper and synchronize hook.
struct S_CampaignMissionNativeHooks {
    std::function<E_CampaignReason()> m_SynchronizeControllers;
    std::function<E_CampaignReason()> m_RetireBeforeReconstruction;
    std::function<std::optional<PlayerId>(int)> m_FindTransportPlayer;
    std::function<E_CampaignReason(PlayerId)> m_Disconnected;
    std::function<void(bool)> m_CampaignResult;
};
void InstallCampaignMissionNative(const S_CampaignNativeCatalog *catalog,
    S_CampaignMissionNativeHooks hooks = {});
bool SpawnCampaignMissionSquad();
bool SynchronizeCampaignMissionControllers();
void CommitCampaignMissionSoldiers();
void HandleCampaignMissionDeparture(int connection);
void PublishCampaignMissionResult(bool success);
C_Human *ResolveCampaignHuman(const S_CampaignOwnershipRecord &record);
#endif
