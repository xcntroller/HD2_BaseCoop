#include "campaign_control_native.h"
#include "campaign_control_runtime.h"

#include <algorithm>
#include "game/controller_internal.h"
#include "game/first_person_view.h"
#include "game/game.h"
#include "game/game_internal.h"
#include "game/human.h"
#include "game/human_body.h"
#include "game/human_movement.h"
#include "net/net_object.h"

namespace {
std::array<float, 3> VectorFields(const S_vector &value) {
    return {value.x, value.y, value.z};
}
S_vector4 NativeVector(const std::array<float, 3> &value, float w) {
    return {value[0], value[1], value[2], w};
}
}

bool C_CampaignNativeControlSink::CaptureSquadActor(CampaignProtocol::S_SquadActorState &state) const {
    auto *human = Resolve(state.m_Actor);
    if (!human || !human->GetFrame() || !human->GetController())
        return false;
    const auto *frame = human->GetFrame();
    const auto facing = frame->GetMatrix()(2).Normalized();
    auto view = human->GetViewDir().Normalized();
    if (view.Square() < 0.5f)
        view = facing;
    state.m_Position = VectorFields(frame->GetWorldPos());
    state.m_Facing = VectorFields(facing);
    state.m_ViewDirection = VectorFields(view);
    state.m_Velocity = VectorFields(human->GetVelocity());
    state.m_Health = static_cast<std::uint16_t>(std::clamp(human->GetStat(STAT_HEALTH), 0, 65535));
    state.m_Stance = static_cast<std::uint8_t>(human->GetWork()->GetStance());
    return true;
}

bool C_CampaignNativeControlSink::ApplySquadActors(
    const std::vector<CampaignProtocol::S_SquadActorState> &squad) const {
    std::vector<C_Human *> humans;
    humans.reserve(squad.size());
    for (const auto &actor : squad) {
        auto *human = Resolve(actor.m_Actor);
        if (!human || !human->GetFrame() || !human->GetMovement() || !human->GetController())
            return false;
        humans.push_back(human);
    }
    for (std::size_t i = 0; i < squad.size(); ++i) {
        auto &human = *humans[i];
        const auto &actor = squad[i];
        auto *frame = human.GetFrame();
        const auto pos = NativeVector(actor.m_Position, 1.0f);
        const auto facing = NativeVector(actor.m_Facing, 0.0f);
        const auto view = NativeVector(actor.m_ViewDirection, 0.0f);
        auto &visual = human.GetMovement()->m_visual;
        const bool died = human.IsAlive() && actor.m_Health == 0;
        human.m_stats[STAT_HEALTH] = actor.m_Health;
        if (human.IsAlive() && human.GetWork()->GetStance() != actor.m_Stance)
            human.m_body.SetStance(actor.m_Stance);
        // Only host-authored state can populate these native presentation tracks.
        if (auto *object = g_netReplication->FindObject(human.GetNetId(), false)) {
            if (auto *property = object->GetProperty(1)) {
                property->GetTrack().SetValue(pos);
                property->GetTrack().SetRate(NativeVector(actor.m_Velocity, 0.0f));
                property->GetDirTrack().SetDirection(view, 0.0f);
            }
        }
        if (IsCampaignLocalActor(human.GetNetId())) {
            // The local camera look remains responsive between host state cuts.
            frame->SetPos({pos.x, pos.y, pos.z});
            frame->Update();
            human.SetPosition4(pos);
        } else {
            visual.SetPosition(pos);
            visual.SetDirection(facing, 0.0f);
        }
        if (died)
            human.ShowDeath();
    }
    return true;
}

bool C_CampaignNativeControlSink::ApplyWeaponEvent(const CampaignProtocol::S_WeaponEvent &event) const {
    auto *human = Resolve(event.m_Actor);
    if (!human)
        return false;
    auto *weapon = human->GetHandsWeapon();
    if (!weapon || weapon->GetUid() != static_cast<int>(event.m_ItemUid) ||
        static_cast<float>(event.m_Rounds) > weapon->GetMaxAmount())
        return false;

    const bool reload = event.m_Action == CampaignProtocol::E_WeaponEvent::Reload;
    if (event.m_ConsumedAmmo) {
        auto *ammo = human->FindItemByUid(static_cast<int>(*event.m_ConsumedAmmo));
        if (!ammo || ammo->GetKind() != ITEM_KIND_AMMO || ammo->GetTypeId() != weapon->GetDef()->GetAmmoItemId())
            return false;
        human->RemoveItem(ammo->GetUid(), true);
    }
    weapon->ApplyReplicaRounds(human, event.m_Rounds, event.m_ReloadTime, reload);
    if (!reload)
        weapon->PlayShotSound(human, ShotFrameOf(weapon), false);
    human->ExecuteBodyCommand(MakeBodyCommand(BODY_COMMAND_ACTION, reload ? BODY_ACTION_RELOAD : BODY_ACTION_FIRE));
#ifndef HD2_DEDICATED
    if (IsCampaignLocalActor(human->GetNetId())) {
        const int effect = reload ? FPV_EVENT_RELOAD : (event.m_Rounds ? FPV_EVENT_FIRE : FPV_EVENT_FIRE_LAST);
        GetGame()->GetFirstPersonView()->OnInventoryEvent(effect, 0);
    }
#endif
    return true;
}

namespace {
std::optional<std::uint16_t> FindItemSlot(C_Human &human, C_Item &item, int where)
{
    auto *container = human.GetContainer(where);
    if (!container)
        return std::nullopt;
    if (const auto *slots = container->GetSlots()) {
        for (int index = 0; index < container->GetSlotCount(); ++index) {
            if (std::find(slots[index].begin(), slots[index].end(), &item) != slots[index].end())
                return static_cast<std::uint16_t>(index);
        }
    } else if (container->GetItem() == &item) {
        return 0;
    }
    return std::nullopt;
}
}

E_CampaignReason C_CampaignNativeControlSink::MoveInventory(const ActorRef &actor, CampaignProtocol::S_ItemMove &move) const
{
    using R = E_CampaignReason;
    static_assert(ITEM_CONTAINER_COUNT == 7 && ITEM_CONTAINER_HANDS == 3);
    auto *human = Resolve(actor);
    if (!human)
        return R::ActorUnavailable;
    if (!IsServer() || !CampaignInputAuthorized(human->GetNetId()))
        return R::NotOwner;
    const auto uid = static_cast<int>(move.m_ItemUid);
    auto *item = human->FindItemInContainer(uid, move.m_From);
    if (!item || item->GetOwner() != human)
        return R::InvalidItem;

    int destination = move.m_To;
    if (move.m_ReplacedUid) {
        if (!human->ServerTransferItem(static_cast<int>(*move.m_ReplacedUid), destination,
                                       human->GetNetId(), uid, move.m_From))
            return R::InvalidPlacement;
    } else if (!human->ServerMoveItem(uid, human->GetNetId(), move.m_From, &destination, move.m_Slot)) {
        return R::InvalidPlacement;
    }
    // ServerMoveItem resolves an automatic destination and existing stack layout.
    if (destination < 0 || destination >= ITEM_CONTAINER_COUNT)
        return R::SnapshotGap;
    item = human->FindItemInContainer(uid, destination);
    const auto actual = item ? FindItemSlot(*human, *item, destination) : std::nullopt;
    if (!actual || *actual >= 256)
        return R::SnapshotGap;
    move.m_To = static_cast<std::uint8_t>(destination);
    move.m_Slot = *actual;
    return R::None;
}

bool C_CampaignNativeControlSink::ApplyInventoryEvent(const CampaignProtocol::S_InventoryEvent &event) const
{
    auto *human = Resolve(event.m_Actor);
    if (!human || IsServer())
        return false;
    const auto &move = event.m_Move;
    const auto uid = static_cast<int>(move.m_ItemUid);
    auto *item = human->FindItemInContainer(uid, move.m_From);
    if (!item || item->GetOwner() != human)
        return false;
    if (move.m_ReplacedUid) {
        const auto replaced = static_cast<int>(*move.m_ReplacedUid);
        if (!human->FindItemInContainer(replaced, move.m_To))
            return false;
        human->ClientTransferItem(replaced, move.m_To, human->GetNetId(), uid, move.m_From);
    } else {
        human->ClientMoveItem(uid, human->GetNetId(), move.m_From, move.m_To, move.m_Slot);
    }
    item = human->FindItemInContainer(uid, move.m_To);
    return item && FindItemSlot(*human, *item, move.m_To) == move.m_Slot;
}
