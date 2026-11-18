# COOP 006 Protocol codecs and compatibility

## Coordination

- Status: review
- Owner/session: Codex COOP-006 / thread `01a1ec01-a68f-7170-9f75-7536a079675b`, user assignment; coordinator acknowledgment published on `coord/coop-work-queue`
- Canonical branch: `feature/coop-006-protocol`
- Base commit: `dbc9a85a7161d8844f2907dbaf6586c7f3b67d19`
- Dependencies: COOP-002, COOP-011 (merged shared declarations)
- Last checkpoint: `08a2f5e2` implementation; `5bcdae38` main sync; `f4ed401c` validation/attachment evidence
- Checkpoint location: `.worktrees/COOP-006`, `origin/feature/coop-006-protocol` (after push)
- Review/integration: independent review requested from coordinator; component validation passed; no integration yet
- Pull request: [#13](https://github.com/xcntroller/HD2_BaseCoop/pull/13) (ready for independent review)

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
- Validation (2026-11-18), implementation `08a2f5e2`: final **8 suites,
  541,790 checks passed** separately on GCC16.2.1, MSVC19.44.35230 **x86** /W4 /WX
  with SDK10.0.26100.0 and GCC ASan+UBSan (leak detection, fail-fast UB). Standalone
  header and independent Python scalar golden fixtures pass. No runtime diagnostics.
  Source/fixtures unchanged by main sync `5bcdae38`.
- Reproduce portable target: `python3 _re/harness/campaign_protocol/run.py --compiler gcc`
  (`build/campaign_protocol/gcc-d_6v167o/` logs/argv/environment). Required Windows
  command: `python _re\harness\campaign_protocol\run.py --compiler msvc` with
  VS2022 x86/SDK setup or `HD2_VCVARS32`. Actual local run:
  `env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-007\build\tooling\run-python.bat' 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-006\_re\harness\campaign_protocol\run.py' --compiler msvc`.
  Read-only compiler borrowing acknowledged by COOP-007; own prefix and newly unique
  outputs `build/campaign_protocol/msvc-nauerudj/`; no shared global temp paths.
- Sanitizer check: normal hosts use `python3 _re/harness/campaign_protocol/run.py --compiler gcc --sanitize`.
  This Fedora image lacked GCC sanitizer shared objects. Downloaded/extracted only
  local matching libasan/libubsan16.2.1-3.fc44 RPMs under ignored `build/tooling/`;
  no system packages/settings changed. Actual compile:
  `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -L build/tooling/sanitizers/usr/lib64 src/net/campaign_protocol.cpp _re/harness/campaign_protocol/conformance.cpp -o build/campaign_protocol/sanitized`.
  Actual execution:
  `env LD_LIBRARY_PATH="$PWD/build/tooling/sanitizers/usr/lib64" ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 build/campaign_protocol/sanitized _re/harness/campaign_protocol/fixtures`.
  Passed same 541,790 checks, no sanitizer findings. Earlier seven-suite checks also
  passed GCC/MSVCx86/sanitizers; final evidence supersedes them.
- `git diff --check` passed; complete diff scope reviewed, no legacy/generated code,
  data, artifacts or shared declaration edits. Own separate review pass checked
  bounds/UTF8/float/counter schemas, early decode whitelist, role/sender direction,
  authority tuple, exact-base immutable installation and chunk integrity/timeout.
- Separate review refinement: regression reproduced acceptance of a preparation
  actor reference from another session/generation (`gcc-ukn2hg4s`, expected failure),
  then checked all prep/ownership actor fences and duplicate actor coordinates
  including distinct slot-use values. Corrected WrongSession fixture to remain
  internally coherent. COOP-005 confirmed result metadata/generation equality for
  attached state variants; codec now enforces it, preserving cached duplicate
  state+metadata. No schema/golden-fixture changes. Expanded **541,799 checks** pass
  GCC (`gcc-dskpzcou`); latest target/sanitizer repeats pending.
- Independent reviewer: COOP-001 session accepted full read-only review after its
  completed task. Reviewer caught the transient WrongSession fixture race; fixed
  as above before checkpoint. No reviewer approval inferred until final report.
- Remaining: independent review, resolve any findings, mark PR ready and coordinator integration handoff. Ordinary
  runtime/game build not claimed; this independently testable component does not
  attach a live campaign handshake or world synchronization.
- Next action: independent COOP-004 review through coordinator; resolve findings,
  publish final review/evidence and request integration of PR #13.
- Blockers: none. COOP-010 installs the exact live admission/dispatch/replication
  and reconstruction hooks described in harness README after downstream adapters.
- Source-only: no runnable game/engine artifact, installed data, compiler or build
  output is supplied or committed. Local fixture executable is validation only.
