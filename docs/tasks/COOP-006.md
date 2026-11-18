# COOP 006 Protocol codecs and compatibility

## Coordination

- Status: active
- Owner/session: Codex COOP-006 / thread `01a1ec01-a68f-7170-9f75-7536a079675b`, user assignment; coordinator acknowledgment published on `coord/coop-work-queue`
- Canonical branch: `feature/coop-006-protocol`
- Base commit: `dbc9a85a7161d8844f2907dbaf6586c7f3b67d19`
- Dependencies: COOP-002, COOP-011 (merged shared declarations)
- Last checkpoint: `12625f6c` claim; implementation checkpoint follows below
- Checkpoint location: `.worktrees/COOP-006`, `origin/feature/coop-006-protocol` (after push)
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

The assigned agent has full control of this task's branch: editing, committing, rebasing,
resolving conflicts, pushing to the project remote, and creating/updating its PR without
repeated user approval. Preserve other workers' work and shared checkpoints. When the
implementation and required validation are complete, push and create/update a PR targeting
`main`. Include reproducible **How to test** steps, prerequisites, and expected results;
separate checks actually run from proposed reviewer checks. State **Source-only** explicitly
when no runnable artifact is supplied, or **Documentation-only** for documentation tasks.
Record the PR URL in this handoff. Follow the [completion PR requirements](../agent_workflow.md#review-and-integration);
source-only delivery does not waive required runtime acceptance evidence.

## Result and scope

Implement the agreed campaign protocol codecs and compatibility negotiation component
over the existing transport interfaces. Provide an independently testable adapter boundary;
live campaign session attachment and world synchronization remain later integration work.

## Files and shared contracts

Own new protocol/codec files and network harness additions at paths agreed in COOP-002.
Inspect `src/net/bitstream.*`, `net_game.*`, `net_commands.cpp`, and message dispatch before
choosing an extension point. Serialize edits to existing dispatch files with COOP-008/009.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own `src/net/campaign_protocol.{h,cpp}`
and `_re/harness/campaign_protocol/`. Follow [the negotiation and snapshot contract](../coop_contracts.md#protocol-compatibility-and-snapshot-boundaries).
Specify version, command reservation after dispatch audit, framing, byte/bit order,
limits, content descriptors and deterministic fixtures. Keep `S_JoinRequest` and native
message layouts unchanged. Public codec binding uses merged COOP-011 DTOs.
Deliver the concrete admission/quarantine and actor-input envelope attachment plan;
COOP-010 installs live `net_game.cpp`/`net_commands.cpp`/`net_messages.cpp` hooks after
adapters merge. Tests must reject pre-admission mutation without claiming a live handshake.

## Acceptance criteria

- Encode/decode the agreed identities, revisions, requests, acknowledgments, and snapshots
  without process pointers or unchecked assumptions about legacy IDs and bit widths.
- The negotiation component accepts compatible peers and rejects unsupported campaign
  versions before processing campaign payloads; test this sequencing and malformed input.
- Verify repeated/stale messages, bounds, and deterministic round trips using fixtures.
- Ordinary multiplayer message handling remains compatible and does not inherit campaign
  UI/scoring semantics. No incompatible packet is emitted in an ordinary session.
- Document the exact live attachment point and its later test gate; codec tests alone do
  not establish that a real host/client handshake or world synchronization works.

## Handoff

- Completed: coordinator claim acknowledgment `95b46fcc`; canonical branch/worktree
  created at `dbc9a85a`; read-only native dispatch/channel/bitstream/socket audit;
  version 1.0 explicit DTO codecs, bounded admission and sender/direction checks,
  safe unaligned bit bridge, preliminary input gate, immutable replica and bounded
  CRC world assembler. No legacy dispatch or shared DTO header edits.
- Scope: `src/net/campaign_protocol.{h,cpp}`, `_re/harness/campaign_protocol/`
  (README normative schema/attachment plan, explicit fields, 27 golden fixtures,
  standalone header and eight-suite conformance runner), this task record only.
- Contract: reserved positive 32-bit command `0x434f4f50`, existing five-bit native
  GAME type unchanged. Full envelope <=24576 bytes, chunk <=16384, world <=8 MiB;
  native packet signed16 and 0x8000 socket buffer require aggregate send budgeting.
  Delta is exact base view revision plus complete replacement snapshot. Descriptor
  SHA-256 manifests and all wire encodings/limits are explicit in harness README.
- Model consumers: COOP-004 confirmed its private handoff barrier additionally
  requires authoritative `ValidateInput` at enqueue/application. COOP-005 confirmed
  native quantity cap32767 (stricter than wire1e6); live composition must configure
  <=256 collection entries or review a larger/chunked protocol after catalog audit.
  Codec never commits/retries/deduplicates model requests. No product policy chosen.
- Validation so far (2026-11-18): first 7 suites, 71,322 checks passed GCC16.2.1,
  MSVC19.44.35230 x86 /W4 /WX with SDK10.0.26100.0 and ASan/UBSan. Final bit bridge
  added eighth suite: 512,891 checks passed GCC; nested truncation expansion and
  final GCC/MSVC/sanitizer repeats pending before review. Logs under ignored
  `build/campaign_protocol/`; target runner uses own `build/wine-prefix` and
  read-only COOP-007 portable compiler/Python launcher, with owner acknowledgment.
- Remaining: final checks, separate review, published PR/review handoff. Ordinary
  runtime/game build not claimed; this independently testable component does not
  attach a live campaign handshake or world synchronization.
- Next action: rerun final eight-suite GCC, MSVCx86 and ASan/UBSan checks; inspect
  complete diff, commit/push checkpoint, request independent review and PR to main.
- Blockers: none. COOP-010 installs the exact live admission/dispatch/replication
  and reconstruction hooks described in harness README after downstream adapters.
- Source-only: no runnable game/engine artifact, installed data, compiler or build
  output is supplied or committed. Local fixture executable is validation only.
