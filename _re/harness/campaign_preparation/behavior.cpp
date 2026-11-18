#include "support.h"
void Reject(Fixture &f, const S_CampaignEditRequest &r, R expected) {
    auto before = Digest(f.Snapshot()); const auto result = f.Apply(r);
    CHECK(result.m_Reason == expected); CHECK(result.m_Outcome == O::Rejected); CHECK(Digest(f.Snapshot()) == before);
}
void PermissionsAndAtomicity() {
    Fixture f;
    CHECK(f.Snapshot()->m_Soldiers[0].m_CanEdit.m_Allowed);
    CHECK(!f.Snapshot()->m_Soldiers[1].m_CanEdit.m_Allowed);
    CHECK(f.Snapshot()->m_Soldiers[2].m_CanEdit.m_Allowed);
    CHECK(f.Snapshot(2)->m_Soldiers[1].m_CanEdit.m_Allowed);
    CHECK(!f.Snapshot(2)->m_Soldiers[2].m_CanEdit.m_Allowed);
    Reject(f, f.Take(1, 20, 1, "hands"), R::NotOwner);
    Reject(f, f.Take(2, 30, 1, "hands"), R::NotOwner);
    auto invalid = f.Edit(1, {10, 20}, S_CampaignAutoEquip{AutoEquipPolicyKey("fixture/ammo"), {}});
    Reject(f, invalid, R::NotOwner); CHECK(f.m_Rules.m_Plans == 0);
    auto bulk = f.Edit(1, {10, 30, 40}, S_CampaignAutoEquip{AutoEquipPolicyKey("fixture/ammo"), {}});
    CHECK(f.Apply(bulk).m_Reason == R::None); CHECK(f.Snapshot()->m_Stock[1].m_State.m_Quantity == 17);
    f.m_Rules.m_ForbiddenPlan = true;
    Reject(f, f.Edit(1, {10}, S_CampaignAutoEquip{AutoEquipPolicyKey("fixture/ammo"), {}}), R::NotOwner);
    f.m_Rules.m_ForbiddenPlan = false;
    auto duplicate = bulk; duplicate.m_Context = f.Context(); duplicate.m_Targets.push_back(duplicate.m_Targets[0]);
    Reject(f, duplicate, R::Malformed);
    auto wrongTarget = f.Take(1, 10, 2, "belt"); std::get<S_CampaignTakeFromStock>(wrongTarget.m_Operation).m_Soldier = SoldierId(30);
    Reject(f, wrongTarget, R::InvalidSoldier);
}
void ScarcityAndJournal() {
    Fixture f;
    const auto old = f.Snapshot();
    auto a = f.Take(1, 10, 1, "hands"), b = f.Take(2, 20, 1, "hands");
    auto accepted = f.Apply(a); CHECK(accepted.m_Reason == R::None);
    Reject(f, b, R::StaleRevision);
    b = f.Take(2, 20, 1, "hands"); Reject(f, b, R::StockUnavailable);
    const auto before = Digest(f.Snapshot());
    auto repeat = f.Apply(a); CHECK(repeat.m_Outcome == O::Duplicate && repeat.m_OriginalOutcome == O::Accepted);
    CHECK(Digest(f.Snapshot()) == before); CHECK(old->m_Stock[0].m_State.m_Quantity == 1);
    auto conflict = a; std::get<S_CampaignTakeFromStock>(conflict.m_Operation).m_Count = 2;
    CHECK(f.Apply(conflict).m_Reason == R::RequestConflict);
    CHECK(f.m_Model->SetReady(PlayerId(1), {a.m_Context, true}).m_Reason == R::RequestConflict);
    f.Ready(1); f.Ready(1);
    CHECK(f.Apply(a).m_Reason == R::RequestExpired); CHECK(f.Apply(a).m_Outcome == O::ResyncRequired);
    auto badCount = f.Take(1, 10, 2, "belt", 0); Reject(f, badCount, R::InvalidCount);
    auto lower = f.Take(1, 10, 2, "belt"); lower.m_Context.m_Request.m_Request = RequestId(badCount.m_Context.m_Request.m_Request.Value() - 1);
    const auto lowerReason = f.Apply(lower).m_Reason; CHECK(lowerReason == R::RequestConflict || lowerReason == R::RequestExpired);
    auto wrongGen = a; wrongGen.m_Context.m_Generation = PreparationGeneration(2); Reject(f, wrongGen, R::WrongGeneration);
    auto spoof = f.Take(1, 10, 2, "belt"); CHECK(f.m_Model->ApplyEdit(PlayerId(2), spoof).m_Reason == R::NotAdmitted);
    auto wrongSession = spoof; wrongSession.m_Context.m_Request.m_Session = SessionId(2); Reject(f, wrongSession, R::WrongSession);
}
void ValidationAndVariants() {
    Fixture f;
    Reject(f, f.Take(1, 10, 1, "unknown"), R::InvalidPlacement);
    Reject(f, f.Take(1, 10, 2, "belt", 13), R::CapacityExceeded);
    Reject(f, f.Take(1, 10, 2, "backpack", 1), R::CapacityExceeded);
    Reject(f, f.Take(1, 10, 2, "hands", 1), R::InvalidPlacement);
    Reject(f, f.Take(1, 10, 99, "belt"), R::InvalidItem);
    CHECK(f.Apply(f.Take(1, 10, 1, "hands")).m_Reason == R::None);
    auto rifle = f.Snapshot()->m_Loadouts[0].m_Items.back();
    CHECK(rifle.m_State.m_Rounds == 5u && rifle.m_State.m_Condition == 0.75f);
    CHECK(f.Apply(f.Take(1, 10, 5, "hands")).m_Reason == R::None);
    CHECK(f.Snapshot()->m_Stock[0].m_State.m_Quantity == 1); // displaced rifle returned once, condition retained
    CHECK(f.Snapshot()->m_Stock[0].m_State.m_Rounds == 5u);
    CHECK(f.Apply(f.Take(1, 10, 3, "pack")).m_Reason == R::None);
    CHECK(f.Apply(f.Take(1, 10, 2, "belt", 4)).m_Reason == R::None);
    const auto ammo = f.Snapshot()->m_Loadouts[0].m_Items.back().m_Entry;
    CHECK(f.Apply(f.Edit(1, {10}, S_CampaignMoveWithinLoadout{SoldierId(10), ammo, Place("backpack"), 2})).m_Reason == R::None);
    const auto pack = Find(f.Snapshot()->m_Loadouts[0].m_Items, [](const auto &i) { return i.m_State.m_Item == ItemCatalogKey("pack"); })->m_Entry;
    Reject(f, f.Edit(1, {10}, S_CampaignReturnToStock{SoldierId(10), pack, 1}), R::CapacityExceeded);
    CHECK(f.Snapshot()->m_Loadouts[0].m_Skills[0].m_Value == 80.25f);
    auto retained = f.Snapshot(); f.m_Model.reset(); CHECK(retained->m_Loadouts[0].m_Skills[0].m_Value == 80.25f);
}
void OwnershipRacesAndDisconnect() {
    Fixture f;
    auto pending = f.Take(1, 30, 2, "belt");
    f.m_Ownership.Assign(1, std::nullopt); f.m_Ownership.Assign(2, PlayerId(2));
    CHECK(f.m_Model->SynchronizeOwnership() == R::None);
    Reject(f, pending, R::StaleRevision);
    Reject(f, f.Take(1, 30, 2, "belt"), R::NotOwner);
    f.Ready(1);
    const auto stock = f.Snapshot()->m_Stock[1].m_State.m_Quantity;
    f.m_Ownership.Assign(2, std::nullopt);
    f.m_Ownership.m_State.m_Participants[1].m_Connected = false;
    f.m_Ownership.m_State.m_Participants[1].m_Admitted = false;
    CHECK(f.m_Model->SynchronizeOwnership() == R::None);
    auto after = Digest(f.Snapshot()); CHECK(f.m_Model->SynchronizeOwnership() == R::None);
    CHECK(Digest(f.Snapshot()) == after); CHECK(f.Snapshot()->m_Stock[1].m_State.m_Quantity == stock);
    CHECK(!f.Snapshot()->m_Participants[0].m_ReadyAt);
    CHECK(f.Apply(f.Take(1, 30, 2, "belt")).m_Reason == R::None);
    f.m_Rules.m_DuringPlan = [&]() { f.m_Ownership.Assign(2, PlayerId(1)); };
    // Invalid injected mid-plan cut still must never commit the proposal.
    Reject(f, f.Take(1, 40, 2, "belt"), R::StaleRevision);
}
void ReadinessAndFreeze() {
    Fixture f;
    auto r = f.Snapshot()->m_PreparationRevision; auto v = f.Snapshot()->m_ViewRevision;
    f.Ready(1); CHECK(f.Snapshot()->m_PreparationRevision == r); CHECK(f.Snapshot()->m_ViewRevision.Value() == v.Value() + 1);
    v = f.Snapshot()->m_ViewRevision; f.Ready(1); CHECK(f.Snapshot()->m_ViewRevision == v);
    f.Ready(2); CHECK(f.Snapshot()->m_Participants[0].m_ReadyAt == r);
    CHECK(f.m_Model->RequestDeploy(PlayerId(1), {f.Context()}).m_Reason == R::PolicyRequired);
    f.m_Policy.m_Resolved = true;
    CHECK(f.m_Model->RevalidateRules() == R::None);
    CHECK(!f.Snapshot()->m_Participants[0].m_ReadyAt && !f.Snapshot()->m_Participants[1].m_ReadyAt);
    CHECK(f.m_Model->RequestDeploy(PlayerId(2), {f.Context(2)}).m_Reason == R::NotHost);
    CHECK(f.Apply(f.Take(1, 10, 2, "belt")).m_Reason == R::None);
    CHECK(!f.Snapshot()->m_Participants[0].m_ReadyAt && !f.Snapshot()->m_Participants[1].m_ReadyAt);
    CHECK(f.Apply(f.Edit(1, {10}, S_CampaignReturnToStock{SoldierId(10), PreparationItemId(1), 1})).m_Reason == R::None);
    f.Ready(1); f.Ready(2);
    CHECK(f.m_Model->RequestDeploy(PlayerId(1), {f.Context()}).m_Reason == R::RequiredItemsMissing);
    CHECK(f.Apply(f.Take(1, 10, 4, "belt")).m_Reason == R::None);
    f.Ready(1); f.Ready(2);
    auto pending = f.Take(1, 10, 2, "belt");
    auto deploy = S_CampaignDeployRequest{f.Context()};
    CHECK(f.m_Model->RequestDeploy(PlayerId(1), deploy).m_Reason == R::None);
    CHECK(f.m_Model->GetFrozenManifest()->m_FrozenPreparationRevision == f.Snapshot()->m_PreparationRevision);
    CHECK(f.m_Model->GetFrozenManifest()->m_RequiredParticipants.size() == 2);
    // This lower request has been overtaken by deploy; use a new ID to check Frozen.
    pending.m_Context.m_Request.m_Request = RequestId(++f.m_Request); Reject(f, pending, R::Frozen);
    CHECK(f.m_Model->RequestDeploy(PlayerId(1), deploy).m_Outcome == O::Duplicate);
    f.m_Ownership.Assign(1, std::nullopt);
    CHECK(f.m_Model->SynchronizeOwnership() == R::InvalidPhase);
    CHECK(!f.m_Model->GetFrozenManifest()); CHECK(f.Snapshot()->m_Sync == E_CampaignSyncStatus::Resyncing);
}
void Counters() {
    Fixture f(PreparationRevision(UINT64_MAX));
    Reject(f, f.Take(1, 10, 2, "belt"), R::SnapshotUnavailable);
    Fixture v(PreparationRevision(1), ViewRevision(UINT64_MAX));
    CHECK(v.m_Model->SetReady(PlayerId(1), {v.Context(), true}).m_Reason == R::SnapshotUnavailable);
}
int main() {
    try {
        PermissionsAndAtomicity(); ScarcityAndJournal(); ValidationAndVariants();
        OwnershipRacesAndDisconnect(); ReadinessAndFreeze(); Counters();
        std::cout << "campaign preparation: permissions, atomic stock, journal, validation, ownership races, readiness and freeze passed\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
}
