# Campaign protocol 1.0 component

**Source-only.** COOP-006 supplies bounded engine-independent codecs, per-connection
admission, safe bit adapters, preliminary input validation, atomic preparation
replicas and a one-transfer world assembler. Nothing registers a game mode,
changes a native dispatch handler or attaches a live handshake. A byte-complete
world transfer is **not** a reconstructed world. The later COOP-008/009/010 hooks
and two-instance acceptance gates remain required.

## Build and test

Prerequisites: Python 3, C++17 GCC for portable checks; Windows Python 3, Visual
Studio 2022 **x86 target** and Windows SDK for the intended target. No game data,
engine libraries, DirectPlay or model implementations are required.

From the worktree root:

```bash
python3 _re/harness/campaign_protocol/run.py --compiler gcc
python3 _re/harness/campaign_protocol/run.py --compiler gcc --sanitize
```

The second command needs the GCC AddressSanitizer/UndefinedBehaviorSanitizer
runtime libraries. It runs with leak detection and fail-fast UBSan enabled.
From an x86 developer command prompt:

```bat
python _re\harness\campaign_protocol\run.py --compiler msvc
```

Alternatively pass `--vcvars C:\toolchain\vcvars32.bat` or set `HD2_VCVARS32`.
Hostx64/x86 is supported; the fixture rejects an x64 MSVC target. The compiler
uses `/std:c++17 /EHsc /W4 /WX /permissive- /Zc:__cplusplus /arch:IA32 /MT /O2`
and `/MACHINE:X86`. The standalone-header probe and conformance executable must
both compile; execution must print `campaign protocol: 8 suites, ... checks passed`.
Outputs/logs/argv/environment go into a new private ignored
`build/campaign_protocol/{gcc,msvc}-*` directory per invocation. No legacy fixed
temporary output is used. Exact local validation is in
[the task handoff](../../../docs/tasks/COOP-006.md).

Eight suites cover 27 deterministic fixtures across all 17 message families and
all operation/state variants, fractional skill bits, signed64/unsigned64 extrema,
zero actor coordinates, every byte truncation (also with a rewritten frame length
so nested decoding is exercised), malformed counts/flags/enums/variants/UTF-8,
unsupported mod/version/features/content/rules, early payload quarantine,
transport sender/session and direction checks, duplicate handshake messages,
rate/deadline limits, stale authority/sequence input, replica gaps/rollback,
partial/out-of-order/duplicate/conflicting/corrupt chunks, all 64 initial bit
offsets, every bit truncation at those offsets and 10,000 deterministic mutated
frames. Decoding and bit extraction commit outputs/positions only on success.
The Python runner independently calculates the scalar negotiation-ack and
max-width snapshot-ack fixtures using `struct.pack`; C++ round trips also compare
all committed `.hex` files. Golden fixtures change only with an intentional
schema change and review; `conformance <fixtures-dir> --write-fixtures` is an
authoring aid, never part of normal validation.

## Normative framing and field schema

Command reservation: **0x434f4f50** (`COOP`, positive 32-bit). Native message type
stays `NET_MESSAGE_GAME = 0x14` in its existing **five bits**. After the native
32-bit command comes the following byte-oriented envelope, beginning at the
current bit offset (usually 37); it is **not byte-aligned** in the native stream.

| Offset (bytes in envelope) | Width | Meaning |
| --- | --- | --- |
| 0 | u32 LE | 0x50433248, literal bytes `48 32 43 50` / `H2CP` |
| 4 | u16 LE | major, 1 |
| 6 | u16 LE | minor, 0 |
| 8 | u16 LE | explicit message tag, 1..17 |
| 10 | u16 LE | flags, exactly zero; unknown flags fail closed |
| 12 | u32 LE | exact body byte count, excluding the 16-byte header |
| 16 | bytes | message body; no trailing bytes or implicit padding |

Tags 1..17 are Hello, Accept, NegotiationAck, Reject, OwnershipRequest,
EditRequest, ReadyRequest, DeployRequest, RequestResult, OwnershipSnapshot,
PreparationSnapshot, PreparationDelta, BeginMission, SnapshotAck,
StartSimulation, Input, WorldChunk. Tags never depend on C++ struct layout.
`T_Message` order is intentionally bound to these tags; a reorder is a wire change.
Version 1.0 is the only implemented body schema. Unknown major/minor/tag/flags
reject; no guessed decoding or fallback. Negotiation's bootstrap header remains
1.0 while Hello's body advertises a supported version range. Both endpoints must
have a codec for the selected minor; this implementation offers only minor 0,
selecting 0 if it lies in the peer's range.

[fields.txt](fields.txt) is the **explicit normative ordered field list** for each
body/nested DTO, matching the reviewed `Fields` functions. No reflection, native
layout, pointer, bool representation, `size_t`, legacy actor width or enum storage
is copied onto the wire. The encodings are:

- Generated IDs and revisions: u64 LE. Required request/ownership/control/baseline
  identities reject zero. Optional identity presence is explicit; present zero
  rejects. Session context may have mission generation zero before reconstruction;
  resync/result baseline revisions may be zero when no corresponding state exists.
- Actor ID and slot-use: separate u32 LE, **zero valid**. ActorRef includes u64
  session and mission generation. The codec does not truncate them to legacy bits;
  the live adapter must validate actual actor catalog/engine limits before use.
- u8/u16/u32/u64: exact named width LE. i64 findings: two's-complement 64-bit LE,
  converted arithmetically (no implementation-defined signed narrowing).
- Float fields: IEEE-754 binary32 bits in u32 LE, preserving fractional skills and
  finite float bits. NaN and infinity reject on encode and decode; capacities,
  weights, charges and condition must be nonnegative. Catalog units/upper limits
  remain model validation; the codec does not guess item physics.
- All enums: u16 LE, zero-based in the merged declarations' explicit enumerator
  order; values outside the declared range reject. `E_InputKind` is 0 ControlBits,
  1 ActorAction, 2 VehicleAction. Reject has only the nine contract refusal reasons.
  Changing any enumerator order requires a new reviewed wire version.
- Bool and optional presence: u8 **exactly 0 or 1**. Present optionals immediately
  contain their value. Absent fields have no value bytes.
- Strings/tagged keys: u16 byte length, then canonical UTF-8, no terminator.
  Reject malformed/overlong UTF-8, surrogate codepoints, NUL and ASCII controls.
  Required catalog keys are nonempty; absent presentation text/assets may be empty.
  Localized briefing/finding bodies use catalog keys, not arbitrary remote UI text.
- Vectors: u32 count then ordered elements. Arrays: exactly their fixed elements,
  no count (four presentation slots). Variants: u8 zero-based explicit alternative
  tag then selected body, in the merged contract alias order. The ownership
  operations are Claim/Switch/Release; edit operations Take/Return/Move/Preset/
  AutoEquip; authoritative result state is Resync/Preparation/Ownership.
- Immutable snapshot pointers: inline the complete selected snapshot, **no pointer
  bytes/presence flag**. Null pointers reject. LocalSubmissionId, UI events,
  interfaces, callbacks and policy/model pointers have no wire encoding.
- Input: full control token, u64 nonzero sequence, u16 kind, u32 payload byte count,
  bytes. Payload is bounded adapter-owned native data, never an actor pointer or
  a directly dispatched legacy command. Input ordering is scoped to the complete
  control tuple and verified transport incarnation.

The decoder enforces structural validity and transport limits, unique roster,
loadout, stock and participant IDs, exclusive human ownership, declared participant
membership, matching loadout roster, valid slot membership and actor/session
fences where those snapshots supply a baseline. It does **not** establish catalog
membership, current model revisions, permission, content authenticity, actor
existence or equipment rules. Every decoded request still passes the serialized
authority/model validator and COOP-004's cross-family request journal; repeats,
content-conflicting replays and cache-expired IDs retain their original IDs and
bytes. The codec never retries or commits a mutation. COOP-005's native signed
quantity ceiling (32767) remains stricter than this wire ceiling.

## Limits and content fingerprints

| Resource | Protocol 1.0 limit |
| --- | --- |
| Complete envelope | 24,576 bytes including header |
| Participants / catalog soldiers | 4 / 64 |
| General vector / string | 256 elements / 256 UTF-8 bytes |
| Total nested vector elements | 8,192 per frame |
| Decoder allocation accounting | 8 MiB per frame (container elements and strings) |
| Immutable inline snapshot nesting | 16 maximum |
| Item quantity / placement coordinate / rounds | 1,000,000; model may be stricter |
| Input payload | 1..1,024 bytes |
| Chunk payload | up to 16,384 bytes |
| World transfer | 1..8 MiB, at most 512 chunks |
| In-flight world transfers | 1 per connection/assembler |
| Negotiation / incomplete world transfer deadline | 10,000 monotonic milliseconds |
| Inbound frames | 256 per one-second window per connection |

Allocation/count/remaining-byte arithmetic is checked before allocation or reads.
Byte blobs and string bytes charge memory, not DTO element counts. Limits in
Accept are explicit and must match this version; no peer-selected larger bounds.
The 24 KiB frame cap leaves packet/header headroom below native signed16 packet
sizes and the socket's 0x8000-byte reassembly buffer. **Live scheduling must also
bound aggregate/coalesced traffic**; a frame bound alone cannot bound Gather's
entire datagram. Send chunks on the reliable ordered path with an explicit byte
budget/flush, never enqueue an entire 8 MiB transfer in one Gather.

Model and UI producers must be configured to fit these limits (COOP-005 defaults
allow 4096 entries, so composition must set its bound to <=256 or agree on a new
version after measuring real catalogs). Do not silently truncate a snapshot or
split one immutable publication into uncorrelated field patches. A too-large
preparation publication fails encoding and requires a future reviewed chunked
preparation schema or lower declared catalog bound. 8 MiB world chunks do not
waive this preparation-frame constraint.

Each content descriptor string is exactly `sha256:` followed by 64 lowercase hex
digits. Five independent hashes cover campaign, mission, soldiers, items and
rules. The **catalog producer** supplies/verifies hashes; this codec checks syntax
and compares exact values and never assumes matching map names imply matching
content. Define each hash as SHA-256 of this canonical byte manifest:

```text
ASCII "HD2COOP-CONTENT-v1" + byte 0
u8 category (1 campaign, 2 mission, 3 soldiers, 4 items, 5 rules)
u32 LE entry count
for each entry sorted by raw UTF-8 relative path bytes:
    u16 LE path byte count + path bytes
    u64 LE original byte count
    32 raw bytes SHA-256(original content bytes)
```

Paths are UTF-8, normalized `/` separators, case-preserving, relative, nonempty,
with no NUL, `.`/`..`/empty components or duplicate names. Hash the exact effective
content/rule definition bytes used by both machines, including replacements and
versioned policy/algorithm definitions. Production catalog assembly and selecting
real game definitions remain later integration; no approved product policy is
invented by these tests. `a`..`e` repeated hash fixtures are deliberately synthetic
negotiation values, not fingerprints of a game installation. Requests/session
IDs and CRCs are freshness/integrity data, **not cryptographic authentication**.

## State, replica and reconstruction boundaries

Server: AwaitHello -> AwaitAck -> Admitted. Client: AwaitAccept -> Admitted after
returning a matching reliable NegotiationAck to be **queued before any payload**.
Caller installs one `C_Admission` per verified transport connection incarnation;
never share one object across reused slots or let a packet select its own object.
The server issues SessionId/PlayerId, accepts only the exact ack and rejects native
unmodded peers by deadline before spawn. Unknown required features reject;
unsupported optional features are disabled. Feature bits 0/1/2 mean distinct
identities, atomic preparation, guarded input; all three are required by defaults.
No other feature has a codec/handler in 1.0. Matching duplicate Hello/Accept/Ack
is harmless and deadline/rate-bounded; rejected/closed endpoints stay terminal.
`Tick` must run even with no incoming traffic. On timeout, the caller sends one
bounded Reject if appropriate and closes the transport.

Pre-admission Receive reads/validates only the fixed header before whitelisting
negotiation families; it never decodes a mutation/world body or calls a model.
After admission it checks direction and authenticated connection's session/player
echoes. Returned payloads are values for the authority queue, **not authorization
to call a legacy handler**. Wrong-direction/spoofed messages reject terminally.
Dormant solo/native multiplayer endpoints produce no Start/reply traffic;
`Route` preserves their existing native handlers and drops accidental extension
traffic. In campaign mode all unadapted native commands/actor/replication paths
are quarantined even after admission until their negotiated wrappers exist.

`C_InputGate` is a **preliminary** guard, not a substitute for
`C_CampaignOwnership::ValidateInput`. COOP-008 must call authoritative ownership
validation both when enqueuing and when applying actions, with the private pending
handoff barrier cleared only through `CompleteControlHandoff`. The codec gate
checks admission, Running phase, reconstruction success, sender/session/mission,
owner/soldier/actor/slot-use/control revision, script suspension and monotonic input
sequence. Keep one instance per connection/soldier, reset only when retiring the
binding, and never reset sequence while the same tuple remains usable. Drain or
invalidate queued legacy actions on transfer/load. Validate typed payload length,
actor class and opcode against the negotiated adapter before calling any handler;
the opaque payload codec cannot establish those engine-specific facts.

A PreparationDelta is exact base ViewRevision plus a **complete** replacement
snapshot, including stock/loadouts/presence/rights/findings/readiness. Replica
installation publishes a detached `shared_ptr<const ...>` only after complete
validation. Stale/old-generation data never rolls back a view. A gap or regressing
baseline disables editing (`Resyncing`, null current snapshot) until a full current
snapshot arrives. Retained handles remain immutable for display history; they
cannot re-enable editing. A duplicate result's old inline state must use this same
install rule; the local request correlation lifecycle remains the UI adapter's job.

World chunks contain the complete SnapshotAck baseline tuple (including recipient
PlayerId), total bytes, index/count, CRC32 and bytes. Fixed offsets are
`index * 16384`; every nonfinal chunk is exactly 16384 bytes and the final one is
the exact remainder. Count must equal ceil(total/16384). CRC is standard reflected
IEEE CRC32 (poly 0xedb88320, initial/final XOR 0xffffffff) across all assembled
bytes in order. Identical duplicate chunks do not advance progress; conflicting
bytes reject. All chunks match the exact externally expected manifest/generation/
revisions/recipient and transfer metadata. One bounded buffer is exposed by Take
only after completeness and CRC success. Timeout discards the partial buffer.
Only the lifecycle adapter can validate/install world semantics, actor bindings,
script/objective baseline and buffered changes; **only successful reconstruction**
permits a real SnapshotAck or input. A manifest is not that world body. Mid-mission
admission, save/load, reconnect and current-world synchronization remain later work.

## Dispatch audit and concrete live attachment plan (COOP-010)

The audit is against task base `dbc9a85a` (same native paths after COOP-001 merged).
Full handler switches, their queue writers, bitstream, transport/channels and
socket fragmentation were read. No native file is edited by COOP-006.

| Audited path | Finding / required attachment |
| --- | --- |
| `net_messages.cpp`, BeginGameMessage/BeginCommand/BeginReply | Writes five-bit 0x14 then **32-bit** command ID. Server-bound native handlers use 0..11; client-bound handlers use 0..23. Writers select those existing commands. 0x434f4f50 has no prior source collision (`rg -n '0x434[fF]4[fF]50\|1129271120' src`); reserve only this value, never widen message type or repurpose 0..23. |
| `net_messages.cpp`, HandleServerMessage INDICATE_CONNECT/CREATE_PLAYER/RECEIVE | S_JoinRequest/version/password/slot allocation stay native (0x54-byte layout unchanged). CREATE_PLAYER creates the transport link. Campaign adapter creates a quarantine record with fresh PlayerId at this verified connection-incarnation boundary; no campaign soldier/native join/spawn is allowed yet. Ticks expire unmodded peers before spawning. |
| `net_game.cpp`, TransportCallback/QueueReceived/QueueOutgoing | Callback's first connection ID is currently discarded; queue has only time/flags/size/data, not a durable verified sender. Add a campaign-owned side queue/context capturing verified link incarnation and PlayerId; never recover authorization from payload echoes, reused slot bits or current global sender. Preserve ordinary queue/layout behavior. DirectPlay RECEIVE similarly derives context from its verified player context. |
| `net_transport.cpp`, ReceiveAll/HandleDatagram | Current native packet-header connection ID lookup can update link endpoint address/port. That header alone does not prove sender identity. COOP-010 must establish/validate its connection/endpoint/session binding against the accepted transport record before selecting C_Admission; no cryptographic authentication is claimed here. Carry that binding through delayed queues, retire it on disconnect and reject old incarnation traffic. |
| `net_game.cpp`, ReceiveMessages/DispatchNetMessage | In explicit campaign mode, select the connection's admission object **before dispatch**. Check type bits and command bits are present, intercept kCommand before HandleServerCommand/HandleClientCommand. Extract exactly one envelope using ExtractFrameBits from the original byte storage and remaining bit count, call Receive, enqueue reply, then route only returned typed payload to the authority queue. On frame error abort the packet; do not continue at an untrusted offset. |
| `net_commands.cpp`, HandleServerCommand 0/1/2/4/9 and HandleClientCommand 0/3/5/20/21 | Native login/join invokes OnPlayerJoined, respawn invokes OnRespawnRequest, other commands request state, switch/remove actors or install native full state/rules/ranks. **Quarantine all campaign native game commands**, not just command 0. Allow only individually audited nonmutating presentation commands if a later explicit adapter is supplied. Native MP keeps all original handlers. |
| `net_game.cpp`, DispatchNetMessage module branch | Non-GAME types can go straight to ModuleManager::DispatchMessage/CallUnknown28. Quarantine those paths in campaign until equivalent generation-tagged wrappers are installed; gating reliable GAME messages alone is insufficient. |
| `net_game.cpp`, ReceiveMessages replication flags & 0x20; Process replication writes | Bypasses DispatchNetMessage and goes straight to g_netReplication->Read. Gate BEFORE that call; likewise suppress unwrapped campaign replication sends. COOP-008 must adapt actor/control/object payloads inside S_Input or a separately negotiated generation-tagged world path. Native replication remains unchanged in ordinary sessions. |
| `net_channel.cpp`, C_NetPacket::Read/Write; `net_socket.cpp`, Reassemble; `net_transport.cpp`, Gather | Packet length read is signed16; socket reassembly buffer 0x8000; Gather can coalesce many packets without a campaign byte budget. Use <=24KiB frame, <=16KiB chunk, <=32767-byte native command packet including all native framing; explicitly budget/flush aggregate transport output below reassembly limits. Preflight peer packet counts/lengths before unchecked native packet parsing. The codec is not a fix for legacy raw datagram parsing. |
| `bitstream.cpp`, ReadBits/ReadRawBytes/WriteRawBytes | Reads can touch a word before bounds are checked; IsValid after reading cannot prevent an overread. RawBytes floors an unaligned bit position, corrupting an offset-37 command body. Use the byte-backed safe bit adapter or an equivalently preflighted/padded bridge; emit each envelope byte through WriteBits(byte,8), never WriteRawBytes at offset 37. |
| `net_game.cpp`, SendToPlayers/SendsTo and net_messages.cpp join flow | Existing joinStage==2 and native login command conventions must not block campaign negotiation or force native join as a shortcut. The campaign path needs a quarantined-link targeted send route independent of native world admission; keep campaign admission separate from native joinStage. |

Concrete execution order:

1. Explicit campaign opt-in only. Preserve native transport handshake/layout;
   allocate fresh verified connection incarnation, session/player binding and
   admission object. Quarantine all native game/actor/world paths on both ends;
   dormant ordinary sessions never send the reserved command.
2. Client sends Hello through a targeted reliable ordered transport command.
   Host validates compatibility/content and replies Accept. Client validates
   Accept, **queues its reliable Ack first**, then exposes admitted state. Host
   admits only its matching Ack. Tick timeout/rate handling even for silent peers.
3. Pass decoded client requests through verified sender binding and one serialized
   authority queue/shared cross-family journal. Map results/publications back
   through the same codec. Model limits/catalog contents must fit the declared
   wire version; compare descriptors from real effective content.
4. COOP-009 supplies immutable manifest and world baseline, required participant
   set and barrier. Client assembles bytes, verifies manifest/baselines and
   reconstructs/binds the world. It sends SnapshotAck only after this succeeds.
   Host validates **every ack field**, exact sender and still-required membership;
   departures are not acknowledgments. Only the satisfied barrier can emit
   StartSimulation for the exact mission/snapshot. Reset assembler/replica/input
   bindings on generation/session change, without reusing old authority tokens.
5. COOP-008 enqueues/applies input only after admission and successful barrier,
   both codec preliminary guard and authoritative ownership.ValidateInput,
   CompleteControlHandoff/queue invalidation, then bounded typed payload validation.
   Do not let unreliable controls, vehicle messages or cached actions bypass it.
6. COOP-010's required **two actual running instances** test proves compatible
   admission, rejects unmodded/version/content peers without spawn or state/input
   handling, checks input during quarantine/reconstruction, and logs session,
   generation, soldier and control revision. Verify move/fire/reload/inventory,
   two human + two AI squad and original objective progress; disconnect preserves
   the same soldier/gear/position and returns it to AI. Repeat ordinary solo/native
   co-op regression and verify absence of campaign packets. These runtime checks
   have **not** been executed by this component.

The component deliberately cannot create a production session or bypass these
obligations. Its tests prove codec/admission sequencing at an injected boundary,
not a live DirectPlay handshake, controller handoff or world synchronization.
