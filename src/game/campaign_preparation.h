#ifndef HD2_CAMPAIGN_PREPARATION_H
#define HD2_CAMPAIGN_PREPARATION_H

#include "campaign_contracts.h"
#include <functional>

// Synchronous authority component, not a screen/transport adapter. Every method,
// ownership publication and the shared journal must run on ONE nonreentrant
// authority queue. Authenticate the sender before calling; echo is checked here.
// No engine/profile mutation, background ownership polling or implicit stock reset.
using T_CampaignPreparationStep = std::variant<S_CampaignTakeFromStock,
    S_CampaignReturnToStock, S_CampaignMoveWithinLoadout>;
struct S_CampaignEquipmentPlan {
    E_CampaignReason m_Reason = E_CampaignReason::None;
    std::vector<T_CampaignPreparationStep> m_Steps;
};
struct S_CampaignEquipmentEvaluation {
    E_CampaignReason m_EditRejection = E_CampaignReason::None;
    // Derived metadata only. Original items/skills are retained by the authority.
    std::vector<S_CampaignSoldierLoadout> m_LoadoutMetrics;
    std::vector<S_CampaignEssential> m_Essentials;
    std::vector<S_CampaignFinding> m_Findings;
    std::vector<S_CampaignFinding> m_DeployBlockers;
};
class I_CampaignEquipmentRules {
public:
    virtual ~I_CampaignEquipmentRules() = default;
    // Expand preset/auto-equip and displacement into explicit bounded transfers.
    // Never touch live profile data. Direct operations may also need expansion
    // (backpack removal/displaced single slots). All steps remain target-scoped.
    virtual S_CampaignEquipmentPlan Plan(const S_CampaignPreparationSnapshot &state,
        const S_CampaignEditRequest &request) const = 0;
    // Validate catalog, ammo variants, placement, stacks, mission and capacity;
    // report essentials deficits as deploy blockers, not edit rejections.
    virtual S_CampaignEquipmentEvaluation Evaluate(const S_CampaignPreparationSnapshot &state) const = 0;
};
struct S_CampaignDeployEvaluation {
    E_CampaignReason m_Reason = E_CampaignReason::PolicyRequired;
    std::vector<PlayerId> m_RequiredParticipants;
    std::vector<S_CampaignFinding> m_Blockers;
};
class I_CampaignDeploymentPolicy {
public:
    virtual ~I_CampaignDeploymentPolicy() = default;
    // Explicit injected decision, including quorum/departure policy. Missing policy
    // is PolicyRequired; this component never assumes that all-ready is approved.
    virtual S_CampaignDeployEvaluation Evaluate(const S_CampaignPreparationSnapshot &state) const = 0;
};
// Detached, trusted campaign catalog inputs for the concrete rule evaluator.
// Populate once from the item definitions, original mission record and roster.
// No native layout, type ID, pointer or player slot is serialized in these values.
enum class E_CampaignEquipmentContainer { Head, Uniform, Hands, Backpack, LongGun, Belt, BackpackContents };
struct S_CampaignEquipmentPlacementRule {
    PlacementKey m_Key;
    E_CampaignEquipmentContainer m_Container = E_CampaignEquipmentContainer::Belt;
};
struct S_CampaignEquipmentItemRule {
    ItemCatalogKey m_Item;
    float m_Weight = 0;
    std::uint32_t m_InventorySize = 0; // volume: zero -> 1, otherwise twice size
    std::vector<PlacementKey> m_AllowedPlacements;
    std::optional<ItemCatalogKey> m_AmmoItem;
    std::uint32_t m_MaxRounds = 0;
    std::optional<float> m_MaxCharges;
    bool m_HasCondition = false;
    bool m_IsBackpack = false;
    float m_BackpackMaxWeight = 0;
    std::uint32_t m_BackpackVolume = 0;
    bool m_IsAxis = false;
    bool m_IsUniform = false;
    bool m_IsDivingGear = false;
    bool m_IsAnimated = false;
};
struct S_CampaignEquipmentSoldierRule {
    SoldierId m_Soldier;
    float m_Strength = 0; // original profile strength is truncated to capacity
    std::vector<ItemCatalogKey> m_ForbiddenItems; // mission restrictions only remove rights
};
struct S_CampaignEquipmentRequirement { ItemCatalogKey m_Item; std::uint32_t m_Count = 0; };
struct S_CampaignEquipmentRecipeItem {
    ItemCatalogKey m_Item;
    S_CampaignPlacement m_Placement;
    std::uint32_t m_Count = 0;
};
struct S_CampaignEquipmentRecipe {
    // Exactly one key is set. Auto-equip policy is explicit catalog data, never
    // inferred whole-squad work. Recipes use existing stock, never create supply.
    std::optional<PresetKey> m_Preset;
    std::optional<AutoEquipPolicyKey> m_AutoEquip;
    std::vector<S_CampaignEquipmentRecipeItem> m_Items;
};
struct S_CampaignEquipmentRuleCatalog {
    std::vector<S_CampaignEquipmentPlacementRule> m_Placements;
    std::vector<S_CampaignEquipmentItemRule> m_Items;
    std::vector<S_CampaignEquipmentSoldierRule> m_Soldiers;
    std::vector<S_CampaignEquipmentRequirement> m_Required;
    std::vector<S_CampaignEquipmentRecipe> m_Recipes;
};
struct S_CampaignSupplyItem {
    ItemCatalogKey m_Item;
    std::uint32_t m_Count = 0;
    bool m_Ammunition = false; // original campaign screen's ammunition flag
};
class C_CampaignEquipmentRules final : public I_CampaignEquipmentRules {
public:
    explicit C_CampaignEquipmentRules(S_CampaignEquipmentRuleCatalog catalog);
    S_CampaignEquipmentPlan Plan(const S_CampaignPreparationSnapshot &state,
        const S_CampaignEditRequest &request) const override;
    S_CampaignEquipmentEvaluation Evaluate(const S_CampaignPreparationSnapshot &state) const override;
    // Available supply only, excludes equipped items; call exactly once when
    // importing a campaign step. Weapon ammunition flags resolve through the
    // weapon's GetAmmoItemId equivalent, not extra weapon stock or loaded rounds.
    std::vector<S_CampaignStockEntry> NormalizeSupply(const std::vector<S_CampaignSupplyItem> &supply) const;
private:
    S_CampaignEquipmentRuleCatalog m_Catalog;
};
struct S_CampaignPreparationJournal {
    // Bound to the SAME cross-family journal as ownership dispatch. Authenticate
    // and generation-fence before Begin. Begin consumes new high-water IDs;
    // Finish caches semantic acceptance/rejection. Canonical content is local
    // comparison data (not wire encoding), includes family/context/all parameters.
    std::function<std::optional<S_CampaignRequestResult>(const S_CampaignRequestContext &,
        E_CampaignRequestFamily, const std::string &, const S_CampaignRequestResult &)> m_Begin;
    std::function<void(const S_CampaignRequestContext &, E_CampaignRequestFamily,
        const std::string &, const S_CampaignRequestResult &)> m_Finish;
};
struct S_CampaignPreparationLimits {
    std::size_t m_MaxSteps = 256;
    std::uint32_t m_MaxCount = 32767; // legacy signed-short count boundary
    std::size_t m_MaxEntries = 4096;
    std::size_t m_MaxKeyBytes = 256;
    std::size_t m_MaxCanonicalBytes = 4096; // common journal boundary
};
class C_CampaignPreparation {
public:
    // Supply normalized available stock exactly once, excluding equipped items.
    // Invalid trusted initialization throws invalid_argument; no partial model.
    C_CampaignPreparation(S_CampaignPreparationSnapshot initial,
        const I_CampaignOwnershipView &ownership, const I_CampaignEquipmentRules &rules,
        const I_CampaignDeploymentPolicy &policy, S_CampaignPreparationJournal journal,
        S_CampaignPreparationLimits limits = {});
    T_CampaignPreparationSnapshot GetSnapshot(PlayerId viewer) const;
    S_CampaignRequestResult ApplyEdit(PlayerId sender, const S_CampaignEditRequest &request);
    S_CampaignRequestResult SetReady(PlayerId sender, const S_CampaignReadyRequest &request);
    S_CampaignRequestResult RequestDeploy(PlayerId sender, const S_CampaignDeployRequest &request);
    // Call immediately after an ownership/membership commit on the SAME queue,
    // before publishing either model. No equipped items return to stock here.
    // Frozen membership/rights changes fail closed: abort/reopen in a NEW model
    // with a fresh preparation generation. Never thaw a frozen generation.
    E_CampaignReason SynchronizeOwnership();
    // Notify trusted catalog/rule/policy changes in the same queue transaction.
    // Dependency objects otherwise stay immutable for the model lifetime.
    E_CampaignReason RevalidateRules();
    const std::optional<S_CampaignMissionManifest> &GetFrozenManifest() const;
private:
    E_CampaignReason RefreshOwnership(bool initializing = false);
    E_CampaignReason Gate(PlayerId sender, const S_CampaignPreparationRequestContext &context);
    E_CampaignReason CanEdit(PlayerId sender, SoldierId soldier) const;
    E_CampaignReason CheckTargets(PlayerId sender, const S_CampaignEditRequest &request) const;
    E_CampaignReason ValidateShape(const S_CampaignEditRequest &request) const;
    E_CampaignReason ApplyStep(S_CampaignPreparationSnapshot &candidate,
        const T_CampaignPreparationStep &step, std::uint64_t &nextEntry) const;
    E_CampaignReason ValidateState(const S_CampaignPreparationSnapshot &state) const;
    void Evaluate(S_CampaignPreparationSnapshot &state) const;
    bool CanAdvance(bool preparation) const;
    void Advance(S_CampaignPreparationSnapshot &state, bool preparation) const;
    S_CampaignRequestResult Result(const S_CampaignPreparationRequestContext &context,
        E_CampaignRequestFamily family, E_CampaignReason reason) const;
    S_CampaignRequestResult Finish(const S_CampaignPreparationRequestContext &context,
        E_CampaignRequestFamily family, const std::string &content, E_CampaignReason reason);
    S_CampaignPreparationSnapshot m_State;
    const I_CampaignOwnershipView &m_Ownership;
    const I_CampaignEquipmentRules &m_Rules;
    const I_CampaignDeploymentPolicy &m_Policy;
    S_CampaignPreparationJournal m_Journal;
    S_CampaignPreparationLimits m_Limits;
    T_CampaignOwnershipSnapshot m_OwnershipSnapshot;
    std::uint64_t m_NextEntry = 1;
    std::optional<S_CampaignMissionManifest> m_FrozenManifest;
    bool m_Invalidated = false;
};
#endif
