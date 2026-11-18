#ifndef HD2_CAMPAIGN_PROTOCOL_H
#define HD2_CAMPAIGN_PROTOCOL_H

#include "../game/campaign_contracts.h"
#include <cstddef>
#include <functional>

// Engine-independent opt-in component. See _re/harness/campaign_protocol/README.md
// for the normative wire schema and live attachment obligations. No native hooks.
namespace CampaignProtocol {
constexpr std::uint32_t kCommand = 0x434f4f50;
constexpr std::uint16_t kMajor = 1, kMinor = 0;
constexpr std::uint64_t kFeatures = 7; // identities, atomic preparation, guarded input
// Bit 3 identified the earlier mission update layout.
constexpr std::uint64_t kMissionUpdates = 64, kWeaponEvents = 16, kInventoryMoves = 32;
constexpr std::size_t kHeaderBytes = 16, kMaxFrameBytes = 24576;
constexpr std::size_t kMaxStringBytes = 256, kMaxCollection = 256;
constexpr std::size_t kMaxSoldiers = 64, kMaxParticipants = 4;
constexpr std::size_t kMaxInputBytes = 1024, kMaxChunkBytes = 16384;
constexpr std::uint32_t kMaxWorldBytes = 8 * 1024 * 1024;
constexpr std::uint32_t kMaxChunks = 512, kTimeoutMs = 10000;
constexpr std::uint32_t kFramesPerSecond = 256, kMaxQuantity = 1000000;
using T_Bytes = std::vector<std::uint8_t>;

struct S_Hello {
    std::string m_Mod = "hd2.base-campaign-coop";
    std::uint16_t m_Major = kMajor, m_MinMinor = kMinor, m_MaxMinor = kMinor;
    std::uint64_t m_Required = kFeatures, m_Offered = kFeatures;
    E_CampaignSessionKind m_Kind = E_CampaignSessionKind::CampaignCoop;
    S_CampaignContentDescriptor m_Content;
};
struct S_Accept {
    std::uint16_t m_Major = kMajor, m_Minor = kMinor;
    std::uint64_t m_Features = kFeatures;
    SessionId m_Session;
    PlayerId m_Player;
    S_CampaignContentDescriptor m_Content;
    std::uint32_t m_MaxFrameBytes = static_cast<std::uint32_t>(kMaxFrameBytes);
    std::uint32_t m_MaxWorldBytes = kMaxWorldBytes;
    std::uint16_t m_MaxParticipants = static_cast<std::uint16_t>(kMaxParticipants);
};
struct S_NegotiationAck {
    std::uint16_t m_Major = kMajor, m_Minor = kMinor;
    std::uint64_t m_Features = kFeatures;
    SessionId m_Session;
    PlayerId m_Player;
};
struct S_Reject { E_CampaignReason m_Reason = E_CampaignReason::Malformed; };
// A delta is deliberately a COMPLETE replacement at an exact base, avoiding mixed
// stock/loadout/rights publications. No field patches in protocol 1.0.
struct S_PreparationDelta {
    ViewRevision m_Base;
    S_CampaignPreparationSnapshot m_Replacement;
};
enum class E_InputKind { ControlBits, ActorAction, VehicleAction, DoorAction, WireCut };
struct S_Input {
    S_CampaignControlToken m_Control;
    std::uint64_t m_Sequence = 0; // strictly increasing per control token and connection
    E_InputKind m_Kind = E_InputKind::ControlBits;
    T_Bytes m_Payload; // negotiated legacy adapter bytes, never directly dispatched
};
// Versioned ControlBits body for on-foot development campaign input.
enum E_PlayerControl : std::uint16_t {
    kMoveForward = 1u << 0,
    kMoveBack = 1u << 1,
    kMoveLeft = 1u << 2,
    kMoveRight = 1u << 3,
    kLeanLeft = 1u << 4,
    kLeanRight = 1u << 5,
    kFire = 1u << 6,
    kCrouch = 1u << 7,
    kProne = 1u << 8,
    kReload = 1u << 9,
    kPaceUp = 1u << 10,
    kPaceDown = 1u << 11
};
struct S_PlayerControls {
    std::uint16_t m_Held = 0;
    std::uint16_t m_Pressed = 0;
    std::array<float, 3> m_ViewDirection{0.0f, 0.0f, 1.0f};
};
E_CampaignReason EncodePlayerControls(const S_PlayerControls &controls, T_Bytes &out);
E_CampaignReason DecodePlayerControls(const T_Bytes &bytes, S_PlayerControls &out);

// The world body is supplied/validated by the lifecycle adapter, not this codec.
// CRC32 detects incomplete/corrupt transfer; it is NOT authentication.
struct S_WorldChunk {
    S_CampaignSnapshotAck m_Baseline;
    std::uint32_t m_TotalBytes = 0, m_Index = 0, m_Count = 0, m_Crc32 = 0;
    T_Bytes m_Bytes;
};
// Running state is sent only by the host, with the committed startup identity.
struct S_SquadActorState {
    ActorRef m_Actor;
    std::array<float, 3> m_Position{}, m_Facing{}, m_ViewDirection{}, m_Velocity{};
    std::uint16_t m_Health = 0;
    std::uint8_t m_Stance = 0;
};
struct S_MissionCamera {
    std::array<float, 3> m_Position{}, m_Direction{}, m_Up{};
    float m_Fov = 0;
};
enum class E_MissionOutcome : std::uint8_t { Running, Success, Failure };

enum class E_MissionPresentation : std::uint8_t {
    FrameOn, Speech, SubtitlesOn, Subtitle, ClearHints, Hint,
    Animation, StopAnimation, Sound, CutsceneMusic, WeatherOn, WeatherMode, Clouds, Door, Clone, Attach, ReleaseClone, WireCut
};

// Commands already evaluated by the host VM. A guest executes only the
// corresponding scene, sound or interface operation.
struct S_MissionPresentation {
    E_MissionPresentation m_Action = E_MissionPresentation::FrameOn;
    std::string m_Frame;
    std::string m_Resource;
    std::array<std::int32_t, 3> m_Args{};
};

struct S_PresentationFramePose {
    std::string m_Frame;
    std::array<float, 3> m_Position{}, m_Direction{}, m_Up{};
};

struct S_AmbientHumanState {
    // Static original mission actor, resolved against the verified startup cut.
    S_SquadActorState m_Pose;
    bool m_Visible = false;
};

struct S_MissionUpdate {
    SessionId m_Session;
    MissionGeneration m_Generation;
    SnapshotId m_Snapshot;
    OwnershipRevision m_OwnershipRevision;
    std::uint64_t m_Sequence = 0;
    std::uint32_t m_MissionTime = 0;
    bool m_PlayerInputEnabled = false;
    E_MissionOutcome m_Outcome = E_MissionOutcome::Running;
    std::optional<S_MissionCamera> m_CutsceneCamera;
    std::vector<S_SquadActorState> m_Squad;
    std::vector<std::uint8_t> m_Objectives;
    std::vector<S_MissionPresentation> m_Presentation;
    std::vector<S_AmbientHumanState> m_Ambient;
    std::vector<S_PresentationFramePose> m_FramePoses;
};
enum class E_WeaponEvent : std::uint8_t { Fire, Reload };
struct S_WeaponEvent {
    ActorRef m_Actor;
    SnapshotId m_Snapshot;
    OwnershipRevision m_OwnershipRevision;
    std::uint64_t m_Sequence = 0;
    E_WeaponEvent m_Action = E_WeaponEvent::Fire;
    std::uint32_t m_ItemUid = 0;
    std::uint16_t m_Rounds = 0;
    std::uint16_t m_ReloadTime = 0;
    std::optional<std::uint32_t> m_ConsumedAmmo;
};
// A single soldier rearranges an item already in their own mission inventory.
struct S_ItemMove {
    std::uint32_t m_ItemUid = 0;
    std::uint8_t m_From = 0, m_To = 0;
    std::uint16_t m_Slot = 0;
    std::optional<std::uint32_t> m_ReplacedUid;
};
E_CampaignReason EncodeItemMove(const S_ItemMove &move, T_Bytes &out);
E_CampaignReason DecodeItemMove(const T_Bytes &bytes, S_ItemMove &out);
enum class E_DoorAction : std::uint8_t { Open, Close, Lock, Unlock, PickLock };
struct S_DoorAction {
    std::uint32_t m_Door = 0;
    E_DoorAction m_Action = E_DoorAction::Open;
};
E_CampaignReason EncodeDoorAction(const S_DoorAction &action, T_Bytes &out);
E_CampaignReason DecodeDoorAction(const T_Bytes &bytes, S_DoorAction &out);
E_CampaignReason EncodeWireCut(const std::string &frame, T_Bytes &out);
E_CampaignReason DecodeWireCut(const T_Bytes &bytes, std::string &frame);

struct S_InventoryEvent {
    ActorRef m_Actor;
    SnapshotId m_Snapshot;
    OwnershipRevision m_OwnershipRevision;
    std::uint64_t m_Sequence = 0;
    S_ItemMove m_Move;
};
using T_Message = std::variant<S_Hello, S_Accept, S_NegotiationAck, S_Reject,
    S_CampaignOwnershipRequest, S_CampaignEditRequest, S_CampaignReadyRequest,
    S_CampaignDeployRequest, S_CampaignRequestResult, S_CampaignOwnershipSnapshot,
    S_CampaignPreparationSnapshot, S_PreparationDelta, S_CampaignMissionManifest,
    S_CampaignSnapshotAck, S_CampaignStartSimulation, S_Input, S_WorldChunk, S_MissionUpdate, S_WeaponEvent, S_InventoryEvent>;
// Stable explicit tags 1..20 in the above order; variant object layout is never wire.
enum class E_Message : std::uint16_t {
    Hello = 1, Accept, NegotiationAck, Reject, OwnershipRequest, EditRequest,
    ReadyRequest, DeployRequest, RequestResult, OwnershipSnapshot,
    PreparationSnapshot, PreparationDelta, BeginMission, SnapshotAck,
    StartSimulation, Input, WorldChunk, MissionUpdate, WeaponEvent, InventoryEvent
};
struct S_FrameInfo { E_Message m_Type = E_Message::Reject; std::uint32_t m_BodyBytes = 0; };
E_CampaignReason Inspect(const std::uint8_t *data, std::size_t size, S_FrameInfo &out);
E_CampaignReason Encode(const T_Message &message, T_Bytes &out);
E_CampaignReason Decode(const std::uint8_t *data, std::size_t size, T_Message &out);
inline E_CampaignReason Decode(const T_Bytes &data, T_Message &out) { return Decode(data.data(), data.size(), out); }
// Safe bridge for unaligned native command bodies. Position/output commit only
// on success. validBits excludes padding; no C_BitStream unchecked word loads.
E_CampaignReason ExtractFrameBits(const std::uint8_t *packet, std::size_t packetBytes,
    std::size_t validBits, std::size_t &position, T_Bytes &out);
E_CampaignReason AppendFrameBits(const T_Bytes &frame, T_Bytes &packet, std::size_t &position);
// A DirectPlay message carries exactly one reserved game command, terminated by
// the native END marker with zero padding to the next byte.
constexpr std::uint8_t kGameMessageType = 0x14, kEndMessageType = 0x13;
constexpr std::size_t kCommandPacketOverhead = 6; // five type + 32 command + five end bits
E_CampaignReason EncodeCommandPacket(const T_Message &message, T_Bytes &out);
E_CampaignReason ExtractCommandFrame(const void *data, std::size_t size, T_Bytes &out);
std::uint32_t Crc32(const T_Bytes &bytes);

enum class E_Route { Legacy, Campaign, Drop };
enum class E_Traffic { Extension, NativeCommand, ActorObject, Replication };
E_Route Route(E_CampaignSessionKind kind, E_Traffic traffic);
enum class E_Admission { Dormant, AwaitHello, AwaitAccept, AwaitAck, Admitted, Rejected, Closed };
struct S_Receive {
    E_CampaignReason m_Reason = E_CampaignReason::None;
    std::optional<T_Message> m_Reply;
    std::optional<T_Message> m_Payload; // only admitted, sender-checked direction-valid data
};
// One object per verified transport connection incarnation. Caller serializes all
// calls on the authority queue. nowMs is monotonic; no codec thread touches models.
class C_Admission {
public:
    C_Admission(E_CampaignSessionKind kind, bool server, S_Hello local,
        SessionId session, PlayerId assignedPlayer, std::uint64_t nowMs);
    std::optional<T_Message> Start() const; // client Hello only; dormant emits nothing
    S_Receive Receive(const T_Bytes &frame, std::uint64_t nowMs);
    E_CampaignReason Tick(std::uint64_t nowMs);
    void Close();
    E_Admission GetState() const { return m_State; }
    const S_Accept &GetAgreement() const { return m_Agreement; }
private:
    S_Receive Reject(E_CampaignReason reason);
    bool m_Server;
    E_Admission m_State;
    S_Hello m_Local;
    S_Accept m_Agreement;
    std::uint64_t m_Started, m_Window;
    std::uint32_t m_Frames = 0;
};
// Preliminary check only: COOP-008 must ALSO call ownership.ValidateInput at
// enqueue AND application, including its private CompleteControlHandoff barrier.
// Keep one gate per connection/soldier, reset only when retiring that binding.
// Checks sender, complete authority tuple, Running phase, scripted suspension,
// reconstruction barrier and monotonic sequence BEFORE any legacy handler call.
class C_InputGate {
public:
    E_CampaignReason Validate(const S_Input &input, PlayerId sender,
        const S_CampaignSessionContext &context, const S_CampaignOwnershipRecord &soldier,
        bool admitted, bool reconstructionComplete);
    void Reset();
private:
    std::optional<S_CampaignControlToken> m_Control;
    std::uint64_t m_Sequence = 0;
};
// Atomic immutable installation. A gap disables edits until a full snapshot.
class C_PreparationReplica {
public:
    explicit C_PreparationReplica(SessionId session) : m_Session(session) {}
    E_CampaignReason Install(const S_CampaignPreparationSnapshot &snapshot);
    E_CampaignReason Apply(const S_PreparationDelta &delta);
    S_CampaignPreparationView GetView() const;
private:
    void Observe(PreparationGeneration generation, ViewRevision view);
    SessionId m_Session;
    PreparationGeneration m_MinGeneration;
    ViewRevision m_MinView;
    T_CampaignPreparationSnapshot m_Snapshot;
    bool m_Current = false;
};
// At most ONE transfer per instance. Caller creates only one per connection;
// fixed chunk size, bounded total, exact baseline, duplicate conflict and timeout.
// Sender for one recipient's sealed baseline. Pump() sends a bounded number
// of ordered chunks per game frame and keeps the sustained rate below Admission's
// receive cap. The caller sends the matching manifest first and aborts the cut
// on any non-success result.
class C_WorldSender {
public:
    using T_SendChunk = std::function<bool(const S_WorldChunk &)>;
    E_CampaignReason Begin(const S_CampaignSnapshotAck &baseline, T_Bytes world, std::uint64_t nowMs);
    E_CampaignReason Pump(std::uint64_t nowMs, const T_SendChunk &send);
    bool IsComplete() const { return m_Count != 0 && m_Next == m_Count; }
    void Reset();
private:
    S_CampaignSnapshotAck m_Baseline;
    T_Bytes m_World;
    std::uint32_t m_Count = 0, m_Next = 0, m_Crc = 0, m_SentInWindow = 0;
    std::uint64_t m_Started = 0, m_LastPump = 0, m_Window = 0;
};

// Take returns bytes only; lifecycle reconstruction must succeed BEFORE SnapshotAck.
class C_WorldAssembler {
public:
    E_CampaignReason Add(const S_WorldChunk &chunk, const S_CampaignSnapshotAck &expected,
        std::uint64_t nowMs);
    std::optional<T_Bytes> Take();
    E_CampaignReason Tick(std::uint64_t nowMs);
    void Reset();
private:
    std::optional<S_WorldChunk> m_Meta;
    T_Bytes m_Bytes;
    std::vector<bool> m_Received;
    std::uint64_t m_Started = 0;
    std::uint32_t m_ReceivedCount = 0;
    bool m_Complete = false;
};
} // namespace CampaignProtocol
#endif
