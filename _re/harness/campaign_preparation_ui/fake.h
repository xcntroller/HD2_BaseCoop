#pragma once
#include "game/campaign_preparation_screen.h"
#include <algorithm>
#include <type_traits>
#include <deque>
#include <map>
#include <stdexcept>

// Synthetic fixture, never a production equipment validator or ownership authority.
class C_FakePreparation final : public I_CampaignPreparation, public I_CampaignPreparationPresentation {
public:
    S_CampaignPreparationSnapshot m_State;
    E_CampaignSyncStatus m_Status = E_CampaignSyncStatus::Current;
    std::deque<T_CampaignPreparationEvent> m_Events;
    std::optional<S_CampaignEditRequest> m_Edit;
    std::optional<S_CampaignReadyRequest> m_Ready;
    std::optional<S_CampaignOwnershipRequest> m_Ownership;
    std::optional<S_CampaignDeployRequest> m_Deploy;
    std::uint64_t m_Next = 1, m_NextToken = 1, m_NextGeneration = 1;
    LocalSubmissionId m_LastToken;
    int m_Submissions = 0, m_Resyncs = 0, m_Polls = 0;
    std::map<std::string, std::string> m_Labels{
        {"COOP_PREP_TITLE", "CO-OP CAMPAIGN / PREPARATION"},
        {"COOP_PREP_RECEIVING", "Receiving preparation - actions unavailable"},
        {"COOP_PREP_DISCONNECTED", "Disconnected - actions unavailable"},
        {"COOP_PREP_FROZEN", "Preparation frozen / mission loading"},
        {"COOP_PREP_PENDING", "Request pending - committed equipment shown"},
        {"COOP_PREP_COMMITTED", "Committed equipment / fixture policy: all deployed humans ready"},
        {"COOP_OWNER_AI", "AI"}, {"COOP_OWNER_SCRIPTED", "Script controlled"}, {"COOP_OWNER_HUMAN", "Human"},
        {"COOP_OWNER_EDITABLE", "Equipment available"}, {"COOP_OWNER_READ_ONLY", "READ ONLY"},
        {"COOP_OWNER_EDIT_OWN", "Editing your soldier"}, {"COOP_OWNER_EDIT_AI", "Editing AI soldier"},
        {"COOP_SLOT_UNAVAILABLE", "Unavailable"}, {"COOP_SLOT_CLAIM", "Claim"}, {"COOP_SLOT_RELEASE", "Release"},
        {"COOP_BRIEFING_TITLE", "MISSION BRIEFING"}, {"COOP_BRIEFING_PAGE", "Open briefing"}, {"COOP_EQUIP_PAGE", "Equipment"},
        {"COOP_OWNER_NO_SOLDIER", "No assigned soldier / spectating. Host may equip eligible AI."}, {"COOP_PAGE_PREVIOUS", "Previous page"}, {"COOP_PAGE_NEXT", "Next page"}, {"COOP_SKILLS", "Skills / equipment"}, {"COOP_PAGE_KEYS", "Arrow keys: page"},
        {"COOP_EQUIP_NO_DESTINATION", "No valid destination"}, {"COOP_EQUIP_TAKE", "Equip"}, {"COOP_EQUIP_RETURN", "Return"},
        {"COOP_EQUIP_WEIGHT", "Weight"}, {"COOP_EQUIP_BELT_PACK", "Belt / backpack"}, {"COOP_EQUIP_STOCK", "SHARED MILITARY STOCK"},
        {"COOP_ESSENTIAL_DETAILS", "Essentials / restrictions"}, {"COOP_ESSENTIAL_TITLE", "MISSION ESSENTIALS:"}, {"COOP_READY_SET", "Ready"}, {"COOP_READY_UNSET", "Not ready"},
        {"COOP_READY_YES", "Ready"}, {"COOP_READY_NO", "Preparing"}, {"COOP_READY_DEPLOY", "Deploy"}, {"COOP_READY_LEAVE", "Leave"},
        {"COOP_READY_CONFIRM_LEAVE", "Confirm leave"}, {"COOP_READY_CANCEL", "Cancel"},
        {"COOP_READY_HOST_LEAVE", "End this fixture session for everyone?"}, {"COOP_READY_GUEST_LEAVE", "Leave preparation?"}
    };
    C_FakePreparation() { Reset(); }
    void Reset() {
        m_Edit.reset(); m_Ready.reset(); m_Ownership.reset(); m_Deploy.reset();
        m_State = {}; m_Status = E_CampaignSyncStatus::Current; m_Events.clear();
        auto &s = m_State;
        s.m_Context.m_Kind = E_CampaignSessionKind::CampaignCoop; s.m_Context.m_Session = SessionId(1);
        s.m_Context.m_LocalPlayer = PlayerId(1); s.m_Context.m_HostPlayer = PlayerId(1);
        s.m_Context.m_Phase = E_CampaignPhase::Preparation; s.m_Context.m_UsesCampaignRules = true;
        s.m_Generation = PreparationGeneration(m_NextGeneration++); s.m_PreparationRevision = PreparationRevision(1);
        s.m_ViewRevision = ViewRevision(1); s.m_OwnershipRevision = OwnershipRevision(1);
        s.m_Connection = E_CampaignConnectionStatus::Admitted; s.m_Sync = E_CampaignSyncStatus::Current;
        s.m_Briefing = {TextKey("1"), TextKey("2"), TextKey("3"), AssetKey("1")};
        s.m_Participants = {{PlayerId(1), "Morgan (host)", true, true, {}}, {PlayerId(2), "Ellis (guest)", true, true, {}}};
        for (std::uint64_t i = 1; i <= 4; ++i) {
            S_CampaignSoldierPresentation soldier;
            soldier.m_Soldier = SoldierId(i); soldier.m_DisplayName = std::array<const char *,4>{"Arthur Hale", "Daniel Reed", "Peter Shaw", "Samuel Ward"}[static_cast<std::size_t>(i-1)];
            soldier.m_Portrait = AssetKey(std::to_string(i)); soldier.m_Preview = AssetKey(std::to_string(i));
            soldier.m_Ownership = {SoldierId(i), true, true, true, {}, {}, false, ControlRevision(1)};
            if (i <= 2) soldier.m_Ownership.m_Owner = PlayerId(i);
            soldier.m_CanView = {true, E_CampaignReason::None};
            s.m_Slots[static_cast<std::size_t>(i-1)].m_Soldier = SoldierId(i); s.m_Soldiers.push_back(soldier);
            S_CampaignSoldierLoadout loadout; loadout.m_Soldier = SoldierId(i);
            loadout.m_Weight = 7; loadout.m_WeightCapacity = 30; loadout.m_BeltCapacity = 8;
            loadout.m_BackpackCapacity = 18; loadout.m_BackpackWeightCapacity = 15;
            loadout.m_Skills = {{SkillKey("1"), 63.5f}, {SkillKey("2"), 80.25f}};
            loadout.m_Items.push_back({PreparationItemId(i), {ItemCatalogKey("1"), 1, {}, {}, {}, {}}, {PlacementKey("1"), 0}});
            s.m_Loadouts.push_back(loadout);
        }
        S_CampaignStockEntry stock; stock.m_Entry = StockEntryId(1); stock.m_State = {ItemCatalogKey("2"), 1, {}, {}, {}, {}};
        for (std::uint64_t i = 1; i <= 4; ++i) stock.m_Destinations.push_back({SoldierId(i), {PlacementKey("1"), PlacementKey("2")}, {true, E_CampaignReason::None}});
        s.m_Stock.push_back(stock); s.m_Essentials.push_back({ItemCatalogKey("2"), 1, 0, 1, {SoldierId(1), SoldierId(3)}, {}});
        s.m_Policies.m_Decisions.push_back({E_CampaignPolicy::DeploymentQuorum, E_CampaignPolicyState::Permitted, TextKey("fixture.all-deployed-humans")});
        s.m_Policies.m_Decisions.push_back({E_CampaignPolicy::SoldierSelection, E_CampaignPolicyState::Permitted, TextKey("fixture.release-then-claim")});
        Rights();
    }
    void Rights() {
        for (auto &s : m_State.m_Soldiers) {
            const auto owner = s.m_Ownership.m_Owner;
            const bool own = owner && owner == m_State.m_Context.m_LocalPlayer;
            s.m_CanEdit = {own || (!owner && m_State.m_Context.m_LocalPlayer == m_State.m_Context.m_HostPlayer), E_CampaignReason::NotOwner};
            if (s.m_CanEdit.m_Allowed) s.m_CanEdit.m_Reason = E_CampaignReason::None;
            s.m_CanClaim = {!owner && !HasOwn(), E_CampaignReason::AlreadyControlsSoldier};
            s.m_CanRelease = {own, E_CampaignReason::NotOwner};
            if (s.m_Ownership.m_Scripted) s.m_CanEdit = {false, E_CampaignReason::Scripted};
        }
        bool all = true;
        for (const auto &p : m_State.m_Participants) if (p.m_Connected && p.m_ReadyAt != m_State.m_PreparationRevision) all = false;
        const bool missing = m_State.m_Essentials.front().m_Deficit != 0;
        m_State.m_CanDeploy = {all && !missing, missing ? E_CampaignReason::RequiredItemsMissing : E_CampaignReason::PolicyRequired};
        m_State.m_DeployBlockers.clear();
        if (!m_State.m_CanDeploy.m_Allowed) { S_CampaignFinding f; f.m_Reason = m_State.m_CanDeploy.m_Reason; m_State.m_DeployBlockers.push_back(f); }
    }
    bool HasOwn() const {
        for (const auto &s : m_State.m_Soldiers) if (s.m_Ownership.m_Owner == m_State.m_Context.m_LocalPlayer) return true;
        return false;
    }
    void Publish(bool equipment = false) {
        m_State.m_ViewRevision = ViewRevision(m_State.m_ViewRevision.Value() + 1);
        if (equipment) {
            m_State.m_PreparationRevision = PreparationRevision(m_State.m_PreparationRevision.Value() + 1);
            for (auto &p : m_State.m_Participants) p.m_ReadyAt.reset();
        }
        Rights(); m_Events.push_back(S_CampaignSnapshotChanged{GetSnapshot().m_Snapshot});
    }
    S_CampaignPreparationView GetSnapshot() const override {
        return {m_Status, E_CampaignReason::None, m_Status == E_CampaignSyncStatus::Loading ? nullptr : std::make_shared<const S_CampaignPreparationSnapshot>(m_State)};
    }
    T_CampaignSubmission SubmitEdit(const S_CampaignEditRequest &r) override { m_Edit = r; return Queue(); }
    T_CampaignSubmission SubmitOwnership(const S_CampaignOwnershipRequest &r) override { m_Ownership = r; return Queue(); }
    T_CampaignSubmission SetReady(const S_CampaignReadyRequest &r) override { m_Ready = r; return Queue(); }
    T_CampaignSubmission RequestDeploy(const S_CampaignDeployRequest &r) override { m_Deploy = r; return Queue(); }
    T_CampaignSubmission Queue() { ++m_Submissions; m_LastToken = LocalSubmissionId(m_NextToken++); return S_CampaignQueuedSubmission{m_LastToken}; }
    void RequestResync() override { ++m_Resyncs; m_Status = E_CampaignSyncStatus::Resyncing; }
    std::optional<T_CampaignPreparationEvent> PollEvent() override {
        ++m_Polls; if (m_Events.empty()) return {}; auto event = m_Events.front(); m_Events.pop_front(); return event;
    }
    S_CampaignRequestResultEvent Result(E_CampaignReason reason = E_CampaignReason::None) const {
        S_CampaignRequestResult r;
        r.m_Context = m_Edit ? m_Edit->m_Context.m_Request : m_Ready ? m_Ready->m_Context.m_Request : m_Ownership ? m_Ownership->m_Context : m_Deploy->m_Context.m_Request;
        r.m_Family = m_Edit ? E_CampaignRequestFamily::Edit : m_Ready ? E_CampaignRequestFamily::Ready : m_Ownership ? E_CampaignRequestFamily::Ownership : E_CampaignRequestFamily::Deploy;
        r.m_PreparationGeneration = m_State.m_Generation; r.m_Reason = reason;
        r.m_OwnershipRevision = m_State.m_OwnershipRevision; r.m_PreparationRevision = m_State.m_PreparationRevision; r.m_ViewRevision = m_State.m_ViewRevision;
        r.m_Outcome = reason == E_CampaignReason::None ? E_CampaignRequestOutcome::Accepted : E_CampaignRequestOutcome::Rejected;
        r.m_State = GetSnapshot().m_Snapshot;
        return {m_LastToken, r};
    }
    void Resolve(E_CampaignReason reason = E_CampaignReason::None) {
        if (!m_Edit && !m_Ready && !m_Ownership && !m_Deploy) return;
        if (reason == E_CampaignReason::None && m_Edit) {
            const auto *take = std::get_if<S_CampaignTakeFromStock>(&m_Edit->m_Operation);
            const auto *ret = std::get_if<S_CampaignReturnToStock>(&m_Edit->m_Operation);
            if (take) {
                auto &l = m_State.m_Loadouts.at(static_cast<std::size_t>(take->m_Soldier.Value()-1));
                if (m_State.m_Stock[0].m_State.m_Quantity < take->m_Count) reason = E_CampaignReason::StockUnavailable;
                else { m_State.m_Stock[0].m_State.m_Quantity -= take->m_Count;
                    l.m_Items.push_back({PreparationItemId(m_Next + 100), {ItemCatalogKey("2"), take->m_Count, {}, {}, {}, {}}, take->m_Destination}); }
            } else if (ret) {
                auto &l = m_State.m_Loadouts.at(static_cast<std::size_t>(ret->m_Soldier.Value()-1));
                auto it = std::find_if(l.m_Items.begin(), l.m_Items.end(), [ret](const auto &v){return v.m_Entry == ret->m_Entry;});
                if (it == l.m_Items.end() || it->m_State.m_Item != ItemCatalogKey("2") || it->m_State.m_Quantity < ret->m_Count) reason = E_CampaignReason::InvalidCount;
                else { it->m_State.m_Quantity -= ret->m_Count; m_State.m_Stock[0].m_State.m_Quantity += ret->m_Count; if (!it->m_State.m_Quantity) l.m_Items.erase(it); }
            }
            std::uint32_t carried = 0;
            for (const auto &l : m_State.m_Loadouts) for (const auto &v : l.m_Items) if (v.m_State.m_Item == ItemCatalogKey("2")) carried += v.m_State.m_Quantity;
            m_State.m_Essentials[0].m_Carried = carried; m_State.m_Essentials[0].m_Deficit = carried ? 0 : 1;
            if (reason == E_CampaignReason::None) Publish(true);
        } else if (reason == E_CampaignReason::None && m_Ready) {
            for (auto &p : m_State.m_Participants) if (p.m_Player == m_Ready->m_Context.m_Request.m_PlayerEcho)
                p.m_ReadyAt = m_Ready->m_Ready ? std::optional<PreparationRevision>(m_State.m_PreparationRevision) : std::nullopt;
            Publish();
        } else if (reason == E_CampaignReason::None && m_Ownership) {
            std::visit([this](const auto &op) {
                using T = std::decay_t<decltype(op)>;
                if constexpr (!std::is_same_v<T, S_CampaignSwitch>) {
                    auto &s = m_State.m_Soldiers.at(static_cast<std::size_t>(op.m_Target.m_Soldier.Value()-1));
                    if constexpr (std::is_same_v<T, S_CampaignClaim>) s.m_Ownership.m_Owner = m_State.m_Context.m_LocalPlayer;
                    else s.m_Ownership.m_Owner.reset();
                    s.m_Ownership.m_ControlRevision = ControlRevision(s.m_Ownership.m_ControlRevision.Value()+1);
                }
            }, m_Ownership->m_Operation);
            m_State.m_OwnershipRevision = OwnershipRevision(m_State.m_OwnershipRevision.Value()+1); Publish(true);
        } else if (reason == E_CampaignReason::None && m_Deploy) { m_State.m_Context.m_Phase = E_CampaignPhase::Frozen; Publish(); }
        m_Events.push_back(Result(reason)); m_Edit.reset(); m_Ready.reset(); m_Ownership.reset(); m_Deploy.reset();
    }
    RequestId NextRequestId() override { return RequestId(m_Next++); }
    std::string Text(TextKey key) const override {
        if (key == TextKey("1")) return "OPERATION LANTERN (SYNTHETIC FIXTURE)";
        if (key == TextKey("2")) return "Night reconnaissance / difficulty: campaign";
        return "Cross the valley, observe the depot and recover the field documents. Keep the original squad together. One medical kit is essential. Synthetic mission and equipment; no connected game or approved deployment policy.";
    }
    std::string Item(ItemCatalogKey key) const override { return key == ItemCatalogKey("1") ? "Service rifle" : "Medical kit"; }
    std::string Placement(PlacementKey key) const override { return key == PlacementKey("1") ? "Belt" : "Backpack"; }
    std::string Skill(SkillKey key) const override { return key == SkillKey("1") ? "Shooting" : "Endurance"; }
    std::string Label(const std::string &key) const override { auto it = m_Labels.find(key); return it == m_Labels.end() ? key : it->second; }
    std::string Reason(E_CampaignReason reason) const override {
        switch (reason) {
        case E_CampaignReason::None: return "";
        case E_CampaignReason::InvalidItem: return "Select a carried item to return";
        case E_CampaignReason::InvalidCount: return "Choose a quantity carried by this soldier";
        case E_CampaignReason::PendingTransaction: return "Wait for the pending request";
        case E_CampaignReason::SnapshotUnavailable: return "Receiving a complete preparation snapshot";
        case E_CampaignReason::Disconnected: return "Disconnected - requests unavailable";
        case E_CampaignReason::Frozen: return "Preparation is frozen";
        case E_CampaignReason::Ineligible: return "No eligible assigned soldier";
        case E_CampaignReason::NotOwner: return "Read only - another human equips this soldier";
        case E_CampaignReason::StaleRevision: return "Equipment changed; review and try again";
        case E_CampaignReason::StockUnavailable: return "Item no longer available";
        case E_CampaignReason::MissionRestriction: return "Mission permits one deployed soldier";
        case E_CampaignReason::RequiredItemsMissing: return "Missing mission essential: medical kit";
        case E_CampaignReason::Scripted: return "Script controlled - equipment unavailable";
        case E_CampaignReason::PolicyRequired: return "Deployment policy unresolved / waiting for fixture readiness";
        case E_CampaignReason::AlreadyControlsSoldier: return "Release your soldier before claiming another (fixture policy)";
        default: return "Request unavailable / reason " + std::to_string(static_cast<int>(reason));
        }
    }
};
