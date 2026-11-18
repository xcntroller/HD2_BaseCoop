# COOP 008 Controller and input adapters

## Coordination

- Status: review
- Owner/session: Codex COOP-008 / thread `01a1ec1f-4589-7051-9994-2a236a14b49b`
- Canonical branch: `feature/coop-008-controller-adapters`
- Base commit: `0fcea78f85d0fe9348c3b8e13d1a0f23f50a482a`
- Dependencies: COOP-001, COOP-004, COOP-006
- Last checkpoint: source `1a992bba`; integrated-main sync `26303c03`
- Checkpoint location: `.worktrees/COOP-008` / canonical branch on origin
- Review/integration: independent review requested; not merged
- Pull request: https://github.com/xcntroller/HD2_BaseCoop/pull/17

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

Adapt controller selection, player input routing, and disconnect handling to explicit
soldier ownership. Finish and test these adapters behind an inactive campaign-session
boundary before COOP-010 enables the first playable development path.

## Files and shared contracts

Expected hotspots: `human_control.cpp`, `player_net.cpp`, `player_controller.cpp`, `team.cpp`,
`net_players.cpp`, and controller handlers. `game_rounds.cpp`, shared profile/header changes,
and actor creation overlap COOP-009; the coordinator must assign exact edits before starting.
Use the COOP-002 coupling audit to split this task further if needed.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own `src/game/campaign_control_adapter.{h,cpp}`
and the controller/input/presentation consumers in [the coupling audit](../coop_contracts.md#legacy-coupling-audit-and-adapter-allocation),
including `actors_module.cpp` controller dispatch/removal safety, human/player/remote
controllers, local team/camera/inventory/weapon view and driver ownership checks.
Export a committed handoff and exact-incarnation disconnect helper for COOP-009.
Do not edit `game_rounds.cpp`, `spawn_point.cpp`, `game_app.cpp` or profile layouts;
COOP-009 owns lifecycle call sites and calls the helper before native destructive logic.
Audit damage attribution, medic scoring, class messages and unowned AI indexing, not
just movement. Tag/guard net-object control bits, reliable actions and queued operations
with generation and control revision; a reliable-command guard alone is insufficient.
Full vehicle gameplay acceptance remains a later interaction gate.

## Acceptance criteria

- Player input resolves the controlled soldier independently of actor/player-slot equality.
- Ownership transfer updates host/guest controllers, input handlers and local camera/view
  targets while preserving soldier state. Stale input loses authority after transfer.
- Disconnect preserves the soldier/equipment and returns eligible soldiers to squad AI.
- Audit scoring/class, activity, inventory and damage paths that index players by actor ID.
  Campaign behavior preserves soldier skills rather than assigning multiplayer ranks.
- Build and run focused adapter/harness regressions for local and native co-op paths.
  Live behavior is enabled and demonstrated jointly with lifecycle work in COOP-010.

## Handoff

- Completed: dormant host/guest controller reconciliation, native engine sink,
  exact-incarnation disconnect bridge and bounded tagged input queue composed with
  the merged ownership/protocol models. Native ownership, scoring/class, damage,
  progression, inventory/view and driver consumers are adapted behind an uninstalled
  explicit campaign boundary. See [the coupling audit and composition guide](../coop_control_adapters.md).
- Source checkpoint: `1a992bba` (independent review fixes); `26303c03` merges finished main `577c04d8` with no
  COOP-008 source changes. This documentation checkpoint records the final evidence.
- Delivery: **Source-only**, inactive adapter component. No production context,
  admission, opcode, transport dispatch or menu entry is enabled. Trial-linked
  binaries and harness executables are local ignored artifacts, not distributed.
- Remaining: independent source/evidence review and integration through PR #17.
  COOP-010 supplies negotiated payload dispatch and tagged authoritative replication,
  safe lifecycle composition with COOP-009, and actual two-instance campaign behavior.
  Full vehicle/mission interaction remains a later runtime gate. These are the
  existing task boundaries, not waived acceptance for an active playable mode.
- Coordinator: COOP-009 accepted COOP-007 stopped-writer handoff at `9e051608`;
  COOP-008 has not written the ledger since `bf838b1e`. Both claims/reserved file
  scopes are acknowledged. Lifecycle/spawn/game_app/profile files remain untouched.
- Next action: reviewer reads exact candidate and full diff, reproduces harnesses,
  reviews native sink/authority fences and handoff integration contract, then handles
  review findings before merger. No uncommitted implementation changes.
- Decisions: preserve explicit simulation authority, soldier identity/state and all
  original legacy routes; no production selection/readiness policy is newly chosen.

## Validation actually performed

Commands below are run from this task's worktree. Unique output directories retain
exact compiler/link commands and diagnostics. MSVC is 19.44.35230 x86 with SDK
10.0.26100.0, Windows Python 3.13.9 via Wine and `/W4 /WX` for component suites;
GCC/MinGW is 16.2.1. Only this worktree's private Wine prefix and outputs are used.

| Check | Command / evidence | Result |
| --- | --- | --- |
| Ownership/protocol composition + actual native sink with instrumented engine | `python _re/harness/campaign_control/run.py --compiler gcc`; `build/campaign_control/gcc-u8xwbefx/` | 1,272 model/control + 232 native sink + 22 actual-source team checks PASS |
| Intended x86 component execution | Same runner `--compiler msvc` using Windows Python; `build/campaign_control/msvc-cy92fi0m/` | 1,272 + 232 + 22 checks PASS; zero compiler diagnostics |
| Full hand-port target compile | Windows Python `_re/tools/build_isolated.py check`; `build/coop008/full-check-restored.log` | 279 translation units PASS, 0 errors/warnings |
| Final native changes | Same isolated `check` for the 17 native consumers/sink; exact list in `build/coop008/final-native-check.log` and build command record | 17 files PASS, 0 errors/warnings |
| Finished-main synchronization | Isolated `check src/game/campaign_preparation_screen.cpp`; `build/coop008/main-sync-check.log` | Integrated COOP-007 source compiled separately; no COOP-008 code changed by merge |
| Whole-port trial link | Private `build/coop008/link-final.py` wraps tracked `link_check` using own isolated check objects and read-only COOP-009 engine import libraries; `build/coop008/link-final.log` | 279 objects, 0 unresolved symbols, 0 other linker messages, exit 0; main-sync extra object repeated separately |
| Final review fixes | Isolated target `check` for native sink/adapter/team/human combat; `build/coop008/retirement-final-compile.log`; whole-port link `retirement-final-link.log` | 4 files, 0 errors/warnings; 280 objects, 0 unresolved symbols |
| ASan/UBSan | `LIBRARY_PATH=.../COOP-009/build/tooling/gccsan/usr/lib64 LD_LIBRARY_PATH=.../COOP-009/build/tooling/gccsan/usr/lib64 python _re/harness/campaign_control/run.py --compiler gcc --sanitize`; `build/campaign_control/gcc-na4zcj7t/` | All 1,526 checks PASS |
| Dormant human control regression | Existing `_re/harness/human/build_control.bat` translated to isolated Linux MinGW invocation; `build/coop008/legacy-control/commands.json` and `execute-isolated-dlls.log` | 149,953 original-executable comparisons, 0 failures |
| Dormant native player/input regression | `python _re/harness/campaign_control/run_legacy.py --suite playernet ...`; `build/campaign_control_legacy/playernet-fjtjr35l/commands.json` and `execute.log` | 154,818 comparisons, 0 failures |
| Dormant remote-controller regression | Same runner `--suite remote`; `build/campaign_control_legacy/remote-tr004q5f/` | 80,002 comparisons, 0 failures |
| Diff/runner hygiene | `git diff --check`; Python `py_compile` of both runners | PASS |

The native sink test compiles the actual adapter implementation against instrumented
engine interfaces; it is not a running engine or host/guest mission. The differential
suites use the local unpacked reference executable and isolated original DLLs plus
installed Proton d3d8. One unresolved unused reference import is reported; every
selected comparison completes successfully. Reproduction prerequisites and commands
are in [the harness guide](../../_re/harness/campaign_control/README.md).

Intermediate failures were corrected and checks repeated: a shell backslash quoting
mistake; a vehicle C_Actor/C_Human cast; a legacy activity path whose extra validity
guard changed original behavior; missing DLL prerequisites for original comparison;
and a GCC-16 standard attribute-order issue in the historical harness. The corrected
activity adapter preserves the exact legacy branch and bounds only campaign mapping.

A full compile was also interrupted when the entire borrowed COOP-007 worktree and
its ignored compiler resources disappeared externally. Its owner confirms they did
not remove them. Historical passing checks remain valid; those paths are unavailable.
COOP-009 restored its own pinned toolchain and authorized read-only borrowing; the
complete successful target repeats above use that restored toolchain. No resources
were removed or changed by COOP-008. Optional ASan/UBSan initially could not link missing system `libasan.so.8.0.0`;
COOP-009 restored private matching runtime libraries, after which the complete final
suite passed ASan/UBSan using process-local library paths.

No original-mission campaign launch, two-instance campaign movement/fire/reload,
mission completion or full vehicle behavior has been tested by this task. Required
live behavior is demonstrated jointly with lifecycle integration in COOP-010,
consistent with the acceptance criteria above.

## Independent review fixes

COOP-009 reviewed the complete initial candidate `374a9347`, reproduced its component
suite and identified native retirement after catalog unbind, retained script handler
contexts, and missing host squad death/environment rules. Source `1a992bba` resolves
these with actual-sink/real-model and actual-source team regressions. It also checks
native plans against the exact committed control revision, adds pre-reconstruction
retirement seams, restores original address comments to their original signatures,
and records legacy runner environment/compiler versions. Independent final-delta
review is pending; no integration approval is inferred from these fixes.
