#include "campaign_mission_native.h"
#include "actors_module.h"
#include "game.h"
#include "human.h"
#include "item_defs.h"
#include "pending.h"
#include "spawn_point.h"
#include "team.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace {
const S_CampaignNativeCatalog *g_Catalog = nullptr;
S_CampaignMissionNativeHooks g_Hooks;
using R = E_CampaignReason;
}
E_CampaignReason BuildCampaignNativeSpawns(const S_CampaignMissionManifest &manifest,
    const C_CampaignSession &session, const S_CampaignNativeCatalog &catalog,
    std::vector<S_CampaignNativeSpawn> &spawns) {
    std::vector<S_CampaignNativeSpawn> candidate;
    for (const auto &loadout : manifest.m_Loadouts) {
        auto seed = catalog.m_Soldiers.find(loadout.m_Soldier);
        const auto profile = session.FindProfile(loadout.m_Soldier);
        if (!profile || seed == catalog.m_Soldiers.end() || seed->second.profileIndex != *profile) return R::InvalidSoldier;
        S_CampaignNativeSpawn spawn; spawn.m_Soldier = loadout.m_Soldier; spawn.m_State = seed->second;
        spawn.m_State.ClearItems(); spawn.m_State.deployed = true;
        spawn.m_State.spectating = false; spawn.m_State.vip = false;
        spawn.m_Stats = {800,80,80,80,80,80,50,30,20,50,0,80};
        std::set<int> skills;
        for (const auto &skill : loadout.m_Skills) {
            auto stat = catalog.m_Skills.find(skill.m_Key);
            if (stat == catalog.m_Skills.end() || stat->second < 0 || stat->second >= STAT_COUNT ||
                !skills.insert(stat->second).second || !std::isfinite(skill.m_Value) || skill.m_Value < 0 ||
                skill.m_Value > 1000000) return R::Malformed;
            spawn.m_Stats[stat->second] = static_cast<int>(skill.m_Value);
        }
        // Require the original profile stats in the committed cut; never import
        // multiplayer classes/ranks or silently fill absent campaign skills.
        for (int stat = STAT_HEALTH; stat <= STAT_LOCKPICK; ++stat) if (!skills.count(stat)) return R::Malformed;
        if (spawn.m_Stats[STAT_HEALTH] <= 0) return R::Dead;
        std::set<std::pair<uint8_t, std::uint32_t>> placements;
        auto orderedItems = loadout.m_Items;
        std::stable_sort(orderedItems.begin(), orderedItems.end(), [](const auto &a, const auto &b) {
            return a.m_Placement.m_Position < b.m_Placement.m_Position;
        });
        if (orderedItems.size() > 4096) return R::CapacityExceeded;
        std::uint32_t totalItems = 0;
        for (const auto &item : orderedItems) {
            auto type = catalog.m_Items.find(item.m_State.m_Item);
            auto slot = catalog.m_Placements.find(item.m_Placement.m_Slot);
            if (type == catalog.m_Items.end() || type->second < 0 || type->second > 32767 || slot == catalog.m_Placements.end()) return R::InvalidItem;
            if (!item.m_State.m_Quantity || item.m_State.m_Quantity > 32767) return R::InvalidCount;
            if (item.m_State.m_Quantity > 4096 - totalItems) return R::CapacityExceeded;
            totalItems += item.m_State.m_Quantity;
            // Legacy campaign preparation carries type/count only. Rich live
            // item variants require the initial-world inventory reconstruction,
            // never lossy conversion through the six-byte soldier item record.
            if (item.m_State.m_AmmoType || item.m_State.m_Rounds || item.m_State.m_Charges || item.m_State.m_Condition) return R::InvalidItem;
            if (!placements.insert({slot->second, item.m_Placement.m_Position}).second) return R::InvalidPlacement;
            S_SoldierItem entry; entry.typeId = static_cast<int16_t>(type->second);
            entry.present = static_cast<int16_t>(item.m_State.m_Quantity);
            auto assign = [&](S_SoldierItem &target) {
                if (item.m_Placement.m_Position || entry.present != 1 || target.typeId != -1) return false;
                target = entry; return true;
            };
            switch (slot->second) {
            case SQUAD_SLOT_HEAD: if (!assign(spawn.m_State.head)) return R::InvalidPlacement; break;
            case SQUAD_SLOT_UNIFORM: if (!assign(spawn.m_State.uniform)) return R::InvalidPlacement; break;
            case SQUAD_SLOT_HANDS: if (!assign(spawn.m_State.hands)) return R::InvalidPlacement; break;
            case SQUAD_SLOT_BACKPACK: if (!assign(spawn.m_State.backpack)) return R::InvalidPlacement; break;
            case SQUAD_SLOT_LONG_GUN: if (!assign(spawn.m_State.longGun)) return R::InvalidPlacement; break;
            case SQUAD_SLOT_BELT: spawn.m_State.belt.push_back(entry); break;
            case SQUAD_SLOT_BACKPACK_CONTENTS: spawn.m_State.backpackContents.push_back(entry); break;
            default: return R::InvalidPlacement;
            }
        }
        if (spawn.m_State.uniform.typeId < 0) return R::RequiredItemsMissing;
        spawn.m_State.SplitStacks(); spawn.m_State.UpdateLoad();
        candidate.push_back(std::move(spawn));
    }
    if (candidate.empty() || candidate.size() > 4) return R::MissionRestriction;
    spawns = std::move(candidate); return R::None;
}
void InstallCampaignMissionNative(const S_CampaignNativeCatalog *catalog, S_CampaignMissionNativeHooks hooks) {
    g_Catalog = catalog; g_Hooks = std::move(hooks);
    if (GetCampaignMissionAdapter()) GetCampaignMissionAdapter()->SetRetirementHook(g_Hooks.m_RetireBeforeReconstruction);
}
C_Human *ResolveCampaignHuman(const S_CampaignOwnershipRecord &record) {
    const auto snapshot = GetCampaignMissionSnapshot();
    if (!snapshot || !record.m_Actor || record.m_Actor->m_Session != snapshot->m_Context.m_Session ||
        record.m_Actor->m_Generation != snapshot->m_Context.m_MissionGeneration ||
        record.m_Actor->m_Actor.Value() >= C_ActorsModule::PLAYER_SLOTS || record.m_Actor->m_SlotUse.Value() > 255) return nullptr;
    const int id = static_cast<int>(record.m_Actor->m_Actor.Value());
    C_Actor *actor = g_actorsModule.GetActorById(id);
    if (!actor || actor->GetType() != ACTOR_HUMAN ||
        static_cast<uint8_t>(g_actorsModule.GetPlayerSlotUse(id)) != record.m_Actor->m_SlotUse.Value()) return nullptr;
    return static_cast<C_Human *>(actor);
}
bool SpawnCampaignMissionSquad() {
    auto *adapter = GetCampaignMissionAdapter();
    if (!adapter || !g_Catalog || !IsCampaignSimulationAuthority() || !g_Hooks.m_SynchronizeControllers) return false;
    // The caller installs the frozen manifest with Begin before loading the map.
    const auto manifest = adapter->GetLoadingManifest();
    if (!manifest || adapter->GetContext().m_Phase != E_CampaignPhase::Loading) return false;
    std::vector<S_CampaignNativeSpawn> spawns;
    auto reason = BuildCampaignNativeSpawns(*manifest, adapter->GetOwnership().GetSession(), *g_Catalog, spawns);
    if (reason != R::None) { adapter->Abort(reason); return false; }
    // Preflight every original spawn point and free actor slot before effects.
    int freeSlots = 0;
    for (int id = 0; id < C_ActorsModule::PLAYER_SLOTS; ++id)
        if (!g_actorsModule.GetActorById(id)) ++freeSlots;
    if (freeSlots < static_cast<int>(spawns.size())) { adapter->Abort(R::ActorUnavailable); return false; }
    for (const auto &spawn : spawns) {
        if (!GetSoldierProfile(spawn.m_State.profileIndex)) { adapter->Abort(R::InvalidSoldier); return false; }
        const auto validItem = [](const S_SoldierItem &i) { return i.typeId < 0 || GetItemDef(i.typeId) != nullptr; };
        if (!validItem(spawn.m_State.head) || !validItem(spawn.m_State.uniform) ||
            !validItem(spawn.m_State.hands) || !validItem(spawn.m_State.backpack) || !validItem(spawn.m_State.longGun) ||
            !std::all_of(spawn.m_State.belt.begin(), spawn.m_State.belt.end(), validItem) ||
            !std::all_of(spawn.m_State.backpackContents.begin(), spawn.m_State.backpackContents.end(), validItem)) {
            adapter->Abort(R::InvalidItem); return false;
        }
    }
    int points = 0;
    for (int i = 0; i < g_actorsModule.GetActorCount(); ++i) {
        C_Actor *actor = g_actorsModule.GetActorById(i);
        if (actor && actor->GetType() == ACTOR_SPAWN_POINT) {
            auto *point = static_cast<C_SpawnPoint *>(actor);
            if (point->GetTeam() == 0 && point->IsEnabled()) ++points;
        }
    }
    if (points < static_cast<int>(spawns.size())) { adapter->Abort(R::ActorUnavailable); return false; }
    for (auto &spawn : spawns) {
        auto *point = static_cast<C_SpawnPoint *>(g_actorsModule.FindSpawnPoint(0));
        C_Human *human = point ? point->SpawnCampaign(spawn.m_State, spawn.m_Stats.data()) : nullptr;
        if (!human) { adapter->Abort(R::ActorUnavailable); return false; }
        const auto context = adapter->GetContext();
        ActorRef actor{context.m_Session, context.m_MissionGeneration,
            ActorId(static_cast<uint32_t>(human->GetNetId())),
            ActorSlotUse(static_cast<uint8_t>(g_actorsModule.GetPlayerSlotUse(human->GetNetId())))};
        reason = adapter->Bind(spawn.m_Soldier, actor);
        if (reason != R::None) { adapter->Abort(reason); return false; }
        point->NotifyCampaignSpawned();
    }
    reason = g_Hooks.m_SynchronizeControllers();
    if (reason != R::None) { adapter->Abort(reason); return false; }
    // Remain Loading: COOP-010 captures the complete initial world and calls
    // SealInitialWorld, then distributes it and collects reconstruction acks.
    return true;
}
bool SynchronizeCampaignMissionControllers() {
    if (!IsCampaignMission() || !IsCampaignSimulationAuthority()) return true;
    const auto reason = g_Hooks.m_SynchronizeControllers ? g_Hooks.m_SynchronizeControllers() : R::SnapshotUnavailable;
    if (reason == R::None) return true;
    if (GetCampaignMissionAdapter()) GetCampaignMissionAdapter()->Abort(reason);
    return false;
}
void CommitCampaignMissionSoldiers() {
    auto *adapter = GetCampaignMissionAdapter();
    if (!IsCampaignSimulationAuthority() || !adapter || adapter->GetFailure() != R::None ||
        (adapter->GetContext().m_Phase != E_CampaignPhase::Running &&
         adapter->GetContext().m_Phase != E_CampaignPhase::Ending)) return;
    const auto state = GetCampaignMissionSnapshot();
    std::vector<std::pair<S_SquadSoldier *, S_SquadSoldier>> updates;
    for (const auto &record : state->m_Soldiers) {
        if (!record.m_Deployed) continue;
        C_Human *human = ResolveCampaignHuman(record);
        const auto profile = adapter->GetOwnership().GetSession().FindProfile(record.m_Soldier);
        if (!human || !profile || human->GetProfileIndex() != *profile) { adapter->Abort(R::ActorUnavailable); return; }
        S_SquadSoldier *target = g_gameProfile.FindSoldierState(*profile);
        if (!target || !GetSoldierProfile(*profile)) { adapter->Abort(R::InvalidSoldier); return; }
        S_SquadSoldier result = *target;
        ClearSoldierItems(&result); human->GetSoldierState(result);
        // Live network item UIDs identify this world only, never the next mission.
        for (auto *item : {&result.head,&result.uniform,&result.hands,&result.backpack,&result.longGun}) item->uid = -1;
        for (auto &item : result.belt) item.uid = -1;
        for (auto &item : result.backpackContents) item.uid = -1;
        MergeSoldierItems(&result); UpdateSoldierLoad(&result);
        updates.emplace_back(target, std::move(result));
    }
    for (auto &update : updates) {
        *update.first = std::move(update.second);
        GetSoldierProfile(update.first->profileIndex)->dead = update.first->health < 1;
    }
}
void HandleCampaignMissionDeparture(int connection) {
    if (connection == 0) return;
    // Resolve BEFORE clearing connection fields; never infer PlayerId from a slot.
    S_NetPlayerState *players = g_netGame.LockPlayers();
    std::optional<PlayerId> player;
    int departedSlot = -1;
    for (int slot = 0; slot < g_netGame.GetPlayerCount(); ++slot) if (players[slot].connection == connection) {
        departedSlot = slot;
        if (g_Hooks.m_FindTransportPlayer) player = g_Hooks.m_FindTransportPlayer(slot);
        break;
    }
    g_netGame.UnlockPlayers();
    if (departedSlot < 0) return; // repeated exact departure is already retired
    if (player && g_Hooks.m_Disconnected) {
        const auto reason = g_Hooks.m_Disconnected(*player);
        if (reason != R::None && GetCampaignMissionAdapter()) GetCampaignMissionAdapter()->Abort(reason);
    } else if (GetCampaignMissionAdapter()) GetCampaignMissionAdapter()->Abort(R::NotAdmitted);
    // Retire connection bookkeeping only AFTER ownership/handoff. The actor and
    // mines remain in the world and the full player-team membership is preserved.
    players = g_netGame.LockPlayers();
    if (departedSlot >= 0 && players[departedSlot].connection == connection) {
        auto &departed = players[departedSlot];
        departed.connection = 0; departed.name[0] = 0; departed.left = true;
        departed.human = nullptr;
        departed.stats = S_NetStats{};
    }
    g_netGame.UnlockPlayers();
}
void PublishCampaignMissionResult(bool success) {
    if (!IsCampaignSimulationAuthority()) return;
    auto *adapter = GetCampaignMissionAdapter();
    if (adapter && adapter->GetContext().m_Phase == E_CampaignPhase::Running) adapter->End();
    if (g_Hooks.m_CampaignResult) g_Hooks.m_CampaignResult(success);
}
