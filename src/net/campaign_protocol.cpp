#include "campaign_protocol.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace CampaignProtocol {
namespace {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559, "wire requires IEEE binary32");
constexpr std::uint32_t kMagic = 0x50433248; // bytes H2CP
constexpr std::size_t kAllocationBudget = 8 * 1024 * 1024;
constexpr std::size_t kElementBudget = 8192;
template<class T> bool Valid(const T &) { return true; }
bool Valid(const ActorRef &x);
bool Valid(const S_CampaignContentDescriptor &x);
bool Valid(const S_CampaignRulesDescriptor &x);
bool Valid(const S_CampaignSessionContext &x);
bool Valid(const S_CampaignCapability &x);
bool Valid(const S_CampaignPolicyDecision &x);
bool Valid(const S_CampaignPolicySnapshot &x);
bool Valid(const S_CampaignOwnershipRecord &x);
bool Valid(const S_CampaignParticipant &x);
bool Valid(const S_CampaignOwnershipSnapshot &x);
bool Valid(const S_CampaignControlToken &x);
bool Valid(const S_CampaignRequestContext &x);
bool Valid(const S_CampaignOwnershipTarget &x);
bool Valid(const S_CampaignClaim &x);
bool Valid(const S_CampaignRelease &x);
bool Valid(const S_CampaignSwitch &x);
bool Valid(const S_CampaignOwnershipRequest &x);
bool Valid(const S_CampaignPlacement &x);
bool Valid(const S_CampaignItemState &x);
bool Valid(const S_CampaignLoadoutItem &x);
bool Valid(const S_CampaignSkill &x);
bool Valid(const S_CampaignFinding &x);
bool Valid(const S_CampaignConstraintSummary &x);
bool Valid(const S_CampaignSoldierLoadout &x);
bool Valid(const S_CampaignAllowedDestination &x);
bool Valid(const S_CampaignStockEntry &x);
bool Valid(const S_CampaignEssential &x);
bool Valid(const S_CampaignSoldierPresentation &x);
bool Valid(const S_CampaignPresentationSlot &x);
bool Valid(const S_CampaignBriefing &x);
bool Valid(const S_CampaignPreparationSnapshot &x);
bool Valid(const S_CampaignTakeFromStock &x);
bool Valid(const S_CampaignReturnToStock &x);
bool Valid(const S_CampaignMoveWithinLoadout &x);
bool Valid(const S_CampaignApplyPreset &x);
bool Valid(const S_CampaignAutoEquip &x);
bool Valid(const S_CampaignPreparationRequestContext &x);
bool Valid(const S_CampaignEditRequest &x);
bool Valid(const S_CampaignReadyRequest &x);
bool Valid(const S_CampaignDeployRequest &x);
bool Valid(const S_CampaignResyncDescriptor &x);
bool Valid(const S_CampaignRequestResult &x);
bool Valid(const S_CampaignActorBinding &x);
bool Valid(const S_CampaignMissionManifest &x);
bool Valid(const S_CampaignSnapshotAck &x);
bool Valid(const S_CampaignStartSimulation &x);
bool Valid(const S_Hello &x);
bool Valid(const S_Accept &x);
bool Valid(const S_NegotiationAck &x);
bool Valid(const S_Reject &x);
bool Valid(const S_PreparationDelta &x);
bool Valid(const S_Input &x);
bool Valid(const S_WorldChunk &x);
bool Valid(const S_MissionUpdate &x);
bool Valid(const S_WeaponEvent &x);
bool Valid(const S_ItemMove &x);
bool Valid(const S_InventoryEvent &x);
template<class T> struct S_EnumMax;
template<> struct S_EnumMax<E_CampaignSessionKind> { static constexpr auto value = E_CampaignSessionKind::CampaignCoop; };
template<> struct S_EnumMax<E_CampaignPhase> { static constexpr auto value = E_CampaignPhase::Closed; };
template<> struct S_EnumMax<E_CampaignConnectionStatus> { static constexpr auto value = E_CampaignConnectionStatus::Rejected; };
template<> struct S_EnumMax<E_CampaignSyncStatus> { static constexpr auto value = E_CampaignSyncStatus::Disconnected; };
template<> struct S_EnumMax<E_CampaignReason> { static constexpr auto value = E_CampaignReason::JoinNotSupported; };
template<> struct S_EnumMax<E_CampaignPolicy> { static constexpr auto value = E_CampaignPolicy::HostLoss; };
template<> struct S_EnumMax<E_CampaignPolicyState> { static constexpr auto value = E_CampaignPolicyState::Denied; };
template<> struct S_EnumMax<E_CampaignFindingSeverity> { static constexpr auto value = E_CampaignFindingSeverity::DeploymentBlocker; };
template<> struct S_EnumMax<E_CampaignRequestFamily> { static constexpr auto value = E_CampaignRequestFamily::Deploy; };
template<> struct S_EnumMax<E_CampaignRequestOutcome> { static constexpr auto value = E_CampaignRequestOutcome::ResyncRequired; };
template<> struct S_EnumMax<E_InputKind> { static constexpr auto value = E_InputKind::WireCut; };
template<> struct S_EnumMax<E_MissionPresentation> { static constexpr auto value = E_MissionPresentation::WireCut; };
template<> struct S_EnumMax<E_MissionOutcome> { static constexpr auto value = E_MissionOutcome::Failure; };
template<> struct S_EnumMax<E_DoorAction> { static constexpr auto value = E_DoorAction::PickLock; };
template<> struct S_EnumMax<E_WeaponEvent> { static constexpr auto value = E_WeaponEvent::Reload; };
template<class A> bool Fields(A &a, ActorRef &x);
template<class A> bool Fields(A &a, S_CampaignContentDescriptor &x);
template<class A> bool Fields(A &a, S_CampaignRulesDescriptor &x);
template<class A> bool Fields(A &a, S_CampaignSessionContext &x);
template<class A> bool Fields(A &a, S_CampaignCapability &x);
template<class A> bool Fields(A &a, S_CampaignPolicyDecision &x);
template<class A> bool Fields(A &a, S_CampaignPolicySnapshot &x);
template<class A> bool Fields(A &a, S_CampaignOwnershipRecord &x);
template<class A> bool Fields(A &a, S_CampaignParticipant &x);
template<class A> bool Fields(A &a, S_CampaignOwnershipSnapshot &x);
template<class A> bool Fields(A &a, S_CampaignControlToken &x);
template<class A> bool Fields(A &a, S_CampaignRequestContext &x);
template<class A> bool Fields(A &a, S_CampaignOwnershipTarget &x);
template<class A> bool Fields(A &a, S_CampaignClaim &x);
template<class A> bool Fields(A &a, S_CampaignRelease &x);
template<class A> bool Fields(A &a, S_CampaignSwitch &x);
template<class A> bool Fields(A &a, S_CampaignOwnershipRequest &x);
template<class A> bool Fields(A &a, S_CampaignPlacement &x);
template<class A> bool Fields(A &a, S_CampaignItemState &x);
template<class A> bool Fields(A &a, S_CampaignLoadoutItem &x);
template<class A> bool Fields(A &a, S_CampaignSkill &x);
template<class A> bool Fields(A &a, S_CampaignFinding &x);
template<class A> bool Fields(A &a, S_CampaignConstraintSummary &x);
template<class A> bool Fields(A &a, S_CampaignSoldierLoadout &x);
template<class A> bool Fields(A &a, S_CampaignAllowedDestination &x);
template<class A> bool Fields(A &a, S_CampaignStockEntry &x);
template<class A> bool Fields(A &a, S_CampaignEssential &x);
template<class A> bool Fields(A &a, S_CampaignSoldierPresentation &x);
template<class A> bool Fields(A &a, S_CampaignPresentationSlot &x);
template<class A> bool Fields(A &a, S_CampaignBriefing &x);
template<class A> bool Fields(A &a, S_CampaignPreparationSnapshot &x);
template<class A> bool Fields(A &a, S_CampaignTakeFromStock &x);
template<class A> bool Fields(A &a, S_CampaignReturnToStock &x);
template<class A> bool Fields(A &a, S_CampaignMoveWithinLoadout &x);
template<class A> bool Fields(A &a, S_CampaignApplyPreset &x);
template<class A> bool Fields(A &a, S_CampaignAutoEquip &x);
template<class A> bool Fields(A &a, S_CampaignPreparationRequestContext &x);
template<class A> bool Fields(A &a, S_CampaignEditRequest &x);
template<class A> bool Fields(A &a, S_CampaignReadyRequest &x);
template<class A> bool Fields(A &a, S_CampaignDeployRequest &x);
template<class A> bool Fields(A &a, S_CampaignResyncDescriptor &x);
template<class A> bool Fields(A &a, S_CampaignRequestResult &x);
template<class A> bool Fields(A &a, S_CampaignActorBinding &x);
template<class A> bool Fields(A &a, S_CampaignMissionManifest &x);
template<class A> bool Fields(A &a, S_CampaignSnapshotAck &x);
template<class A> bool Fields(A &a, S_CampaignStartSimulation &x);
template<class A> bool Fields(A &a, S_Hello &x);
template<class A> bool Fields(A &a, S_Accept &x);
template<class A> bool Fields(A &a, S_NegotiationAck &x);
template<class A> bool Fields(A &a, S_Reject &x);
template<class A> bool Fields(A &a, S_PreparationDelta &x);
template<class A> bool Fields(A &a, S_Input &x);
template<class A> bool Fields(A &a, S_WorldChunk &x);
template<class A> bool Fields(A &a, S_SquadActorState &x);
template<class A> bool Fields(A &a, S_MissionCamera &x);
template<class A> bool Fields(A &a, S_MissionUpdate &x);
template<class A> bool Fields(A &a, S_AmbientHumanState &x);
template<class A> bool Fields(A &a, S_MissionPresentation &x);
template<class A> bool Fields(A &a, S_PresentationFramePose &x);
template<class A> bool Fields(A &a, S_WeaponEvent &x);
template<class A> bool Fields(A &a, S_ItemMove &x);
template<class A> bool Fields(A &a, S_InventoryEvent &x);

class C_Archive {
public:
    explicit C_Archive(T_Bytes &out) : m_Out(&out) {}
    C_Archive(const std::uint8_t *data, std::size_t size) : m_Data(data), m_Size(size) {}
    template<class... T> bool operator()(T &...x) { return (Value(x) && ...); }
    bool Done() const { return m_Out || m_Pos == m_Size; }
private:
    bool Bytes(std::uint8_t *p, std::size_t n) {
        if (m_Out) {
            if (n > kMaxFrameBytes - kHeaderBytes - m_Out->size()) return false;
            m_Out->insert(m_Out->end(), p, p + n);
        } else {
            if (n > m_Size - m_Pos) return false;
            if (n) std::memcpy(p, m_Data + m_Pos, n);
            m_Pos += n;
        }
        return true;
    }
    bool Budget(std::size_t n, std::size_t width) {
        if (n > m_Elements || n > m_Allocation / width) return false;
        m_Elements -= n;
        m_Allocation -= n * width;
        return true;
    }
    template<class U> bool Unsigned(U &x) {
        std::uint8_t bytes[sizeof(U)]{};
        if (m_Out) for (std::size_t i = 0; i < sizeof(U); ++i) bytes[i] = static_cast<std::uint8_t>(x >> (8 * i));
        if (!Bytes(bytes, sizeof(U))) return false;
        if (!m_Out) {
            x = 0;
            for (std::size_t i = 0; i < sizeof(U); ++i) x |= static_cast<U>(static_cast<U>(bytes[i]) << (8 * i));
        }
        return true;
    }
    bool Value(std::uint8_t &x) { return Unsigned(x); }
    bool Value(std::uint16_t &x) { return Unsigned(x); }
    bool Value(std::uint32_t &x) { return Unsigned(x); }
    bool Value(std::uint64_t &x) { return Unsigned(x); }
    bool Value(std::int32_t &x) {
        auto bits = static_cast<std::uint32_t>(x);
        if (!Unsigned(bits)) return false;
        if (!m_Out) x = bits <= INT32_MAX ? static_cast<std::int32_t>(bits) : -1 - static_cast<std::int32_t>(~bits);
        return true;
    }
    bool Value(std::int64_t &x) {
        auto bits = static_cast<std::uint64_t>(x);
        if (!Unsigned(bits)) return false;
        if (!m_Out) x = bits <= static_cast<std::uint64_t>(INT64_MAX) ? static_cast<std::int64_t>(bits) : -1 - static_cast<std::int64_t>(~bits);
        return true;
    }
    bool Value(float &x) {
        std::uint32_t bits = 0;
        if (m_Out) std::memcpy(&bits, &x, 4);
        if (!Unsigned(bits)) return false;
        if (!m_Out) std::memcpy(&x, &bits, 4);
        return std::isfinite(x);
    }
    bool Value(bool &x) {
        std::uint8_t b = x ? 1 : 0;
        if (!Value(b) || b > 1) return false;
        x = b != 0;
        return true;
    }
    bool Value(std::string &x) {
        if (m_Out && x.size() > kMaxStringBytes) return false;
        std::uint16_t size = static_cast<std::uint16_t>(x.size());
        if (!Value(size) || size > kMaxStringBytes || size > m_Allocation) return false;
        m_Allocation -= size;
        if (!m_Out) { if (size > m_Size - m_Pos) return false; x.resize(size); }
        if (!Bytes(reinterpret_cast<std::uint8_t *>(x.data()), size)) return false;
        // Canonical UTF-8, no NUL/control characters. Catalog validation is later.
        for (std::size_t i = 0; i < x.size();) {
            auto c = static_cast<std::uint8_t>(x[i++]);
            if (c < 0x20 || c == 0x7f) return false;
            if (c < 0x80) continue;
            unsigned count; std::uint32_t cp;
            if (c >= 0xc2 && c <= 0xdf) { count = 1; cp = c & 0x1f; }
            else if (c >= 0xe0 && c <= 0xef) { count = 2; cp = c & 0x0f; }
            else if (c >= 0xf0 && c <= 0xf4) { count = 3; cp = c & 7; }
            else return false;
            const unsigned n = count;
            while (count--) {
                if (i == x.size()) return false;
                c = static_cast<std::uint8_t>(x[i++]);
                if ((c & 0xc0) != 0x80) return false;
                cp = (cp << 6) | (c & 0x3f);
            }
            if ((n == 1 && cp < 0x80) || (n == 2 && cp < 0x800) ||
                (n == 3 && cp < 0x10000) || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) return false;
        }
        return true;
    }
    template<class T> bool Value(C_CampaignValue<T> &x) {
        auto value = x.Value(); if (!Value(value)) return false;
        x = C_CampaignValue<T>(value); return true;
    }
    template<class T> bool Value(C_CampaignKey<T> &x) {
        auto value = x.Value(); if (!Value(value)) return false;
        x = C_CampaignKey<T>(std::move(value)); return true;
    }
    template<class T> bool Value(C_CampaignActorCoordinate<T> &x) {
        auto value = x.Value(); if (!Value(value)) return false;
        x = C_CampaignActorCoordinate<T>(value); return true;
    }
    template<class T> bool Value(std::optional<T> &x) {
        bool present = x.has_value(); if (!Value(present)) return false;
        if (!present) { x.reset(); return true; }
        if (!x) x.emplace();
        return Value(*x);
    }
    template<class T, std::size_t N> bool Value(std::array<T, N> &x) {
        for (auto &v : x) if (!Value(v)) return false;
        return true;
    }
    template<class T> bool Value(std::vector<T> &x) {
        constexpr std::size_t limit = std::is_same<T, std::uint8_t>::value ? kMaxChunkBytes : kMaxCollection;
        if (m_Out && x.size() > limit) return false;
        auto size = static_cast<std::uint32_t>(x.size());
        if (!Value(size) || size > limit) return false;
        if constexpr (std::is_same<T, std::uint8_t>::value) {
            if (size > m_Allocation) return false;
            m_Allocation -= size;
        } else if (!Budget(size, sizeof(T))) return false;
        if (!m_Out) {
            // Every element consumes at least one byte in this schema.
            if (size > m_Size - m_Pos) return false;
            x.resize(size);
        }
        for (auto &v : x) if (!Value(v)) return false;
        return true;
    }
    template<std::size_t I = 0, class... T> bool Variant(std::uint8_t index, std::variant<T...> &x) {
        if constexpr (I == sizeof...(T)) return false;
        else {
            if (index == I) {
                if (x.index() != I) x.template emplace<I>();
                return Value(std::get<I>(x));
            }
            return Variant<I + 1>(index, x);
        }
    }
    template<class... T> bool Value(std::variant<T...> &x) {
        auto index = static_cast<std::uint8_t>(x.index());
        return Value(index) && Variant(index, x);
    }
    template<class T> bool Value(std::shared_ptr<const T> &x) {
        if (m_Out && !x) return false;
        if (++m_Depth > 16 || !Budget(1, sizeof(T))) return false;
        T candidate = m_Out ? *x : T{};
        bool ok = Value(candidate);
        --m_Depth;
        if (ok && !m_Out) x = std::make_shared<const T>(std::move(candidate));
        return ok;
    }
    template<class T> bool Value(T &x) {
        if constexpr (std::is_enum<T>::value) {
            const auto number = static_cast<std::int64_t>(x);
            if (m_Out && (number < 0 || number > static_cast<std::int64_t>(S_EnumMax<T>::value))) return false;
            auto value = static_cast<std::uint16_t>(x);
            if (!Value(value) || value > static_cast<std::uint16_t>(S_EnumMax<T>::value)) return false;
            x = static_cast<T>(value); return true;
        } else return Fields(*this, x);
    }
    T_Bytes *m_Out = nullptr;
    const std::uint8_t *m_Data = nullptr;
    std::size_t m_Size = 0, m_Pos = 0;
    std::size_t m_Elements = kElementBudget, m_Allocation = kAllocationBudget;
    unsigned m_Depth = 0;
};
template<class A> bool Fields(A &a, ActorRef &x) { return a(x.m_Session, x.m_Generation, x.m_Actor, x.m_SlotUse) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignContentDescriptor &x) { return a(x.m_CampaignContent, x.m_MissionContent, x.m_SoldierContent, x.m_ItemContent, x.m_Rules) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignRulesDescriptor &x) { return a(x.m_Content, x.m_Difficulty) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignSessionContext &x) { return a(x.m_Kind, x.m_Session, x.m_Campaign, x.m_Mission, x.m_MissionGeneration, x.m_Phase, x.m_UsesCampaignRules, x.m_IsTransportServer, x.m_IsTransportClient, x.m_LocalPlayer, x.m_HostPlayer, x.m_Rules) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignCapability &x) { return a(x.m_Allowed, x.m_Reason) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignPolicyDecision &x) { return a(x.m_Policy, x.m_State, x.m_Explanation) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignPolicySnapshot &x) { return a(x.m_Decisions) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignOwnershipRecord &x) { return a(x.m_Soldier, x.m_Eligible, x.m_Deployed, x.m_Alive, x.m_Owner, x.m_Actor, x.m_Scripted, x.m_ControlRevision) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignParticipant &x) { return a(x.m_Player, x.m_DisplayName, x.m_Connected, x.m_Admitted, x.m_ReadyAt) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignOwnershipSnapshot &x) { return a(x.m_Context, x.m_Revision, x.m_Soldiers, x.m_Participants) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignControlToken &x) { return a(x.m_Session, x.m_Generation, x.m_Player, x.m_Soldier, x.m_Actor, x.m_ControlRevision) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignRequestContext &x) { return a(x.m_Session, x.m_PlayerEcho, x.m_Request) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignOwnershipTarget &x) { return a(x.m_Soldier, x.m_ExpectedControlRevision) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignClaim &x) { return a(x.m_Target) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignRelease &x) { return a(x.m_Target) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignSwitch &x) { return a(x.m_From, x.m_To) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignOwnershipRequest &x) { return a(x.m_Context, x.m_PreparationGeneration, x.m_MissionGeneration, x.m_ExpectedOwnershipRevision, x.m_Operation) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignPlacement &x) { return a(x.m_Slot, x.m_Position) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignItemState &x) { return a(x.m_Item, x.m_Quantity, x.m_AmmoType, x.m_Rounds, x.m_Charges, x.m_Condition) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignLoadoutItem &x) { return a(x.m_Entry, x.m_State, x.m_Placement) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignSkill &x) { return a(x.m_Key, x.m_Value) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignFinding &x) { return a(x.m_Reason, x.m_Severity, x.m_Message, x.m_Soldier, x.m_Item, x.m_Entry, x.m_Slot, x.m_UnresolvedPolicy, x.m_Required, x.m_Actual) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignConstraintSummary &x) { return a(x.m_Key, x.m_Description, x.m_Findings) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignSoldierLoadout &x) { return a(x.m_Soldier, x.m_Items, x.m_Weight, x.m_WeightCapacity, x.m_BeltVolume, x.m_BeltCapacity, x.m_BackpackVolume, x.m_BackpackCapacity, x.m_BackpackWeight, x.m_BackpackWeightCapacity, x.m_BeltWeight, x.m_Skills, x.m_Findings) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignAllowedDestination &x) { return a(x.m_Soldier, x.m_Placements, x.m_CanTake) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignStockEntry &x) { return a(x.m_Entry, x.m_State, x.m_Destinations, x.m_Constraints) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignEssential &x) { return a(x.m_Item, x.m_Required, x.m_Carried, x.m_Deficit, x.m_EligibleCarriers, x.m_Findings) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignSoldierPresentation &x) { return a(x.m_Soldier, x.m_Profile, x.m_DisplayName, x.m_Portrait, x.m_Preview, x.m_Ownership, x.m_CanView, x.m_CanEdit, x.m_CanClaim, x.m_CanRelease, x.m_Restrictions) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignPresentationSlot &x) { return a(x.m_Soldier, x.m_UnavailableReason, x.m_Restrictions) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignBriefing &x) { return a(x.m_OperationTitle, x.m_MissionTitle, x.m_Body, x.m_Image) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignPreparationSnapshot &x) { return a(x.m_Context, x.m_Generation, x.m_PreparationRevision, x.m_ViewRevision, x.m_OwnershipRevision, x.m_Connection, x.m_Sync, x.m_Briefing, x.m_Slots, x.m_Soldiers, x.m_Loadouts, x.m_Stock, x.m_Essentials, x.m_Participants, x.m_Findings, x.m_CanDeploy, x.m_DeployBlockers, x.m_Policies) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignTakeFromStock &x) { return a(x.m_Soldier, x.m_StockEntry, x.m_Destination, x.m_Count) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignReturnToStock &x) { return a(x.m_Soldier, x.m_Entry, x.m_Count) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignMoveWithinLoadout &x) { return a(x.m_Soldier, x.m_Entry, x.m_Destination, x.m_Count) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignApplyPreset &x) { return a(x.m_Preset) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignAutoEquip &x) { return a(x.m_Policy, x.m_PreferredItems) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignPreparationRequestContext &x) { return a(x.m_Request, x.m_Generation, x.m_ExpectedPreparationRevision) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignEditRequest &x) { return a(x.m_Context, x.m_Targets, x.m_Operation) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignReadyRequest &x) { return a(x.m_Context, x.m_Ready) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignDeployRequest &x) { return a(x.m_Context) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignResyncDescriptor &x) { return a(x.m_Session, x.m_PreparationGeneration, x.m_MissionGeneration, x.m_OwnershipRevision, x.m_PreparationRevision, x.m_ViewRevision, x.m_Snapshot, x.m_Reason) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignRequestResult &x) { return a(x.m_Context, x.m_Family, x.m_Outcome, x.m_Reason, x.m_PreparationGeneration, x.m_MissionGeneration, x.m_OwnershipRevision, x.m_PreparationRevision, x.m_ViewRevision, x.m_OriginalOutcome, x.m_State, x.m_Findings) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignActorBinding &x) { return a(x.m_Soldier, x.m_Actor, x.m_ControlRevision) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignMissionManifest &x) { return a(x.m_Snapshot, x.m_Context, x.m_PreparationGeneration, x.m_FrozenPreparationRevision, x.m_ViewRevision, x.m_OwnershipRevision, x.m_Soldiers, x.m_Loadouts, x.m_Bindings, x.m_RequiredParticipants) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignSnapshotAck &x) { return a(x.m_Session, x.m_PlayerEcho, x.m_Generation, x.m_Snapshot, x.m_PreparationGeneration, x.m_PreparationRevision, x.m_ViewRevision, x.m_OwnershipRevision) && Valid(x); }
template<class A> bool Fields(A &a, S_CampaignStartSimulation &x) { return a(x.m_Session, x.m_Generation, x.m_Snapshot) && Valid(x); }
template<class A> bool Fields(A &a, S_Hello &x) { return a(x.m_Mod, x.m_Major, x.m_MinMinor, x.m_MaxMinor, x.m_Required, x.m_Offered, x.m_Kind, x.m_Content) && Valid(x); }
template<class A> bool Fields(A &a, S_Accept &x) { return a(x.m_Major, x.m_Minor, x.m_Features, x.m_Session, x.m_Player, x.m_Content, x.m_MaxFrameBytes, x.m_MaxWorldBytes, x.m_MaxParticipants) && Valid(x); }
template<class A> bool Fields(A &a, S_NegotiationAck &x) { return a(x.m_Major, x.m_Minor, x.m_Features, x.m_Session, x.m_Player) && Valid(x); }
template<class A> bool Fields(A &a, S_Reject &x) { return a(x.m_Reason) && Valid(x); }
template<class A> bool Fields(A &a, S_PreparationDelta &x) { return a(x.m_Base, x.m_Replacement) && Valid(x); }
template<class A> bool Fields(A &a, S_Input &x) { return a(x.m_Control, x.m_Sequence, x.m_Kind, x.m_Payload) && Valid(x); }
template<class A> bool Fields(A &a, S_WorldChunk &x) { return a(x.m_Baseline, x.m_TotalBytes, x.m_Index, x.m_Count, x.m_Crc32, x.m_Bytes) && Valid(x); }
template<class A> bool Fields(A &a, S_SquadActorState &x) {
    return a(x.m_Actor, x.m_Position, x.m_Facing, x.m_ViewDirection, x.m_Velocity, x.m_Health, x.m_Stance);
}
template<class A> bool Fields(A &a, S_MissionCamera &x) {
    return a(x.m_Position, x.m_Direction, x.m_Up, x.m_Fov);
}
template<class A> bool Fields(A &a, S_MissionPresentation &x) {
    return a(x.m_Action, x.m_Frame, x.m_Resource, x.m_Args);
}
template<class A> bool Fields(A &a, S_PresentationFramePose &x) {
    return a(x.m_Frame, x.m_Position, x.m_Direction, x.m_Up);
}
template<class A> bool Fields(A &a, S_AmbientHumanState &x) {
    return a(x.m_Pose, x.m_Visible);
}
template<class A> bool Fields(A &a, S_MissionUpdate &x) {
    return a(x.m_Session, x.m_Generation, x.m_Snapshot, x.m_OwnershipRevision, x.m_Sequence,
             x.m_MissionTime, x.m_PlayerInputEnabled, x.m_Outcome, x.m_CutsceneCamera, x.m_Squad, x.m_Objectives, x.m_Presentation, x.m_Ambient, x.m_FramePoses) && Valid(x);
}

template<class A> bool Fields(A &a, S_ItemMove &x) {
    return a(x.m_ItemUid, x.m_From, x.m_To, x.m_Slot, x.m_ReplacedUid) && Valid(x);
}
template<class A> bool Fields(A &a, S_InventoryEvent &x) {
    return a(x.m_Actor, x.m_Snapshot, x.m_OwnershipRevision, x.m_Sequence, x.m_Move) && Valid(x);
}

// Structural/transport validation. Catalog membership, equipment physics and
// ownership/request replay decisions remain injected model/authority obligations.
template<class T> bool Id(const T &x) { return x.IsValid(); }
template<class T> bool OptionalId(const std::optional<T> &x) { return !x || Id(*x); }
template<class A> bool Fields(A &a, S_WeaponEvent &x) {
    return a(x.m_Actor, x.m_Snapshot, x.m_OwnershipRevision, x.m_Sequence,
             x.m_Action, x.m_ItemUid, x.m_Rounds, x.m_ReloadTime, x.m_ConsumedAmmo) && Valid(x);
}
template<class T, class F> bool Unique(const std::vector<T> &x, F key) {
    std::set<decltype(key(std::declval<T>()))> keys;
    for (const auto &v : x) if (!keys.insert(key(v)).second) return false;
    return true;
}
bool Fingerprint(const std::string &s) {
    return s.size() == 71 && s.compare(0, 7, "sha256:") == 0 &&
        std::all_of(s.begin() + 7, s.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}
bool Valid(const ActorRef &x) { return Id(x.m_Session) && Id(x.m_Generation); }
bool Valid(const S_CampaignContentDescriptor &x) {
    return Fingerprint(x.m_CampaignContent) && Fingerprint(x.m_MissionContent) &&
        Fingerprint(x.m_SoldierContent) && Fingerprint(x.m_ItemContent) && Fingerprint(x.m_Rules);
}
bool Valid(const S_CampaignRulesDescriptor &) { return true; }
bool Valid(const S_CampaignSessionContext &x) {
    return x.m_Kind == E_CampaignSessionKind::CampaignCoop && Id(x.m_Session) && Id(x.m_Campaign) &&
        Id(x.m_Mission) && x.m_UsesCampaignRules && OptionalId(x.m_LocalPlayer) && OptionalId(x.m_HostPlayer) &&
        ((x.m_Phase != E_CampaignPhase::Running && x.m_Phase != E_CampaignPhase::Loading) || Id(x.m_MissionGeneration));
}
bool Valid(const S_CampaignCapability &) { return true; }
bool Valid(const S_CampaignPolicyDecision &) { return true; }
bool Valid(const S_CampaignPolicySnapshot &x) { return Unique(x.m_Decisions, [](const auto &v) { return v.m_Policy; }); }
bool Valid(const S_CampaignOwnershipRecord &x) {
    return Id(x.m_Soldier) && Id(x.m_ControlRevision) && OptionalId(x.m_Owner);
}
bool Valid(const S_CampaignParticipant &x) { return Id(x.m_Player) && OptionalId(x.m_ReadyAt) && (!x.m_Admitted || x.m_Connected); }
bool Catalog(const std::vector<S_CampaignOwnershipRecord> &soldiers, const std::vector<S_CampaignParticipant> &peers) {
    if (soldiers.size() > kMaxSoldiers || peers.size() > kMaxParticipants ||
        !Unique(soldiers, [](const auto &s) { return s.m_Soldier; }) ||
        !Unique(peers, [](const auto &p) { return p.m_Player; })) return false;
    std::set<PlayerId> owners;
    for (const auto &s : soldiers) if (s.m_Owner) {
        if (!owners.insert(*s.m_Owner).second || std::none_of(peers.begin(), peers.end(),
            [&](const auto &p) { return p.m_Player == *s.m_Owner && p.m_Connected && p.m_Admitted; })) return false;
    }
    return true;
}
bool ConsistentActors(const std::vector<S_CampaignOwnershipRecord> &soldiers, const S_CampaignSessionContext &context) {
    std::set<std::uint32_t> actors;
    for (const auto &s : soldiers) if (s.m_Actor && (s.m_Actor->m_Session != context.m_Session ||
        s.m_Actor->m_Generation != context.m_MissionGeneration || !actors.insert(s.m_Actor->m_Actor.Value()).second)) return false;
    return true;
}
bool Valid(const S_CampaignOwnershipSnapshot &x) {
    return Id(x.m_Revision) && Catalog(x.m_Soldiers, x.m_Participants) && ConsistentActors(x.m_Soldiers, x.m_Context);
}
bool Valid(const S_CampaignControlToken &x) {
    return Id(x.m_Session) && Id(x.m_Generation) && Id(x.m_Player) && Id(x.m_Soldier) &&
        Id(x.m_ControlRevision) && x.m_Actor.m_Session == x.m_Session && x.m_Actor.m_Generation == x.m_Generation;
}
bool Valid(const S_CampaignRequestContext &x) { return Id(x.m_Session) && Id(x.m_PlayerEcho) && Id(x.m_Request); }
bool Valid(const S_CampaignOwnershipTarget &x) { return Id(x.m_Soldier) && Id(x.m_ExpectedControlRevision); }
bool Valid(const S_CampaignClaim &) { return true; }
bool Valid(const S_CampaignRelease &) { return true; }
bool Valid(const S_CampaignSwitch &x) { return x.m_From.m_Soldier != x.m_To.m_Soldier; }
bool Valid(const S_CampaignOwnershipRequest &x) {
    return Id(x.m_ExpectedOwnershipRevision) && OptionalId(x.m_PreparationGeneration) && OptionalId(x.m_MissionGeneration) &&
        !(x.m_PreparationGeneration && x.m_MissionGeneration);
}
bool Valid(const S_CampaignPlacement &x) { return Id(x.m_Slot) && x.m_Position <= kMaxQuantity; }
bool Valid(const S_CampaignItemState &x) {
    return Id(x.m_Item) && x.m_Quantity <= kMaxQuantity && OptionalId(x.m_AmmoType) &&
        (!x.m_Rounds || *x.m_Rounds <= kMaxQuantity) && (!x.m_Charges || *x.m_Charges >= 0) &&
        (!x.m_Condition || *x.m_Condition >= 0);
}
bool Valid(const S_CampaignLoadoutItem &x) { return Id(x.m_Entry) && x.m_State.m_Quantity != 0; }
bool Valid(const S_CampaignSkill &x) { return Id(x.m_Key); }
bool Valid(const S_CampaignFinding &x) {
    return OptionalId(x.m_Soldier) && OptionalId(x.m_Item) && OptionalId(x.m_Entry) && OptionalId(x.m_Slot);
}
bool Valid(const S_CampaignConstraintSummary &x) { return Id(x.m_Key); }
bool Valid(const S_CampaignSoldierLoadout &x) {
    return Id(x.m_Soldier) && Unique(x.m_Items, [](const auto &i) { return i.m_Entry; }) &&
        Unique(x.m_Skills, [](const auto &s) { return s.m_Key; }) && x.m_Weight >= 0 && x.m_WeightCapacity >= 0 &&
        x.m_BeltVolume >= 0 && x.m_BeltCapacity >= 0 && x.m_BackpackVolume >= 0 && x.m_BackpackCapacity >= 0 &&
        x.m_BackpackWeight >= 0 && x.m_BackpackWeightCapacity >= 0 && x.m_BeltWeight >= 0;
}
bool Valid(const S_CampaignAllowedDestination &x) {
    return Id(x.m_Soldier) && std::all_of(x.m_Placements.begin(), x.m_Placements.end(), [](const auto &p) { return Id(p); });
}
bool Valid(const S_CampaignStockEntry &x) { return Id(x.m_Entry) && x.m_Destinations.size() <= kMaxSoldiers; }
bool Valid(const S_CampaignEssential &x) {
    return Id(x.m_Item) && x.m_Required <= kMaxQuantity && x.m_Carried <= kMaxQuantity && x.m_Deficit <= kMaxQuantity &&
        x.m_EligibleCarriers.size() <= kMaxSoldiers && std::all_of(x.m_EligibleCarriers.begin(), x.m_EligibleCarriers.end(), [](const auto &s) { return Id(s); });
}
bool Valid(const S_CampaignSoldierPresentation &x) { return Id(x.m_Soldier) && Id(x.m_Profile) && x.m_Soldier == x.m_Ownership.m_Soldier; }
bool Valid(const S_CampaignPresentationSlot &x) { return OptionalId(x.m_Soldier); }
bool Valid(const S_CampaignBriefing &) { return true; }
bool Roster(const std::vector<S_CampaignSoldierPresentation> &soldiers, const std::vector<S_CampaignSoldierLoadout> &loadouts) {
    if (soldiers.size() > kMaxSoldiers || soldiers.size() != loadouts.size() ||
        !Unique(soldiers, [](const auto &s) { return s.m_Soldier; }) || !Unique(loadouts, [](const auto &s) { return s.m_Soldier; })) return false;
    for (const auto &s : soldiers) if (std::none_of(loadouts.begin(), loadouts.end(), [&](const auto &l) { return l.m_Soldier == s.m_Soldier; })) return false;
    return true;
}
bool Valid(const S_CampaignPreparationSnapshot &x) {
    if (!Id(x.m_Generation) || !Id(x.m_PreparationRevision) || !Id(x.m_ViewRevision) || !Id(x.m_OwnershipRevision) ||
        !Roster(x.m_Soldiers, x.m_Loadouts) || !Unique(x.m_Stock, [](const auto &s) { return s.m_Entry; })) return false;
    std::vector<S_CampaignOwnershipRecord> records;
    for (const auto &s : x.m_Soldiers) records.push_back(s.m_Ownership);
    if (!Catalog(records, x.m_Participants) || !ConsistentActors(records, x.m_Context)) return false;
    std::set<SoldierId> slots;
    for (const auto &slot : x.m_Slots) if (slot.m_Soldier &&
        (!slots.insert(*slot.m_Soldier).second || std::none_of(x.m_Soldiers.begin(), x.m_Soldiers.end(),
            [&](const auto &s) { return s.m_Soldier == *slot.m_Soldier; }))) return false;
    return true;
}
bool Valid(const S_CampaignTakeFromStock &x) { return Id(x.m_Soldier) && Id(x.m_StockEntry) && x.m_Count && x.m_Count <= kMaxQuantity; }
bool Valid(const S_CampaignReturnToStock &x) { return Id(x.m_Soldier) && Id(x.m_Entry) && x.m_Count && x.m_Count <= kMaxQuantity; }
bool Valid(const S_CampaignMoveWithinLoadout &x) { return Id(x.m_Soldier) && Id(x.m_Entry) && x.m_Count && x.m_Count <= kMaxQuantity; }
bool Valid(const S_CampaignApplyPreset &x) { return Id(x.m_Preset); }
bool Valid(const S_CampaignAutoEquip &x) {
    return Id(x.m_Policy) && std::all_of(x.m_PreferredItems.begin(), x.m_PreferredItems.end(), [](const auto &s) { return Id(s); });
}
bool Valid(const S_CampaignPreparationRequestContext &x) { return Id(x.m_Generation) && Id(x.m_ExpectedPreparationRevision); }
bool Valid(const S_CampaignEditRequest &x) {
    if (x.m_Targets.empty() || x.m_Targets.size() > kMaxSoldiers || !Unique(x.m_Targets, [](const auto &s) { return s.m_Soldier; })) return false;
    return std::visit([&](const auto &op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same<T, S_CampaignTakeFromStock>::value || std::is_same<T, S_CampaignReturnToStock>::value || std::is_same<T, S_CampaignMoveWithinLoadout>::value)
            return x.m_Targets.size() == 1 && x.m_Targets[0].m_Soldier == op.m_Soldier;
        else return true;
    }, x.m_Operation);
}
bool Valid(const S_CampaignReadyRequest &) { return true; }
bool Valid(const S_CampaignDeployRequest &) { return true; }
bool Valid(const S_CampaignResyncDescriptor &x) {
    return Id(x.m_Session) && OptionalId(x.m_PreparationGeneration) && OptionalId(x.m_MissionGeneration) && OptionalId(x.m_Snapshot);
}
template<class T> bool CurrentGeneration(const std::optional<T> &returned, T current) {
    return current.IsValid() ? returned && *returned == current : !returned;
}
bool Valid(const S_CampaignRequestResult &x) {
    if (!OptionalId(x.m_PreparationGeneration) || !OptionalId(x.m_MissionGeneration) ||
        (x.m_Outcome == E_CampaignRequestOutcome::Duplicate) != x.m_OriginalOutcome.has_value() ||
        (x.m_OriginalOutcome && *x.m_OriginalOutcome != E_CampaignRequestOutcome::Accepted && *x.m_OriginalOutcome != E_CampaignRequestOutcome::Rejected)) return false;
    // Baselines/generations describe the attached immutable state, including an
    // original cached duplicate. Never mix new metadata with an old publication.
    return std::visit([&](const auto &state) {
        using T = std::decay_t<decltype(state)>;
        if constexpr (std::is_same<T, S_CampaignResyncDescriptor>::value) {
            return state.m_Session == x.m_Context.m_Session && state.m_PreparationGeneration == x.m_PreparationGeneration &&
                state.m_MissionGeneration == x.m_MissionGeneration && state.m_OwnershipRevision == x.m_OwnershipRevision &&
                state.m_PreparationRevision == x.m_PreparationRevision && state.m_ViewRevision == x.m_ViewRevision;
        } else {
            if (!state || state->m_Context.m_Session != x.m_Context.m_Session ||
                !CurrentGeneration(x.m_MissionGeneration, state->m_Context.m_MissionGeneration)) return false;
            if constexpr (std::is_same<T, T_CampaignPreparationSnapshot>::value) {
                return CurrentGeneration(x.m_PreparationGeneration, state->m_Generation) && state->m_OwnershipRevision == x.m_OwnershipRevision &&
                    state->m_PreparationRevision == x.m_PreparationRevision && state->m_ViewRevision == x.m_ViewRevision;
            } else return state->m_Revision == x.m_OwnershipRevision;
        }
    }, x.m_State);
}
bool Valid(const S_CampaignActorBinding &x) { return Id(x.m_Soldier) && Id(x.m_ControlRevision); }
bool Valid(const S_CampaignMissionManifest &x) {
    if (!Id(x.m_Snapshot) || !Id(x.m_PreparationGeneration) || !Id(x.m_FrozenPreparationRevision) ||
        !Id(x.m_ViewRevision) || !Id(x.m_OwnershipRevision) || !Id(x.m_Context.m_MissionGeneration) ||
        !Roster(x.m_Soldiers, x.m_Loadouts) || x.m_Bindings.size() > kMaxSoldiers || x.m_RequiredParticipants.size() > kMaxParticipants ||
        !Unique(x.m_Bindings, [](const auto &b) { return b.m_Soldier; }) ||
        !Unique(x.m_RequiredParticipants, [](const auto &p) { return p; })) return false;
    std::vector<S_CampaignOwnershipRecord> records;
    for (const auto &soldier : x.m_Soldiers) records.push_back(soldier.m_Ownership);
    if (!ConsistentActors(records, x.m_Context)) return false;
    std::set<std::uint32_t> actors;
    for (const auto &b : x.m_Bindings) if (b.m_Actor.m_Session != x.m_Context.m_Session ||
        b.m_Actor.m_Generation != x.m_Context.m_MissionGeneration ||
        !actors.insert(b.m_Actor.m_Actor.Value()).second ||
        std::none_of(x.m_Soldiers.begin(), x.m_Soldiers.end(), [&](const auto &s) { return s.m_Soldier == b.m_Soldier; })) return false;
    for (const auto &soldier : x.m_Soldiers) if (soldier.m_Ownership.m_Actor) {
        const auto binding = std::find_if(x.m_Bindings.begin(), x.m_Bindings.end(),
            [&](const auto &entry) { return entry.m_Soldier == soldier.m_Soldier; });
        if (binding != x.m_Bindings.end() && (binding->m_Actor != *soldier.m_Ownership.m_Actor ||
            binding->m_ControlRevision != soldier.m_Ownership.m_ControlRevision)) return false;
    }
    return std::all_of(x.m_RequiredParticipants.begin(), x.m_RequiredParticipants.end(), [](const auto &p) { return Id(p); });
}
bool Valid(const S_CampaignSnapshotAck &x) {
    return Id(x.m_Session) && Id(x.m_PlayerEcho) && Id(x.m_Generation) && Id(x.m_Snapshot) &&
        Id(x.m_PreparationGeneration) && Id(x.m_PreparationRevision) && Id(x.m_ViewRevision) && Id(x.m_OwnershipRevision);
}
bool Valid(const S_CampaignStartSimulation &x) { return Id(x.m_Session) && Id(x.m_Generation) && Id(x.m_Snapshot); }
bool Valid(const S_Hello &x) { return x.m_MinMinor <= x.m_MaxMinor && (x.m_Required & ~x.m_Offered) == 0; }
bool Valid(const S_Accept &x) {
    return Id(x.m_Session) && Id(x.m_Player) && x.m_MaxFrameBytes == kMaxFrameBytes &&
        x.m_MaxWorldBytes == kMaxWorldBytes && x.m_MaxParticipants == kMaxParticipants;
}
bool Valid(const S_NegotiationAck &x) { return Id(x.m_Session) && Id(x.m_Player); }
bool Valid(const S_Reject &x) {
    switch (x.m_Reason) {
    case E_CampaignReason::ModRequired: case E_CampaignReason::UnsupportedVersion:
    case E_CampaignReason::MissingFeature: case E_CampaignReason::ContentMismatch:
    case E_CampaignReason::RulesMismatch: case E_CampaignReason::Malformed:
    case E_CampaignReason::Timeout: case E_CampaignReason::SessionFull: case E_CampaignReason::JoinNotSupported: return true;
    default: return false;
    }
}
bool Valid(const S_PreparationDelta &x) { return Id(x.m_Base) && x.m_Replacement.m_ViewRevision.Value() > x.m_Base.Value(); }
bool Valid(const S_Input &x) {
    return x.m_Sequence && !x.m_Payload.empty() && x.m_Payload.size() <= kMaxInputBytes &&
        static_cast<unsigned>(x.m_Kind) <= static_cast<unsigned>(S_EnumMax<E_InputKind>::value);
}
bool Valid(const S_WorldChunk &x) {
    if (!x.m_TotalBytes || x.m_TotalBytes > kMaxWorldBytes || !x.m_Count || x.m_Count > kMaxChunks ||
        x.m_Index >= x.m_Count || x.m_Count != (x.m_TotalBytes + kMaxChunkBytes - 1) / kMaxChunkBytes) return false;
    const auto offset = static_cast<std::size_t>(x.m_Index) * kMaxChunkBytes;
    return x.m_Bytes.size() == std::min(kMaxChunkBytes, static_cast<std::size_t>(x.m_TotalBytes) - offset);
}
float Dot(const std::array<float, 3> &a, const std::array<float, 3> &b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
bool IsDirection(const std::array<float, 3> &v) {
    const float length = Dot(v, v);
    return length >= 0.98f && length <= 1.02f;
}
bool ValidControls(const S_PlayerControls &input) {
    constexpr std::uint16_t held = kMoveForward | kMoveBack | kMoveLeft | kMoveRight | kLeanLeft | kLeanRight | kFire;
    constexpr std::uint16_t pressed = kFire | kCrouch | kProne | kReload | kPaceUp | kPaceDown;
    return !(input.m_Held & ~held) && !(input.m_Pressed & ~pressed) && IsDirection(input.m_ViewDirection);
}
bool ValidPresentation(const S_MissionPresentation &event) {
    using Action = E_MissionPresentation;
    const auto [value, second, third] = event.m_Args;
    const bool frame = !event.m_Frame.empty();
    const bool resource = !event.m_Resource.empty();
    const bool flag = value == 0 || value == 1;
    if (event.m_Action != Action::Animation && event.m_Action != Action::Clone &&
        event.m_Action != Action::Attach && resource)
        return false;
    switch (event.m_Action) {
    case Action::FrameOn: return frame && flag && !second && !third;
    case Action::Speech: return frame && value >= 0 && second >= 0 && second <= third && third <= 10000;
    case Action::Animation: return frame && resource && value >= 0 && value < 7 && (second == 0 || second == 1) && !third;
    case Action::StopAnimation: return frame && value >= 0 && value < 7 && (second == 0 || second == 1) && !third;
    case Action::Clone: return frame && resource && !value && !second && !third;
    case Action::Attach: return frame && resource && flag && !second && !third;
    case Action::ReleaseClone: return frame && !value && !second && !third;
    case Action::Sound: return frame && value >= 0 && second >= 0 && !third;
    case Action::Door: return !frame && value >= 0 && second >= 0 && second <= 5 && !third;
    case Action::WireCut: return frame && value >= -31416 && value <= 31416
        && (second == 0 || second == 1) && !third;
    case Action::ClearHints: return !frame && !value && !second && !third;
    case Action::Subtitle:
    case Action::Hint: return !frame && value >= 0 && !second && !third;
    case Action::CutsceneMusic: return !frame && value >= -1 && !second && !third;
    case Action::SubtitlesOn:
    case Action::WeatherOn:
    case Action::WeatherMode:
    case Action::Clouds: return !frame && flag && !second && !third;
    }
    return false;
}

bool Valid(const S_MissionUpdate &x) {
    if (!Id(x.m_Session) || !Id(x.m_Generation) || !Id(x.m_Snapshot) || !Id(x.m_OwnershipRevision) ||
        !x.m_Sequence || x.m_Squad.empty() || x.m_Squad.size() > kMaxSoldiers ||
        x.m_Objectives.size() > kMaxCollection || x.m_Presentation.size() > 64 || x.m_FramePoses.size() > 64 ||
        !Unique(x.m_FramePoses, [](const auto &pose) { return pose.m_Frame; }) ||
        !std::all_of(x.m_Presentation.begin(), x.m_Presentation.end(), ValidPresentation) ||
        !Unique(x.m_Squad, [](const auto &record) { return record.m_Actor.m_Actor.Value(); }))
        return false;
    if (x.m_Outcome != E_MissionOutcome::Running) {
        if (x.m_PlayerInputEnabled || x.m_CutsceneCamera ||
            std::find(x.m_Objectives.begin(), x.m_Objectives.end(), 0) != x.m_Objectives.end())
            return false;
        if (x.m_Outcome == E_MissionOutcome::Success &&
            (std::find(x.m_Objectives.begin(), x.m_Objectives.end(), 1) == x.m_Objectives.end() ||
             std::find(x.m_Objectives.begin(), x.m_Objectives.end(), 3) != x.m_Objectives.end()))
            return false;
    }
    if (x.m_CutsceneCamera) {
        const auto &camera = *x.m_CutsceneCamera;
        if (x.m_PlayerInputEnabled || camera.m_Fov <= 0 || camera.m_Fov >= 3.142f ||
            !IsDirection(camera.m_Direction) || !IsDirection(camera.m_Up) ||
            std::fabs(Dot(camera.m_Direction, camera.m_Up)) > 0.02f)
            return false;
    }
    for (const auto &pose : x.m_FramePoses) {
        if (pose.m_Frame.empty() || !IsDirection(pose.m_Direction) || !IsDirection(pose.m_Up) ||
            std::fabs(Dot(pose.m_Direction, pose.m_Up)) > 0.02f)
            return false;
    }
    const auto poseInGeneration = [&](const S_SquadActorState &actor) {
        return actor.m_Actor.m_Session == x.m_Session && actor.m_Actor.m_Generation == x.m_Generation &&
            actor.m_Stance <= 2 && IsDirection(actor.m_Facing) && IsDirection(actor.m_ViewDirection);
    };
    if (!std::all_of(x.m_Squad.begin(), x.m_Squad.end(), poseInGeneration) ||
        !Unique(x.m_Ambient, [](const auto &record) { return record.m_Pose.m_Actor.m_Actor.Value(); }))
        return false;
    for (const auto &ambient : x.m_Ambient) {
        const auto &pose = ambient.m_Pose;
        if (pose.m_Actor.m_SlotUse.Value() != 0 || !poseInGeneration(pose) ||
            std::any_of(x.m_Squad.begin(), x.m_Squad.end(), [&](const auto &soldier) {
                return soldier.m_Actor.m_Actor == pose.m_Actor.m_Actor;
            }))
            return false;
    }
    return std::all_of(x.m_Objectives.begin(), x.m_Objectives.end(), [](auto status) { return status <= 4; });
}

bool Valid(const S_WeaponEvent &x) {
    return Id(x.m_Snapshot) && Id(x.m_OwnershipRevision) && x.m_Sequence &&
        x.m_ItemUid <= INT32_MAX && (x.m_Action == E_WeaponEvent::Reload || !x.m_ReloadTime) &&
        (!x.m_ConsumedAmmo ||
            (x.m_Action == E_WeaponEvent::Reload && *x.m_ConsumedAmmo <= INT32_MAX &&
             *x.m_ConsumedAmmo != x.m_ItemUid));
}

bool Valid(const S_ItemMove &x) {
    return x.m_ItemUid <= INT32_MAX && x.m_From < 7 && x.m_To <= 7 && x.m_Slot < 256 &&
        (!x.m_ReplacedUid || (x.m_To == 3 && x.m_From != x.m_To &&
            *x.m_ReplacedUid <= INT32_MAX && *x.m_ReplacedUid != x.m_ItemUid));
}
bool Valid(const S_InventoryEvent &x) {
    return Id(x.m_Snapshot) && Id(x.m_OwnershipRevision) && x.m_Sequence && x.m_Move.m_To < 7;
}

std::uint16_t Read16(const std::uint8_t *p) { return static_cast<std::uint16_t>(p[0] | (p[1] << 8)); }
std::uint32_t Read32(const std::uint8_t *p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
        (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}
void Append(T_Bytes &out, std::uint64_t value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; ++i) out.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
}
template<std::size_t I = 0> bool Body(std::size_t index, C_Archive &a, T_Message &message) {
    if constexpr (I == std::variant_size<T_Message>::value) return false;
    else {
        if (index == I) { message.emplace<I>(); return a(std::get<I>(message)) && a.Done(); }
        return Body<I + 1>(index, a, message);
    }
}
bool SameContent(const S_CampaignContentDescriptor &a, const S_CampaignContentDescriptor &b, bool rules) {
    return rules ? a.m_Rules == b.m_Rules : a.m_CampaignContent == b.m_CampaignContent &&
        a.m_MissionContent == b.m_MissionContent && a.m_SoldierContent == b.m_SoldierContent && a.m_ItemContent == b.m_ItemContent;
}
bool SameControl(const S_CampaignControlToken &a, const S_CampaignControlToken &b) {
    return a.m_Session == b.m_Session && a.m_Generation == b.m_Generation && a.m_Player == b.m_Player &&
        a.m_Soldier == b.m_Soldier && a.m_Actor == b.m_Actor && a.m_ControlRevision == b.m_ControlRevision;
}
bool SameBaseline(const S_CampaignSnapshotAck &a, const S_CampaignSnapshotAck &b) {
    return a.m_Session == b.m_Session && a.m_PlayerEcho == b.m_PlayerEcho && a.m_Generation == b.m_Generation &&
        a.m_Snapshot == b.m_Snapshot && a.m_PreparationGeneration == b.m_PreparationGeneration &&
        a.m_PreparationRevision == b.m_PreparationRevision && a.m_ViewRevision == b.m_ViewRevision && a.m_OwnershipRevision == b.m_OwnershipRevision;
}
bool Matches(const S_NegotiationAck &ack, const S_Accept &a) {
    return ack.m_Major == a.m_Major && ack.m_Minor == a.m_Minor && ack.m_Features == a.m_Features && ack.m_Session == a.m_Session && ack.m_Player == a.m_Player;
}
S_NegotiationAck Ack(const S_Accept &a) { return {a.m_Major, a.m_Minor, a.m_Features, a.m_Session, a.m_Player}; }
bool BoundPayload(const T_Message &message, bool server, const S_Accept &agreement) {
    return std::visit([&](const auto &x) {
        using T = std::decay_t<decltype(x)>;
        const auto session = agreement.m_Session;
        const auto player = agreement.m_Player;
        if constexpr (std::is_same<T, S_CampaignOwnershipRequest>::value)
            return server && x.m_Context.m_Session == session && x.m_Context.m_PlayerEcho == player;
        else if constexpr (std::is_same<T, S_CampaignEditRequest>::value || std::is_same<T, S_CampaignReadyRequest>::value || std::is_same<T, S_CampaignDeployRequest>::value)
            return server && x.m_Context.m_Request.m_Session == session && x.m_Context.m_Request.m_PlayerEcho == player;
        else if constexpr (std::is_same<T, S_CampaignSnapshotAck>::value)
            return server && x.m_Session == session && x.m_PlayerEcho == player;
        else if constexpr (std::is_same<T, S_Input>::value)
            return server && x.m_Control.m_Session == session && x.m_Control.m_Player == player;
        else if constexpr (std::is_same<T, S_CampaignRequestResult>::value)
            return !server && x.m_Context.m_Session == session; // other players' results may be published
        else if constexpr (std::is_same<T, S_CampaignOwnershipSnapshot>::value || std::is_same<T, S_CampaignPreparationSnapshot>::value || std::is_same<T, S_CampaignMissionManifest>::value)
            return !server && x.m_Context.m_Session == session;
        else if constexpr (std::is_same<T, S_PreparationDelta>::value)
            return !server && x.m_Replacement.m_Context.m_Session == session;
        else if constexpr (std::is_same<T, S_CampaignStartSimulation>::value)
            return !server && x.m_Session == session;
        else if constexpr (std::is_same<T, S_WorldChunk>::value)
            return !server && x.m_Baseline.m_Session == session && x.m_Baseline.m_PlayerEcho == player;
        else if constexpr (std::is_same<T, S_MissionUpdate>::value)
            return !server && (agreement.m_Features & kMissionUpdates) && x.m_Session == session;
        else if constexpr (std::is_same<T, S_WeaponEvent>::value)
            return !server && (agreement.m_Features & kWeaponEvents) && x.m_Actor.m_Session == session;
        else if constexpr (std::is_same<T, S_InventoryEvent>::value)
            return !server && (agreement.m_Features & kInventoryMoves) && x.m_Actor.m_Session == session;
        else return false;
    }, message);
}
} // namespace

E_CampaignReason Inspect(const std::uint8_t *data, std::size_t size, S_FrameInfo &out) {
    if (!data || size < kHeaderBytes || size > kMaxFrameBytes || Read32(data) != kMagic) return E_CampaignReason::Malformed;
    if (Read16(data + 4) != kMajor || Read16(data + 6) != kMinor) return E_CampaignReason::UnsupportedVersion;
    const auto type = Read16(data + 8);
    const auto body = Read32(data + 12);
    if (!type || type > std::variant_size<T_Message>::value || Read16(data + 10) != 0 || body != size - kHeaderBytes) return E_CampaignReason::Malformed;
    out = {static_cast<E_Message>(type), body};
    return E_CampaignReason::None;
}
E_CampaignReason Encode(const T_Message &message, T_Bytes &out) {
    if (message.valueless_by_exception()) return E_CampaignReason::Malformed;
    try {
        T_Message copy = message;
        T_Bytes body;
        C_Archive a(body);
        if (!std::visit([&](auto &x) { return a(x); }, copy)) return E_CampaignReason::Malformed;
        T_Bytes frame;
        frame.reserve(kHeaderBytes + body.size());
        Append(frame, kMagic, 4); Append(frame, kMajor, 2); Append(frame, kMinor, 2);
        Append(frame, message.index() + 1, 2); Append(frame, 0, 2); Append(frame, body.size(), 4);
        frame.insert(frame.end(), body.begin(), body.end());
        out = std::move(frame);
        return E_CampaignReason::None;
    } catch (const std::bad_alloc &) { return E_CampaignReason::Malformed; }
      catch (const std::length_error &) { return E_CampaignReason::Malformed; }
}
E_CampaignReason Decode(const std::uint8_t *data, std::size_t size, T_Message &out) {
    S_FrameInfo info;
    const auto status = Inspect(data, size, info);
    if (status != E_CampaignReason::None) return status;
    try {
        C_Archive a(data + kHeaderBytes, info.m_BodyBytes);
        T_Message candidate;
        if (!Body(static_cast<std::size_t>(info.m_Type) - 1, a, candidate)) return E_CampaignReason::Malformed;
        out = std::move(candidate);
        return E_CampaignReason::None;
    } catch (const std::bad_alloc &) { return E_CampaignReason::Malformed; }
      catch (const std::length_error &) { return E_CampaignReason::Malformed; }
}
E_CampaignReason ExtractFrameBits(const std::uint8_t *packet, std::size_t packetBytes,
    std::size_t validBits, std::size_t &position, T_Bytes &out) {
    // Native transport reassembly buffer is 0x8000 bytes. Bound before products.
    if (!packet || packetBytes > 32768 || validBits > packetBytes * 8 || position > validBits ||
        kHeaderBytes * 8 > validBits - position) return E_CampaignReason::Malformed;
    auto read = [&](std::size_t offset) {
        std::uint8_t byte = 0;
        for (unsigned bit = 0; bit < 8; ++bit) {
            const auto index = position + offset * 8 + bit;
            byte |= static_cast<std::uint8_t>(((packet[index / 8] >> (index % 8)) & 1) << bit);
        }
        return byte;
    };
    T_Bytes candidate(kHeaderBytes);
    for (std::size_t i = 0; i < kHeaderBytes; ++i) candidate[i] = read(i);
    const auto body = Read32(candidate.data() + 12);
    if (body > kMaxFrameBytes - kHeaderBytes || (kHeaderBytes + body) * 8 > validBits - position) return E_CampaignReason::Malformed;
    candidate.resize(kHeaderBytes + body);
    for (std::size_t i = kHeaderBytes; i < candidate.size(); ++i) candidate[i] = read(i);
    S_FrameInfo info;
    const auto reason = Inspect(candidate.data(), candidate.size(), info);
    if (reason != E_CampaignReason::None) return reason;
    position += candidate.size() * 8;
    out = std::move(candidate);
    return E_CampaignReason::None;
}
E_CampaignReason AppendFrameBits(const T_Bytes &frame, T_Bytes &packet, std::size_t &position) {
    S_FrameInfo info;
    const auto reason = Inspect(frame.data(), frame.size(), info);
    if (reason != E_CampaignReason::None) return reason;
    // One isolated command per packet; preserve headroom below signed16/native
    // reassembly limits. A scheduler must also bound coalesced channel output.
    if (packet.size() > 32767 || position > packet.size() * 8 ||
        frame.size() * 8 > 32767 * 8 - position) return E_CampaignReason::Malformed;
    T_Bytes candidate = packet;
    candidate.resize(std::max(candidate.size(), (position + frame.size() * 8 + 7) / 8), 0);
    for (std::size_t i = 0; i < frame.size() * 8; ++i) {
        const auto index = position + i;
        const auto mask = static_cast<std::uint8_t>(1u << (index % 8));
        const auto bit = static_cast<std::uint8_t>(((frame[i / 8] >> (i % 8)) & 1) << (index % 8));
        candidate[index / 8] = static_cast<std::uint8_t>((candidate[index / 8] & ~mask) | bit);
    }
    position += frame.size() * 8;
    packet = std::move(candidate);
    return E_CampaignReason::None;
}
namespace {
constexpr std::size_t kNativeTypeBits = 5;
constexpr std::size_t kCommandBits = kNativeTypeBits + 32;
constexpr std::uint64_t kCommandPrefix = kGameMessageType | (std::uint64_t{kCommand} << kNativeTypeBits);
constexpr std::uint16_t kEndSuffix = kEndMessageType << (kCommandBits % 8);
}

E_CampaignReason EncodeCommandPacket(const T_Message &message, T_Bytes &out)
{
    T_Bytes frame;
    if (const auto reason = Encode(message, frame); reason != E_CampaignReason::None)
        return reason;

    T_Bytes packet(kCommandPacketOverhead, 0);
    for (std::size_t i = 0; i < (kCommandBits + 7) / 8; ++i)
        packet[i] = static_cast<std::uint8_t>(kCommandPrefix >> (i * 8));

    std::size_t position = kCommandBits;
    if (const auto reason = AppendFrameBits(frame, packet, position); reason != E_CampaignReason::None)
        return reason;

    const auto end = position / 8;
    packet.resize(frame.size() + kCommandPacketOverhead, 0);
    packet[end] |= static_cast<std::uint8_t>(kEndSuffix);
    packet[end + 1] = static_cast<std::uint8_t>(kEndSuffix >> 8);
    out = std::move(packet);
    return E_CampaignReason::None;
}

E_CampaignReason ExtractCommandFrame(const void *data, std::size_t size, T_Bytes &out)
{
    using R = E_CampaignReason;
    if (!data || size < kHeaderBytes + kCommandPacketOverhead || size > kMaxFrameBytes + kCommandPacketOverhead)
        return R::Malformed;

    const auto *bytes = static_cast<const std::uint8_t *>(data);
    std::uint64_t prefix = 0;
    for (std::size_t i = 0; i < (kCommandBits + 7) / 8; ++i)
        prefix |= std::uint64_t{bytes[i]} << (i * 8);
    if ((prefix & ((std::uint64_t{1} << kCommandBits) - 1)) != kCommandPrefix)
        return R::Malformed;

    const auto end = static_cast<std::uint16_t>(bytes[size - 2] | (bytes[size - 1] << 8));
    if ((end >> (kCommandBits % 8)) != kEndMessageType)
        return R::Malformed;

    T_Bytes frame;
    std::size_t position = kCommandBits;
    if (const auto reason = ExtractFrameBits(bytes, size, size * 8, position, frame); reason != R::None)
        return reason;
    if (frame.size() + kCommandPacketOverhead != size)
        return R::Malformed;

    out = std::move(frame);
    return R::None;
}

E_CampaignReason EncodePlayerControls(const S_PlayerControls &controls, T_Bytes &out) {
    if (!ValidControls(controls)) return E_CampaignReason::Malformed;
    auto input = controls;
    std::uint8_t version = 1;
    T_Bytes bytes;
    C_Archive archive(bytes);
    if (!archive(version, input.m_Held, input.m_Pressed, input.m_ViewDirection))
        return E_CampaignReason::Malformed;
    out = std::move(bytes);
    return E_CampaignReason::None;
}

E_CampaignReason DecodePlayerControls(const T_Bytes &bytes, S_PlayerControls &out) {
    std::uint8_t version = 0;
    S_PlayerControls input;
    C_Archive archive(bytes.data(), bytes.size());
    if (!archive(version, input.m_Held, input.m_Pressed, input.m_ViewDirection) ||
        !archive.Done() || version != 1 || !ValidControls(input))
        return E_CampaignReason::Malformed;
    out = input;
    return E_CampaignReason::None;
}

E_CampaignReason EncodeItemMove(const S_ItemMove &move, T_Bytes &out) {
    auto candidate = move;
    std::uint8_t version = 1;
    T_Bytes bytes;
    C_Archive archive(bytes);
    if (!archive(version, candidate)) return E_CampaignReason::Malformed;
    out = std::move(bytes);
    return E_CampaignReason::None;
}

E_CampaignReason DecodeItemMove(const T_Bytes &bytes, S_ItemMove &out) {
    std::uint8_t version = 0;
    S_ItemMove move;
    C_Archive archive(bytes.data(), bytes.size());
    if (!archive(version, move) || !archive.Done() || version != 1) return E_CampaignReason::Malformed;
    out = move;
    return E_CampaignReason::None;
}

E_CampaignReason EncodeDoorAction(const S_DoorAction &action, T_Bytes &out) {
    auto candidate = action;
    std::uint8_t version = 1;
    T_Bytes bytes;
    C_Archive archive(bytes);
    if (candidate.m_Door > INT32_MAX || !archive(version, candidate.m_Door, candidate.m_Action))
        return E_CampaignReason::Malformed;
    out = std::move(bytes);
    return E_CampaignReason::None;
}

E_CampaignReason DecodeDoorAction(const T_Bytes &bytes, S_DoorAction &out) {
    S_DoorAction candidate;
    std::uint8_t version = 0;
    C_Archive archive(bytes.data(), bytes.size());
    if (!archive(version, candidate.m_Door, candidate.m_Action) || !archive.Done() ||
        version != 1 || candidate.m_Door > INT32_MAX)
        return E_CampaignReason::Malformed;
    out = candidate;
    return E_CampaignReason::None;
}

E_CampaignReason EncodeWireCut(const std::string &frame, T_Bytes &out) {
    auto name = frame;
    std::uint8_t version = 1;
    T_Bytes bytes;
    C_Archive archive(bytes);
    if (name.empty() || !archive(version, name))
        return E_CampaignReason::Malformed;
    out = std::move(bytes);
    return E_CampaignReason::None;
}

E_CampaignReason DecodeWireCut(const T_Bytes &bytes, std::string &frame) {
    std::string name;
    std::uint8_t version = 0;
    C_Archive archive(bytes.data(), bytes.size());
    if (!archive(version, name) || !archive.Done() || version != 1 || name.empty())
        return E_CampaignReason::Malformed;
    frame = std::move(name);
    return E_CampaignReason::None;
}

std::uint32_t Crc32(const T_Bytes &bytes) {
    std::uint32_t crc = 0xffffffff;
    for (auto byte : bytes) {
        crc ^= byte;
        for (unsigned i = 0; i < 8; ++i) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
    }
    return ~crc;
}
E_Route Route(E_CampaignSessionKind kind, E_Traffic traffic) {
    if (kind != E_CampaignSessionKind::CampaignCoop) return traffic == E_Traffic::Extension ? E_Route::Drop : E_Route::Legacy;
    return traffic == E_Traffic::Extension ? E_Route::Campaign : E_Route::Drop;
}
constexpr std::uint64_t kSupportedFeatures = kFeatures | kMissionUpdates | kWeaponEvents | kInventoryMoves;

C_Admission::C_Admission(E_CampaignSessionKind kind, bool server, S_Hello local,
    SessionId session, PlayerId assignedPlayer, std::uint64_t nowMs)
    : m_Server(server), m_State(kind == E_CampaignSessionKind::CampaignCoop ?
        (server ? E_Admission::AwaitHello : E_Admission::AwaitAccept) : E_Admission::Dormant),
      m_Local(std::move(local)), m_Started(nowMs), m_Window(nowMs) {
    m_Agreement.m_Session = session;
    m_Agreement.m_Player = assignedPlayer;
    m_Agreement.m_Content = m_Local.m_Content;
    if (m_State != E_Admission::Dormant && (!Valid(m_Local) || !Valid(m_Local.m_Content) ||
        m_Local.m_Mod != "hd2.base-campaign-coop" || m_Local.m_Kind != E_CampaignSessionKind::CampaignCoop ||
        m_Local.m_Major != kMajor || m_Local.m_MinMinor != kMinor || m_Local.m_MaxMinor != kMinor ||
        (m_Local.m_Offered & ~kSupportedFeatures) || (server && (!Id(session) || !Id(assignedPlayer))))) m_State = E_Admission::Rejected;
}
std::optional<T_Message> C_Admission::Start() const {
    if (!m_Server && m_State == E_Admission::AwaitAccept) return T_Message(m_Local);
    return std::nullopt;
}
S_Receive C_Admission::Reject(E_CampaignReason reason) {
    m_State = E_Admission::Rejected;
    return {reason, T_Message(S_Reject{reason}), std::nullopt};
}
E_CampaignReason C_Admission::Tick(std::uint64_t nowMs) {
    if (m_State == E_Admission::AwaitHello || m_State == E_Admission::AwaitAccept || m_State == E_Admission::AwaitAck) {
        if (nowMs < m_Started || nowMs - m_Started >= kTimeoutMs) { m_State = E_Admission::Rejected; return E_CampaignReason::Timeout; }
    }
    return E_CampaignReason::None;
}
void C_Admission::Close() { m_State = E_Admission::Closed; }
S_Receive C_Admission::Receive(const T_Bytes &frame, std::uint64_t nowMs) {
    if (m_State == E_Admission::Dormant || m_State == E_Admission::Closed || m_State == E_Admission::Rejected)
        return {E_CampaignReason::NotAdmitted, std::nullopt, std::nullopt};
    if (Tick(nowMs) != E_CampaignReason::None) return Reject(E_CampaignReason::Timeout);
    if (nowMs < m_Window) return Reject(E_CampaignReason::Malformed);
    if (nowMs - m_Window >= 1000) { m_Window = nowMs; m_Frames = 0; }
    if (++m_Frames > kFramesPerSecond) return Reject(E_CampaignReason::Malformed);
    S_FrameInfo info;
    auto reason = Inspect(frame.data(), frame.size(), info);
    if (reason != E_CampaignReason::None) return Reject(reason);
    // Header whitelist BEFORE payload decode/allocation or model access.
    if (m_State != E_Admission::Admitted && info.m_Type != E_Message::Hello && info.m_Type != E_Message::Accept &&
        info.m_Type != E_Message::NegotiationAck && info.m_Type != E_Message::Reject) return Reject(E_CampaignReason::Malformed);
    T_Message message;
    reason = Decode(frame, message);
    if (reason != E_CampaignReason::None) return Reject(reason);
    if (auto *rejection = std::get_if<S_Reject>(&message)) {
        m_State = E_Admission::Rejected;
        return {rejection->m_Reason, std::nullopt, std::nullopt};
    }
    if (m_Server && (m_State == E_Admission::AwaitHello || m_State == E_Admission::AwaitAck)) {
        if (auto *hello = std::get_if<S_Hello>(&message)) {
            if (hello->m_Mod != m_Local.m_Mod || hello->m_Kind != E_CampaignSessionKind::CampaignCoop) return Reject(E_CampaignReason::ModRequired);
            if (hello->m_Major != kMajor || hello->m_MinMinor > kMinor || hello->m_MaxMinor < kMinor) return Reject(E_CampaignReason::UnsupportedVersion);
            const auto features = hello->m_Offered & m_Local.m_Offered & kSupportedFeatures;
            if ((hello->m_Required & ~features) || (m_Local.m_Required & ~features)) return Reject(E_CampaignReason::MissingFeature);
            if (!SameContent(hello->m_Content, m_Local.m_Content, false)) return Reject(E_CampaignReason::ContentMismatch);
            if (!SameContent(hello->m_Content, m_Local.m_Content, true)) return Reject(E_CampaignReason::RulesMismatch);
            if (m_State == E_Admission::AwaitAck && features != m_Agreement.m_Features) return Reject(E_CampaignReason::Malformed);
            m_Agreement.m_Features = features;
            m_State = E_Admission::AwaitAck;
            return {E_CampaignReason::None, T_Message(m_Agreement), std::nullopt};
        }
        if (m_State == E_Admission::AwaitAck) {
            if (auto *ack = std::get_if<S_NegotiationAck>(&message); ack && Matches(*ack, m_Agreement)) {
                m_State = E_Admission::Admitted;
                return {};
            }
        }
        return Reject(E_CampaignReason::Malformed);
    }
    if (!m_Server && m_State == E_Admission::AwaitAccept) {
        auto *accept = std::get_if<S_Accept>(&message);
        if (!accept) return Reject(E_CampaignReason::Malformed);
        if (accept->m_Major != kMajor || accept->m_Minor != kMinor) return Reject(E_CampaignReason::UnsupportedVersion);
        if ((accept->m_Features & ~m_Local.m_Offered) || (accept->m_Features & ~kSupportedFeatures) ||
            (m_Local.m_Required & ~accept->m_Features)) return Reject(E_CampaignReason::MissingFeature);
        if (!SameContent(accept->m_Content, m_Local.m_Content, false)) return Reject(E_CampaignReason::ContentMismatch);
        if (!SameContent(accept->m_Content, m_Local.m_Content, true)) return Reject(E_CampaignReason::RulesMismatch);
        m_Agreement = *accept;
        m_State = E_Admission::Admitted;
        // Caller must enqueue this reliable ack BEFORE using any admitted payload.
        return {E_CampaignReason::None, T_Message(Ack(*accept)), std::nullopt};
    }
    if (m_State == E_Admission::Admitted) {
        if (m_Server) {
            if (auto *ack = std::get_if<S_NegotiationAck>(&message); ack && Matches(*ack, m_Agreement)) return {};
        } else if (auto *accept = std::get_if<S_Accept>(&message)) {
            if (Matches(Ack(*accept), m_Agreement) && SameContent(accept->m_Content, m_Agreement.m_Content, false) && SameContent(accept->m_Content, m_Agreement.m_Content, true))
                return {E_CampaignReason::None, T_Message(Ack(*accept)), std::nullopt};
        }
        if (!BoundPayload(message, m_Server, m_Agreement)) return Reject(E_CampaignReason::Malformed);
        return {E_CampaignReason::None, std::nullopt, std::move(message)};
    }
    return Reject(E_CampaignReason::Malformed);
}
E_CampaignReason C_InputGate::Validate(const S_Input &input, PlayerId sender,
    const S_CampaignSessionContext &context, const S_CampaignOwnershipRecord &soldier,
    bool admitted, bool reconstructionComplete) {
    if (!admitted) return E_CampaignReason::NotAdmitted;
    if (context.m_Kind != E_CampaignSessionKind::CampaignCoop || !context.m_IsTransportServer ||
        context.m_Phase != E_CampaignPhase::Running || !reconstructionComplete) return E_CampaignReason::InvalidPhase;
    if (!Valid(input) || !Valid(input.m_Control) || !Valid(soldier)) return E_CampaignReason::Malformed;
    const auto &token = input.m_Control;
    if (context.m_Session != token.m_Session) return E_CampaignReason::WrongSession;
    if (context.m_MissionGeneration != token.m_Generation) return E_CampaignReason::WrongGeneration;
    if (sender != token.m_Player || !soldier.m_Owner || *soldier.m_Owner != sender || token.m_Soldier != soldier.m_Soldier) return E_CampaignReason::NotOwner;
    if (soldier.m_Scripted) return E_CampaignReason::Scripted;
    if (!soldier.m_Alive) return E_CampaignReason::Dead;
    if (!soldier.m_Eligible || !soldier.m_Deployed) return E_CampaignReason::Ineligible;
    if (!soldier.m_Actor || *soldier.m_Actor != token.m_Actor) return E_CampaignReason::ActorUnavailable;
    if (soldier.m_ControlRevision != token.m_ControlRevision) return E_CampaignReason::StaleRevision;
    if (m_Control && SameControl(*m_Control, token) && input.m_Sequence <= m_Sequence) return E_CampaignReason::StaleRevision;
    m_Control = token;
    m_Sequence = input.m_Sequence;
    return E_CampaignReason::None;
}
void C_InputGate::Reset() { m_Control.reset(); m_Sequence = 0; }
void C_PreparationReplica::Observe(PreparationGeneration generation, ViewRevision view) {
    if (m_MinGeneration < generation) { m_MinGeneration = generation; m_MinView = view; }
    else if (generation == m_MinGeneration && m_MinView < view) m_MinView = view;
}
E_CampaignReason C_PreparationReplica::Install(const S_CampaignPreparationSnapshot &snapshot) {
    T_Bytes encoded;
    if (Encode(T_Message(snapshot), encoded) != E_CampaignReason::None) return E_CampaignReason::Malformed;
    if (snapshot.m_Context.m_Session != m_Session) return E_CampaignReason::WrongSession;
    if (snapshot.m_Sync != E_CampaignSyncStatus::Current || snapshot.m_Connection != E_CampaignConnectionStatus::Admitted) return E_CampaignReason::SnapshotUnavailable;
    if (m_Snapshot && snapshot.m_Generation < m_Snapshot->m_Generation) return E_CampaignReason::WrongGeneration;
    if (snapshot.m_Generation < m_MinGeneration ||
        (snapshot.m_Generation == m_MinGeneration && snapshot.m_ViewRevision < m_MinView)) {
        // A delayed full snapshot cannot erase knowledge of a newer publication
        // already observed in a gapped delta. Keep editing disabled until >=floor.
        if (!m_Current) return E_CampaignReason::SnapshotGap;
        return E_CampaignReason::StaleRevision;
    }
    if (m_Snapshot) {
        if (snapshot.m_Generation == m_Snapshot->m_Generation) {
            if (snapshot.m_ViewRevision < m_Snapshot->m_ViewRevision) return E_CampaignReason::StaleRevision;
            if (snapshot.m_ViewRevision == m_Snapshot->m_ViewRevision) {
                T_Bytes previous;
                if (Encode(T_Message(*m_Snapshot), previous) != E_CampaignReason::None || previous != encoded) return E_CampaignReason::Malformed;
                m_Current = true;
                return E_CampaignReason::None;
            }
            if (snapshot.m_PreparationRevision < m_Snapshot->m_PreparationRevision || snapshot.m_OwnershipRevision < m_Snapshot->m_OwnershipRevision ||
                snapshot.m_Context.m_MissionGeneration < m_Snapshot->m_Context.m_MissionGeneration) {
                m_Current = false;
                Observe(snapshot.m_Generation, snapshot.m_ViewRevision);
                return E_CampaignReason::SnapshotGap;
            }
        }
    }
    m_Snapshot = std::make_shared<const S_CampaignPreparationSnapshot>(snapshot);
    Observe(snapshot.m_Generation, snapshot.m_ViewRevision);
    m_Current = true;
    return E_CampaignReason::None;
}
E_CampaignReason C_PreparationReplica::Apply(const S_PreparationDelta &delta) {
    T_Bytes bytes;
    if (Encode(T_Message(delta), bytes) != E_CampaignReason::None) return E_CampaignReason::Malformed;
    if (delta.m_Replacement.m_Context.m_Session != m_Session) return E_CampaignReason::WrongSession;
    if (m_Snapshot && delta.m_Replacement.m_Generation < m_Snapshot->m_Generation) return E_CampaignReason::WrongGeneration;
    if (m_Snapshot && delta.m_Replacement.m_Generation == m_Snapshot->m_Generation && !(m_Snapshot->m_ViewRevision < delta.m_Replacement.m_ViewRevision)) return E_CampaignReason::StaleRevision;
    if (!m_Current || !m_Snapshot || delta.m_Replacement.m_Generation != m_Snapshot->m_Generation || delta.m_Base != m_Snapshot->m_ViewRevision) {
        m_Current = false;
        Observe(delta.m_Replacement.m_Generation, delta.m_Replacement.m_ViewRevision);
        return E_CampaignReason::SnapshotGap;
    }
    return Install(delta.m_Replacement);
}
S_CampaignPreparationView C_PreparationReplica::GetView() const {
    if (!m_Current) return {E_CampaignSyncStatus::Resyncing, E_CampaignReason::SnapshotGap, nullptr};
    return {E_CampaignSyncStatus::Current, E_CampaignReason::None, m_Snapshot};
}
E_CampaignReason C_WorldSender::Begin(const S_CampaignSnapshotAck &baseline, T_Bytes world, std::uint64_t nowMs)
{
    using R = E_CampaignReason;
    if (m_Count != 0)
        return R::PendingTransaction;
    if (!Valid(baseline) || world.empty() || world.size() > kMaxWorldBytes)
        return R::Malformed;
    const auto count = (world.size() + kMaxChunkBytes - 1) / kMaxChunkBytes;
    m_Baseline = baseline;
    m_Crc = Crc32(world);
    m_World = std::move(world);
    m_Count = static_cast<std::uint32_t>(count);
    m_Started = m_LastPump = m_Window = nowMs;
    return R::None;
}

E_CampaignReason C_WorldSender::Pump(std::uint64_t nowMs, const T_SendChunk &send)
{
    using R = E_CampaignReason;
    constexpr std::uint32_t chunksPerSecond = 96;
    constexpr std::uint32_t chunksPerPump = 4;
    if (!m_Count || !send)
        return R::InvalidPhase;
    if (IsComplete())
        return R::None;
    if (nowMs < m_LastPump || nowMs - m_Started >= kTimeoutMs) {
        Reset();
        return R::Timeout;
    }

    m_LastPump = nowMs;
    if (nowMs - m_Window >= 1000) {
        m_Window = nowMs;
        m_SentInWindow = 0;
    }
    const auto due = static_cast<std::uint32_t>((nowMs - m_Started) * chunksPerSecond / 1000) + 1;
    const auto limit = (std::min)({due, m_Next + chunksPerPump, m_Count,
                                 m_Next + chunksPerSecond - m_SentInWindow});
    while (m_Next < limit) {
        const auto offset = m_Next * kMaxChunkBytes;
        const auto size = (std::min)(kMaxChunkBytes, m_World.size() - offset);
        const auto copied = m_World.data() + offset;
        S_WorldChunk chunk{
            m_Baseline, static_cast<std::uint32_t>(m_World.size()), m_Next, m_Count, m_Crc,
            T_Bytes(copied, copied + size)
        };
        if (!send(chunk)) {
            Reset();
            return R::Disconnected;
        }
        ++m_Next;
        ++m_SentInWindow;
    }
    if (IsComplete())
        m_World = T_Bytes{};
    return R::None;
}

void C_WorldSender::Reset()
{
    m_World = T_Bytes{};
    m_Count = m_Next = m_Crc = m_SentInWindow = 0;
}

E_CampaignReason C_WorldAssembler::Add(const S_WorldChunk &chunk, const S_CampaignSnapshotAck &expected, std::uint64_t nowMs) {
    if (!Valid(chunk) || !Valid(chunk.m_Baseline) || !Valid(expected)) return E_CampaignReason::Malformed;
    if (!SameBaseline(chunk.m_Baseline, expected)) return E_CampaignReason::WrongGeneration;
    if (Tick(nowMs) != E_CampaignReason::None) return E_CampaignReason::Timeout;
    if (!m_Meta) {
        m_Meta = chunk;
        m_Meta->m_Bytes.clear();
        m_Bytes.resize(chunk.m_TotalBytes);
        m_Received.assign(chunk.m_Count, false);
        m_Started = nowMs;
    } else if (!SameBaseline(m_Meta->m_Baseline, chunk.m_Baseline) || m_Meta->m_TotalBytes != chunk.m_TotalBytes ||
        m_Meta->m_Count != chunk.m_Count || m_Meta->m_Crc32 != chunk.m_Crc32) return E_CampaignReason::Malformed;
    const auto offset = static_cast<std::size_t>(chunk.m_Index) * kMaxChunkBytes;
    auto begin = m_Bytes.begin() + static_cast<std::ptrdiff_t>(offset);
    if (m_Received[chunk.m_Index]) {
        return std::equal(chunk.m_Bytes.begin(), chunk.m_Bytes.end(), begin) ? E_CampaignReason::None : E_CampaignReason::RequestConflict;
    }
    std::copy(chunk.m_Bytes.begin(), chunk.m_Bytes.end(), begin);
    m_Received[chunk.m_Index] = true;
    if (++m_ReceivedCount == chunk.m_Count) {
        if (Crc32(m_Bytes) != chunk.m_Crc32) { Reset(); return E_CampaignReason::Malformed; }
        m_Complete = true;
    }
    return E_CampaignReason::None;
}
std::optional<T_Bytes> C_WorldAssembler::Take() {
    if (!m_Complete) return std::nullopt;
    T_Bytes result = std::move(m_Bytes);
    Reset();
    return result;
}
E_CampaignReason C_WorldAssembler::Tick(std::uint64_t nowMs) {
    if (m_Meta && (nowMs < m_Started || nowMs - m_Started >= kTimeoutMs)) { Reset(); return E_CampaignReason::Timeout; }
    return E_CampaignReason::None;
}
void C_WorldAssembler::Reset() { m_Meta.reset(); m_Bytes.clear(); m_Received.clear(); m_ReceivedCount = 0; m_Started = 0; m_Complete = false; }
} // namespace CampaignProtocol
