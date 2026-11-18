#include "game/campaign_dev_fixture.h"

#include <algorithm>
#include <stdexcept>

#include "game/campaign_dev.h"
#include "game/campaign.h"
#include "game/campaign_preparation.h"
#include "game/item_defs.h"
#include "game/loadout_presets.h"
#include "game/profile.h"
#include "net/campaign_protocol.h"

namespace {

using R = E_CampaignReason;
using Container = E_CampaignEquipmentContainer;

struct S_Placement {
    const char *m_Name;
    std::uint8_t m_Slot;
    Container m_Container;
};

constexpr S_Placement kPlacements[] = {
    {"head", SQUAD_SLOT_HEAD, Container::Head},
    {"uniform", SQUAD_SLOT_UNIFORM, Container::Uniform},
    {"hands", SQUAD_SLOT_HANDS, Container::Hands},
    {"backpack", SQUAD_SLOT_BACKPACK, Container::Backpack},
    {"long-gun", SQUAD_SLOT_LONG_GUN, Container::LongGun},
    {"belt", SQUAD_SLOT_BELT, Container::Belt},
    {"backpack-contents", SQUAD_SLOT_BACKPACK_CONTENTS, Container::BackpackContents},
};

constexpr int kDivingGear = 231;
constexpr int kAxisItem = 1;
constexpr int kKitLimit = 4;
constexpr PlayerId kHost{1};

class C_DevelopmentQuorum final : public I_CampaignDeploymentPolicy {
public:
    explicit C_DevelopmentQuorum(PlayerId guest) : m_Guest(guest) {}

    S_CampaignDeployEvaluation Evaluate(const S_CampaignPreparationSnapshot &snapshot) const override
    {
        if (snapshot.m_Participants.size() != 2)
            return {R::NotAdmitted, {}, {}};
        for (const auto &participant : snapshot.m_Participants) {
            if (!participant.m_Admitted || !participant.m_Connected ||
                (participant.m_Player != kHost && participant.m_Player != m_Guest))
                return {R::NotAdmitted, {}, {}};
            if (participant.m_ReadyAt != snapshot.m_PreparationRevision)
                return {R::PendingTransaction, {}, {}};
        }
        return {R::None, {kHost, m_Guest}, {}};
    }

private:
    PlayerId m_Guest;
};

ItemCatalogKey ItemKey(int type)
{
    return ItemCatalogKey("original-item/" + std::to_string(type));
}

bool ImportDefinition(int type, S_CampaignNativeCatalog &native, S_CampaignEquipmentRuleCatalog &catalog,
                      bool includeAmmo = false)
{
    const auto key = ItemKey(type);
    if (native.m_Items.count(key))
        return true;
    auto *definition = GetItemDef(type);
    if (!definition || definition->GetInventorySize() < 0)
        return false;

    S_CampaignEquipmentItemRule rule;
    rule.m_Item = key;
    rule.m_Weight = definition->GetWeight();
    rule.m_InventorySize = static_cast<std::uint32_t>(definition->GetInventorySize());
    rule.m_IsAnimated = definition->GetRecordType() == ITEM_ANIMATED;
    rule.m_IsAxis = definition->GetTeam() == kAxisItem;
    rule.m_IsUniform = definition->GetBehaviour() == ITEM_BEHAVIOUR_UNIFORM;
    rule.m_IsDivingGear = type == kDivingGear;
    const auto *component = definition->GetComponent();
    if (component && component->GetKind() == ICK_BACKPACK) {
        const auto *backpack = static_cast<const C_BackpackComp *>(component);
        rule.m_IsBackpack = true;
        rule.m_BackpackMaxWeight = backpack->GetMaxWeight();
        rule.m_BackpackVolume = static_cast<std::uint32_t>(backpack->GetCapacity());
    }
    for (const auto &placement : kPlacements) {
        if (definition->GetAllowedContainers() & placement.m_Slot)
            rule.m_AllowedPlacements.emplace_back(placement.m_Name);
    }
    if (rule.m_AllowedPlacements.empty())
        return false;
    // This fixture imports preparation type/count records. Live variations
    // and instance UIDs belong to the initial-world snapshot.
    native.m_Items.emplace(key, type);
    catalog.m_Items.push_back(std::move(rule));
    if (includeAmmo) {
        const int ammo = definition->GetAmmoItemId();
        auto *ammoDefinition = ammo >= 0 ? GetItemDef(ammo) : nullptr;
        if (ammoDefinition && ammoDefinition->GetRecordType() == ITEM_AMMO) {
            catalog.m_Items.back().m_AmmoItem = ItemKey(ammo);
            catalog.m_Items.back().m_MaxRounds = static_cast<std::uint32_t>(ammoDefinition->GetAmmoAmount());
            if (!ImportDefinition(ammo, native, catalog, true))
                return false;
        }
    }
    return true;
}

bool ImportKit(const S_MissionItems::S_Slot &slot, const S_LoadoutPreset &kit, S_CampaignSoldierLoadout &loadout,
    S_CampaignNativeCatalog &native, S_CampaignEquipmentRuleCatalog &catalog, std::uint64_t &entry,
    bool includeAmmo)
{
    if (kit.beltCount < 0 || kit.beltCount > S_LoadoutPreset::BELT_ITEMS ||
        kit.backpackCount < 0 || kit.backpackCount > S_LoadoutPreset::BACKPACK_ITEMS)
        return false;

    const auto add = [&](const char *placement, int type, int quantity, std::uint32_t position = 0) {
        if (type == -1)
            return true;
        if (quantity < 1 || quantity > 32767 || !ImportDefinition(type, native, catalog, includeAmmo)) {
            LogDevelopmentCampaign("invalid original item or quantity", static_cast<std::uint64_t>(type),
                                   static_cast<std::uint64_t>(quantity));
            return false;
        }
        S_CampaignLoadoutItem item;
        item.m_Entry = PreparationItemId(++entry);
        item.m_State.m_Item = ItemKey(type);
        item.m_State.m_Quantity = static_cast<std::uint32_t>(quantity);
        item.m_Placement = {PlacementKey(placement), position};
        loadout.m_Items.push_back(std::move(item));
        return true;
    };

    const int uniform = slot.c >= 0 ? slot.c : kit.uniform;
    if (uniform < 0 || !add("uniform", uniform, 1) || !add("head", slot.b >= 0 ? slot.b : kit.head, 1) ||
        !add("hands", kit.hands, 1) || !add("backpack", kit.backpack, 1) || !add("long-gun", kit.longGun, 1))
        return false;

    for (int i = 0; i < kit.beltCount; ++i) {
        const auto &item = kit.belt[i];
        if (!add("belt", item.typeId, item.count, static_cast<std::uint32_t>(i)))
            return false;
    }
    for (int i = 0; i < kit.backpackCount; ++i) {
        const auto &item = kit.backpackItems[i];
        if (!add("backpack-contents", item.typeId, item.count, static_cast<std::uint32_t>(i)))
            return false;
    }
    return true;
}

R Claim(C_CampaignOwnership &ownership, PlayerId player, std::size_t index)
{
    const auto state = ownership.GetSnapshot();
    const auto &soldier = state->m_Soldiers[index];
    S_CampaignOwnershipRequest request;
    request.m_Context = {state->m_Context.m_Session, player, RequestId(1)};
    request.m_PreparationGeneration = ownership.GetSession().GetPreparationGeneration();
    request.m_ExpectedOwnershipRevision = state->m_Revision;
    request.m_Operation = S_CampaignClaim{{soldier.m_Soldier, soldier.m_ControlRevision}};
    return ownership.ApplyRequest(player, request).m_Reason;
}

bool IssueEssential(const C_CampaignEquipmentRules &rules, const S_CampaignEssential &required,
    const S_CampaignNativeCatalog &catalog, S_CampaignPreparationSnapshot &snapshot, std::uint64_t &entry)
{
    auto *type = GetItemDef(catalog.m_Items.at(required.m_Item));
    if (!type)
        return false;
    for (auto &loadout : snapshot.m_Loadouts) {
        if (std::find(required.m_EligibleCarriers.begin(), required.m_EligibleCarriers.end(), loadout.m_Soldier) ==
            required.m_EligibleCarriers.end())
            continue;
        for (const auto &placement : kPlacements) {
            if (!(type->GetAllowedContainers() & placement.m_Slot))
                continue;
            const bool container = placement.m_Container == Container::Belt ||
                                   placement.m_Container == Container::BackpackContents;
            const PlacementKey key(placement.m_Name);
            std::uint32_t position = 0;
            for (const auto &item : loadout.m_Items) {
                if (item.m_Placement.m_Slot == key)
                    position = (std::max)(position, item.m_Placement.m_Position + 1);
            }
            if (!container && position != 0)
                continue;
            S_CampaignLoadoutItem item;
            item.m_Entry = PreparationItemId(entry + 1);
            item.m_State.m_Item = required.m_Item;
            item.m_State.m_Quantity = 1;
            item.m_Placement = {key, position};
            loadout.m_Items.push_back(std::move(item));
            const auto evaluation = rules.Evaluate(snapshot);
            if (evaluation.m_EditRejection == R::None) {
                ++entry;
                return true;
            }
            loadout.m_Items.pop_back();
        }
    }
    return false;
}

R IssueRequiredEquipment(const C_CampaignEquipmentRules &rules, const S_CampaignNativeCatalog &catalog,
    S_CampaignPreparationSnapshot &snapshot, std::uint64_t &entry)
{
    auto evaluation = rules.Evaluate(snapshot);
    if (evaluation.m_EditRejection != R::None)
        return evaluation.m_EditRejection;
    // Fixed development issue of original mission essentials. Use the same
    // capacity and placement checks as the preparation cut and respect the
    // receiver's total item-entry limit.
    while (entry < CampaignProtocol::kMaxCollection) {
        auto required = std::find_if(evaluation.m_Essentials.begin(), evaluation.m_Essentials.end(),
                                     [](const auto &item) { return item.m_Deficit != 0; });
        if (required == evaluation.m_Essentials.end())
            return R::None;
        const auto item = required->m_Item;
        const auto deficit = required->m_Deficit;
        if (!IssueEssential(rules, *required, catalog, snapshot, entry)) {
            LogDevelopmentCampaign("unable to issue a required original item", catalog.m_Items.at(item), deficit);
            return R::RequiredItemsMissing;
        }
        evaluation = rules.Evaluate(snapshot);
        const auto updated = std::find_if(evaluation.m_Essentials.begin(), evaluation.m_Essentials.end(),
                                          [&](const auto &candidate) { return candidate.m_Item == item; });
        if (updated == evaluation.m_Essentials.end() || updated->m_Deficit >= deficit)
            return R::RequiredItemsMissing;
        LogDevelopmentCampaign("issued a required original item", catalog.m_Items.at(item), updated->m_Deficit);
    }
    return R::CapacityExceeded;
}

R ImportFixture(const C_CampaignSession &session, int missionNumber, S_CampaignNativeCatalog &native,
    S_CampaignEquipmentRuleCatalog &catalog, S_CampaignPreparationSnapshot &snapshot, std::uint64_t &entry,
    bool includeAmmo = false)
{
    C_MissionItems missionItems;
    if (includeAmmo)
        missionItems.Setup(0);
    else
        missionItems.Setup();
    C_LoadoutPresets presets;
    const auto *mission = missionItems.GetMissionRecord(missionNumber);
    if (!mission || !presets.LoadDefaults())
        return R::SnapshotUnavailable;

    for (const auto &placement : kPlacements) {
        PlacementKey key(placement.m_Name);
        native.m_Placements.emplace(key, placement.m_Slot);
        catalog.m_Placements.push_back({key, placement.m_Container});
    }
    for (int stat = STAT_HEALTH; stat <= STAT_LOCKPICK; ++stat)
        native.m_Skills.emplace(SkillKey("original-stat/" + std::to_string(stat)), stat);

    const auto &roster = session.GetRoster();
    if (roster.size() != kKitLimit)
        return R::InvalidSoldier;
    for (std::size_t i = 0; i < roster.size(); ++i) {
        const auto &record = roster[i];
        const auto &slot = mission->slots[i];
        if (slot.a < 0 || slot.a >= presets.GetPresetCount()) {
            LogDevelopmentCampaign("missing original kit", i, static_cast<std::uint64_t>(slot.a));
            return R::InvalidItem;
        }

        const auto *profile = GetSoldierProfile(record.m_ProfileIndex);
        S_SquadSoldier seed;
        if (!profile || !seed.SetProfile(record.m_ProfileIndex, 0))
            return R::InvalidSoldier;
        seed.name = profile->fullName;
        seed.team = 0;
        seed.speechSet = -1;

        S_CampaignSoldierLoadout loadout;
        loadout.m_Soldier = record.m_Soldier;
        for (const auto &stat : native.m_Skills)
            loadout.m_Skills.push_back({stat.first, profile->stats[stat.second]});
        if (!ImportKit(slot, *presets.GetPresets()[slot.a], loadout, native, catalog, entry, includeAmmo)) {
            LogDevelopmentCampaign("invalid original kit", i, static_cast<std::uint64_t>(slot.a));
            return R::InvalidItem;
        }
        LogDevelopmentCampaign("imported original kit", record.m_Soldier.Value(), static_cast<std::uint64_t>(slot.a));
        snapshot.m_Loadouts.push_back(std::move(loadout));

        S_CampaignSoldierPresentation presentation;
        presentation.m_Soldier = record.m_Soldier;
        presentation.m_Profile = SoldierProfileKey("original-roster/" + std::to_string(record.m_ProfileIndex));
        presentation.m_DisplayName = profile->fullName.c_str();
        if (includeAmmo)
            presentation.m_Preview = AssetKey("original-portrait/" + std::to_string(record.m_ProfileIndex));
        snapshot.m_Soldiers.push_back(std::move(presentation));
        snapshot.m_Slots[i].m_Soldier = record.m_Soldier;
        catalog.m_Soldiers.push_back({record.m_Soldier, profile->stats[STAT_STRENGTH], {}});
        native.m_Soldiers.emplace(record.m_Soldier, std::move(seed));
    }
    for (const auto &[item, quantity] : mission->required) {
        if (quantity < 1 || !ImportDefinition(item, native, catalog, includeAmmo))
            return R::InvalidItem;
        catalog.m_Required.push_back({ItemKey(item), static_cast<std::uint32_t>(quantity)});
    }
    return R::None;
}

} // namespace

R ImportInitialCampaignPreparation(const C_CampaignSession &session, int missionNumber,
    const C_CampaignScreen &equipment, S_CampaignNativeCatalog &native,
    S_CampaignEquipmentRuleCatalog &catalog, S_CampaignPreparationSnapshot &snapshot)
{
    std::uint64_t entry = 0;
    auto reason = ImportFixture(session, missionNumber, native, catalog, snapshot, entry, true);
    if (reason != R::None)
        return reason;
    for (auto &soldier : snapshot.m_Soldiers) {
        soldier.m_Ownership.m_Soldier = soldier.m_Soldier;
        soldier.m_Ownership.m_Deployed = true;
    }

    std::vector<S_CampaignSupplyItem> supply;
    for (const auto &item : equipment.GetItems()) {
        if (item.count < 0 || !ImportDefinition(item.itemId, native, catalog, true)) {
            LogDevelopmentCampaign("invalid campaign stock row", static_cast<std::uint64_t>(item.itemId), item.count);
            return R::InvalidItem;
        }
        supply.push_back({ItemKey(item.itemId), static_cast<std::uint32_t>(item.count), item.ammunition});
    }
    try {
        C_CampaignEquipmentRules rules(catalog);
        reason = IssueRequiredEquipment(rules, native, snapshot, entry);
        if (reason != R::None)
            return reason;
        snapshot.m_Stock = rules.NormalizeSupply(supply);
        for (auto &stock : snapshot.m_Stock) {
            const auto definition = std::find_if(catalog.m_Items.begin(), catalog.m_Items.end(),
                [&](const auto &rule) { return rule.m_Item == stock.m_State.m_Item; });
            // The original first issue draws from available stock and fills any
            // shortfall for a fresh soldier. All later edits conserve supply.
            for (const auto &loadout : snapshot.m_Loadouts) {
                for (const auto &item : loadout.m_Items)
                    if (item.m_State.m_Item == stock.m_State.m_Item)
                        stock.m_State.m_Quantity -= (std::min)(stock.m_State.m_Quantity, item.m_State.m_Quantity);
                stock.m_Destinations.push_back({loadout.m_Soldier, definition->m_AllowedPlacements, {true, R::None}});
            }
        }
    } catch (const std::invalid_argument &error) {
        LogDevelopmentCampaign(error.what());
        return R::InvalidItem;
    }
    LogDevelopmentCampaign("imported initial campaign stock", snapshot.m_Stock.size(), entry);
    return R::None;
}

R FreezeDevelopmentCampaign(C_CampaignOwnership &ownership, C_CampaignRequestJournal &journal,
    int missionNumber, PlayerId guest, S_CampaignNativeCatalog &native, S_CampaignMissionManifest &manifest)
{
    const auto roster = ownership.GetSession().GetRoster();
    for (const auto &record : roster) {
        const auto reason = ownership.SetAvailability(record.m_Soldier, true, true);
        if (reason != R::None)
            return reason;
    }

    auto reason = ownership.StartPreparation();
    if (reason != R::None)
        return reason;
    if ((reason = Claim(ownership, kHost, 0)) != R::None || (reason = Claim(ownership, guest, 1)) != R::None)
        return reason;

    S_CampaignEquipmentRuleCatalog catalog;
    S_CampaignPreparationSnapshot snapshot;
    const auto ownershipCut = ownership.GetSnapshot();
    snapshot.m_Context = ownershipCut->m_Context;
    snapshot.m_Generation = ownership.GetSession().GetPreparationGeneration();
    snapshot.m_PreparationRevision = PreparationRevision(1);
    snapshot.m_ViewRevision = ViewRevision(1);
    snapshot.m_OwnershipRevision = ownershipCut->m_Revision;
    std::uint64_t entry = 0;
    reason = ImportFixture(ownership.GetSession(), missionNumber, native, catalog, snapshot, entry);
    if (reason != R::None)
        return reason;
    for (std::size_t i = 0; i < snapshot.m_Soldiers.size(); ++i)
        snapshot.m_Soldiers[i].m_Ownership = ownershipCut->m_Soldiers[i];

    try {
        C_CampaignEquipmentRules rules(std::move(catalog));
        reason = IssueRequiredEquipment(rules, native, snapshot, entry);
        if (reason != R::None)
            return reason;
        C_DevelopmentQuorum policy(guest);
        S_CampaignPreparationJournal bridge;
        bridge.m_Begin = [&journal](const auto &context, auto family, const auto &data, const auto &state) {
            return journal.Begin(context, family, data, state);
        };
        bridge.m_Finish = [&journal](const auto &context, auto family, const auto &data, const auto &state) {
            journal.Finish(context, family, data, state);
        };
        S_CampaignPreparationLimits limits;
        limits.m_MaxEntries = CampaignProtocol::kMaxCollection;
        const S_CampaignPreparationRequestContext hostReady{
            {snapshot.m_Context.m_Session, kHost, RequestId(2)}, snapshot.m_Generation, snapshot.m_PreparationRevision};
        auto guestReady = hostReady;
        guestReady.m_Request.m_PlayerEcho = guest;
        auto deploy = hostReady;
        deploy.m_Request.m_Request = RequestId(3);

        C_CampaignPreparation preparation(std::move(snapshot), ownership, rules, policy, std::move(bridge), limits,
                                          [&ownership] { return ownership.Freeze(); });
        if ((reason = preparation.SetReady(kHost, {hostReady, true}).m_Reason) != R::None ||
            (reason = preparation.SetReady(guest, {guestReady, true}).m_Reason) != R::None ||
            (reason = preparation.RequestDeploy(kHost, {deploy}).m_Reason) != R::None)
            return reason;

        const auto frozen = preparation.GetFrozenManifest();
        if (!frozen)
            return R::SnapshotUnavailable;
        manifest = *frozen;
        return R::None;
    } catch (const std::invalid_argument &error) {
        LogDevelopmentCampaign(error.what());
        return R::InvalidItem;
    }
}

E_CampaignReason ImportDevelopmentReplica(const C_CampaignSession &roster, int missionNumber,
    const S_CampaignMissionManifest &manifest, S_CampaignNativeCatalog &native,
    std::vector<S_CampaignNativeSpawn> &spawns)
{
    S_CampaignPreparationSnapshot local;
    S_CampaignEquipmentRuleCatalog rules;
    std::uint64_t entry = 0;
    const auto reason = ImportFixture(roster, missionNumber, native, rules, local, entry);
    if (reason != R::None)
        return reason;
    // Reject quantities, placements, and equipment outside this agreed fixture.
    for (std::size_t i = 0; i < local.m_Soldiers.size(); ++i) {
        local.m_Soldiers[i].m_Ownership.m_Soldier = local.m_Soldiers[i].m_Soldier;
        local.m_Soldiers[i].m_Ownership.m_Deployed = true;
    }
    try {
        C_CampaignEquipmentRules equipment(std::move(rules));
        const auto issued = IssueRequiredEquipment(equipment, native, local, entry);
        if (issued != R::None)
            return issued;
    } catch (const std::invalid_argument &) {
        return R::InvalidItem;
    }
    if (local.m_Loadouts.size() != manifest.m_Loadouts.size() || local.m_Soldiers.size() != manifest.m_Soldiers.size())
        return R::ContentMismatch;
    for (std::size_t i = 0; i < local.m_Loadouts.size(); ++i) {
        const auto &expected = local.m_Loadouts[i];
        const auto &received = manifest.m_Loadouts[i];
        if (expected.m_Soldier != received.m_Soldier || expected.m_Items.size() != received.m_Items.size() ||
            expected.m_Skills.size() != received.m_Skills.size() ||
            local.m_Soldiers[i].m_Profile != manifest.m_Soldiers[i].m_Profile)
            return R::ContentMismatch;
        for (std::size_t j = 0; j < expected.m_Skills.size(); ++j) {
            if (expected.m_Skills[j].m_Key != received.m_Skills[j].m_Key ||
                expected.m_Skills[j].m_Value != received.m_Skills[j].m_Value)
                return R::ContentMismatch;
        }
        for (std::size_t j = 0; j < expected.m_Items.size(); ++j) {
            const auto &a = expected.m_Items[j];
            const auto &b = received.m_Items[j];
            if (a.m_Entry != b.m_Entry || a.m_Placement.m_Slot != b.m_Placement.m_Slot ||
                a.m_Placement.m_Position != b.m_Placement.m_Position || a.m_State.m_Item != b.m_State.m_Item ||
                a.m_State.m_Quantity != b.m_State.m_Quantity || b.m_State.m_AmmoType || b.m_State.m_Rounds ||
                b.m_State.m_Charges || b.m_State.m_Condition)
                return R::ContentMismatch;
        }
    }
    return BuildCampaignNativeSpawns(manifest, roster, native, spawns);
}
