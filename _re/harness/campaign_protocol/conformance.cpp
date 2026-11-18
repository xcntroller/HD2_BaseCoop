#include "../../../src/net/campaign_protocol.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

using namespace CampaignProtocol;
using R = E_CampaignReason;
#ifdef _MSC_VER
static_assert(sizeof(void *) == 4, "required MSVC x86 target");
#ifndef _M_IX86
#error Intended target is MSVC x86
#endif
#endif
namespace {
unsigned g_Checks = 0;
#define CHECK(x) do { ++g_Checks; if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while (false)
S_CampaignContentDescriptor Content() {
    return {"sha256:" + std::string(64, 'a'), "sha256:" + std::string(64, 'b'),
        "sha256:" + std::string(64, 'c'), "sha256:" + std::string(64, 'd'), "sha256:" + std::string(64, 'e')};
}
S_Hello Hello() { S_Hello h; h.m_Content = Content(); return h; }
S_CampaignSessionContext Context() {
    S_CampaignSessionContext c;
    c.m_Kind = E_CampaignSessionKind::CampaignCoop;
    c.m_Session = SessionId(0x1122334455667788ull); c.m_Campaign = CampaignId(2);
    c.m_Mission = MissionKey("base/operation/step@1"); c.m_MissionGeneration = MissionGeneration(3);
    c.m_Phase = E_CampaignPhase::Preparation; c.m_UsesCampaignRules = true;
    c.m_IsTransportServer = true; c.m_IsTransportClient = true;
    c.m_LocalPlayer = PlayerId(5); c.m_HostPlayer = PlayerId(5);
    c.m_Rules.m_Content = Content(); c.m_Rules.m_Difficulty = 2;
    return c;
}
S_CampaignOwnershipRecord Soldier() {
    return {SoldierId(101), true, true, true, PlayerId(5),
        ActorRef{Context().m_Session, MissionGeneration(3), ActorId(0), ActorSlotUse(0)}, false, ControlRevision(7)};
}
S_CampaignParticipant Participant() { return {PlayerId(5), "Guest \xc4\x8c", true, true, PreparationRevision(8)}; }
S_CampaignItemState Item() {
    return {ItemCatalogKey("item/rifle@1"), 1, ItemCatalogKey("ammo/762@1"), 6, 0.5f, 0.875f};
}
S_CampaignFinding Finding() {
    S_CampaignFinding f;
    f.m_Reason = R::PolicyRequired; f.m_Severity = E_CampaignFindingSeverity::DeploymentBlocker;
    f.m_Message = TextKey("policy/test"); f.m_Soldier = SoldierId(101); f.m_Item = ItemCatalogKey("item/rifle@1");
    f.m_Entry = PreparationItemId(9); f.m_Slot = PlacementKey("hand");
    f.m_UnresolvedPolicy = E_CampaignPolicy::DeploymentQuorum;
    f.m_Required = std::numeric_limits<std::int64_t>::min(); f.m_Actual = std::numeric_limits<std::int64_t>::max();
    return f;
}
S_CampaignSoldierLoadout Loadout() {
    S_CampaignSoldierLoadout l; l.m_Soldier = SoldierId(101);
    l.m_Items = {{PreparationItemId(9), Item(), {PlacementKey("hand"), 0}}};
    l.m_Weight = 7.5f; l.m_WeightCapacity = 20; l.m_BeltVolume = 1; l.m_BeltCapacity = 3;
    l.m_BackpackVolume = 2; l.m_BackpackCapacity = 6; l.m_BackpackWeight = 3;
    l.m_BackpackWeightCapacity = 12; l.m_BeltWeight = 0.75f;
    l.m_Skills = {{SkillKey("shooting"), 72.125f}}; l.m_Findings = {Finding()};
    return l;
}
S_CampaignPreparationSnapshot Snapshot() {
    S_CampaignPreparationSnapshot s;
    s.m_Context = Context(); s.m_Generation = PreparationGeneration(4);
    s.m_PreparationRevision = PreparationRevision(8); s.m_ViewRevision = ViewRevision(10); s.m_OwnershipRevision = OwnershipRevision(6);
    s.m_Connection = E_CampaignConnectionStatus::Admitted; s.m_Sync = E_CampaignSyncStatus::Current;
    s.m_Briefing = {TextKey("operation"), TextKey("mission"), TextKey("briefing"), AssetKey("map")};
    s.m_Slots[0].m_Soldier = SoldierId(101); s.m_Slots[1].m_Restrictions = {Finding()};
    S_CampaignSoldierPresentation p;
    p.m_Soldier = SoldierId(101); p.m_Profile = SoldierProfileKey("roster/test"); p.m_DisplayName = "Soldier";
    p.m_Portrait = AssetKey("portrait"); p.m_Preview = AssetKey("preview"); p.m_Ownership = Soldier();
    p.m_CanView = {true, R::None}; p.m_CanEdit = {true, R::None}; p.m_CanClaim = {false, R::AlreadyOwned};
    p.m_CanRelease = {true, R::None}; p.m_Restrictions = {Finding()}; s.m_Soldiers = {p}; s.m_Loadouts = {Loadout()};
    s.m_Stock = {{StockEntryId(11), Item(), {{SoldierId(101), {PlacementKey("hand")}, {true, R::None}}},
        {{ConstraintKey("weight"), TextKey("weight/help"), {Finding()}}}}};
    s.m_Essentials = {{ItemCatalogKey("item/rifle@1"), 2, 1, 1, {SoldierId(101)}, {Finding()}}};
    s.m_Participants = {Participant()}; s.m_Findings = {Finding()}; s.m_DeployBlockers = {Finding()};
    s.m_CanDeploy = {false, R::PolicyRequired};
    s.m_Policies.m_Decisions = {{E_CampaignPolicy::DeploymentQuorum, E_CampaignPolicyState::Unresolved, TextKey("policy/test")}};
    return s;
}
S_CampaignOwnershipSnapshot Ownership() { return {Context(), OwnershipRevision(6), {Soldier()}, {Participant()}}; }
S_CampaignRequestContext Request() { return {Context().m_Session, PlayerId(5), RequestId(12)}; }
S_CampaignPreparationRequestContext PrepRequest() { return {Request(), PreparationGeneration(4), PreparationRevision(8)}; }
S_CampaignSnapshotAck Baseline() {
    return {Context().m_Session, PlayerId(5), MissionGeneration(3), SnapshotId(13),
        PreparationGeneration(4), PreparationRevision(8), ViewRevision(10), OwnershipRevision(6)};
}
S_Input Input() {
    return {{Context().m_Session, MissionGeneration(3), PlayerId(5), SoldierId(101), *Soldier().m_Actor, ControlRevision(7)},
        1, E_InputKind::ControlBits, {0x01, 0x80, 0xff}};
}
S_CampaignMissionManifest Manifest() {
    auto s = Snapshot(); auto c = Context(); c.m_Phase = E_CampaignPhase::Loading;
    return {SnapshotId(13), c, PreparationGeneration(4), PreparationRevision(8), ViewRevision(10), OwnershipRevision(6),
        s.m_Soldiers, s.m_Loadouts, {{SoldierId(101), *Soldier().m_Actor, ControlRevision(7)}}, {PlayerId(5)}};
}
S_WorldChunk Chunk() { return {Baseline(), 3, 0, 1, Crc32({1,2,3}), {1,2,3}}; }
S_MissionUpdate MissionUpdate() {
    S_MissionUpdate state{Context().m_Session, MissionGeneration(3), SnapshotId(13), OwnershipRevision(6),
        25, 7500, false, E_MissionOutcome::Running, S_MissionCamera{{10, 20, -30}, {0, 0, 1}, {0, 1, 0}, 1.3f},
        {{*Soldier().m_Actor, {1, 2, -3}, {0, 0, 1}, {1, 0, 0}, {2, 0, 0}, 740, 2}}, {0, 1, 4}, {}, {}, {}};
    return state;
}

S_WeaponEvent WeaponEvent() {
    return {*Soldier().m_Actor, SnapshotId(13), OwnershipRevision(6), 1, E_WeaponEvent::Reload, 21, 5, 650, 90};
}
S_InventoryEvent InventoryEvent() {
    return {*Soldier().m_Actor, SnapshotId(13), OwnershipRevision(6), 2, {21, 2, 3, 0, 8}};
}
T_Bytes Frame(const T_Message &m) { T_Bytes bytes; CHECK(Encode(m, bytes) == R::None); return bytes; }
std::string Hex(const T_Bytes &bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (auto b : bytes) { result += digits[b >> 4]; result += digits[b & 15]; }
    return result;
}
void Set32(T_Bytes &b, std::size_t pos, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) b[pos + i] = static_cast<std::uint8_t>(value >> (8 * i));
}
std::vector<T_Message> Messages() {
    S_Accept accept; accept.m_Session = Context().m_Session; accept.m_Player = PlayerId(5); accept.m_Content = Content();
    S_CampaignOwnershipRequest claim{Request(), PreparationGeneration(4), std::nullopt, OwnershipRevision(6), S_CampaignClaim{{SoldierId(101), ControlRevision(7)}}};
    S_CampaignEditRequest edit{PrepRequest(), {{SoldierId(101), ControlRevision(7)}}, S_CampaignTakeFromStock{SoldierId(101), StockEntryId(11), {PlacementKey("hand"), 0}, 1}};
    S_CampaignRequestResult result;
    result.m_Context = Request(); result.m_Outcome = E_CampaignRequestOutcome::Duplicate; result.m_Reason = R::None;
    result.m_PreparationGeneration = PreparationGeneration(4); result.m_MissionGeneration = MissionGeneration(3);
    result.m_OwnershipRevision = OwnershipRevision(6); result.m_PreparationRevision = PreparationRevision(8); result.m_ViewRevision = ViewRevision(10);
    result.m_OriginalOutcome = E_CampaignRequestOutcome::Accepted; result.m_State = std::make_shared<const S_CampaignPreparationSnapshot>(Snapshot()); result.m_Findings = {Finding()};
    auto next = Snapshot(); next.m_ViewRevision = ViewRevision(11);
    std::vector<T_Message> all = {Hello(), accept, S_NegotiationAck{kMajor, kMinor, kFeatures, Context().m_Session, PlayerId(5)},
        S_Reject{R::UnsupportedVersion}, claim, edit, S_CampaignReadyRequest{PrepRequest(), true},
        S_CampaignDeployRequest{PrepRequest()}, result, Ownership(), Snapshot(), S_PreparationDelta{ViewRevision(10), next},
        Manifest(), Baseline(), S_CampaignStartSimulation{Context().m_Session, MissionGeneration(3), SnapshotId(13)}, Input(), Chunk()};
    claim.m_Operation = S_CampaignSwitch{{SoldierId(101), ControlRevision(7)}, {SoldierId(102), ControlRevision(8)}}; all.push_back(claim);
    claim.m_Operation = S_CampaignRelease{{SoldierId(101), ControlRevision(7)}}; all.push_back(claim);
    edit.m_Operation = S_CampaignReturnToStock{SoldierId(101), PreparationItemId(9), 1}; all.push_back(edit);
    edit.m_Operation = S_CampaignMoveWithinLoadout{SoldierId(101), PreparationItemId(9), {PlacementKey("belt"), 3}, 1}; all.push_back(edit);
    edit.m_Operation = S_CampaignApplyPreset{PresetKey("preset/test")}; all.push_back(edit);
    edit.m_Operation = S_CampaignAutoEquip{AutoEquipPolicyKey("fixture/proposed"), {ItemCatalogKey("item/rifle@1")}}; all.push_back(edit);
    result.m_State = std::make_shared<const S_CampaignOwnershipSnapshot>(Ownership()); all.push_back(result);
    result.m_State = S_CampaignResyncDescriptor{Context().m_Session, PreparationGeneration(4), MissionGeneration(3), OwnershipRevision(6), PreparationRevision(8), ViewRevision(10), SnapshotId(13), R::SnapshotGap}; all.push_back(result);
    auto maxAck = Baseline(); maxAck.m_Session = SessionId(UINT64_MAX); maxAck.m_PlayerEcho = PlayerId(UINT64_MAX);
    all.push_back(maxAck);
    auto noActor = Ownership(); noActor.m_Soldiers[0].m_Actor.reset(); noActor.m_Soldiers[0].m_Owner.reset();
    noActor.m_Participants[0].m_ReadyAt.reset(); all.push_back(noActor);
    all.push_back(MissionUpdate());
    all.push_back(WeaponEvent());
    all.push_back(InventoryEvent());
    return all;
}
void RoundTrips(const std::string &dir, bool write) {
    auto messages = Messages();
    for (std::size_t i = 0; i < messages.size(); ++i) {
        const auto frame = Frame(messages[i]);
        S_FrameInfo info; CHECK(Inspect(frame.data(), frame.size(), info) == R::None);
        CHECK(static_cast<std::size_t>(info.m_Type) == messages[i].index() + 1);
        T_Message decoded; CHECK(Decode(frame, decoded) == R::None);
        CHECK(Frame(decoded) == frame);
        const auto path = dir + "/" + std::to_string(i + 1) + ".hex";
        if (write) { std::ofstream f(path); f << Hex(frame) << '\n'; CHECK(static_cast<bool>(f)); }
        else { std::ifstream f(path); std::string expected; f >> expected; CHECK(expected == Hex(frame)); }
        // Every truncation boundary, with unchanged output on failure.
        T_Message sentinel = S_Reject{R::Timeout}; const auto original = Frame(sentinel);
        for (std::size_t n = 0; n < frame.size(); ++n) {
            CHECK(Decode(frame.data(), n, sentinel) != R::None);
            CHECK(Frame(sentinel) == original);
            if (n >= kHeaderBytes) {
                T_Bytes truncated(frame.begin(), frame.begin() + static_cast<std::ptrdiff_t>(n));
                Set32(truncated, 12, static_cast<std::uint32_t>(n - kHeaderBytes));
                CHECK(Decode(truncated, sentinel) != R::None);
                CHECK(Frame(sentinel) == original);
            }
        }
    }
    // Preserve fractional skill IEEE bits and zero actor coordinates.
    T_Message decoded; CHECK(Decode(Frame(Snapshot()), decoded) == R::None);
    CHECK(std::get<S_CampaignPreparationSnapshot>(decoded).m_Loadouts[0].m_Skills[0].m_Value == 72.125f);
    CHECK(std::get<S_CampaignPreparationSnapshot>(decoded).m_Soldiers[0].m_Ownership.m_Actor->m_Actor.Value() == 0);
}
void Malformed() {
    T_Message out = S_Reject{R::Timeout}; const auto sentinel = Frame(out);
    auto bad = Frame(Hello()); bad[4] = 2; CHECK(Decode(bad, out) == R::UnsupportedVersion);
    bad = Frame(Hello()); bad[8] = 255; CHECK(Decode(bad, out) == R::Malformed);
    bad = Frame(Hello()); bad[10] = 1; CHECK(Decode(bad, out) == R::Malformed);
    bad = Frame(Hello()); bad[0] = 0; CHECK(Decode(bad, out) == R::Malformed);
    bad = Frame(Hello()); bad.push_back(0); CHECK(Decode(bad, out) == R::Malformed);
    Set32(bad, 12, static_cast<std::uint32_t>(bad.size() - kHeaderBytes)); CHECK(Decode(bad, out) == R::Malformed);
    bad.assign(kMaxFrameBytes + 1, 0); CHECK(Decode(bad, out) == R::Malformed);
    CHECK(Decode(nullptr, 100, out) == R::Malformed); CHECK(Frame(out) == sentinel);
    // String length before allocation; enum, bool, variant and optional tags.
    bad = Frame(Hello()); bad[16] = 0xff; bad[17] = 0xff; CHECK(Decode(bad, out) == R::Malformed);
    bad = Frame(S_Reject{R::Timeout}); bad[16] = 0xff; CHECK(Decode(bad, out) == R::Malformed);
    auto ready = S_CampaignReadyRequest{PrepRequest(), true}; bad = Frame(ready); bad.back() = 2; CHECK(Decode(bad, out) == R::Malformed);
    auto edit = std::get<S_CampaignEditRequest>(Messages()[5]); bad = Frame(edit);
    // Context 40 bytes + target count4 + one target16: edit variant at body60.
    bad[16 + 60] = 255; CHECK(Decode(bad, out) == R::Malformed);
    bad = Frame(edit); Set32(bad, 16 + 40, UINT32_MAX); CHECK(Decode(bad, out) == R::Malformed);
    auto ownership = std::get<S_CampaignOwnershipRequest>(Messages()[4]); bad = Frame(ownership);
    bad[16 + 24] = 2; CHECK(Decode(bad, out) == R::Malformed);
    // Encoder rejects bad source without replacing caller's output.
    T_Bytes bytes = {0x42}; auto invalid = Hello(); invalid.m_Mod.assign(257, 'x'); CHECK(Encode(invalid, bytes) == R::Malformed && bytes == T_Bytes{0x42});
    invalid = Hello(); invalid.m_Mod = std::string("\xc0\x80", 2); CHECK(Encode(invalid, bytes) == R::Malformed);
    invalid.m_Mod = std::string("a\0b", 3); CHECK(Encode(invalid, bytes) == R::Malformed);
    invalid = Hello(); invalid.m_Content.m_Rules[7] = 'A'; CHECK(Encode(invalid, bytes) == R::Malformed);
    ready.m_Context.m_Request.m_Request = RequestId(); CHECK(Encode(ready, bytes) == R::Malformed);
    auto snap = Snapshot(); snap.m_Loadouts[0].m_Skills[0].m_Value = std::numeric_limits<float>::quiet_NaN(); CHECK(Encode(snap, bytes) == R::Malformed);
    snap = Snapshot(); snap.m_Loadouts[0].m_Weight = std::numeric_limits<float>::infinity(); CHECK(Encode(snap, bytes) == R::Malformed);
    snap = Snapshot(); snap.m_Participants.assign(5, Participant()); CHECK(Encode(snap, bytes) == R::Malformed);
    snap = Snapshot(); snap.m_Soldiers.push_back(snap.m_Soldiers[0]); snap.m_Loadouts.push_back(snap.m_Loadouts[0]); CHECK(Encode(snap, bytes) == R::Malformed);
    snap = Snapshot(); snap.m_Soldiers[0].m_Ownership.m_Actor->m_Session = SessionId(1); CHECK(Encode(snap, bytes) == R::Malformed);
    snap = Snapshot(); snap.m_Soldiers[0].m_Ownership.m_Actor->m_Generation = MissionGeneration(2); CHECK(Encode(snap, bytes) == R::Malformed);
    auto duplicateActor = Ownership(); auto secondSoldier = Soldier(); secondSoldier.m_Soldier = SoldierId(102); secondSoldier.m_Owner.reset();
    duplicateActor.m_Soldiers.push_back(secondSoldier); CHECK(Encode(duplicateActor, bytes) == R::Malformed);
    duplicateActor.m_Soldiers[1].m_Actor->m_SlotUse = ActorSlotUse(1); CHECK(Encode(duplicateActor, bytes) == R::Malformed);
    snap = Snapshot(); snap.m_Slots[1].m_Soldier = SoldierId(101); CHECK(Encode(snap, bytes) == R::Malformed);
    snap = Snapshot(); snap.m_Loadouts[0].m_Items[0].m_State.m_Quantity = kMaxQuantity + 1; CHECK(Encode(snap, bytes) == R::Malformed);
    snap = Snapshot(); snap.m_Findings.resize(kMaxCollection + 1); CHECK(Encode(snap, bytes) == R::Malformed);
    auto result = std::get<S_CampaignRequestResult>(Messages()[8]); result.m_State = T_CampaignPreparationSnapshot{}; CHECK(Encode(result, bytes) == R::Malformed);
    result = std::get<S_CampaignRequestResult>(Messages()[8]); result.m_OriginalOutcome.reset(); CHECK(Encode(result, bytes) == R::Malformed);
    result = std::get<S_CampaignRequestResult>(Messages()[8]); result.m_ViewRevision = ViewRevision(11); CHECK(Encode(result, bytes) == R::Malformed);
    result = std::get<S_CampaignRequestResult>(Messages()[8]); result.m_MissionGeneration.reset(); CHECK(Encode(result, bytes) == R::Malformed);
    result = std::get<S_CampaignRequestResult>(Messages()[8]); result.m_PreparationGeneration = PreparationGeneration(99); CHECK(Encode(result, bytes) == R::Malformed);
    result = std::get<S_CampaignRequestResult>(Messages()[23]); result.m_OwnershipRevision = OwnershipRevision(99); CHECK(Encode(result, bytes) == R::Malformed);
    result = std::get<S_CampaignRequestResult>(Messages()[24]); result.m_ViewRevision = ViewRevision(99); CHECK(Encode(result, bytes) == R::Malformed);
    CHECK(Encode(S_Reject{static_cast<R>(65536)}, bytes) == R::Malformed);
    auto input = Input(); input.m_Payload.resize(kMaxInputBytes + 1); CHECK(Encode(input, bytes) == R::Malformed);
    auto chunk = Chunk(); chunk.m_Count = UINT32_MAX; CHECK(Encode(chunk, bytes) == R::Malformed);
    auto manifest = Manifest(); manifest.m_Soldiers[0].m_Ownership.m_Actor->m_Session = SessionId(1); CHECK(Encode(manifest, bytes) == R::Malformed);
    manifest = Manifest(); manifest.m_Soldiers[0].m_Ownership.m_Actor->m_Generation = MissionGeneration(99); CHECK(Encode(manifest, bytes) == R::Malformed);
    manifest = Manifest(); manifest.m_Bindings[0].m_Actor.m_SlotUse = ActorSlotUse(1); CHECK(Encode(manifest, bytes) == R::Malformed);
    manifest = Manifest(); manifest.m_Bindings[0].m_ControlRevision = ControlRevision(8); CHECK(Encode(manifest, bytes) == R::Malformed);
    manifest = Manifest(); manifest.m_Soldiers[0].m_Ownership.m_Actor.reset(); CHECK(Encode(manifest, bytes) == R::None); // reconstruction absence remains valid
}
C_Admission Server() { return {E_CampaignSessionKind::CampaignCoop, true, Hello(), Context().m_Session, PlayerId(5), 0}; }
C_Admission Client() { return {E_CampaignSessionKind::CampaignCoop, false, Hello(), {}, {}, 0}; }
void Admit(C_Admission &server, C_Admission &client) {
    auto hello = client.Start(); CHECK(hello.has_value());
    auto offer = server.Receive(Frame(*hello), 1); CHECK(offer.m_Reason == R::None && offer.m_Reply && !offer.m_Payload);
    CHECK(server.GetState() == E_Admission::AwaitAck);
    auto ack = client.Receive(Frame(*offer.m_Reply), 2); CHECK(ack.m_Reason == R::None && ack.m_Reply && !ack.m_Payload);
    auto finish = server.Receive(Frame(*ack.m_Reply), 3); CHECK(finish.m_Reason == R::None && !finish.m_Payload);
    CHECK(server.GetState() == E_Admission::Admitted && client.GetState() == E_Admission::Admitted);
    CHECK(server.Receive(Frame(*ack.m_Reply), 4).m_Reason == R::None); // duplicate ack is harmless
    CHECK(client.Receive(Frame(*offer.m_Reply), 4).m_Reply.has_value()); // retransmitted accept
}
void Negotiation() {
    for (const auto kind : {E_CampaignSessionKind::Solo, E_CampaignSessionKind::LegacyMultiplayer}) {
        C_Admission dormant(kind, false, Hello(), {}, {}, 0);
        CHECK(!dormant.Start() && dormant.GetState() == E_Admission::Dormant);
        auto got = dormant.Receive(Frame(Hello()), 0); CHECK(!got.m_Reply && !got.m_Payload);
        CHECK(Route(kind, E_Traffic::Extension) == E_Route::Drop);
        for (auto traffic : {E_Traffic::NativeCommand, E_Traffic::ActorObject, E_Traffic::Replication}) CHECK(Route(kind, traffic) == E_Route::Legacy);
    }
    CHECK(Route(E_CampaignSessionKind::CampaignCoop, E_Traffic::NativeCommand) == E_Route::Drop);
    CHECK(Route(E_CampaignSessionKind::CampaignCoop, E_Traffic::ActorObject) == E_Route::Drop);
    CHECK(Route(E_CampaignSessionKind::CampaignCoop, E_Traffic::Replication) == E_Route::Drop);
    CHECK(Route(E_CampaignSessionKind::CampaignCoop, E_Traffic::Extension) == E_Route::Campaign);
    for (int which = 0; which < 7; ++which) {
        auto server = Server(); auto h = Hello(); R expected = R::Malformed;
        switch (which) {
        case 0: h.m_Mod = "vanilla"; expected = R::ModRequired; break;
        case 1: h.m_Major = 2; expected = R::UnsupportedVersion; break;
        case 2: h.m_MinMinor = 1; h.m_MaxMinor = 2; expected = R::UnsupportedVersion; break;
        case 3: h.m_Required |= 8; h.m_Offered |= 8; expected = R::MissingFeature; break;
        case 4: h.m_Required = 3; h.m_Offered = 3; expected = R::MissingFeature; break;
        case 5: h.m_Content.m_ItemContent[7] = '0'; expected = R::ContentMismatch; break;
        case 6: h.m_Content.m_Rules[7] = '0'; expected = R::RulesMismatch; break;
        }
        auto got = server.Receive(Frame(h), 1); CHECK(got.m_Reason == expected && got.m_Reply && !got.m_Payload);
        CHECK(server.GetState() == E_Admission::Rejected);
        CHECK(!server.Receive(Frame(Hello()), 2).m_Reply); // terminal, no amplification
    }
    {
        auto server = Server(); auto h = Hello(); h.m_Offered |= 0x100; h.m_MaxMinor = 9;
        auto offer = server.Receive(Frame(h), 1); CHECK(offer.m_Reply);
        CHECK(std::get<S_Accept>(*offer.m_Reply).m_Features == kFeatures);
        CHECK(std::get<S_Accept>(*offer.m_Reply).m_Minor == 0);
        CHECK(server.Receive(Frame(h), 2).m_Reply); // stable repeated Hello
        auto ack = S_NegotiationAck{kMajor, kMinor, kFeatures, Context().m_Session, PlayerId(6)};
        CHECK(server.Receive(Frame(ack), 3).m_Reason == R::Malformed);
    }
    // Every payload family is rejected before decode/handler in both handshake directions.
    for (const auto &m : Messages()) if (m.index() >= 4) {
        auto server = Server(); auto client = Client(); unsigned mutations = 0;
        auto early = server.Receive(Frame(m), 1); if (early.m_Payload) ++mutations;
        CHECK(early.m_Reason == R::Malformed && mutations == 0);
        early = client.Receive(Frame(m), 1); if (early.m_Payload) ++mutations;
        CHECK(early.m_Reason == R::Malformed && mutations == 0);
        server = Server(); CHECK(server.Receive(Frame(Hello()), 0).m_Reply);
        early = server.Receive(Frame(m), 1); CHECK(!early.m_Payload && early.m_Reason == R::Malformed);
    }
    auto server = Server(); auto client = Client(); Admit(server, client);
    auto ready = S_CampaignReadyRequest{PrepRequest(), true};
    auto got = server.Receive(Frame(ready), 5); CHECK(got.m_Payload && got.m_Reason == R::None);
    CHECK(server.Receive(Frame(ready), 6).m_Payload.has_value()); // model journal, never decoder retry
    auto decoded = std::get<S_CampaignReadyRequest>(*got.m_Payload);
    CHECK(decoded.m_Context.m_Request.m_Request == Request().m_Request);
    ready.m_Ready = false; auto different = Frame(ready); CHECK(different != Frame(decoded));
    CHECK(server.Receive(different, 7).m_Payload.has_value()); // conflicting replay belongs to shared journal
    auto snap = Snapshot(); CHECK(client.Receive(Frame(snap), 5).m_Payload.has_value());
    CHECK(client.Receive(Frame(ready), 6).m_Reason == R::Malformed); // wrong direction
    ready.m_Context.m_Request.m_PlayerEcho = PlayerId(6); CHECK(server.Receive(Frame(ready), 8).m_Reason == R::Malformed);
    server = Server(); CHECK(server.Tick(kTimeoutMs - 1) == R::None); CHECK(server.Tick(kTimeoutMs) == R::Timeout);
    server = Server(); auto offer = server.Receive(Frame(Hello()), 0); CHECK(offer.m_Reply);
    CHECK(server.Tick(kTimeoutMs) == R::Timeout); // repeated Hello never extends deadline
    server = Server(); CHECK(server.Receive({}, 0).m_Reason == R::Malformed);
    server = Server(); server.Close(); CHECK(!server.Receive(Frame(Hello()), 0).m_Reply);
    server = Server(); for (unsigned i = 0; i < kFramesPerSecond; ++i) CHECK(server.Receive(Frame(Hello()), 1).m_Reason == R::None);
    CHECK(server.Receive(Frame(Hello()), 1).m_Reason == R::Malformed);
    // Validate server offer on the client, not only peer Hello on server.
    for (int i = 0; i < 5; ++i) {
        client = Client(); auto accept = std::get<S_Accept>(Messages()[1]); R expected;
        if (i == 0) { accept.m_Major = 2; expected = R::UnsupportedVersion; }
        else if (i == 1) { accept.m_Features |= 8; expected = R::MissingFeature; }
        else if (i == 2) { accept.m_Content.m_Rules[7] = '0'; expected = R::RulesMismatch; }
        else if (i == 3) { accept.m_Content.m_SoldierContent[7] = '0'; expected = R::ContentMismatch; }
        else { accept.m_Features = 0; expected = R::MissingFeature; }
        CHECK(client.Receive(Frame(accept), 0).m_Reason == expected);
    }
}
void CampaignProgression() {
    const S_ReportKey key{Context().m_Session, MissionGeneration(3), SnapshotId(13)};
    S_CampaignReport report{key, true,
        {{PlayerId(5), "Host", true, false}, {PlayerId(6), "Guest", false, true}},
        {{SoldierId(101), 795, 84, {0, 0, 1.5f}, 1, std::uint8_t{2}, std::uint8_t{3}, {1, 2}}}, {101, 402}};
    report.m_ScriptValues[1] = 7;
    const S_ReportReady ready{key};
    S_CampaignStep step{key, 1, E_CampaignStep::Video, MissionKey("Arctic2"), "BriefArctic1b.bik", 0};
    auto local = Hello();
    local.m_Offered |= kCampaignProgression;
    local.m_Required |= kCampaignProgression;
    auto server = C_Admission(E_CampaignSessionKind::CampaignCoop, true, local, Context().m_Session, PlayerId(6), 0);
    auto client = C_Admission(E_CampaignSessionKind::CampaignCoop, false, local, {}, {}, 0);
    Admit(server, client);
    CHECK(server.Receive(Frame(ready), 5).m_Payload.has_value());
    const auto accepted = client.Receive(Frame(report), 5);
    CHECK(accepted.m_Payload.has_value());
    const auto &decoded = std::get<S_CampaignReport>(*accepted.m_Payload);
    CHECK(decoded.m_Report == key && decoded.m_Participants[1].m_Spectating);
    CHECK(decoded.m_Soldiers[0].m_Gains[2] == 1.5f && decoded.m_Diary == report.m_Diary);
    CHECK(decoded.m_ScriptValues[1] == 7);
    CHECK(client.Receive(Frame(step), 6).m_Payload.has_value());
    CHECK(!server.Receive(Frame(step), 7).m_Payload);
    CHECK(!client.Receive(Frame(ready), 7).m_Payload);

    auto oldServer = Server();
    auto oldClient = Client();
    Admit(oldServer, oldClient);
    CHECK(!oldServer.Receive(Frame(ready), 5).m_Payload);
    CHECK(!oldClient.Receive(Frame(report), 5).m_Payload);

    T_Bytes bytes;
    CHECK(Encode(S_ReportReady{{key.m_Session, {}, key.m_Snapshot}}, bytes) == R::Malformed);
    step.m_Ordinal = 0;
    CHECK(Encode(step, bytes) == R::Malformed);
    step.m_Ordinal = 1;
    step.m_Screen = 4;
    CHECK(Encode(step, bytes) == R::Malformed);
    report.m_Participants.push_back(report.m_Participants.front());
    CHECK(Encode(report, bytes) == R::Malformed);
    report.m_Participants.pop_back();
    report.m_Soldiers[0].m_Gains[2] = std::numeric_limits<float>::infinity();
    CHECK(Encode(report, bytes) == R::Malformed);
}

void MissionUpdates() {
    auto local = Hello();
    local.m_Offered |= kMissionUpdates;
    local.m_Required |= kMissionUpdates;
    auto server = C_Admission(E_CampaignSessionKind::CampaignCoop, true, local, Context().m_Session, PlayerId(5), 0);
    auto client = C_Admission(E_CampaignSessionKind::CampaignCoop, false, local, {}, {}, 0);
    auto unmapped = Client();
    CHECK(server.Receive(Frame(Hello()), 1).m_Reason == R::MissingFeature);
    server = C_Admission(E_CampaignSessionKind::CampaignCoop, true, local, Context().m_Session, PlayerId(5), 0);
    Admit(server, client);
    auto state = MissionUpdate();
    auto got = client.Receive(Frame(state), 5);
    CHECK(got.m_Payload && std::get_if<S_MissionUpdate>(&*got.m_Payload));
    CHECK(server.Receive(Frame(state), 5).m_Reason == R::Malformed);
    auto oldServer = Server();
    Admit(oldServer, unmapped);
    CHECK(unmapped.Receive(Frame(state), 5).m_Reason == R::Malformed);
    state.m_Session = SessionId(55);
    state.m_Squad[0].m_Actor.m_Session = state.m_Session;
    CHECK(client.Receive(Frame(state), 5).m_Reason == R::Malformed);

    T_Bytes encoded;
    state = MissionUpdate();
    state.m_Squad.push_back(state.m_Squad.front());
    CHECK(Encode(state, encoded) == R::Malformed);
    state = MissionUpdate();
    state.m_Squad.front().m_Actor.m_Generation = MissionGeneration(4);
    CHECK(Encode(state, encoded) == R::Malformed);
    state = MissionUpdate();
    state.m_Squad.front().m_ViewDirection = {};
    CHECK(Encode(state, encoded) == R::Malformed);
    state = MissionUpdate();
    state.m_Squad.front().m_Position[0] = std::numeric_limits<float>::infinity();
    CHECK(Encode(state, encoded) == R::Malformed);
    state = MissionUpdate();
    state.m_PlayerInputEnabled = true;
    CHECK(Encode(state, encoded) == R::Malformed);
    state = MissionUpdate();
    state.m_Objectives[0] = 5;
    CHECK(Encode(state, encoded) == R::Malformed);

    state = MissionUpdate();
    state.m_PlayerInputEnabled = false;
    state.m_CutsceneCamera.reset();
    state.m_Outcome = E_MissionOutcome::Success;
    state.m_Objectives = {1, 1, 4};
    CHECK(Encode(state, encoded) == R::None);
    T_Message terminal;
    CHECK(Decode(encoded, terminal) == R::None);
    CHECK(std::get<S_MissionUpdate>(terminal).m_Outcome == E_MissionOutcome::Success);
    state.m_Objectives[0] = 3;
    CHECK(Encode(state, encoded) == R::Malformed);
    state.m_Outcome = E_MissionOutcome::Failure;
    CHECK(Encode(state, encoded) == R::None);
    CHECK(Decode(encoded, terminal) == R::None);
    CHECK(std::get<S_MissionUpdate>(terminal).m_Outcome == E_MissionOutcome::Failure);
    state.m_Objectives[1] = 0;
    CHECK(Encode(state, encoded) == R::Malformed);
    state.m_Objectives[1] = 2;
    state.m_PlayerInputEnabled = true;
    CHECK(Encode(state, encoded) == R::Malformed);

    state = MissionUpdate();
    state.m_Presentation = {
        {E_MissionPresentation::Speech, "instructor", {}, {1990274, 1000, 1000}},
        {E_MissionPresentation::Door, {}, {}, {32, 1}}
    };
    auto ambient = state.m_Squad.front();
    ambient.m_Actor.m_Actor = ActorId(32);
    ambient.m_Actor.m_SlotUse = ActorSlotUse(0);
    state.m_Ambient = {{ambient, true}};
    state.m_FramePoses = {{"aircraft", {10, 220, -50}, {0, 0, 1}, {0, 1, 0}}};
    CHECK(Encode(state, encoded) == R::None);
    CHECK(Decode(encoded, terminal) == R::None);
    const auto &presentation = std::get<S_MissionUpdate>(terminal);
    CHECK(presentation.m_Presentation[0].m_Args[2] == 1000);
    CHECK(presentation.m_FramePoses[0].m_Frame == "aircraft");
    CHECK(presentation.m_Ambient[0].m_Visible);
    state.m_Presentation[1].m_Args[1] = 6;
    CHECK(Encode(state, encoded) == R::Malformed);
    state.m_Presentation.pop_back();
    state.m_Ambient.push_back(state.m_Ambient.front());
    CHECK(Encode(state, encoded) == R::Malformed);
    state.m_Ambient.pop_back();
    state.m_FramePoses.front().m_Up = {0, 0, 1};
    CHECK(Encode(state, encoded) == R::Malformed);

    const S_DoorAction door{32, E_DoorAction::Unlock};
    S_DoorAction decodedDoor;
    CHECK(EncodeDoorAction(door, encoded) == R::None);
    CHECK(DecodeDoorAction(encoded, decodedDoor) == R::None);
    CHECK(decodedDoor.m_Door == door.m_Door && decodedDoor.m_Action == door.m_Action);
    encoded.back() = 255;
    CHECK(DecodeDoorAction(encoded, decodedDoor) == R::Malformed);
    CHECK(decodedDoor.m_Action == E_DoorAction::Unlock);

    std::string fence;
    CHECK(EncodeWireCut("fence_panel", encoded) == R::None);
    CHECK(DecodeWireCut(encoded, fence) == R::None && fence == "fence_panel");
    CHECK(EncodeWireCut("", encoded) == R::Malformed);
    encoded.push_back(0);
    CHECK(DecodeWireCut(encoded, fence) == R::Malformed && fence == "fence_panel");

    state.m_FramePoses.clear();
    state.m_Presentation.push_back({E_MissionPresentation::WireCut, "fence_panel", {}, {-31416, 0}});
    CHECK(Encode(state, encoded) == R::None);
    state.m_Presentation.back().m_Args[0] = 31417;
    CHECK(Encode(state, encoded) == R::Malformed);
}
void WeaponEvents() {
    auto local = Hello();
    local.m_Offered |= kWeaponEvents;
    local.m_Required |= kWeaponEvents;
    auto server = C_Admission(E_CampaignSessionKind::CampaignCoop, true, local, Context().m_Session, PlayerId(5), 0);
    auto client = C_Admission(E_CampaignSessionKind::CampaignCoop, false, local, {}, {}, 0);
    Admit(server, client);
    const auto event = WeaponEvent();
    const auto got = client.Receive(Frame(event), 5);
    CHECK(got.m_Payload && std::get_if<S_WeaponEvent>(&*got.m_Payload));
    CHECK(server.Receive(Frame(event), 5).m_Reason == R::Malformed);
    auto oldServer = Server();
    auto oldClient = Client();
    Admit(oldServer, oldClient);
    CHECK(oldClient.Receive(Frame(event), 5).m_Reason == R::Malformed);
    auto invalid = event;
    T_Bytes output;
    invalid.m_Action = E_WeaponEvent::Fire;
    CHECK(Encode(invalid, output) == R::Malformed);
    invalid.m_ReloadTime = 0;
    CHECK(Encode(invalid, output) == R::Malformed);
    invalid.m_ConsumedAmmo.reset();
    CHECK(Encode(invalid, output) == R::None);
    invalid.m_ItemUid = UINT32_MAX;
    CHECK(Encode(invalid, output) == R::Malformed);
    invalid = event;
    invalid.m_ConsumedAmmo = invalid.m_ItemUid;
    CHECK(Encode(invalid, output) == R::Malformed);
}
void InventoryEvents() {
    auto local = Hello();
    local.m_Offered |= kInventoryMoves;
    local.m_Required |= kInventoryMoves;
    auto server = C_Admission(E_CampaignSessionKind::CampaignCoop, true, local, Context().m_Session, PlayerId(5), 0);
    auto client = C_Admission(E_CampaignSessionKind::CampaignCoop, false, local, {}, {}, 0);
    Admit(server, client);
    const auto event = InventoryEvent();
    const auto got = client.Receive(Frame(event), 5);
    CHECK(got.m_Payload && std::get_if<S_InventoryEvent>(&*got.m_Payload));
    CHECK(server.Receive(Frame(event), 5).m_Reason == R::Malformed);

    T_Bytes bytes;
    auto invalid = event;
    invalid.m_Move.m_To = 7;
    CHECK(EncodeItemMove(invalid.m_Move, bytes) == R::Malformed); // swap must name the hands
    invalid.m_Move.m_ReplacedUid.reset();
    CHECK(Encode(invalid, bytes) == R::Malformed);
    CHECK(EncodeItemMove(invalid.m_Move, bytes) == R::None); // automatic destination is a request only
    CHECK(bytes == T_Bytes({1, 21, 0, 0, 0, 2, 7, 0, 0, 0}));
    S_ItemMove decoded;
    for (std::size_t i = 0; i < bytes.size(); ++i)
        CHECK(DecodeItemMove(T_Bytes(bytes.begin(), bytes.begin() + i), decoded) == R::Malformed);
    CHECK(DecodeItemMove(bytes, decoded) == R::None);
    CHECK(decoded.m_ItemUid == 21 && decoded.m_From == 2 && decoded.m_To == 7 && decoded.m_Slot == 0);
    bytes[0] = 0;
    CHECK(DecodeItemMove(bytes, decoded) == R::Malformed && decoded.m_ItemUid == 21);
    bytes[0] = 1;
    bytes[5] = 7;
    CHECK(DecodeItemMove(bytes, decoded) == R::Malformed && decoded.m_From == 2);
    invalid = event;
    invalid.m_Move.m_Slot = 256;
    CHECK(EncodeItemMove(invalid.m_Move, bytes) == R::Malformed);
    invalid.m_Move.m_Slot = 0;
    invalid.m_Move.m_ItemUid = UINT32_MAX;
    CHECK(EncodeItemMove(invalid.m_Move, bytes) == R::Malformed);
}

void PreparationResync() {
    const S_PreparationResync request{Context().m_Session, PlayerId(5)};
    auto local = Hello();
    local.m_Offered |= kLivePreparation;
    local.m_Required |= kLivePreparation;
    auto server = C_Admission(E_CampaignSessionKind::CampaignCoop, true, local, Context().m_Session, PlayerId(5), 0);
    auto client = C_Admission(E_CampaignSessionKind::CampaignCoop, false, local, {}, {}, 0);
    Admit(server, client);
    const auto frame = Frame(request);
    S_FrameInfo info;
    CHECK(Inspect(frame.data(), frame.size(), info) == R::None && info.m_Type == E_Message::PreparationResync);
    CHECK(Hex(frame) == "4832435001000000150000001000000088776655443322110500000000000000");
    const auto got = server.Receive(frame, 5);
    CHECK(got.m_Payload && std::get_if<S_PreparationResync>(&*got.m_Payload));
    CHECK(client.Receive(frame, 5).m_Reason == R::Malformed);
    CHECK(server.Receive(Frame(S_PreparationResync{request.m_Session, PlayerId(6)}), 6).m_Reason == R::Malformed);

    server = Server();
    client = Client();
    Admit(server, client);
    CHECK(server.Receive(frame, 5).m_Reason == R::Malformed);
}
void Inputs() {
    const S_PlayerControls controls{kMoveForward | kLeanRight, kFire | kReload, {1, 0, 0}};
    T_Bytes payload;
    CHECK(EncodePlayerControls(controls, payload) == R::None);
    CHECK(payload == T_Bytes({1, 0x21, 0, 0x40, 2, 0, 0, 0x80, 0x3f, 0, 0, 0, 0, 0, 0, 0, 0}));
    S_PlayerControls native;
    CHECK(DecodePlayerControls(payload, native) == R::None);
    CHECK(native.m_Held == controls.m_Held && native.m_Pressed == controls.m_Pressed &&
          native.m_ViewDirection == controls.m_ViewDirection);
    for (std::size_t i = 0; i < payload.size(); ++i) {
        CHECK(DecodePlayerControls(T_Bytes(payload.begin(), payload.begin() + i), native) == R::Malformed);
        CHECK(native.m_Held == controls.m_Held && native.m_Pressed == controls.m_Pressed);
    }
    auto bad = payload;
    bad.push_back(0);
    CHECK(DecodePlayerControls(bad, native) == R::Malformed);
    bad = payload;
    bad[0] = 2;
    CHECK(DecodePlayerControls(bad, native) == R::Malformed);
    bad = payload;
    bad[1] = 0x80;
    CHECK(DecodePlayerControls(bad, native) == R::Malformed);
    bad = payload;
    bad[3] = 1;
    CHECK(DecodePlayerControls(bad, native) == R::Malformed);
    C_InputGate gate; auto context = Context(); context.m_Phase = E_CampaignPhase::Running;
    auto soldier = Soldier(); auto input = Input();
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, false, true) == R::NotAdmitted);
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, false) == R::InvalidPhase);
    CHECK(gate.Validate(input, PlayerId(6), context, soldier, true, true) == R::NotOwner);
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::None);
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::StaleRevision);
    input.m_Sequence = 2; CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::None);
    input.m_Sequence = 1; CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::StaleRevision);
    input.m_Sequence = 3; input.m_Control.m_Player = PlayerId(6); CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::NotOwner);
    input = Input(); input.m_Control.m_Generation = MissionGeneration(2); input.m_Control.m_Actor.m_Generation = MissionGeneration(2);
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::WrongGeneration);
    input = Input(); soldier.m_ControlRevision = ControlRevision(8); CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::StaleRevision);
    input.m_Control.m_ControlRevision = ControlRevision(8); CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::None);
    soldier.m_Scripted = true; input.m_Sequence = 2; CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::Scripted);
    soldier.m_Scripted = false; soldier.m_Actor->m_SlotUse = ActorSlotUse(1); CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::ActorUnavailable);
    soldier = Soldier(); soldier.m_Alive = false; CHECK(gate.Validate(Input(), PlayerId(5), context, soldier, true, true) == R::Dead);
    soldier = Soldier(); soldier.m_Owner.reset(); CHECK(gate.Validate(Input(), PlayerId(5), context, soldier, true, true) == R::NotOwner);
    soldier = Soldier();
    gate.Reset();
    input = Input();
    input.m_Kind = E_InputKind::DoorAction;
    CHECK(EncodeDoorAction({32, E_DoorAction::Open}, input.m_Payload) == R::None);
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::None);
    input.m_Sequence++;
    input.m_Kind = E_InputKind::WireCut;
    CHECK(EncodeWireCut("fence_panel", input.m_Payload) == R::None);
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::None);
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::StaleRevision);
    input.m_Sequence++;
    input.m_Kind = static_cast<E_InputKind>(255);
    CHECK(gate.Validate(input, PlayerId(5), context, soldier, true, true) == R::Malformed);
    context.m_MissionGeneration = MissionGeneration(4); CHECK(gate.Validate(Input(), PlayerId(5), context, Soldier(), true, true) == R::WrongGeneration);
}
void Replicas() {
    C_PreparationReplica replica(Context().m_Session); CHECK(!replica.GetView().m_Snapshot);
    auto s = Snapshot(); CHECK(replica.Install(s) == R::None); auto retained = replica.GetView().m_Snapshot;
    CHECK(replica.Install(s) == R::None);
    auto next = s; next.m_ViewRevision = ViewRevision(11); next.m_PreparationRevision = PreparationRevision(9);
    next.m_Stock[0].m_State.m_Quantity = 0; next.m_Loadouts[0].m_Items[0].m_State.m_Quantity = 2;
    CHECK(replica.Apply({ViewRevision(10), next}) == R::None);
    CHECK(replica.GetView().m_Snapshot->m_Stock[0].m_State.m_Quantity == 0);
    CHECK(replica.GetView().m_Snapshot->m_Loadouts[0].m_Items[0].m_State.m_Quantity == 2);
    CHECK(retained->m_Stock[0].m_State.m_Quantity == 1); // immutable retained publication
    CHECK(replica.Install(s) == R::StaleRevision); CHECK(replica.Apply({ViewRevision(10), next}) == R::StaleRevision);
    auto future = next; future.m_ViewRevision = ViewRevision(13);
    CHECK(replica.Apply({ViewRevision(12), future}) == R::SnapshotGap); CHECK(!replica.GetView().m_Snapshot);
    CHECK(replica.Apply({ViewRevision(11), future}) == R::SnapshotGap); // only full snapshot repairs gap
    CHECK(replica.Install(next) == R::SnapshotGap); // delayed old full cannot erase observed view13
    CHECK(!replica.GetView().m_Snapshot);
    auto conflict = next; conflict.m_Loadouts[0].m_Weight = 1; CHECK(replica.Install(conflict) == R::SnapshotGap);
    CHECK(replica.Install(future) == R::None); CHECK(replica.GetView().m_Snapshot->m_ViewRevision == ViewRevision(13));
    conflict = future; conflict.m_Loadouts[0].m_Weight = 1; CHECK(replica.Install(conflict) == R::Malformed);
    auto old = s; old.m_Generation = PreparationGeneration(3); CHECK(replica.Install(old) == R::WrongGeneration);
    CHECK(replica.GetView().m_Status == E_CampaignSyncStatus::Current);
    future.m_ViewRevision = ViewRevision(14); future.m_PreparationRevision = PreparationRevision(7);
    CHECK(replica.Install(future) == R::SnapshotGap); CHECK(!replica.GetView().m_Snapshot);
    future = s; future.m_Generation = PreparationGeneration(5); future.m_ViewRevision = ViewRevision(1); future.m_PreparationRevision = PreparationRevision(1);
    CHECK(replica.Install(future) == R::None);
    // Observing a newer generation also raises the floor; old full generations
    // and a low full view in that new generation cannot enable editing.
    auto newGeneration = future; newGeneration.m_Generation = PreparationGeneration(6); newGeneration.m_ViewRevision = ViewRevision(5);
    CHECK(replica.Apply({ViewRevision(4), newGeneration}) == R::SnapshotGap);
    CHECK(replica.Install(future) == R::SnapshotGap);
    auto insufficient = newGeneration; insufficient.m_ViewRevision = ViewRevision(4);
    CHECK(replica.Install(insufficient) == R::SnapshotGap); CHECK(!replica.GetView().m_Snapshot);
    CHECK(replica.Install(newGeneration) == R::None);
    auto wrong = s; wrong.m_Context.m_Session = SessionId(1); wrong.m_Soldiers[0].m_Ownership.m_Actor->m_Session = SessionId(1); CHECK(replica.Install(wrong) == R::WrongSession);
}
void Chunks() {
    const T_Bytes body(kMaxChunkBytes + 3, 0x9a); const auto crc = Crc32(body);
    S_WorldChunk a{Baseline(), static_cast<std::uint32_t>(body.size()), 0, 2, crc, T_Bytes(body.begin(), body.begin() + kMaxChunkBytes)};
    auto b = a; b.m_Index = 1; b.m_Bytes = T_Bytes(body.begin() + kMaxChunkBytes, body.end());
    CHECK(Frame(a).size() > kMaxChunkBytes); // exercise complete 16KiB byte blob
    C_WorldAssembler assembler;
    CHECK(assembler.Add(b, Baseline(), 1) == R::None); CHECK(!assembler.Take());
    CHECK(assembler.Add(b, Baseline(), 2) == R::None); // duplicate no count advance
    b.m_Bytes[0] ^= 1; CHECK(assembler.Add(b, Baseline(), 3) == R::RequestConflict); b.m_Bytes[0] ^= 1;
    auto wrong = b; wrong.m_Baseline.m_Generation = MissionGeneration(2); CHECK(assembler.Add(wrong, Baseline(), 3) == R::WrongGeneration);
    wrong = b; wrong.m_TotalBytes = kMaxWorldBytes + 1; CHECK(assembler.Add(wrong, Baseline(), 3) == R::Malformed);
    CHECK(assembler.Add(a, Baseline(), 4) == R::None); auto complete = assembler.Take(); CHECK(complete && *complete == body); CHECK(!assembler.Take());
    CHECK(assembler.Add(a, Baseline(), 5) == R::None); CHECK(assembler.Tick(5 + kTimeoutMs) == R::Timeout); CHECK(!assembler.Take());
    auto corrupt = Chunk(); corrupt.m_Crc32 ^= 1; CHECK(assembler.Add(corrupt, Baseline(), 1) == R::Malformed); CHECK(!assembler.Take());
    auto empty = Chunk(); empty.m_Bytes.clear(); CHECK(assembler.Add(empty, Baseline(), 2) == R::Malformed);
    // Standard independent CRC check vector.
    CHECK(Crc32({'1','2','3','4','5','6','7','8','9'}) == 0xcbf43926);
}
void WorldSendSchedule()
{
    C_WorldSender sender;
    const auto baseline = Baseline();
    auto invalid = baseline;
    invalid.m_PlayerEcho = PlayerId{};
    CHECK(sender.Begin(invalid, T_Bytes{42}, 0) == R::Malformed);
    CHECK(sender.Begin(baseline, {}, 0) == R::Malformed);
    const T_Bytes world(kMaxWorldBytes, 0xa7);
    CHECK(sender.Begin(baseline, world, 100) == R::None);
    CHECK(sender.Begin(baseline, T_Bytes{42}, 100) == R::PendingTransaction);

    C_WorldAssembler assembler;
    std::uint32_t received = 0;
    for (std::uint64_t now = 100; now < 9000 && !sender.IsComplete(); now += 17) {
        std::uint32_t sentThisTick = 0;
        CHECK(sender.Pump(now, [&](const S_WorldChunk &chunk) {
            CHECK(chunk.m_Index == received++);
            ++sentThisTick;
            CHECK(assembler.Add(chunk, baseline, now) == R::None);
            return true;
        }) == R::None);
        CHECK(sentThisTick <= 4);
    }
    CHECK(sender.IsComplete() && received == kMaxChunks);
    CHECK(assembler.Take() == world);
    sender.Reset();

    CHECK(sender.Begin(baseline, world, 100) == R::None);
    received = 0;
    for (int i = 0; i < 100; ++i)
        CHECK(sender.Pump(9100, [&](const S_WorldChunk &) { ++received; return true; }) == R::None);
    CHECK(received == 96); // Absorb a delayed game frame while retaining the rate cap.
    CHECK(sender.Pump(10100, [](const auto &) { return true; }) == R::Timeout);
    CHECK(!sender.IsComplete());
    CHECK(sender.Pump(10100, [](const auto &) { return true; }) == R::InvalidPhase);

    CHECK(sender.Begin(baseline, T_Bytes{42}, 100) == R::None);
    CHECK(sender.Pump(100, [](const auto &) { return false; }) == R::Disconnected);
    CHECK(!sender.IsComplete());
    CHECK(sender.Begin(baseline, T_Bytes{42}, 100) == R::None);
    CHECK(sender.Pump(99, [](const auto &) { return true; }) == R::Timeout);
}
void TransportBits() {
    const auto frame = Frame(Hello());
    for (std::size_t offset = 0; offset < 64; ++offset) {
        T_Bytes packet((offset + 7) / 8, 0xa5);
        const auto prefix = packet;
        auto position = offset;
        CHECK(AppendFrameBits(frame, packet, position) == R::None);
        CHECK(position == offset + frame.size() * 8);
        for (std::size_t bit = 0; bit < offset; ++bit) CHECK(((packet[bit / 8] >> (bit % 8)) & 1) == ((prefix[bit / 8] >> (bit % 8)) & 1));
        T_Bytes extracted{0x42}; auto readPosition = offset;
        CHECK(ExtractFrameBits(packet.data(), packet.size(), position, readPosition, extracted) == R::None);
        CHECK(extracted == frame && readPosition == position);
        // Complete bit-boundary truncation, including native body offset 37.
        for (std::size_t end = offset; end < position; ++end) {
            extracted = {0x42}; readPosition = offset;
            CHECK(ExtractFrameBits(packet.data(), packet.size(), end, readPosition, extracted) == R::Malformed);
            CHECK(extracted == T_Bytes{0x42} && readPosition == offset);
        }
    }

    for (const auto &message : Messages()) {
        T_Bytes packet;
        CHECK(EncodeCommandPacket(message, packet) == R::None);
        const auto expected = Frame(message);
        CHECK(packet.size() == expected.size() + kCommandPacketOverhead);
        // The first 37 bits are native type 0x14 and command 0x434f4f50, LSB first.
        CHECK(packet[0] == 0x14 && packet[1] == 0xea && packet[2] == 0xe9 &&
              packet[3] == 0x69 && (packet[4] & 0x1f) == 0x08);

        T_Bytes extracted;
        CHECK(ExtractCommandFrame(packet.data(), packet.size(), extracted) == R::None);
        CHECK(extracted == expected);
        extracted = {0x42};
        for (std::size_t length = 0; length < packet.size(); ++length) {
            CHECK(ExtractCommandFrame(packet.data(), length, extracted) == R::Malformed);
            CHECK(extracted == T_Bytes{0x42});
        }
        for (const auto index : {std::size_t{0}, std::size_t{1}, packet.size() - 2, packet.size() - 1}) {
            auto invalid = packet;
            invalid[index] ^= 0x80; // wrong type/command (one of their shared bytes), END or padding
            CHECK(ExtractCommandFrame(invalid.data(), invalid.size(), extracted) == R::Malformed);
            CHECK(extracted == T_Bytes{0x42});
        }
        packet.insert(packet.end() - 2, 0);
        CHECK(ExtractCommandFrame(packet.data(), packet.size(), extracted) == R::Malformed);
        CHECK(extracted == T_Bytes{0x42});
    }
    T_Bytes large;
    const auto baseline = Baseline();
    const S_WorldChunk chunk{baseline, static_cast<std::uint32_t>(kMaxChunkBytes),
                             0, 1, 0, T_Bytes(kMaxChunkBytes, 0xa5)};
    CHECK(EncodeCommandPacket(chunk, large) == R::None);
    T_Bytes extracted;
    CHECK(ExtractCommandFrame(large.data(), large.size(), extracted) == R::None);
    CHECK(extracted == Frame(chunk));
    CHECK(ExtractCommandFrame(nullptr, large.size(), extracted) == R::Malformed);

}
void Hostile() {
    std::uint32_t state = 0x6d2b79f5;
    auto random = [&]() { state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; };
    auto seeds = Messages();
    for (unsigned i = 0; i < 10000; ++i) {
        auto bytes = Frame(seeds[random() % seeds.size()]);
        const auto edits = 1 + random() % 8;
        for (unsigned n = 0; n < edits; ++n) bytes[random() % bytes.size()] ^= static_cast<std::uint8_t>(1 + random() % 255);
        T_Message decoded = S_Reject{R::Timeout}; auto original = Frame(decoded);
        auto reason = Decode(bytes, decoded);
        if (reason == R::None) CHECK(Frame(decoded) == bytes); // canonical supported body
        else CHECK(Frame(decoded) == original);
    }
}
} // namespace
int main(int argc, char **argv) {
    CHECK(argc >= 2);
    RoundTrips(argv[1], argc == 3 && std::string(argv[2]) == "--write-fixtures");
    Malformed(); Negotiation(); MissionUpdates(); WeaponEvents(); InventoryEvents(); PreparationResync(); CampaignProgression(); Inputs(); Replicas(); Chunks(); WorldSendSchedule(); TransportBits(); Hostile();
    std::cout << "campaign protocol: 13 suites, " << g_Checks << " checks passed\n";
}
