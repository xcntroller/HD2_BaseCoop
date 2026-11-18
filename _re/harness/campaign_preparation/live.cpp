#include "game/campaign_preparation_live.h"

#include <algorithm>
#include <cassert>
#include <iostream>

namespace {

using R = E_CampaignReason;
namespace Wire = CampaignProtocol;
const SessionId kSession(51);
const PlayerId kHost(1), kGuest(2);
const PlacementKey kHands("hands");
const ItemCatalogKey kWeapon("weapon");

class C_OwnershipPolicy final : public I_CampaignPolicyView {
public:
    S_CampaignPolicySnapshot GetSnapshot() const override
    {
        return {{{E_CampaignPolicy::SoldierSelection, E_CampaignPolicyState::Permitted, {}}}};
    }
};

Wire::T_Message RoundTrip(const Wire::T_Message &message)
{
    Wire::T_Bytes bytes;
    Wire::T_Message decoded;
    assert(Wire::Encode(message, bytes) == R::None);
    assert(Wire::Decode(bytes, decoded) == R::None);
    return decoded;
}

S_CampaignPreparationRequestContext Context(C_CampaignPreparationLive &adapter, PlayerId player)
{
    const auto snapshot = adapter.GetSnapshot().m_Snapshot;
    return {{kSession, player, adapter.NextRequestId()}, snapshot->m_Generation, snapshot->m_PreparationRevision};
}

S_CampaignEditRequest Take(C_CampaignPreparationLive &adapter, PlayerId player, std::size_t slot)
{
    const auto snapshot = adapter.GetSnapshot().m_Snapshot;
    const auto &soldier = snapshot->m_Soldiers[slot].m_Ownership;
    return {Context(adapter, player), {{soldier.m_Soldier, soldier.m_ControlRevision}},
        S_CampaignTakeFromStock{soldier.m_Soldier, StockEntryId(1), {kHands, 0}, 1}};
}

const S_CampaignRequestResult &Result(const std::optional<T_CampaignPreparationEvent> &event)
{
    assert(event);
    return std::get<S_CampaignRequestResultEvent>(*event).m_Result;
}

void Claim(C_CampaignPreparationLive &adapter, PlayerId player, std::size_t slot)
{
    const auto snapshot = adapter.GetSnapshot().m_Snapshot;
    const auto &record = snapshot->m_Soldiers[slot].m_Ownership;
    const auto context = Context(adapter, player);
    const S_CampaignOwnershipRequest request{context.m_Request, context.m_Generation, {},
        snapshot->m_OwnershipRevision, S_CampaignClaim{{record.m_Soldier, record.m_ControlRevision}}};
    assert(std::holds_alternative<S_CampaignQueuedSubmission>(adapter.SubmitOwnership(request)));
}

} // namespace

int main()
{
    C_OwnershipPolicy policy;
    const std::string hash = "sha256:" + std::string(64, 'a');
    const S_CampaignSessionSetup setup{kSession, CampaignId(51), MissionKey("Arctic1"),
        {{hash, hash, hash, hash, hash}, 2}, true};
    C_CampaignRequestJournal journal(kSession);
    C_CampaignOwnership ownership(C_CampaignSession(setup, {0, 1, 2, 3}), policy, journal);
    assert(ownership.AdmitTransportPlayer(kGuest, "Guest") == R::None);
    const auto roster = ownership.GetSession().GetRoster();
    for (const auto &soldier : roster)
        assert(ownership.SetAvailability(soldier.m_Soldier, true, true) == R::None);
    assert(ownership.StartPreparation() == R::None);

    S_CampaignPreparationSnapshot initial;
    initial.m_Context = ownership.GetSession().GetContext();
    initial.m_Generation = ownership.GetSession().GetPreparationGeneration();
    initial.m_PreparationRevision = PreparationRevision(1);
    initial.m_ViewRevision = ViewRevision(1);
    initial.m_OwnershipRevision = ownership.GetSnapshot()->m_Revision;
    initial.m_Briefing = {TextKey("operation"), TextKey("mission"), TextKey("briefing"), AssetKey("image")};
    S_CampaignEquipmentRuleCatalog rules;
    rules.m_Placements.push_back({kHands, E_CampaignEquipmentContainer::Hands});
    S_CampaignEquipmentItemRule weapon;
    weapon.m_Item = kWeapon;
    weapon.m_IsAnimated = true;
    weapon.m_AllowedPlacements = {kHands};
    rules.m_Items.push_back(weapon);
    initial.m_Stock.push_back({StockEntryId(1), {kWeapon, 1, {}, {}, {}, {}}, {}, {}});
    for (const auto &entry : ownership.GetSession().GetRoster()) {
        S_CampaignSoldierPresentation soldier;
        soldier.m_Soldier = entry.m_Soldier;
        soldier.m_Profile = SoldierProfileKey("test/" + std::to_string(entry.m_ProfileIndex));
        soldier.m_DisplayName = "Soldier";
        initial.m_Slots[initial.m_Soldiers.size()].m_Soldier = soldier.m_Soldier;
        initial.m_Soldiers.push_back(soldier);
        S_CampaignSoldierLoadout loadout;
        loadout.m_Soldier = soldier.m_Soldier;
        initial.m_Loadouts.push_back(loadout);
        rules.m_Soldiers.push_back({soldier.m_Soldier, 20, {}});
    }

    std::deque<Wire::T_Message> outbound, inbound;
    C_CampaignPreparationLive host(kSession, kHost, [&](PlayerId target, const auto &message) {
        assert(target == kGuest);
        outbound.push_back(RoundTrip(message));
        return true;
    });
    C_CampaignPreparationLive guest(kSession, kGuest, [&](PlayerId target, const auto &message) {
        assert(target == kGuest);
        inbound.push_back(RoundTrip(message));
        return true;
    });
    assert(host.OpenHost(ownership, journal, std::move(initial), std::move(rules)) == R::None);
    const auto drive = [&] {
        while (!inbound.empty()) {
            assert(host.Receive(kGuest, inbound.front()));
            inbound.pop_front();
        }
        assert(host.Tick() == R::None);
        while (!outbound.empty()) {
            assert(guest.Receive(kGuest, outbound.front()));
            outbound.pop_front();
        }
    };
    drive();
    Claim(host, kHost, 0);
    drive();
    assert(Result(host.PollEvent()).m_Reason == R::None);
    Claim(guest, kGuest, 1);
    drive();
    assert(Result(guest.PollEvent()).m_Reason == R::None);
    assert(host.GetSnapshot().m_Snapshot->m_Soldiers[2].m_CanEdit.m_Allowed);
    assert(!guest.GetSnapshot().m_Snapshot->m_Soldiers[2].m_CanEdit.m_Allowed);
    assert(!host.GetSnapshot().m_Snapshot->m_Soldiers[1].m_CanEdit.m_Allowed);
    assert(!guest.GetSnapshot().m_Snapshot->m_Soldiers[0].m_CanEdit.m_Allowed);

    auto second = Take(host, kHost, 0);
    auto first = Take(guest, kGuest, 1);
    assert(std::holds_alternative<S_CampaignQueuedSubmission>(host.SubmitEdit(second)));
    assert(std::holds_alternative<S_CampaignQueuedSubmission>(guest.SubmitEdit(first)));
    drive();
    assert(Result(guest.PollEvent()).m_Reason == R::None);
    assert(Result(host.PollEvent()).m_Reason == R::StaleRevision);
    assert(host.GetSnapshot().m_Snapshot->m_Stock.front().m_State.m_Quantity == 0);
    assert(guest.GetSnapshot().m_Snapshot->m_Loadouts[1].m_Items.front().m_State.m_Item == kWeapon);
    assert(host.Receive(kGuest, RoundTrip(first)));
    assert(std::get<S_CampaignRequestResult>(outbound.back()).m_Outcome == E_CampaignRequestOutcome::Duplicate);
    drive();
    assert(!guest.PollEvent());
    assert(std::holds_alternative<S_CampaignQueuedSubmission>(guest.SubmitEdit(Take(guest, kGuest, 0))));
    drive();
    assert(Result(guest.PollEvent()).m_Reason == R::NotOwner);

    // An exact-revision replacement with a skipped base locks out the editor.
    const auto current = guest.GetSnapshot().m_Snapshot;
    auto next = *current;
    next.m_ViewRevision = ViewRevision(current->m_ViewRevision.Value() + 1);
    assert(guest.Receive(kGuest, RoundTrip(Wire::S_PreparationDelta{ViewRevision(1), next})));
    assert(guest.GetSnapshot().m_Status == E_CampaignSyncStatus::Resyncing);
    assert(guest.Tick() == R::None);
    assert(inbound.size() == 1 && std::holds_alternative<Wire::S_PreparationResync>(inbound.front()));
    drive();
    assert(guest.GetSnapshot().m_Status == E_CampaignSyncStatus::Resyncing);

    assert(std::holds_alternative<S_CampaignQueuedSubmission>(host.SetReady({Context(host, kHost), true})));
    drive();
    assert(Result(host.PollEvent()).m_Reason == R::None);
    assert(guest.GetSnapshot().m_Status == E_CampaignSyncStatus::Current);
    assert(std::holds_alternative<S_CampaignQueuedSubmission>(guest.SetReady({Context(guest, kGuest), true})));
    drive();
    assert(Result(guest.PollEvent()).m_Reason == R::None);

    assert(ownership.OnDisconnected(kGuest) == R::None);
    assert(host.Tick() == R::None);
    const auto reopened = host.GetSnapshot().m_Snapshot;
    assert(!reopened->m_Soldiers[1].m_Ownership.m_Owner && reopened->m_Soldiers[1].m_CanEdit.m_Allowed);
    assert(!reopened->m_Participants.front().m_ReadyAt);
    assert(reopened->m_Stock.front().m_State.m_Quantity == 0 && !reopened->m_Loadouts[1].m_Items.empty());

    assert(std::holds_alternative<S_CampaignQueuedSubmission>(host.SetReady({Context(host, kHost), true})));
    assert(host.Tick() == R::None && Result(host.PollEvent()).m_Reason == R::None);
    assert(std::holds_alternative<S_CampaignQueuedSubmission>(host.RequestDeploy({Context(host, kHost)})));
    assert(host.Tick() == R::None && Result(host.PollEvent()).m_Reason == R::None);
    const auto frozen = host.GetFrozenManifest();
    assert(frozen && frozen->m_RequiredParticipants == std::vector<PlayerId>{kHost});
    assert(frozen->m_FrozenPreparationRevision == reopened->m_PreparationRevision);
    assert(frozen->m_Soldiers[1].m_Ownership.m_Deployed);
    assert(!frozen->m_Soldiers[1].m_Ownership.m_Owner && !frozen->m_Loadouts[1].m_Items.empty());
    assert(host.IsFrozenPublished());
    assert(ownership.ReturnToSetup(setup.m_Mission) == R::None && ownership.StartPreparation() == R::None);
    assert(host.ReopenHost() == R::None);
    const auto retry = host.GetSnapshot().m_Snapshot;
    assert(retry->m_Generation != frozen->m_PreparationGeneration && retry->m_PreparationRevision == PreparationRevision(1));
    assert(!retry->m_Participants.front().m_ReadyAt && retry->m_Stock.front().m_State.m_Quantity == 0);
    assert(retry->m_Loadouts[1].m_Items.front().m_State.m_Item == kWeapon && !host.GetFrozenManifest());
    assert(frozen->m_FrozenPreparationRevision == reopened->m_PreparationRevision);
    std::cout << "Live preparation authority, replica, stock race, resync, freeze, and reopen passed\n";
}
