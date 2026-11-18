# COOP 006 Protocol codecs and compatibility

## Coordination

- Status: done
- Owner/session: Codex COOP-006 / thread `01a1ec01-a68f-7170-9f75-7536a079675b`, user assignment; coordinator acknowledgment published on `coord/coop-work-queue`
- Canonical branch: `feature/coop-006-protocol`
- Base commit: `dbc9a85a7161d8844f2907dbaf6586c7f3b67d19`
- Dependencies: COOP-002, COOP-011 (merged shared declarations)
- Last checkpoint: source `5c562770`; current-main sync `135efd77`; final review/evidence in this record
- Checkpoint location: `.worktrees/COOP-006`, `origin/feature/coop-006-protocol`
- Review/integration: independent COOP-001 full implementation + focused final review PASS; final GCC/MSVCx86/sanitizer checks PASS; merged `59cb89441c87b9398c9fe54fd3284c1e53a5ce87`
- Pull request: [#13](https://github.com/xcntroller/HD2_BaseCoop/pull/13) (merged 2026-11-18 at `59cb8944`)

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
  created at `dbc9a85a`; read-only dispatch/channel/bitstream/socket audit; version
  1.0 explicit DTO codecs, compatibility/admission and sender/direction checks,
  safe unaligned bit bridge, preliminary input gate, immutable preparation replica
  and bounded CRC world assembler. No native dispatch/shared DTO header edits.
- Scope: `src/net/campaign_protocol.{h,cpp}`, `_re/harness/campaign_protocol/`
  (normative schema/attachment plan, 53 explicit field schemas, 27 golden fixtures,
  standalone header and eight-suite runner), this task record only.
- Contract: reserved positive 32-bit command `0x434f4f50`, native five-bit GAME type
  unchanged. Envelope <=24576 bytes, chunk <=16384, world <=8 MiB; native signed16
  packets and 0x8000 socket buffer require live aggregate send budgeting. Delta is
  exact base view plus complete replacement snapshot. SHA-256 content manifests,
  scalar/container encodings, limits and concrete live hook/test gates are in
  `_re/harness/campaign_protocol/README.md`.
- Consumers: COOP-004 confirms private handoff barrier still requires authoritative
  `ValidateInput` at enqueue AND application. COOP-005 confirms result metadata
  matches attached state/full current epoch, native quantity cap32767 is stricter
  than wire1e6, and live model collection limits must be <=256 or agree a reviewed
  larger/chunked protocol after catalog audit. Codec never retries/commits or
  deduplicates model requests. No product policy chosen.
- Final source checkpoint `5c562770`; sync `135efd77` includes integrated COOP-004/005
  main `5f561496`, protocol/header/harness byte-for-byte unchanged by sync.
- Validation (2026-11-18): **8 suites / 541,811 checks PASS** separately on GCC16.2.1,
  MSVC19.44.35230 **x86** /W4 /WX, SDK10.0.26100.0, and GCC ASan+UBSan with leak
  detection and fail-fast undefined-behavior checks. Standalone header and independent
  Python scalar golden fixtures pass. No compiler/sanitizer diagnostics. Logs/argv/
  environment are local ignored artifacts; no game/engine artifact supplied.
- Native command: `python3 _re/harness/campaign_protocol/run.py --compiler gcc`;
  final output `build/campaign_protocol/gcc-ihns3m_g/`. Standard intended target:
  `python _re\harness\campaign_protocol\run.py --compiler msvc` with VS2022 x86
  developer environment or `HD2_VCVARS32`. Actual local Wine command:
  `env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-007\build\tooling\run-python.bat' 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-006\_re\harness\campaign_protocol\run.py' --compiler msvc`.
  Final target output `build/campaign_protocol/msvc-q8vheuxf/`; COOP-007 compiler
  borrowed read-only with acknowledgment, own Wine prefix and unique build output.
- Standard sanitizer command:
  `python3 _re/harness/campaign_protocol/run.py --compiler gcc --sanitize`.
  This Fedora image lacked shared sanitizer objects; matching libasan/libubsan
  16.2.1-3.fc44 RPMs were extracted under ignored local `build/tooling/`, without
  system changes. Actual compile:
  `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -L build/tooling/sanitizers/usr/lib64 src/net/campaign_protocol.cpp _re/harness/campaign_protocol/conformance.cpp -o build/campaign_protocol/sanitized`.
  Actual execution:
  `env LD_LIBRARY_PATH="$PWD/build/tooling/sanitizers/usr/lib64" ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 build/campaign_protocol/sanitized _re/harness/campaign_protocol/fixtures`.
  Same final 541,811 checks passed, zero sanitizer findings.
- Independent review: COOP-001 thread `01a1ebd1-3990-7553-920e-7cfee0148527`
  read complete protocol/header/harness/schema/contracts and native attachment paths,
  reproduced final GCC header/fixtures/eight suites (541,811 checks;
  `build/campaign_protocol/gcc-4_7_4vha/`), whitespace clean. Full review + focused
  delta PASS at `5c562770`; reviewer confirmed main sync `135efd77` changes none of
  that source. Reviewer did not claim MSVC/sanitizer/runtime checks.
- Findings resolved: actor session/generation and duplicate-coordinate coherence;
  result metadata equals attached state; replica retains highest observed generation/
  view after a gap so delayed full snapshots cannot re-enable edits; manifest actors
  and corresponding bindings agree when both present; absent reconstruction actors
  remain valid. Duplicate Hello wording accurately limited to AwaitAck.
- `git diff --check` and full scope/history inspection pass. Ad-hoc checks confirmed
  all 53 normative field schemas match codec source and local task/harness Markdown
  links resolve. Malformed decode/extraction outputs/positions remain unchanged.
- Integrated: PR #13 merged `59cb89441c87b9398c9fe54fd3284c1e53a5ce87` after
  independent review and exact-head `cb946f2a` scope/mergeability gate. This
  documentation-only follow-up records done and makes COOP-008/009 ready.
- Remaining: none for COOP-006. All playable/live acceptance belongs to downstream
  adapters/integration and remains unwaived.
- Next action: claim ready COOP-008/009 through the coordinator when assigned;
  use the codec/attachment contract and preserve COOP-007's active UI work.
- Blockers: none. COOP-010 installs live admission/dispatch/replication and world
  reconstruction hooks after COOP-008/009. No live handshake, world synchronization,
  ordinary-game runtime regression or full game build is claimed by this component.
- Delivery: **Source-only**. Local fixture executables are validation only; no game
  binaries/data, compiler, installed resources or build outputs are committed.
