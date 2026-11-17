// Interface conformance fixture only: no production ownership/equipment logic.
#include "../../../src/game/campaign_contracts.h"
#include <cassert>
#include <deque>
#include <iostream>
#include <tuple>
#include <type_traits>

#ifdef _MSC_VER
static_assert(sizeof(void *) == 4 && _M_IX86, "COOP-011 target check requires MSVC x86");
#endif

template<class T, class... TOther> constexpr bool DistinctFrom() {
    return ((!std::is_same_v<T, TOther> && !std::is_convertible_v<T, TOther> &&
        !std::is_convertible_v<TOther, T>) && ...);
}
template<class... T> struct S_Distinct;
template<> struct S_Distinct<> : std::true_type {};
template<class T, class... TRest> struct S_Distinct<T, TRest...>
    : std::bool_constant<DistinctFrom<T, TRest...>() && S_Distinct<TRest...>::value> {};
static_assert(S_Distinct<SessionId, CampaignId, MissionGeneration, PreparationGeneration,
    SoldierId, PlayerId, RequestId, OwnershipRevision, ControlRevision,
    PreparationRevision, ViewRevision, SnapshotId, LocalSubmissionId,
    PreparationItemId, StockEntryId, ActorId, ActorSlotUse>::value);
static_assert(S_Distinct<MissionKey, ItemCatalogKey, PlacementKey, PresetKey,
    SoldierProfileKey, TextKey, AssetKey, SkillKey, ConstraintKey, AutoEquipPolicyKey>::value);
static_assert(!std::is_convertible_v<std::uint64_t, SoldierId>);
static_assert(!std::is_convertible_v<SoldierId, std::uint64_t>);
static_assert(!std::is_convertible_v<std::string, MissionKey>);
static_assert(!std::is_convertible_v<ActorRef, SoldierId>);
static_assert(!std::is_convertible_v<ActorRef, PlayerId>);
static_assert(!SoldierId{}.IsValid() && SoldierId{1}.IsValid());
static_assert(ActorId{0}.Value() == 0 && ActorSlotUse{0}.Value() == 0);
using T_SnapshotReference = decltype(*std::declval<T_CampaignPreparationSnapshot>());
static_assert(std::is_const_v<std::remove_reference_t<T_SnapshotReference>>);
static_assert(!std::is_assignable_v<decltype((std::declval<T_CampaignPreparationSnapshot>()->m_Stock)),
    std::vector<S_CampaignStockEntry>>);
static_assert(std::is_same_v<decltype(std::declval<T_CampaignPreparationSnapshot>()->m_Slots),
    std::array<S_CampaignPresentationSlot, 4>>);
static_assert(std::has_virtual_destructor_v<I_CampaignPreparation>);
static_assert(std::is_abstract_v<I_CampaignPreparation>);

using T_RecordedRequest = std::variant<S_CampaignEditRequest, S_CampaignOwnershipRequest,
    S_CampaignReadyRequest, S_CampaignDeployRequest>;
class C_FakePreparation final : public I_CampaignPreparation {
public:
    S_CampaignPreparationView GetSnapshot() const override { return m_View; }
    T_CampaignSubmission SubmitEdit(const S_CampaignEditRequest &r) override { return Queue(r); }
    T_CampaignSubmission SubmitOwnership(const S_CampaignOwnershipRequest &r) override { return Queue(r); }
    T_CampaignSubmission SetReady(const S_CampaignReadyRequest &r) override { return Queue(r); }
    T_CampaignSubmission RequestDeploy(const S_CampaignDeployRequest &r) override { return Queue(r); }
    void RequestResync() override {
        m_View = {E_CampaignSyncStatus::Resyncing, E_CampaignReason::SnapshotGap, {}};
        S_CampaignResyncDescriptor descriptor;
        descriptor.m_Reason = E_CampaignReason::SnapshotGap;
        m_Events.emplace_back(descriptor);
    }
    std::optional<T_CampaignPreparationEvent> PollEvent() override {
        if (m_Events.empty()) return std::nullopt;
        auto event = std::move(m_Events.front());
        m_Events.pop_front();
        return event;
    }
    void Publish(S_CampaignPreparationSnapshot value) {
        auto snapshot = std::make_shared<const S_CampaignPreparationSnapshot>(std::move(value));
        m_View = {E_CampaignSyncStatus::Current, E_CampaignReason::None, snapshot};
        m_Events.emplace_back(S_CampaignSnapshotChanged{snapshot});
    }
    void Deliver(LocalSubmissionId token, S_CampaignRequestResult result) {
        m_Events.emplace_back(S_CampaignRequestResultEvent{token, std::move(result)});
    }
    std::vector<T_RecordedRequest> m_Recorded; // fixture observations, never profile objects
private:
    T_CampaignSubmission Queue(T_RecordedRequest request) {
        if (m_View.m_Status != E_CampaignSyncStatus::Current)
            return S_CampaignLocalFailure{E_CampaignReason::SnapshotUnavailable};
        m_Recorded.push_back(std::move(request));
        return S_CampaignQueuedSubmission{LocalSubmissionId{++m_Sequence}};
    }
    std::uint64_t m_Sequence = 0;
    S_CampaignPreparationView m_View;
    std::deque<T_CampaignPreparationEvent> m_Events;
};
static_assert(!std::is_abstract_v<C_FakePreparation>);

class C_FakeOwnershipView final : public I_CampaignOwnershipView {
public:
    T_CampaignOwnershipSnapshot GetSnapshot() const override {
        return std::make_shared<const S_CampaignOwnershipSnapshot>(S_CampaignOwnershipSnapshot{});
    }
};
class C_FakePolicyView final : public I_CampaignPolicyView {
public:
    S_CampaignPolicySnapshot GetSnapshot() const override {
        return {{{E_CampaignPolicy::DeploymentQuorum, E_CampaignPolicyState::Unresolved,
            TextKey{"policy.readiness_unconfirmed"}}}};
    }
};
class C_FakeCatalogView final : public I_CampaignEquipmentCatalogView {
public:
    std::shared_ptr<const S_CampaignEquipmentCatalog> GetSnapshot() const override {
        S_CampaignEquipmentCatalog catalog;
        catalog.m_Items.push_back({ItemCatalogKey{"rifle"}, TextKey{"item.rifle"},
            TextKey{"item.rifle.description"}, AssetKey{"rifle.icon"}, AssetKey{"rifle.preview"},
            {PlacementKey{"hands"}}, {}});
        return std::make_shared<const S_CampaignEquipmentCatalog>(std::move(catalog));
    }
};

S_CampaignPreparationSnapshot MakeSnapshot() {
    S_CampaignPreparationSnapshot value;
    value.m_Context.m_Kind = E_CampaignSessionKind::CampaignCoop;
    value.m_Context.m_Session = SessionId{1};
    value.m_Context.m_Campaign = CampaignId{2};
    value.m_Context.m_Mission = MissionKey{"campaign/mission/step@content-v1"};
    value.m_Context.m_Phase = E_CampaignPhase::Preparation;
    value.m_Context.m_UsesCampaignRules = true;
    value.m_Context.m_IsTransportServer = true;
    value.m_Context.m_IsTransportClient = true;
    value.m_Context.m_LocalPlayer = PlayerId{3};
    value.m_Context.m_HostPlayer = PlayerId{3}; // host deliberately has NO soldier
    value.m_Context.m_Rules = {{"campaign-v1", "mission-v1", "roster-v1", "items-v1", "rules-v1"}, 1};
    value.m_Generation = PreparationGeneration{4};
    value.m_PreparationRevision = PreparationRevision{5};
    value.m_ViewRevision = ViewRevision{6};
    value.m_OwnershipRevision = OwnershipRevision{7};
    value.m_Connection = E_CampaignConnectionStatus::Admitted;
    value.m_Sync = E_CampaignSyncStatus::Current;
    value.m_Briefing = {TextKey{"operation"}, TextKey{"mission"}, TextKey{"briefing"}, AssetKey{"briefing.image"}};

    S_CampaignSoldierPresentation ai;
    ai.m_Soldier = SoldierId{8};
    ai.m_Profile = SoldierProfileKey{"roster/soldier-a"};
    ai.m_DisplayName = "Fixture AI soldier";
    ai.m_Portrait = AssetKey{"soldier-a.portrait"};
    ai.m_Preview = AssetKey{"soldier-a.preview"};
    ai.m_Ownership = {ai.m_Soldier, true, true, true, std::nullopt, std::nullopt, false, ControlRevision{9}};
    ai.m_CanView = {true, E_CampaignReason::None};
    ai.m_CanEdit = {true, E_CampaignReason::None}; // host can edit AI without claiming
    ai.m_CanClaim = {false, E_CampaignReason::PolicyRequired};
    ai.m_CanRelease = {false, E_CampaignReason::NotOwner};
    value.m_Soldiers.push_back(ai);
    auto guest = ai;
    guest.m_Soldier = SoldierId{10};
    guest.m_Profile = SoldierProfileKey{"roster/soldier-b"};
    guest.m_DisplayName = "Fixture guest soldier";
    guest.m_Ownership.m_Soldier = guest.m_Soldier;
    guest.m_Ownership.m_Owner = PlayerId{11};
    guest.m_Ownership.m_Scripted = true;
    guest.m_CanEdit = {false, E_CampaignReason::NotOwner};
    guest.m_CanClaim = {false, E_CampaignReason::Scripted};
    value.m_Soldiers.push_back(guest);
    value.m_Slots[0] = {ai.m_Soldier, E_CampaignReason::None, {}};
    value.m_Slots[1] = {guest.m_Soldier, E_CampaignReason::None, {}};
    value.m_Slots[2].m_UnavailableReason = E_CampaignReason::MissionRestriction;
    // Slot 3 remains empty, not a zero/duplicate soldier.
    S_CampaignSoldierLoadout loadout;
    loadout.m_Soldier = ai.m_Soldier;
    loadout.m_Items.push_back({PreparationItemId{12},
        {ItemCatalogKey{"rifle"}, 1, ItemCatalogKey{"rifle.ammo"}, 5, std::nullopt, 1.0f},
        {PlacementKey{"hands"}, 0}});
    loadout.m_Weight = 4.0f;
    loadout.m_WeightCapacity = 30.0f;
    loadout.m_Skills.push_back({SkillKey{"shooting"}, 80.25f});
    value.m_Loadouts.push_back(loadout);
    auto guestLoadout = loadout;
    guestLoadout.m_Soldier = guest.m_Soldier;
    guestLoadout.m_Items[0].m_Entry = PreparationItemId{24};
    value.m_Loadouts.push_back(guestLoadout);
    value.m_Stock.push_back({StockEntryId{13},
        {ItemCatalogKey{"medkit"}, 1, std::nullopt, std::nullopt, 3.0f, std::nullopt},
        {{ai.m_Soldier, {PlacementKey{"belt"}}, {true, E_CampaignReason::None}}}, {}});
    value.m_Essentials.push_back({ItemCatalogKey{"mission.radio"}, 1, 0, 1, {ai.m_Soldier}, {}});
    value.m_Participants = {{PlayerId{3}, "Host", true, true, std::nullopt},
        {PlayerId{11}, "Guest", true, true, PreparationRevision{5}}};
    value.m_CanDeploy = {false, E_CampaignReason::PolicyRequired};
    S_CampaignFinding blocker;
    blocker.m_Reason = E_CampaignReason::PolicyRequired;
    blocker.m_Severity = E_CampaignFindingSeverity::DeploymentBlocker;
    blocker.m_UnresolvedPolicy = E_CampaignPolicy::DeploymentQuorum;
    blocker.m_Message = TextKey{"policy.readiness_unconfirmed"};
    value.m_DeployBlockers.push_back(blocker);
    value.m_Policies = C_FakePolicyView{}.GetSnapshot();
    return value;
}

int main() {
    T_CampaignPreparationSnapshot retained;
    {
        C_FakePreparation fake;
        I_CampaignPreparation &api = fake;
        assert(api.GetSnapshot().m_Status == E_CampaignSyncStatus::Loading);
        assert(std::holds_alternative<S_CampaignLocalFailure>(api.SubmitEdit({})));
        auto source = MakeSnapshot();
        fake.Publish(source);
        retained = api.GetSnapshot().m_Snapshot;
        source.m_Stock[0].m_State.m_Quantity = 99; // publication is detached from its source
        assert(retained->m_Stock[0].m_State.m_Quantity == 1);
        assert(!retained->m_Slots[2].m_Soldier && !retained->m_Slots[3].m_Soldier);
        assert(retained->m_Soldiers[0].m_CanEdit.m_Allowed);
        assert(!retained->m_Soldiers[1].m_CanEdit.m_Allowed);
        assert(retained->m_DeployBlockers[0].m_UnresolvedPolicy == E_CampaignPolicy::DeploymentQuorum);
        assert(std::holds_alternative<S_CampaignSnapshotChanged>(*api.PollEvent()));

        const S_CampaignRequestContext request{SessionId{1}, PlayerId{3}, RequestId{14}};
        const S_CampaignPreparationRequestContext context{request, PreparationGeneration{4}, PreparationRevision{5}};
        const S_CampaignOwnershipTarget target{SoldierId{8}, ControlRevision{9}};
        S_CampaignEditRequest edit{context, {target},
            S_CampaignTakeFromStock{SoldierId{8}, StockEntryId{13}, {PlacementKey{"belt"}, 0}, 1}};
        auto queued = api.SubmitEdit(edit);
        assert(std::holds_alternative<S_CampaignQueuedSubmission>(queued));
        const auto token = std::get<S_CampaignQueuedSubmission>(queued).m_Token;
        assert(api.GetSnapshot().m_Snapshot == retained); // queued != committed
        assert(std::get<S_CampaignEditRequest>(fake.m_Recorded.back()).m_Context.m_Request.m_Request == RequestId{14});
        edit.m_Context.m_Request.m_Request = RequestId{15};
        edit.m_Operation = S_CampaignReturnToStock{SoldierId{8}, PreparationItemId{12}, 1};
        api.SubmitEdit(edit);
        edit.m_Context.m_Request.m_Request = RequestId{16};
        edit.m_Operation = S_CampaignMoveWithinLoadout{SoldierId{8}, PreparationItemId{12}, {PlacementKey{"long-gun"}, 0}, 1};
        api.SubmitEdit(edit);
        edit.m_Context.m_Request.m_Request = RequestId{17};
        edit.m_Operation = S_CampaignApplyPreset{PresetKey{"fixture-preset"}};
        api.SubmitEdit(edit);
        edit.m_Context.m_Request.m_Request = RequestId{18};
        edit.m_Targets.push_back({SoldierId{10}, ControlRevision{9}}); // explicit bulk targets, not implicit squad
        edit.m_Operation = S_CampaignAutoEquip{AutoEquipPolicyKey{"fixture-campaign-default"}, {ItemCatalogKey{"rifle"}}};
        api.SubmitEdit(edit);
        S_CampaignOwnershipRequest ownership{request, PreparationGeneration{4}, std::nullopt,
            OwnershipRevision{7}, S_CampaignClaim{target}};
        ownership.m_Context.m_Request = RequestId{19};
        api.SubmitOwnership(ownership);
        ownership.m_Context.m_Request = RequestId{20};
        ownership.m_Operation = S_CampaignSwitch{target, {SoldierId{10}, ControlRevision{9}}};
        api.SubmitOwnership(ownership);
        ownership.m_Context.m_Request = RequestId{21};
        ownership.m_Operation = S_CampaignRelease{target};
        api.SubmitOwnership(ownership);
        auto readyContext = context;
        readyContext.m_Request.m_Request = RequestId{22};
        api.SetReady({readyContext, true});
        auto deployContext = context;
        deployContext.m_Request.m_Request = RequestId{23};
        api.RequestDeploy({deployContext});
        assert(fake.m_Recorded.size() == 10); // DTO construction, no validation/model claim

        S_CampaignRequestResult result;
        result.m_Context = request;
        result.m_Family = E_CampaignRequestFamily::Edit;
        result.m_Outcome = E_CampaignRequestOutcome::Rejected;
        result.m_Reason = E_CampaignReason::StaleRevision;
        result.m_PreparationGeneration = PreparationGeneration{4};
        result.m_PreparationRevision = PreparationRevision{5};
        result.m_ViewRevision = ViewRevision{6};
        result.m_OwnershipRevision = OwnershipRevision{7};
        result.m_State = retained;
        fake.Deliver(token, result);
        auto event = api.PollEvent();
        const auto &correlated = std::get<S_CampaignRequestResultEvent>(*event);
        assert(correlated.m_LocalToken == token && correlated.m_Result.m_Context.m_Request == RequestId{14});
        result.m_Outcome = E_CampaignRequestOutcome::Duplicate;
        result.m_OriginalOutcome = E_CampaignRequestOutcome::Rejected;
        fake.Deliver(token, result);
        assert(std::get<S_CampaignRequestResultEvent>(*api.PollEvent()).m_Result.m_OriginalOutcome == E_CampaignRequestOutcome::Rejected);

        source.m_PreparationRevision = PreparationRevision{15};
        source.m_ViewRevision = ViewRevision{16};
        fake.Publish(source);
        assert(api.GetSnapshot().m_Snapshot->m_PreparationRevision == PreparationRevision{15});
        assert(retained->m_PreparationRevision == PreparationRevision{5});
        api.PollEvent();
        api.RequestResync();
        assert(!api.GetSnapshot().m_Snapshot && api.GetSnapshot().m_Status == E_CampaignSyncStatus::Resyncing);
        assert(std::holds_alternative<S_CampaignLocalFailure>(api.SetReady({context, true})));
        assert(std::holds_alternative<S_CampaignResyncDescriptor>(*api.PollEvent()));
        assert(!api.PollEvent());
    }
    assert(retained->m_Loadouts[0].m_Skills[0].m_Value == 80.25f);
    assert(retained->m_Loadouts[0].m_Items[0].m_State.m_Rounds == 5u); // survives adapter/screen destruction
    const ActorRef actor{SessionId{1}, MissionGeneration{20}, ActorId{0}, ActorSlotUse{0}};
    const S_CampaignControlToken control{SessionId{1}, MissionGeneration{20}, PlayerId{3}, SoldierId{8}, actor, ControlRevision{21}};
    assert(control.m_Actor == actor);
    auto obsolete = actor;
    obsolete.m_Generation = MissionGeneration{19};
    assert(obsolete != actor);
    S_CampaignMissionManifest manifest;
    manifest.m_Snapshot = SnapshotId{22};
    manifest.m_Context = retained->m_Context;
    manifest.m_Context.m_MissionGeneration = MissionGeneration{20};
    manifest.m_PreparationGeneration = retained->m_Generation;
    manifest.m_FrozenPreparationRevision = retained->m_PreparationRevision;
    manifest.m_Bindings.push_back({SoldierId{8}, actor, ControlRevision{21}});
    manifest.m_RequiredParticipants = {PlayerId{3}, PlayerId{11}};
    const S_CampaignSnapshotAck ack{SessionId{1}, PlayerId{11}, MissionGeneration{20}, SnapshotId{22},
        PreparationGeneration{4}, PreparationRevision{5}, ViewRevision{6}, OwnershipRevision{7}};
    const S_CampaignStartSimulation start{SessionId{1}, MissionGeneration{20}, SnapshotId{22}};
    assert(start.m_Snapshot == ack.m_Snapshot && manifest.m_Bindings[0].m_Actor == actor);
    C_FakeOwnershipView ownershipView;
    C_FakeCatalogView catalogView;
    assert(ownershipView.GetSnapshot()->m_Soldiers.empty());
    const S_CampaignStatusChanged status{SessionId{1}, PreparationGeneration{4},
        MissionGeneration{20}, ViewRevision{6}, E_CampaignConnectionStatus::Admitted,
        E_CampaignSyncStatus::Current, E_CampaignPhase::Preparation, E_CampaignReason::None};
    const T_CampaignPreparationEvent statusEvent = status;
    assert(std::get<S_CampaignStatusChanged>(statusEvent).m_ViewRevision == ViewRevision{6});
    assert(catalogView.GetSnapshot()->m_Items[0].m_Key == ItemCatalogKey{"rifle"});
    std::cout << "campaign contracts: identity, DTO, fake interface and immutable lifetime checks passed\n";
}
