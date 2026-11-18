# COOP 001 Reproducible build and baseline

## Coordination

- Status: done
- Owner/session: Codex / user-assigned COOP-001, 2026-11-18
- Canonical branch: `feature/coop-001-build-baseline`
- Base commit: `7e033adde95881baad06c6877cbbd6eef8197d96`
- Dependencies: none
- Last checkpoint: reviewed implementation `747d0590`; review metadata `949e54e6`; runtime game/engine source `ca28d52c`
- Checkpoint location: `origin/feature/coop-001-build-baseline`; `.worktrees/COOP-001`
- Review/integration: independent COOP-004 review passed at `747d0590`; PR #2 merged into main at `15b572ff2d86425e1e4a7d62d10b4813d0dafb24`
- Pull request: https://github.com/xcntroller/HD2_BaseCoop/pull/2 (merged)

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

Establish the runnable x86 build, its prerequisites, and a reproducible singleplayer/native
co-op baseline. Make concurrent builds safe by isolating outputs or documenting a verified
serialization procedure. Diagnose tooling before gameplay tasks depend on it.

## Files and shared contracts

Own build/run guidance and focused build-tool fixes in `_re/tools/` when needed. Inspect
the engine build scripts as consumers. Coordinate shared build changes with other workers;
do not port the engine or change gameplay as part of this task. Existing scripts use fixed
temporary directories, so verify cache separation across worktrees.

## Acceptance criteria

- Record exact toolchain, dependency, build, and launch commands for this environment.
- Build the playable executable and matching engine libraries with x86/x87 requirements.
- Record a solo mission run and a real host/guest native co-op baseline, with tested commits.
- Demonstrate that concurrent builds cannot consume another worktree's stale objects, or
  validate and document serialized builds until isolated paths are supported.
- Missing game data/toolchains are a blocker to the baseline gate, not a passing result.
  Split independently finished tooling improvements out if runtime access remains blocked.

## Acceptance evidence

| Criterion | Actual evidence |
| --- | --- |
| Exact environment/dependency/build/launch commands | [Build/baseline guide](../build_baseline.md): pinned downloader source, compiler/SDK/Python versions, local RPM extraction, Wine configuration, x87 flags, private game/prefix setup, solo and native host/guest commands. |
| Playable x86 executable and matching DLLs | MSVC 19.44.35230 x86: rw_data link passed, 78 LS3DF units compiled/link passed, 272 game units compiled with zero diagnostics/link passed. All three PE machine headers 0x014c; exact hashes in guide. No gameplay C++ changes. |
| Solo and real native co-op run with commits | Original Campaign/Tutorial: scripted intro, movement, inventory, return/exit. Two actual Wine game processes/prefixes connected to native Sicily2: independent loadouts/spawn, both directions of visible movement replication, inventory and weapon selection, two-player native status/ping/objectives. Source checkpoint `ca28d52c`; tests/scripts repeated at `23079565`. |
| Concurrent/stale output safety | Three process-level tests passed under native Linux and clean embedded Windows Python/Wine; paths with spaces, cross-tree same filename, same-tree engine variant lock, failure exit propagation/release. Two real concurrent MSVC c_str.cpp checks in distinct worktrees each passed with their own root/response/object. |
| Missing prerequisites remain blockers | Initial missing MSVC/SDK/Wine dependencies were resolved locally. Installed game data is present and 1.12.0 menu observed. GCC fallback remains unavailable for game syntax because gnu/stubs-32.h is absent; it is not counted as build/runtime evidence. |

The runtime baseline is bounded: **partial Tutorial, two-player native mission run**.
No mission completion, original campaign co-op mode, save/load, internet/NAT, dedicated
server, healing or full interaction compatibility is claimed. Synthetic firing/reload
input did not establish an ammunition change and is explicitly **not a passed check**.
That remains a required check for later controller/mission acceptance; no gameplay fix
or waiver of those gates is introduced here.

## Validation actually performed

Detailed commands, versions, artifact hashes and exact launch/settings are in the
[guide](../build_baseline.md). Key commands executed from `.worktrees/COOP-001`:

```bash
python _re/tools/test_build_isolated.py -v
python _re/tools/configure_wine_msvc.py --msvc-version 14.44.35207 --sdk-version 10.0.26100.0
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'build\tooling\run-python.bat' '_re\tools\test_build_isolated.py' -v
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'build\tooling\run-python.bat' '_re\tools\build_isolated.py' ls3df
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'build\tooling\run-python.bat' '_re\tools\build_isolated.py' rw-data
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'build\tooling\run-python.bat' '_re\tools\build_isolated.py' game --all --copy 'build\runtime\solo'
git diff --check
```

- Tests: all three passed natively and under Wine. Initial embedded-Python child import
  failure was corrected by an explicit test-child module path; rerun passed with stock
  `python313._pth`. Launcher doesn't require modifying embedded Python configuration.
- Real concurrency check: task and detached cache-probe worktrees independently report
  `1 files, 0 errors, 0 warnings`; commands/paths are in the guide.
- Runnable builds: all three links passed. Game retains `/arch:IA32`; optional missing
  upstream icon/title resources did not prevent menu/mission operation.
- Native runtime: host/guest use separate prefixes/displays, copied installation/data,
  matching port DLLs, installed native DirectPlay components, IPv4, LAN port 31567/31568.
  `port 11001 domain local` command ordering is material. Both appear on native Sicily2
  status with zero kills/deaths and observed guest ping 6; original objectives not complete.
- No `.RPT` crash reports generated. Local compiler/runtime logs and screenshots stay
  ignored under `build/`; the guide lists exact evidence filenames and limitations.
- Documentation checks: relative link targets, Python snippet syntax, shell snippet
  syntax, whitespace and complete diff/scope review are performed before final publication.

## PR test instructions and delivery

**Source-only** build tooling/documentation. Runnable EXE/DLL/PDB files were built and
tested locally, but are not distributed in Git; installed assets/toolchains/credentials
are excluded. This is wired into build guidance, not a new gameplay entry point.

1. Follow `docs/build_baseline.md` prerequisites and setup using a legitimate Sabre
   Squadron1.12 installation; build matching DLLs and x86/x87 executable through wrapper.
2. Run native/Windows process isolation tests and real two-worktree compile check.
   Expect independent object roots, same-tree collision rejection and passing diagnostics.
3. Repeat the original Tutorial flow and native two-client Sicily2 commands/actions.
   Expect normal mission loading, each client's inventory and bidirectional movement
   replication. Confirm firing/reload separately; these were not established here.
4. Compare actual observations with the bounded baseline; review full diff and ensure
   no gameplay, generated tier1, game assets or build outputs are included.

## Handoff

- Completed: canonical branch/worktree and published coordinator claim; diagnosed and
  provisioned portable MSVC/SDK/Windows Python; build wrapper/locks and focused tests;
  explicit toolchain override; reproducible Wine configurator; full matching runnable
  builds; solo Tutorial and real native host/guest baseline; detailed command/evidence guide.
- Files/interfaces: `_re/tools/build_isolated.py`, `test_build_isolated.py`,
  `configure_wine_msvc.py`, `build_check.py`; `ls3df/build.py` environment override only;
  root README, agent workflow, baseline guide and this record. No game/engine C++ edits,
  protocol/layout changes or generated tier1 edits. Scope acknowledged in coordinator ledger.
- Remaining: none for the bounded build/baseline scope. Independent final tooling/document
  review passed; PR #2 integrated at `15b572ff2d86425e1e4a7d62d10b4813d0dafb24`.
- Next action: downstream workers use the guide and retain their own runtime acceptance gates.
  COOP-008/009 remain planned until their other component dependencies merge.
- Known limits: partial runs and loopback only; unconfirmed firing/reload, mission completion,
  saves/healing/vehicles and future campaign gameplay retain their separate evidence gates.
- Local state: implementation branch clean and published. Ignored
  toolchain/download cache, private prefixes/runtime copies, logs/screenshots and detached
  cache-probe remain accessible; installed game/prefix were read-only sources.
- Checkpoint publication: `origin/feature/coop-001-build-baseline`, merged PR #2. No work on
  main; other task branches/worktrees preserved. Operational coordinator ledger is separate.

## Independent final review

COOP-004 worker (`01a1ebf9-60c7-74d0-9d39-cd09295875b0`) reviewed `747d0590`
against integrated main: read all tooling diffs, task/guide, invocation and locking
boundaries; reproduced `python3 _re/tools/test_build_isolated.py -v` (3/3 passed).
No blocking findings. Confirmed child-only temporary environment, per-family exclusive
locks, shared locks for engine variants/check-link, argv/path handling, explicit
portable toolchain override and nine-file build/docs-only scope. Accepted the bounded
build/Tutorial/native two-client baseline with unconfirmed firing/reload/completion
and later runtime gates explicitly retained. Reviewer did not reproduce full MSVC
build or gameplay; those actual local observations are recorded by COOP-001 above.
No reviewer edits to this task tree. Final metadata-only update records this review.
