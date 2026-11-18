#include "support.h"
#include <cmath>
#include <cstring>
#include <random>

// Collaborators for the extracted source functions. These deliberately model
// only fields/methods those functions touch; no native ABI is simulated.
namespace Legacy {
constexpr int ICK_BACKPACK = 7, ITEM_TEAM_AXIS = 1, BEHAVIOUR_UNIFORM = 9, DIVING_GEAR = 5;
constexpr float BELT_VOLUME = 12;
constexpr uint8_t SQUAD_SLOT_HEAD = 1, SQUAD_SLOT_UNIFORM = 2, SQUAD_SLOT_HANDS = 4,
    SQUAD_SLOT_BACKPACK = 8, SQUAD_SLOT_LONG_GUN = 16, SQUAD_SLOT_BELT = 32, SQUAD_SLOT_BACKPACK_CONTENTS = 64;
struct C_ItemComponent { int GetKind() const { return ICK_BACKPACK; } };
struct C_BackpackComp : C_ItemComponent {
    float m_Weight = 0, m_Volume = 0;
    float GetMaxWeight() const { return m_Weight; }
    float GetCapacity() const { return m_Volume; }
};
struct C_ItemDef {
    float m_Weight = 0;
    int m_Size = 0, m_Team = 0, m_Behaviour = 0;
    C_BackpackComp *m_Pack = nullptr;
    float GetWeight() const { return m_Weight; }
    int GetInventorySize() const { return m_Size; }
    C_ItemComponent *GetComponent() const { return m_Pack; }
    int GetTeam() const { return m_Team; }
    int GetBehaviour() const { return m_Behaviour; }
    int GetTextId() const { return 0; }
};
std::vector<C_ItemDef> g_Defs;
C_ItemDef *GetItemDef(int id) { return id >= 0 && static_cast<std::size_t>(id) < g_Defs.size() ? &g_Defs[static_cast<std::size_t>(id)] : nullptr; }
struct Items { void EnsureItemsLoaded() {} } g_itemsModule;
struct S_SoldierItem { int16_t uid = -1, typeId = -1, present = 0; };
struct S_SoldierLoad {
    int capacity = 8;
    float weight = 0, backpackMaxWeight = 0;
    int backpackMaxVolume = 0;
    float backpackWeight = 0;
    int backpackVolume = 0;
    float beltWeight = 0;
    int beltVolume = 0;
    float GetWeightFill() const;
};
struct S_SquadSoldier {
    S_SoldierItem head, uniform, backpack, longGun, hands;
    std::vector<S_SoldierItem> belt, backpackContents;
    S_SoldierLoad load;
    bool deployed = true;
    void PutItem(S_SoldierItem *, uint8_t, const S_SoldierItem &);
    int RemoveItem(uint8_t, S_SoldierItem &);
    bool IsEmpty() const;
};
struct Squad { std::vector<S_SquadSoldier *> states; };
struct Campaign { int GetMissionNumber(int) { return 0; } int GetCurrentIndex() { return 0; } };
using C_Campaign = Campaign;
struct Profile { Squad *m_missionSquad; Campaign *m_campaign; } g_gameProfile;
int g_requiredCarrier = -1;
struct S_MissionItems { std::vector<std::pair<int, int>> required; };
struct MissionItems { S_MissionItems m_Record; S_MissionItems *GetMissionRecord(int) { return &m_Record; } };
using C_str = std::string;
struct Texts { C_str Get(int) { return "fixture"; } } g_texts;
C_str C_fstr(const char *, const char *value) { return value; }
constexpr int MESSAGE_BOX_OK = 0, TEXT_ITEM_MISSING = 0;
struct C_EquipmentMenu {
    MissionItems m_missionItems;
    bool IsSquadWithinWeight(); int CountCarried(const std::pair<int, int> &, int); bool HasRequiredItems();
    void ShowMessage(int, const C_str &, int, int) {}
};
#include "legacy_bodies.inc"
}

S_CampaignEquipmentRuleCatalog Catalog() {
    using C = E_CampaignEquipmentContainer;
    S_CampaignEquipmentRuleCatalog c;
    c.m_Placements = {{PlacementKey("head"), C::Head}, {PlacementKey("uniform"), C::Uniform},
        {PlacementKey("hands"), C::Hands}, {PlacementKey("pack"), C::Backpack}, {PlacementKey("long"), C::LongGun},
        {PlacementKey("belt"), C::Belt}, {PlacementKey("backpack"), C::BackpackContents}};
    Legacy::g_Defs.clear();
    static Legacy::C_BackpackComp large, small;
    large.m_Weight = 5; large.m_Volume = 6; small.m_Weight = 2; small.m_Volume = 2;
    for (unsigned i = 0; i < 8; ++i) {
        Legacy::C_ItemDef native;
        native.m_Weight = i == 0 ? 4 : i == 1 ? 0.25f : i == 3 ? 0.5f : 1;
        native.m_Size = i == 0 ? 2 : i == 2 ? 3 : i == 3 ? 2 : 0;
        native.m_Team = i >= 6 ? Legacy::ITEM_TEAM_AXIS : 0;
        native.m_Behaviour = i == 4 ? Legacy::BEHAVIOUR_UNIFORM : 0;
        if (i == 2) native.m_Pack = &large;
        if (i == 3) native.m_Pack = &small;
        Legacy::g_Defs.push_back(native);
        S_CampaignEquipmentItemRule item;
        item.m_Item = ItemCatalogKey("item" + std::to_string(i)); item.m_Weight = native.m_Weight;
        item.m_InventorySize = static_cast<std::uint32_t>(native.m_Size);
        if (i == 0) { item.m_AllowedPlacements = {PlacementKey("hands"), PlacementKey("long"), PlacementKey("belt"), PlacementKey("backpack")}; item.m_AmmoItem = ItemCatalogKey("item1"); item.m_MaxRounds = 6; item.m_HasCondition = true; item.m_IsAnimated = true; }
        else if (native.m_Pack) {
            item.m_AllowedPlacements = {PlacementKey("pack")}; item.m_IsBackpack = true;
            item.m_BackpackMaxWeight = native.m_Pack->m_Weight; item.m_BackpackVolume = static_cast<std::uint32_t>(native.m_Pack->m_Volume);
        } else if (i == 4) item.m_AllowedPlacements = {PlacementKey("uniform")};
        else item.m_AllowedPlacements = {PlacementKey("belt"), PlacementKey("backpack")};
        item.m_IsAxis = i >= 6; item.m_IsUniform = i == 4; item.m_IsDivingGear = i == 5;
        c.m_Items.push_back(item);
    }
    for (unsigned i = 1; i <= 4; ++i) c.m_Soldiers.push_back({SoldierId(i * 10), 8.9f, {}});
    return c;
}
void Add(S_CampaignSoldierLoadout &l, unsigned type, const char *slot, unsigned count = 1) {
    S_CampaignLoadoutItem i;
    i.m_Entry = PreparationItemId(l.m_Items.size() + 1); i.m_State.m_Item = ItemCatalogKey("item" + std::to_string(type));
    i.m_State.m_Quantity = count; i.m_Placement = Place(slot); l.m_Items.push_back(i);
}
S_CampaignPreparationSnapshot Snapshot(const std::vector<S_CampaignSoldierLoadout> &loads) {
    S_CampaignPreparationSnapshot s; s.m_Loadouts = loads;
    for (const auto &l : loads) { S_CampaignSoldierPresentation p; p.m_Soldier = l.m_Soldier; p.m_Ownership.m_Deployed = true; s.m_Soldiers.push_back(p); }
    return s;
}
void CapacityParity() {
    const auto catalog = Catalog(); C_CampaignEquipmentRules rules(catalog);
    unsigned cases = 0;
    for (int capacity = 4; capacity <= 12; capacity += 4)
    for (unsigned pack = 2; pack <= 3; ++pack)
    for (unsigned belt = 0; belt <= 14; ++belt)
    for (unsigned contents = 0; contents <= 8; ++contents) {
        Legacy::S_SquadSoldier native; native.load.capacity = capacity;
        Legacy::S_SoldierItem left;
        auto put = [&](unsigned type, unsigned count, uint8_t slot) {
            Legacy::S_SoldierItem item; item.typeId = static_cast<int16_t>(type); item.present = static_cast<int16_t>(count);
            native.PutItem(&left, slot, item); return left.typeId == -1 || left.present == 0;
        };
        CHECK(put(pack, 1, Legacy::SQUAD_SLOT_BACKPACK));
        const bool beltFits = belt == 0 || put(1, belt, Legacy::SQUAD_SLOT_BELT);
        const bool packFits = contents == 0 || put(1, contents, Legacy::SQUAD_SLOT_BACKPACK_CONTENTS);
        auto data = catalog; data.m_Soldiers[0].m_Strength = static_cast<float>(capacity) + 0.9f;
        C_CampaignEquipmentRules evaluator(data);
        S_CampaignSoldierLoadout load; load.m_Soldier = SoldierId(10); Add(load, pack, "pack");
        if (belt) Add(load, 1, "belt", belt);
        if (contents) Add(load, 1, "backpack", contents);
        const auto result = evaluator.Evaluate(Snapshot({load}));
        CHECK((result.m_EditRejection == R::None) == (beltFits && packFits));
        if (beltFits && packFits) {
            const auto &metrics = result.m_LoadoutMetrics[0];
            CHECK(metrics.m_Weight == native.load.weight);
            CHECK(metrics.m_BeltVolume == static_cast<float>(native.load.beltVolume));
            CHECK(metrics.m_BackpackVolume == static_cast<float>(native.load.backpackVolume));
            CHECK(metrics.m_BackpackWeight == native.load.backpackWeight);
            // Compare occupied backpack replacement to the original PutItem.
            Legacy::S_SoldierItem replacement; replacement.typeId = 3; replacement.present = 1;
            native.PutItem(&left, Legacy::SQUAD_SLOT_BACKPACK, replacement);
            const bool changed = native.backpack.typeId == 3;
            load.m_Items[0].m_State.m_Item = ItemCatalogKey("item3");
            const auto swapped = evaluator.Evaluate(Snapshot({load}));
            CHECK((swapped.m_EditRejection == R::None) == changed);
        }
        ++cases;
    }
    std::cout << "legacy capacity/backpack differential cases: " << cases << '\n';
}
void EssentialParity() {
    auto catalog = Catalog();
    catalog.m_Required = {{ItemCatalogKey("item4"), 4}, {ItemCatalogKey("item5"), 4},
        {ItemCatalogKey("item6"), 2}, {ItemCatalogKey("item7"), 1}};
    C_CampaignEquipmentRules rules(catalog);
    unsigned cases = 0;
    for (unsigned deployedMask = 1; deployedMask < 16; ++deployedMask)
    for (unsigned axisMask = 0; axisMask < 16; ++axisMask) {
        std::vector<Legacy::S_SquadSoldier> native(4);
        Legacy::Squad squad; Legacy::Campaign campaign;
        std::vector<S_CampaignSoldierLoadout> loads;
        for (unsigned i = 0; i < 4; ++i) {
            native[i].deployed = (deployedMask & (1u << i)) != 0;
            native[i].uniform.typeId = 4; native[i].uniform.present = 1;
            native[i].belt.push_back({-1, 5, 1});
            if (axisMask & (1u << i)) native[i].belt.push_back({-1, 6, 1});
            // The second Axis essential is on a different carrier in some cases.
            if (i == 1) native[i].belt.push_back({-1, 7, 1});
            squad.states.push_back(&native[i]);
            S_CampaignSoldierLoadout load; load.m_Soldier = SoldierId((i + 1) * 10);
            Add(load, 4, "uniform"); Add(load, 5, "belt");
            if (axisMask & (1u << i)) Add(load, 6, "belt");
            if (i == 1) Add(load, 7, "belt");
            loads.push_back(load);
        }
        Legacy::g_gameProfile = {&squad, &campaign};
        Legacy::C_EquipmentMenu menu;
        menu.m_missionItems.m_Record.required = {{4, 4}, {5, 4}, {6, 2}, {7, 1}};
        auto snapshot = Snapshot(loads);
        for (unsigned i = 0; i < 4; ++i) snapshot.m_Soldiers[i].m_Ownership.m_Deployed = native[i].deployed;
        const auto result = rules.Evaluate(snapshot);
        Legacy::g_requiredCarrier = -1;
        bool passes = true;
        for (unsigned i = 0; i < 4; ++i) {
            const auto requirement = menu.m_missionItems.m_Record.required[i];
            const auto carried = menu.CountCarried(requirement, -1);
            CHECK((result.m_Essentials[i].m_Deficit == 0) == (carried != 0));
            if (!carried) passes = false;
        }
        CHECK(menu.HasRequiredItems() == passes);
        ++cases;
    }
    // Squad-size cap for uniforms/diving gear, including undeployed roster members.
    auto data = catalog; data.m_Required.resize(2);
    C_CampaignEquipmentRules capped(data);
    S_CampaignSoldierLoadout only; only.m_Soldier = SoldierId(10); Add(only, 4, "uniform"); Add(only, 5, "belt");
    CHECK(capped.Evaluate(Snapshot({only})).m_Essentials[0].m_Required == 1u);
    CHECK(capped.Evaluate(Snapshot({only})).m_Essentials[1].m_Required == 1u);
    std::cout << "legacy mission-essential/carrier differential cases: " << cases << '\n';
}
void SupplyAndConcreteTransactions() {
    auto data = Catalog();
    S_CampaignEquipmentRecipe recipe;
    recipe.m_AutoEquip = AutoEquipPolicyKey("campaign/fixture/ammo"); recipe.m_Items.push_back({ItemCatalogKey("item1"), Place("belt"), 1});
    data.m_Recipes.push_back(recipe);
    C_CampaignEquipmentRules rules(data);
    auto supply = rules.NormalizeSupply({{ItemCatalogKey("item0"), 1, false}, {ItemCatalogKey("item0"), 6, true},
        {ItemCatalogKey("item2"), 1, false}, {ItemCatalogKey("item3"), 1, false}});
    CHECK(supply[0].m_State.m_Quantity == 1u && supply[1].m_State.m_Quantity == 6u);
    Fixture fixture;
    auto initial = *fixture.Snapshot();
    initial.m_Stock = supply; for (auto &l : initial.m_Loadouts) l.m_Items.clear();
    C_CampaignPreparation model(initial, fixture.m_Ownership, rules, fixture.m_Policy, fixture.m_Journal.Bind());
    auto apply = [&](S_CampaignEditRequest r) { r.m_Context.m_ExpectedPreparationRevision = model.GetSnapshot(PlayerId(1))->m_PreparationRevision; return model.ApplyEdit(PlayerId(1), r); };
    CHECK(apply(fixture.Take(1, 10, 3, "pack")).m_Reason == R::None);
    CHECK(apply(fixture.Take(1, 10, 2, "backpack", 4)).m_Reason == R::None);
    auto before = Digest(model.GetSnapshot(PlayerId(1)));
    CHECK(apply(fixture.Take(1, 10, 4, "pack")).m_Reason == R::CapacityExceeded);
    CHECK(Digest(model.GetSnapshot(PlayerId(1))) == before); // displaced pack and stock rolled back
    auto pack = model.GetSnapshot(PlayerId(1))->m_Loadouts[0].m_Items[0].m_Entry;
    CHECK(apply(fixture.Edit(1, {10}, S_CampaignReturnToStock{SoldierId(10), pack, 1})).m_Reason == R::None);
    CHECK(model.GetSnapshot(PlayerId(1))->m_Loadouts[0].m_Items.empty());
    CHECK(model.GetSnapshot(PlayerId(1))->m_Stock[1].m_State.m_Quantity == 6u);
    CHECK(apply(fixture.Edit(1, {10, 30, 40}, S_CampaignAutoEquip{*recipe.m_AutoEquip, {}})).m_Reason == R::None);
    CHECK(model.GetSnapshot(PlayerId(1))->m_Stock[1].m_State.m_Quantity == 3u);
    CHECK(apply(fixture.Edit(1, {10, 20}, S_CampaignAutoEquip{*recipe.m_AutoEquip, {}})).m_Reason == R::NotOwner);
    CHECK(apply(fixture.Edit(1, {10}, S_CampaignAutoEquip{*recipe.m_AutoEquip, {ItemCatalogKey("item0")}})).m_Reason == R::PolicyRequired);
    auto invalid = model.GetSnapshot(PlayerId(1))->m_Loadouts[0];
    invalid.m_Items[0].m_State.m_AmmoType = ItemCatalogKey("item0");
    CHECK(rules.Evaluate(Snapshot({invalid})).m_EditRejection == R::InvalidItem);
}
int main() {
    try { CapacityParity(); EssentialParity(); SupplyAndConcreteTransactions();
        std::cout << "campaign preparation: original rule parity and concrete transactions passed\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
}
