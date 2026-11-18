#ifndef HD2_CAMPAIGN_CONTRACTS_H
#define HD2_CAMPAIGN_CONTRACTS_H

// COOP-011: engine-independent logical values, never a wire/save/legacy ABI.
// C++17. Do not pack, memcpy-serialize, or infer wire widths from these types.
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// Generated identities/counters: zero invalid; explicit construction at catalog,
// authority or codec boundaries only. No conversion between tags or to integers.
// Ordering is meaningful only inside the documented scope. Never wrap/reuse a
// counter; advance the enclosing generation/session before exhausting its range.
template<class TTag> class C_CampaignValue {
public:
    constexpr C_CampaignValue() = default;
    explicit constexpr C_CampaignValue(std::uint64_t value) : m_Value(value) {}
    constexpr bool IsValid() const { return m_Value != 0; }
    constexpr std::uint64_t Value() const { return m_Value; }
    friend constexpr bool operator==(C_CampaignValue a, C_CampaignValue b) { return a.m_Value == b.m_Value; }
    friend constexpr bool operator!=(C_CampaignValue a, C_CampaignValue b) { return !(a == b); }
    friend constexpr bool operator<(C_CampaignValue a, C_CampaignValue b) { return a.m_Value < b.m_Value; }
private:
    std::uint64_t m_Value = 0;
};

using SessionId = C_CampaignValue<struct S_CampaignSessionIdTag>; // hosting lifetime; fresh after process restart
using CampaignId = C_CampaignValue<struct S_CampaignIdTag>; // campaign instance; retained by future saves
using MissionGeneration = C_CampaignValue<struct S_CampaignMissionGenerationTag>; // per session reconstruction
using PreparationGeneration = C_CampaignValue<struct S_CampaignPreparationGenerationTag>; // per session equipment context
using SoldierId = C_CampaignValue<struct S_CampaignSoldierIdTag>; // validated campaign roster, never a slot
using PlayerId = C_CampaignValue<struct S_CampaignPlayerIdTag>; // session connection incarnation, including host
using RequestId = C_CampaignValue<struct S_CampaignRequestIdTag>; // increasing per session/player, across families
using OwnershipRevision = C_CampaignValue<struct S_CampaignOwnershipRevisionTag>; // session catalog commit
using ControlRevision = C_CampaignValue<struct S_CampaignControlRevisionTag>; // per session/soldier, never actor-local
using PreparationRevision = C_CampaignValue<struct S_CampaignPreparationRevisionTag>; // commit within preparation generation
using ViewRevision = C_CampaignValue<struct S_CampaignViewRevisionTag>; // publication within preparation generation
using SnapshotId = C_CampaignValue<struct S_CampaignSnapshotIdTag>; // session transfer/manifest identity
using LocalSubmissionId = C_CampaignValue<struct S_CampaignSubmissionIdTag>; // local adapter lifetime only, never wire
using PreparationItemId = C_CampaignValue<struct S_CampaignPreparationItemIdTag>; // preparation catalog entry, not live UID
using StockEntryId = C_CampaignValue<struct S_CampaignStockEntryIdTag>; // preparation stock variant/condition entry

// Opaque validated catalog keys. Empty is invalid; strings own their storage.
// MissionKey names the original mission AND step/content version; no map-index
// interpretation. Catalog keys below are scoped by the content/rules descriptor.
template<class TTag> class C_CampaignKey {
public:
    C_CampaignKey() = default;
    explicit C_CampaignKey(std::string value) : m_Value(std::move(value)) {}
    bool IsValid() const { return !m_Value.empty(); }
    const std::string &Value() const { return m_Value; }
    friend bool operator==(const C_CampaignKey &a, const C_CampaignKey &b) { return a.m_Value == b.m_Value; }
    friend bool operator!=(const C_CampaignKey &a, const C_CampaignKey &b) { return !(a == b); }
    friend bool operator<(const C_CampaignKey &a, const C_CampaignKey &b) { return a.m_Value < b.m_Value; }
private:
    std::string m_Value;
};
using MissionKey = C_CampaignKey<struct S_CampaignMissionKeyTag>;
using ItemCatalogKey = C_CampaignKey<struct S_CampaignItemKeyTag>;
using PlacementKey = C_CampaignKey<struct S_CampaignPlacementKeyTag>;
using PresetKey = C_CampaignKey<struct S_CampaignPresetKeyTag>;
using SoldierProfileKey = C_CampaignKey<struct S_CampaignProfileKeyTag>;
using TextKey = C_CampaignKey<struct S_CampaignTextKeyTag>;
using AssetKey = C_CampaignKey<struct S_CampaignAssetKeyTag>;
using SkillKey = C_CampaignKey<struct S_CampaignSkillKeyTag>;
using ConstraintKey = C_CampaignKey<struct S_CampaignConstraintKeyTag>;
using AutoEquipPolicyKey = C_CampaignKey<struct S_CampaignAutoEquipPolicyKeyTag>;

// Legacy actor coordinates: zero IS valid. Presence is an optional ActorRef,
// not a sentinel. Distinct tags prevent using an actor coordinate as an ID.
template<class TTag> class C_CampaignActorCoordinate {
public:
    explicit constexpr C_CampaignActorCoordinate(std::uint32_t value = 0) : m_Value(value) {}
    constexpr std::uint32_t Value() const { return m_Value; }
    friend constexpr bool operator==(C_CampaignActorCoordinate a, C_CampaignActorCoordinate b) { return a.m_Value == b.m_Value; }
    friend constexpr bool operator!=(C_CampaignActorCoordinate a, C_CampaignActorCoordinate b) { return !(a == b); }
private:
    std::uint32_t m_Value;
};
using ActorId = C_CampaignActorCoordinate<struct S_CampaignActorIdTag>;
using ActorSlotUse = C_CampaignActorCoordinate<struct S_CampaignActorSlotUseTag>;
struct ActorRef {
    SessionId m_Session;
    MissionGeneration m_Generation;
    ActorId m_Actor;
    ActorSlotUse m_SlotUse; // bounded legacy discriminator; never freshness by itself
    friend bool operator==(const ActorRef &a, const ActorRef &b) {
        return a.m_Session == b.m_Session && a.m_Generation == b.m_Generation &&
            a.m_Actor == b.m_Actor && a.m_SlotUse == b.m_SlotUse;
    }
    friend bool operator!=(const ActorRef &a, const ActorRef &b) { return !(a == b); }
};

enum class E_CampaignSessionKind { Solo, LegacyMultiplayer, CampaignCoop };
enum class E_CampaignPhase { NoSession, Setup, Preparation, Frozen, Loading, Running, Ending, Closed };
enum class E_CampaignConnectionStatus { Disconnected, Connecting, Connected, Admitted, Rejected };
enum class E_CampaignSyncStatus { Loading, Current, Resyncing, Disconnected };
enum class E_CampaignReason {
    None, WrongSession, WrongGeneration, StaleRevision, NotAdmitted, NotOwner,
    AlreadyOwned, AlreadyControlsSoldier, InvalidSoldier, Ineligible, Dead,
    NotDeployed, ActorUnavailable, Scripted, InvalidPhase, PolicyRequired,
    Frozen, StockUnavailable, InvalidItem, InvalidPlacement, InvalidCount,
    CapacityExceeded, MissionRestriction, RequiredItemsMissing, RequestConflict,
    RequestExpired, Malformed, Disconnected, NotHost, PendingTransaction,
    SnapshotUnavailable, SnapshotGap, ContentMismatch, RulesMismatch,
    ModRequired, UnsupportedVersion, MissingFeature, Timeout, SessionFull,
    JoinNotSupported
};
struct S_CampaignContentDescriptor {
    // Catalog-issued canonical identifiers/fingerprints; COOP-006 defines their
    // bounded encoding/hash scheme. Cover campaign, missions, soldiers and items.
    std::string m_CampaignContent;
    std::string m_MissionContent;
    std::string m_SoldierContent;
    std::string m_ItemContent;
    std::string m_Rules;
};
struct S_CampaignRulesDescriptor {
    S_CampaignContentDescriptor m_Content;
    std::uint32_t m_Difficulty = 0; // authoritative campaign catalog value, not MP settings
};
struct S_CampaignSessionContext {
    E_CampaignSessionKind m_Kind = E_CampaignSessionKind::Solo;
    SessionId m_Session;
    CampaignId m_Campaign;
    MissionKey m_Mission;
    MissionGeneration m_MissionGeneration;
    E_CampaignPhase m_Phase = E_CampaignPhase::NoSession;
    bool m_UsesCampaignRules = false;
    bool m_IsTransportServer = false;
    bool m_IsTransportClient = false; // listen host may set both flags
    std::optional<PlayerId> m_LocalPlayer;
    std::optional<PlayerId> m_HostPlayer;
    S_CampaignRulesDescriptor m_Rules;
};

struct S_CampaignCapability {
    bool m_Allowed = false;
    E_CampaignReason m_Reason = E_CampaignReason::PolicyRequired;
};
enum class E_CampaignPolicy {
    SoldierSelection, SwitchingAndSpectating, DeploymentQuorum, DepartureAndCancel,
    ScarceStock, PostDeploymentEquipment, Persistence, Pause, HostLoss
};
enum class E_CampaignPolicyState { Unresolved, Permitted, Denied };
struct S_CampaignPolicyDecision {
    E_CampaignPolicy m_Policy = E_CampaignPolicy::SoldierSelection;
    E_CampaignPolicyState m_State = E_CampaignPolicyState::Unresolved;
    TextKey m_Explanation;
};
struct S_CampaignPolicySnapshot {
    std::vector<S_CampaignPolicyDecision> m_Decisions; // missing decision is unresolved
};

struct S_CampaignOwnershipRecord {
    SoldierId m_Soldier;
    bool m_Eligible = false;
    bool m_Deployed = false;
    bool m_Alive = false;
    std::optional<PlayerId> m_Owner; // absent: AI if living/eligible/deployed, never player zero
    std::optional<ActorRef> m_Actor; // absent is valid during preparation/reconstruction
    bool m_Scripted = false; // assigned owner retained; effective input suspended
    ControlRevision m_ControlRevision;
};
struct S_CampaignParticipant {
    PlayerId m_Player;
    std::string m_DisplayName; // presentation only; not identity or authorization
    bool m_Connected = false;
    bool m_Admitted = false;
    std::optional<PreparationRevision> m_ReadyAt;
};
struct S_CampaignOwnershipSnapshot {
    S_CampaignSessionContext m_Context;
    OwnershipRevision m_Revision;
    std::vector<S_CampaignOwnershipRecord> m_Soldiers;
    std::vector<S_CampaignParticipant> m_Participants;
};
struct S_CampaignControlToken {
    SessionId m_Session;
    MissionGeneration m_Generation;
    PlayerId m_Player;
    SoldierId m_Soldier;
    ActorRef m_Actor;
    ControlRevision m_ControlRevision;
};
struct S_CampaignRequestContext {
    SessionId m_Session;
    PlayerId m_PlayerEcho; // dispatcher MUST derive/compare authenticated sender; not authority
    RequestId m_Request;
};
struct S_CampaignOwnershipTarget {
    SoldierId m_Soldier;
    ControlRevision m_ExpectedControlRevision;
};
struct S_CampaignClaim { S_CampaignOwnershipTarget m_Target; };
struct S_CampaignRelease { S_CampaignOwnershipTarget m_Target; };
struct S_CampaignSwitch {
    S_CampaignOwnershipTarget m_From;
    S_CampaignOwnershipTarget m_To;
};
using T_CampaignOwnershipOperation = std::variant<S_CampaignClaim, S_CampaignSwitch, S_CampaignRelease>;
struct S_CampaignOwnershipRequest {
    S_CampaignRequestContext m_Context;
    // Phase-specific generation fences: preparation requests use preparation;
    // running requests use mission; setup can omit both. Authority validates phase.
    std::optional<PreparationGeneration> m_PreparationGeneration;
    std::optional<MissionGeneration> m_MissionGeneration;
    OwnershipRevision m_ExpectedOwnershipRevision;
    T_CampaignOwnershipOperation m_Operation;
};

// Catalog/placement references are values scoped to the preparation generation
// and expected revision. Counts are bounded/validated by model and codec, never
// trusted merely because they fit this host integer representation.
struct S_CampaignPlacement {
    PlacementKey m_Slot;
    std::uint32_t m_Position = 0; // position within catalog-defined slot/container
};
struct S_CampaignItemState {
    ItemCatalogKey m_Item;
    std::uint32_t m_Quantity = 0;
    std::optional<ItemCatalogKey> m_AmmoType;
    std::optional<std::uint32_t> m_Rounds;
    std::optional<float> m_Charges;
    std::optional<float> m_Condition; // catalog defines units/range, absent if not applicable
};
struct S_CampaignLoadoutItem {
    PreparationItemId m_Entry;
    S_CampaignItemState m_State;
    S_CampaignPlacement m_Placement;
};
struct S_CampaignSkill {
    SkillKey m_Key;
    float m_Value = 0; // preserve fractional campaign gains; catalog units, finite/range validation
};
enum class E_CampaignFindingSeverity { Information, EditRejection, DeploymentBlocker };
struct S_CampaignFinding {
    E_CampaignReason m_Reason = E_CampaignReason::None;
    E_CampaignFindingSeverity m_Severity = E_CampaignFindingSeverity::Information;
    TextKey m_Message; // localizable catalog reference, never arbitrary network UI text
    std::optional<SoldierId> m_Soldier;
    std::optional<ItemCatalogKey> m_Item;
    std::optional<PreparationItemId> m_Entry;
    std::optional<PlacementKey> m_Slot;
    std::optional<E_CampaignPolicy> m_UnresolvedPolicy;
    std::int64_t m_Required = 0;
    std::int64_t m_Actual = 0;
};
struct S_CampaignConstraintSummary {
    ConstraintKey m_Key;
    TextKey m_Description;
    std::vector<S_CampaignFinding> m_Findings;
};
struct S_CampaignSoldierLoadout {
    SoldierId m_Soldier;
    std::vector<S_CampaignLoadoutItem> m_Items;
    float m_Weight = 0;
    float m_WeightCapacity = 0;
    float m_BeltVolume = 0;
    float m_BeltCapacity = 0;
    float m_BackpackVolume = 0;
    float m_BackpackCapacity = 0;
    float m_BackpackWeight = 0;
    float m_BackpackWeightCapacity = 0;
    float m_BeltWeight = 0;
    std::vector<S_CampaignSkill> m_Skills;
    std::vector<S_CampaignFinding> m_Findings;
};
struct S_CampaignAllowedDestination {
    SoldierId m_Soldier;
    std::vector<PlacementKey> m_Placements;
    S_CampaignCapability m_CanTake;
};
struct S_CampaignStockEntry {
    StockEntryId m_Entry;
    S_CampaignItemState m_State; // quantity is committed available stock of this variant
    std::vector<S_CampaignAllowedDestination> m_Destinations;
    std::vector<S_CampaignConstraintSummary> m_Constraints;
};
struct S_CampaignEssential {
    ItemCatalogKey m_Item;
    std::uint32_t m_Required = 0;
    std::uint32_t m_Carried = 0;
    std::uint32_t m_Deficit = 0;
    std::vector<SoldierId> m_EligibleCarriers;
    std::vector<S_CampaignFinding> m_Findings;
};
struct S_CampaignSoldierPresentation {
    SoldierId m_Soldier;
    SoldierProfileKey m_Profile;
    std::string m_DisplayName;
    AssetKey m_Portrait;
    AssetKey m_Preview;
    S_CampaignOwnershipRecord m_Ownership;
    S_CampaignCapability m_CanView;
    S_CampaignCapability m_CanEdit;
    S_CampaignCapability m_CanClaim;
    S_CampaignCapability m_CanRelease;
    std::vector<S_CampaignFinding> m_Restrictions;
};
struct S_CampaignPresentationSlot {
    std::optional<SoldierId> m_Soldier; // absence never manufactures a roster soldier
    E_CampaignReason m_UnavailableReason = E_CampaignReason::Ineligible;
    std::vector<S_CampaignFinding> m_Restrictions;
};
struct S_CampaignBriefing {
    TextKey m_OperationTitle;
    TextKey m_MissionTitle;
    TextKey m_Body;
    AssetKey m_Image;
};
struct S_CampaignPreparationSnapshot {
    S_CampaignSessionContext m_Context;
    PreparationGeneration m_Generation;
    PreparationRevision m_PreparationRevision;
    ViewRevision m_ViewRevision;
    OwnershipRevision m_OwnershipRevision;
    E_CampaignConnectionStatus m_Connection = E_CampaignConnectionStatus::Disconnected;
    E_CampaignSyncStatus m_Sync = E_CampaignSyncStatus::Loading;
    S_CampaignBriefing m_Briefing;
    std::array<S_CampaignPresentationSlot, 4> m_Slots;
    std::vector<S_CampaignSoldierPresentation> m_Soldiers;
    std::vector<S_CampaignSoldierLoadout> m_Loadouts;
    std::vector<S_CampaignStockEntry> m_Stock;
    std::vector<S_CampaignEssential> m_Essentials;
    std::vector<S_CampaignParticipant> m_Participants;
    std::vector<S_CampaignFinding> m_Findings;
    S_CampaignCapability m_CanDeploy;
    std::vector<S_CampaignFinding> m_DeployBlockers;
    S_CampaignPolicySnapshot m_Policies;
};
// Publish detached values using make_shared<const T>(value/move(candidate));
// never retain a writable alias to published storage. Retained handles survive
// replacement/screen close. Nested fields own their data; no profile/engine refs.
using T_CampaignPreparationSnapshot = std::shared_ptr<const S_CampaignPreparationSnapshot>;
using T_CampaignOwnershipSnapshot = std::shared_ptr<const S_CampaignOwnershipSnapshot>;
struct S_CampaignPreparationView {
    E_CampaignSyncStatus m_Status = E_CampaignSyncStatus::Loading;
    E_CampaignReason m_Reason = E_CampaignReason::SnapshotUnavailable;
    T_CampaignPreparationSnapshot m_Snapshot; // null when no complete usable state
};

struct S_CampaignTakeFromStock {
    SoldierId m_Soldier;
    StockEntryId m_StockEntry;
    S_CampaignPlacement m_Destination;
    std::uint32_t m_Count = 0;
};
struct S_CampaignReturnToStock {
    SoldierId m_Soldier;
    PreparationItemId m_Entry;
    std::uint32_t m_Count = 0;
};
struct S_CampaignMoveWithinLoadout {
    SoldierId m_Soldier;
    PreparationItemId m_Entry;
    S_CampaignPlacement m_Destination;
    std::uint32_t m_Count = 0;
};
struct S_CampaignApplyPreset { PresetKey m_Preset; };
struct S_CampaignAutoEquip {
    AutoEquipPolicyKey m_Policy; // explicit injected campaign algorithm/options catalog key
    std::vector<ItemCatalogKey> m_PreferredItems;
};
using T_CampaignEditOperation = std::variant<S_CampaignTakeFromStock, S_CampaignReturnToStock,
    S_CampaignMoveWithinLoadout, S_CampaignApplyPreset, S_CampaignAutoEquip>;
struct S_CampaignPreparationRequestContext {
    S_CampaignRequestContext m_Request;
    PreparationGeneration m_Generation;
    PreparationRevision m_ExpectedPreparationRevision;
};
struct S_CampaignEditRequest {
    S_CampaignPreparationRequestContext m_Context;
    std::vector<S_CampaignOwnershipTarget> m_Targets; // explicit, nonempty, unique; all-or-nothing
    T_CampaignEditOperation m_Operation; // single-soldier operations must match their sole target
};
struct S_CampaignReadyRequest {
    S_CampaignPreparationRequestContext m_Context;
    bool m_Ready = false; // changes view revision only
};
struct S_CampaignDeployRequest { S_CampaignPreparationRequestContext m_Context; };
enum class E_CampaignRequestFamily { Ownership, Edit, Ready, Deploy };
enum class E_CampaignRequestOutcome { Accepted, Rejected, Duplicate, ResyncRequired };
struct S_CampaignResyncDescriptor {
    SessionId m_Session;
    std::optional<PreparationGeneration> m_PreparationGeneration;
    std::optional<MissionGeneration> m_MissionGeneration;
    OwnershipRevision m_OwnershipRevision;
    PreparationRevision m_PreparationRevision;
    ViewRevision m_ViewRevision;
    std::optional<SnapshotId> m_Snapshot; // transfer to request/install; not an ack
    E_CampaignReason m_Reason = E_CampaignReason::SnapshotUnavailable;
};
// Complete authoritative publications or explicit resync, never revision-only
// success/rejection that could accidentally enable an editor on stale state.
using T_CampaignAuthoritativeState = std::variant<S_CampaignResyncDescriptor,
    T_CampaignPreparationSnapshot, T_CampaignOwnershipSnapshot>;
struct S_CampaignRequestResult {
    S_CampaignRequestContext m_Context;
    E_CampaignRequestFamily m_Family = E_CampaignRequestFamily::Edit;
    E_CampaignRequestOutcome m_Outcome = E_CampaignRequestOutcome::Rejected;
    E_CampaignReason m_Reason = E_CampaignReason::Malformed;
    std::optional<PreparationGeneration> m_PreparationGeneration;
    std::optional<MissionGeneration> m_MissionGeneration;
    OwnershipRevision m_OwnershipRevision;
    PreparationRevision m_PreparationRevision;
    ViewRevision m_ViewRevision;
    // Duplicate replays original acceptance/rejection and records, no second commit.
    std::optional<E_CampaignRequestOutcome> m_OriginalOutcome;
    T_CampaignAuthoritativeState m_State;
    std::vector<S_CampaignFinding> m_Findings;
};
// Submission is local queuing, not acceptance. UI correlates token AND full
// session/player/request/family/generation; unrelated/obsolete results cannot
// clear a newer pending proposal. A screen closing never cancels authority work.
struct S_CampaignQueuedSubmission { LocalSubmissionId m_Token; };
struct S_CampaignLocalFailure { E_CampaignReason m_Reason = E_CampaignReason::Malformed; };
using T_CampaignSubmission = std::variant<S_CampaignQueuedSubmission, S_CampaignLocalFailure>;
struct S_CampaignSnapshotChanged { T_CampaignPreparationSnapshot m_Snapshot; };
struct S_CampaignRequestResultEvent {
    std::optional<LocalSubmissionId> m_LocalToken; // absent for another participant's result
    S_CampaignRequestResult m_Result;
};
struct S_CampaignStatusChanged {
    SessionId m_Session; // invalid only before a session is known
    std::optional<PreparationGeneration> m_PreparationGeneration;
    std::optional<MissionGeneration> m_MissionGeneration;
    ViewRevision m_ViewRevision; // invalid when no preparation view exists
    E_CampaignConnectionStatus m_Connection = E_CampaignConnectionStatus::Disconnected;
    E_CampaignSyncStatus m_Sync = E_CampaignSyncStatus::Disconnected;
    E_CampaignPhase m_Phase = E_CampaignPhase::NoSession;
    E_CampaignReason m_Reason = E_CampaignReason::None;
};
using T_CampaignPreparationEvent = std::variant<S_CampaignSnapshotChanged,
    S_CampaignRequestResultEvent, S_CampaignResyncDescriptor, S_CampaignStatusChanged>;

class I_CampaignPreparation {
public:
    virtual ~I_CampaignPreparation() = default;
    virtual S_CampaignPreparationView GetSnapshot() const = 0;
    virtual T_CampaignSubmission SubmitEdit(const S_CampaignEditRequest &request) = 0;
    virtual T_CampaignSubmission SubmitOwnership(const S_CampaignOwnershipRequest &request) = 0;
    virtual T_CampaignSubmission SetReady(const S_CampaignReadyRequest &request) = 0;
    virtual T_CampaignSubmission RequestDeploy(const S_CampaignDeployRequest &request) = 0;
    virtual void RequestResync() = 0;
    // Status events are invalidation hints: re-read GetSnapshot for current
    // status; never let an old Loading/Closed event override a newer installed
    // session/generation/view. Delivery belongs to the current adapter attachment.
    // UI-thread polling only; no registered callbacks into destroyed screens.
    // Screen close stops polling/releases its adapter attachment. Authority may
    // still commit; reopen obtains GetSnapshot instead of undoing local stock.
    virtual std::optional<T_CampaignPreparationEvent> PollEvent() = 0;
};

// Dependency injection boundaries for isolated ownership/preparation consumers.
// These interfaces expose immutable values, never a mutable implementation model.
class I_CampaignOwnershipView {
public:
    virtual ~I_CampaignOwnershipView() = default;
    virtual T_CampaignOwnershipSnapshot GetSnapshot() const = 0;
};
class I_CampaignPolicyView {
public:
    virtual ~I_CampaignPolicyView() = default;
    virtual S_CampaignPolicySnapshot GetSnapshot() const = 0;
};
struct S_CampaignItemCatalogEntry {
    ItemCatalogKey m_Key;
    TextKey m_Name;
    TextKey m_Description;
    AssetKey m_Icon;
    AssetKey m_Preview;
    std::vector<PlacementKey> m_Placements;
    std::vector<S_CampaignConstraintSummary> m_Constraints;
};
struct S_CampaignEquipmentCatalog {
    S_CampaignContentDescriptor m_Content;
    std::vector<S_CampaignItemCatalogEntry> m_Items;
    std::vector<PlacementKey> m_Placements;
    std::vector<PresetKey> m_Presets;
    std::vector<AutoEquipPolicyKey> m_AutoEquipPolicies;
};
class I_CampaignEquipmentCatalogView {
public:
    virtual ~I_CampaignEquipmentCatalogView() = default;
    virtual std::shared_ptr<const S_CampaignEquipmentCatalog> GetSnapshot() const = 0;
};

// Lifecycle messages remain logical values. COOP-006 owns wire negotiation,
// framing, limits and the bounded input/action payload; no opcode reserved here.
struct S_CampaignActorBinding { SoldierId m_Soldier; ActorRef m_Actor; ControlRevision m_ControlRevision; };
struct S_CampaignMissionManifest {
    SnapshotId m_Snapshot;
    S_CampaignSessionContext m_Context;
    PreparationGeneration m_PreparationGeneration;
    PreparationRevision m_FrozenPreparationRevision;
    ViewRevision m_ViewRevision;
    OwnershipRevision m_OwnershipRevision;
    std::vector<S_CampaignSoldierPresentation> m_Soldiers;
    std::vector<S_CampaignSoldierLoadout> m_Loadouts;
    std::vector<S_CampaignActorBinding> m_Bindings;
    std::vector<PlayerId> m_RequiredParticipants; // departure is never an acknowledgment
};
struct S_CampaignSnapshotAck {
    SessionId m_Session;
    PlayerId m_PlayerEcho;
    MissionGeneration m_Generation;
    SnapshotId m_Snapshot;
    PreparationGeneration m_PreparationGeneration;
    PreparationRevision m_PreparationRevision;
    ViewRevision m_ViewRevision;
    OwnershipRevision m_OwnershipRevision;
};
struct S_CampaignStartSimulation {
    SessionId m_Session;
    MissionGeneration m_Generation;
    SnapshotId m_Snapshot;
};

#endif
