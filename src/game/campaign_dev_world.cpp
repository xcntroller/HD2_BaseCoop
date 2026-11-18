#include "game/campaign_dev_world.h"

#include <algorithm>
#include <cstring>
#include <cmath>

#include "game/actors_module.h"
#include "game/campaign.h"
#include "game/campaign_dev.h"
#include "game/campaign_mission_adapter.h"
#include "game/car.h"
#include "game/game.h"
#include "game/human.h"
#include "game/item_actor.h"
#include "game/items_module.h"
#include "game/team.h"
#include "ls3d/i3d.h"
#include "script/script.h"
#include "script/script_module.h"

namespace {

class C_InitialWorldRecord {
public:
    void Word(std::uint32_t value)
    {
        for (int shift = 0; shift < 32; shift += 8)
            m_Data.push_back(static_cast<std::uint8_t>(value >> shift));
    }

    void Float(float value)
    {
        std::uint32_t bits = 0;
        if (value != 0.0f)
            std::memcpy(&bits, &value, sizeof(bits));
        Word(bits);
    }

    void Vector(const S_vector &value)
    {
        Float(value.x);
        Float(value.y);
        Float(value.z);
    }

    void Text(const char *text)
    {
        const auto count = text ? std::strlen(text) : 0;
        Word(static_cast<std::uint32_t>(count));
        if (count)
            m_Data.insert(m_Data.end(), text, text + count);
    }

    void Section(const char *name)
    {
        LogDevelopmentCampaign(name, m_Data.size(), CampaignProtocol::Crc32(m_Data));
    }

    CampaignProtocol::T_Bytes m_Data;
};

void RecordContainer(C_InitialWorldRecord &record, C_ItemContainer *container)
{
    if (!container) {
        record.Word(0);
        return;
    }
    if (auto *slots = container->GetSlots()) {
        const auto count = container->GetSlotCount();
        record.Word(static_cast<std::uint32_t>(count));
        for (int i = 0; i < count; ++i) {
            record.Word(static_cast<std::uint32_t>(slots[i].size()));
            for (const auto *item : slots[i])
                record.Word(item->GetUid());
        }
    } else if (const auto *item = container->GetItem()) {
        record.Word(1);
        record.Word(1);
        record.Word(item->GetUid());
    } else {
        record.Word(0);
    }
}

void RecordActor(C_InitialWorldRecord &record, C_Actor &actor)
{
    record.Word(actor.GetNetId());
    record.Word(actor.GetType());
    record.Word(actor.IsActive());
    const auto *frame = actor.GetFrame();
    record.Text(frame ? frame->GetName() : nullptr);
    if (actor.GetType() == ACTOR_HUMAN) {
        auto &human = static_cast<C_Human &>(actor);
        record.Word(human.GetProfileIndex());
        record.Word(human.GetRole());
        record.Word(g_teamManager.GetPlayerTeam()->IsMember(&human));
        for (int i = 0; i < STAT_COUNT; ++i)
            record.Word(human.GetStat(i));
        for (int i = 0; i < ITEM_CONTAINER_COUNT; ++i)
            RecordContainer(record, human.GetContainer(i));
    } else if (actor.GetType() == ACTOR_CAR) {
        record.Float(static_cast<C_Car &>(actor).GetVehicle()->GetFuel());
    } else if (actor.GetType() == ACTOR_ITEM) {
        const auto *item = static_cast<C_ItemActor &>(actor).GetItem();
        record.Word(item ? item->GetUid() : -1);
    }
}

void RecordActors(C_InitialWorldRecord &record, const std::vector<C_Actor *> &actors)
{
    record.Word(static_cast<std::uint32_t>(actors.size()));
    for (auto *actor : actors) {
        record.Word(actor != nullptr);
        if (actor)
            RecordActor(record, *actor);
    }
}

void RecordItems(C_InitialWorldRecord &record)
{
    auto items = g_itemsModule.GetInstances();
    std::stable_sort(items.begin(), items.end(), [](const C_Item *a, const C_Item *b) {
        return a->GetUid() < b->GetUid();
    });
    record.Word(static_cast<std::uint32_t>(items.size()));
    for (auto *item : items) {
        record.Word(item->GetUid());
        record.Word(item->GetTypeId());
        const auto *owner = item->GetOwner();
        record.Word(owner ? owner->GetNetId() : -1);
        record.Word(item->GetContainerIndex());
        record.Word(item->IsArmed());
        record.Word(item->IsInWorld());
        record.Word(item->GetTime());
        record.Float(item->GetAmount());
        record.Float(item->GetCharges());
    }
}

void RecordScripts(C_InitialWorldRecord &record)
{
    const auto &scripts = g_scriptModule.GetScripts();
    record.Word(static_cast<std::uint32_t>(scripts.size()));
    for (const auto *script : scripts) {
        record.Text(script->GetFrame()->GetName());
        record.Text(script->GetFileName());
        record.Word(script->GetInstructionPointer());
        record.Word(script->IsRunning());
        record.Word(script->IsFinished());
    }
    const int *values = g_scriptModule.GetSaveGameValues();
    for (int index = 0; index < 256; ++index)
        record.Word(values[index]);
    record.Word(g_scriptModule.IsCutsceneRunning());
    record.Word(g_scriptModule.GetCutscene());
}

struct S_ActorPose {
    C_Actor *m_Actor;
    S_vector m_Position;
    S_vector m_Direction;
    S_vector m_Up;
};

std::vector<C_Actor *> InitialActors()
{
    std::vector<C_Actor *> actors;
    for (const auto *list : {&g_actorsModule.GetActors(), &g_temporaryActors.GetActors()}) {
        for (auto *actor : *list)
            if (actor)
                actors.push_back(actor);
    }
    return actors;
}

S_ActorPose ReadLocalPose(C_Actor &actor)
{
    if (auto *frame = actor.GetFrame()) {
        const auto &matrix = frame->GetMatrix();
        return {&actor, matrix(3), matrix(2).Normalized(), matrix(1).Normalized()};
    }
    return {&actor, actor.GetPosition(), actor.GetDirection(), S_vector(0, 1, 0)};
}

void RecordPoses(C_InitialWorldRecord &record, const std::vector<C_Actor *> &actors)
{
    record.Word(static_cast<std::uint32_t>(actors.size()));
    for (auto *actor : actors) {
        const auto pose = ReadLocalPose(*actor);
        record.Vector(pose.m_Position);
        record.Vector(pose.m_Direction);
        record.Vector(pose.m_Up);
    }
    record.Section("initial world actor poses");
}

class C_PoseReader {
public:
    explicit C_PoseReader(const CampaignProtocol::T_Bytes &bytes) : m_Bytes(bytes) {}

    std::uint32_t Word()
    {
        std::uint32_t value = 0;
        for (int shift = 0; shift < 32; shift += 8)
            value |= std::uint32_t(m_Bytes[m_Position++]) << shift;
        return value;
    }

    S_vector Vector() { return {Float(), Float(), Float()}; }

    std::size_t m_Position = 0;

private:
    float Float()
    {
        const auto bits = Word();
        float value;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    const CampaignProtocol::T_Bytes &m_Bytes;
};

bool Finite(const S_vector &value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool IsCompatiblePose(const S_ActorPose &expected, const S_ActorPose &host)
{
    // Original asset identities already match. Two native startup passes can
    // settle a car slightly differently, which the authoritative pose resolves.
    constexpr float poseDistance = 0.25f;
    constexpr float orientationDistance = 0.05f;
    return Finite(host.m_Position) && Finite(host.m_Direction) && Finite(host.m_Up) &&
        (expected.m_Position - host.m_Position).Magnitude() <= poseDistance &&
        (expected.m_Direction - host.m_Direction).Magnitude() <= orientationDistance &&
        (expected.m_Up - host.m_Up).Magnitude() <= orientationDistance;
}

void ApplyPoses(const std::vector<S_ActorPose> &poses)
{
    // Build and validate every pose before mutating the native scene. Then set
    // every replica actor's simulation coordinates to the committed values.
    for (const auto &pose : poses) {
        auto &actor = *pose.m_Actor;
        if (auto *frame = actor.GetFrame()) {
            frame->SetWorldPosDir3(pose.m_Position, pose.m_Direction, pose.m_Up);
            frame->Update();
        }
        actor.SetPosition(pose.m_Position, actor.GetPosition4().w);
        actor.SetDirection(pose.m_Direction, actor.GetDirection4().w);
    }
}

E_CampaignReason RecordWorldState(C_InitialWorldRecord &record)
{
    auto *game = GetGame();
    const auto state = GetCampaignMissionSnapshot();
    if (!game || !game->IsMissionLoaded() || !game->GetMission() || !state ||
        state->m_Context.m_Phase != E_CampaignPhase::Loading || game->GetMissionTime() != 0)
        return E_CampaignReason::InvalidPhase;

    record.Text(game->GetMissionName());
    record.Word(game->GetMissionTime());
    record.Word(game->IsPlayerInputEnabled());
    record.Word(game->IsUniformDisguise());
    record.Word(game->IsNoTeamDamage());
    record.Word(game->IsTacticalMode());
    record.Word(game->IsMissionFailed());
    record.Word(game->IsAlarmMusic());
    const auto *mission = game->GetMission();
    const int objectiveCount = mission->GetObjectiveCount();
    record.Word(objectiveCount);
    for (int i = 0; i < objectiveCount; ++i) {
        const auto *objective = mission->GetObjective(i);
        record.Word(objective->GetId());
        record.Word(objective->status);
        record.Word(objective->rewarded);
    }
    record.Section("initial world mission cut");
    RecordActors(record, g_actorsModule.GetActors());
    record.Section("initial world mission actors");
    RecordActors(record, g_temporaryActors.GetActors());
    record.Section("initial world temporary actors");
    RecordItems(record);
    record.Section("initial world item instances");
    RecordScripts(record);
    record.Section("initial world script cut");
    return E_CampaignReason::None;
}

constexpr std::uint32_t kWorldVersion = 0x31574943; // CIW1, little-endian
constexpr std::size_t kPoseBytes = 9 * sizeof(std::uint32_t);

} // namespace

E_CampaignReason CaptureDevelopmentInitialWorld(CampaignProtocol::T_Bytes &world)
{
    C_InitialWorldRecord record;
    record.Word(kWorldVersion);
    RecordPoses(record, InitialActors());
    const auto reason = RecordWorldState(record);
    if (reason != E_CampaignReason::None)
        return reason;
    if (record.m_Data.size() > CampaignProtocol::kMaxWorldBytes)
        return E_CampaignReason::CapacityExceeded;
    world = std::move(record.m_Data);
    return E_CampaignReason::None;
}

E_CampaignReason InstallDevelopmentInitialWorld(const CampaignProtocol::T_Bytes &world)
{
    using R = E_CampaignReason;
    const auto actors = InitialActors();
    const std::size_t header = 2 * sizeof(std::uint32_t);
    if (world.size() < header || actors.size() > (world.size() - header) / kPoseBytes)
        return R::Malformed;
    C_PoseReader reader(world);
    if (reader.Word() != kWorldVersion || reader.Word() != actors.size())
        return R::ContentMismatch;

    std::vector<S_ActorPose> poses;
    poses.reserve(actors.size());
    for (auto *actor : actors) {
        const S_ActorPose pose{actor, reader.Vector(), reader.Vector(), reader.Vector()};
        if (!IsCompatiblePose(ReadLocalPose(*actor), pose)) {
            LogDevelopmentCampaign("initial world actor pose mismatch", actor->GetNetId(), actor->GetType());
            return R::ContentMismatch;
        }
        poses.push_back(pose);
    }

    C_InitialWorldRecord expected;
    const auto reason = RecordWorldState(expected);
    if (reason != R::None)
        return reason;
    if (!std::equal(expected.m_Data.begin(), expected.m_Data.end(), world.begin() + reader.m_Position, world.end())) {
        const auto first = std::mismatch(expected.m_Data.begin(), expected.m_Data.end(),
                                        world.begin() + reader.m_Position, world.end());
        LogDevelopmentCampaign("initial world state differs at byte",
                               first.first - expected.m_Data.begin(), world.size() - reader.m_Position);
        return R::ContentMismatch;
    }

    ApplyPoses(poses);
    return R::None;
}
