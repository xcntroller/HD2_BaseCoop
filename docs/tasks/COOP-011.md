# COOP 011 Compiled shared campaign declarations

## Coordination

- Status: done
- Owner/session: Codex COOP-011 / thread `01a1ebe7-2361-7503-8dcb-6e0739646604`, 2026-11-18
- Canonical branch: `feature/coop-011-shared-declarations`
- Base commit: `9aec1d7e1f777de186801cb1b2c320ff851746c6`
- Dependencies: COOP-002 (document-level contracts merged as `9f59ff4790f02ef73e3e02c850f79cfa9ba95130`)
- Last checkpoint: implementation/validation `bfc60e41`, review handoff `de5449b9`; merged `0f367ed6`
- Checkpoint location: `.worktrees/COOP-011`; `origin/feature/coop-011-shared-declarations`; claim acknowledged at coordinator commit `72ced131`
- Review/integration: final coordinator review passed; merged by PR #6 as `0f367ed62af337633941bca0d22d3ff1932129ba`
- Pull request: https://github.com/xcntroller/HD2_BaseCoop/pull/6 (merged)
- Completion metadata: `docs/coop-011-completion`, based on actual merged main `0f367ed6`; completion PR publication follows this checkpoint

## Branch authority and completion PR

The assigned agent has full control of this task's branch, including editing, committing,
rebasing, resolving conflicts, pushing to the project remote, and creating/updating its
PR without repeated user approval. Preserve other workers' work and shared checkpoints.
When implementation and required validation are complete, push and create/update a PR
targeting `main`. Follow the [completion PR requirements](../agent_workflow.md#review-and-integration).
Include reproducible test commands, prerequisites and expected results; distinguish
checks run from suggested reviewer checks. State **Source-only** if no runnable artifact
is supplied. Source-only delivery does not waive the target compile requirement.

## Result and scope

Translate the reviewed [COOP-002 logical contract](../coop_contracts.md) into a small
engine-independent shared header so ownership, preparation, codecs and fake UI consumers
can compile independently against one set of declarations. The coordinator approved this
separate foundation while COOP-002 refined downstream scopes. It replaces the initial
idea of having an implementation task publish common types as a side effect.

This task supplies declarations and a compile/fake-interface conformance fixture. It
implements no ownership transitions, equipment transactions, protocol codec, game adapter
or shipping UI. The downstream tasks retain those results and their acceptance gates.

## Files and shared contracts

Own `src/game/campaign_contracts.h` and `_re/harness/campaign_contracts/`, including a
documented compile command. Cover distinct identity value types, lifecycle/context views,
ownership records/requests/results, preparation operations/snapshots/events and the
`I_CampaignPreparation` boundary. Use the port's PascalCase methods and `S_` data names;
the logical interface spelling above is shared with COOP-003. Do not add engine pointers,
profile globals, Windows transport objects, packed legacy layouts or a speculative wire
ABI to these declarations. Wire widths/encoding remain COOP-006's responsibility.

Review names, snapshot lifetime, optional/invalid identities, request correlation and
construction needs with COOP-004/005/006/007 consumers. Keep ownership and preparation
implementations behind injected catalog/policy views. Shared header edits after merge
require notifying consumers; do not copy competing definitions into implementation files.

## Acceptance criteria

- Declare every shared identity with explicit scope/invalid semantics and type separation;
  do not permit implicit actor-ID/player-slot/SoldierId substitution.
- Express complete immutable snapshot and typed request/result/event boundaries from
  COOP-002, including separate preparation/view revisions, control tokens, capabilities,
  pending outcomes, resync and explicit unresolved-policy blockers.
- Compile a standalone translation unit using the intended MSVC x86 toolchain, independent
  of game data/engine DLLs, and record the exact compiler version/command/result. A g++
  syntax-only check can supplement but cannot replace this target check.
- Compile a minimal fake `I_CampaignPreparation` and representative DTO construction used
  by consumers; verify no writable campaign/profile object is exposed. This is interface
  conformance evidence, not a fake implementation claimed as production gameplay.
- Record explicit consumer/API review and preserve legacy engine/serialized structures.
  No runtime host/guest test is required for this declaration-only component.

## Validation evidence

Declaration-only component; **Source-only**. Implemented shared values/interfaces and
standalone fixtures; not wired into game dispatch, controllers, menus or legacy structures.
No game data, original assets, installed binaries, compiler files or build outputs committed.

The [harness README](../../_re/harness/campaign_contracts/README.md) gives prerequisites,
exact reproduction commands, flags, API semantics and consumer coverage. All outputs are
private invocation directories under this worktree's ignored `build/campaign_contracts/`;
Wine uses this worktree's own `build/wine-prefix`, separate from COOP-001 runtime tests.

Checks actually run on 2026-11-18 (final header/fixture content in this implementation commit):

- Intended target: MSVC **19.44.35230 x86**, toolset `14.44.35207`, Windows SDK
  `10.0.26100.0`, Windows Python `3.13.9`, Wine `11.21 (Staging)`. Exact entry command
  from `.worktrees/COOP-011`:

  ```bash
  env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-001\build\tooling\run-python.bat' 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-011\_re\harness\campaign_contracts\run.py' --compiler msvc
  ```

  Exit **0**. `header_only.cpp` and `conformance.cpp` compile with `/std:c++17 /EHsc
  /W4 /WX /permissive- /Zc:__cplusplus /arch:IA32 /MT /O2`; `_M_IX86` and 32-bit
  pointer assertions pass. Fixture links `/MACHINE:X86 /SUBSYSTEM:CONSOLE` and runs,
  printing the expected identity/DTO/interface/immutable-lifetime success line.
  Six intentional misuse cases fail compilation (exit 2 each). Final logs and exact
  compiler argv: `build/campaign_contracts/msvc-x_yfbzp4/{commands.json,environment.json,*.log}`.
  `/Bv` records compiler passes `19.44.35230.0`. No engine libraries/data used.
- Supplementary native: `python3 _re/harness/campaign_contracts/run.py --compiler gcc`.
  GCC `16.2.1 20261029`, native Python 3, C++17 with `-Wall -Wextra -Werror -pedantic`.
  Exit **0**; header/object compile, fixture execution and all six expected failures
  pass. Final logs: `build/campaign_contracts/gcc-5w9hqyfz/`.
- `git diff --check`: pass; final changed-path/commit review limited to this task.
  Changed Markdown link check: 2 documents, 6 local file links; pass.
- Early target fixture compile exposed a signed/unsigned optional comparison under
  `/WX`; corrected to `5u`. Rerun passed. This was fixture code, not a suppressed warning.

Not run: full game build, native preparation rendering, ownership/equipment model behavior,
codec round trips or host/guest gameplay. They are outside this declaration-only gate;
no playable feature or target game artifact is claimed.

## Explicit consumer/API review

- COOP-004/005/006: coordinator Codex, thread `01a1ebd1-3990-7553-920e-7cfee0148527`,
  read-only header and fixture review on 2026-11-18: **PASS**, no blocking findings.
  Reviewed distinct identities, optional actor zero, generations/control tokens, explicit
  target/control revision sets, immutable lifetimes, all preparation operations/events,
  injected catalog/policy boundaries and codec ownership of framing/negotiation.
- COOP-003/007: UI owner Codex, thread `01a1ebd2-c9c0-73a1-9495-337780d904f6`,
  read-only API/source review on 2026-11-18: **PASS** after corrections below; no remaining
  blocking UI API findings. Reviewed snapshot completeness, four optional slots,
  host-unassigned AI rights, scripted/read-only state, pending/result correlation,
  readiness revisions, catalog injection and screen-close lifetime behavior.
- Resolved findings: changed skill values to floats, preserving fractional campaign
  stats (`src/game/soldier.h` and `debriefing.cpp::AddStat`); fixture retains `80.25f`.
  Scoped status events by session/generations/view and documented invalidation/reread
  semantics so old Loading/Closed cannot replace current state. Fixture requests now
  increase IDs across families. Added backpack weight/capacity and belt weight values.
- No new product decisions: unresolved selection/deployment/other policies remain explicit
  values and blockers. No legacy `IsLocalGame`, engine interfaces or serialized layouts changed.

## Final integration review and merge

- Coordinator/integrator thread `01a1ebd1-3990-7553-920e-7cfee0148527` reviewed the
  complete header/fixture/API boundaries and source-only handoff against main.
- Independently reproduced native GCC compile/execution/six negative cases:
  `build/campaign_contracts/gcc-amf9djfu/` in the preserved COOP-011 worktree; pass.
- Independently reproduced MSVC 19.44 x86 `/W4 /WX` compile/execution/six negative
  cases: `build/campaign_contracts/msvc-vczj8k1p/`; pass. Coordinator diagnostic log:
  `.worktrees/COOP-001/build/tooling/coop-011-review-msvc.log`.
- PR #6 actually merged as `0f367ed62af337633941bca0d22d3ff1932129ba` on 2026-11-18.
  The declaration-only acceptance criteria are met. No gameplay, UI rendering or
  production model implementation is inferred from this completion.
- Documentation follow-up checks: `git diff --check` passed; `git merge-base
  --is-ancestor <merge> origin/main` passed for the COOP-002/003/011 merges above.
  Inline Python verification passed: exactly five task records changed, done/ready
  and unassigned states correct, acceptance sections unchanged, 11 local links valid.
  Documentation-only follow-up; no additional build/runtime check needed.
- Coordinator authorized the narrow documentation follow-up marking this task done
  and COOP-004/005/006/007 ready, after verifying dependency merges. Task branches
  and private worktrees remain preserved; no ledger/index edit is part of that follow-up.

## PR test instructions and delivery

- Delivery: declaration component; **Source-only**. Fixture executables are local ignored
  validation artifacts; production gameplay and fake UI integration are not supplied.
- Prerequisites: Windows Python 3, MSVC x86 C++17 and Windows SDK (or the documented
  local portable MSVC/Wine environment). No game installation, mission or player setup.
- Reviewer steps: from worktree root run `python _re\harness\campaign_contracts\run.py
  --compiler msvc` in an x86 developer environment. Expect exit 0, standalone header and
  executable success, six expected compile failures and the fixture success line.
  Optional native command above reproduces supplemental checks. Inspect each diagnostic,
  `commands.json`, and full diff; do not substitute GCC for target validation.
- Actual checks and limitations are listed separately above.

## Handoff

- Completed: coordinator claim, canonical branch/worktree, distinct identity/key values,
  context/lifecycle/control and ownership DTOs, complete preparation request/snapshot/result/
  event declarations, seven-method interface, immutable injection boundaries, target/native
  fixtures and explicit consumer reviews. Root checkout preserved on main.
- Remaining: none for COOP-011; downstream components retain their own implementation
  and acceptance gates.
- Next action: coordinator updates its operational ledger/index as needed and accepts
  separate ready-task claims. Shared-header edits require consumer notification.
- Decisions: confirmed design and COOP-002 contracts only; the fixture chooses no product policy.
- Publication: PR #6 created through authenticated `gh` after connector create returned
  403 (integration lacks repository PR write access). Branch pushes succeeded normally.
- Local state: canonical feature branch/worktree and origin checkpoints preserved; private
  compiler artifacts remain in that worktree's ignored build directory. Completion metadata
  lives in its separate `.worktrees/coop-011-completion` worktree/branch.
- Review notes: COOP-004/005/006/007 are now dependency-ready; COOP-007 also has
  already-merged COOP-003. They remain unassigned until coordinator acknowledgment.
  No downstream implementation or runtime acceptance has been completed by this task.
