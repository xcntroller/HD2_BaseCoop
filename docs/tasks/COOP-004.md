# COOP 004 Session and soldier ownership model

## Coordination

- Status: review
- Owner/session: Codex COOP-004 / thread `01a1ebf9-60c7-74d0-9d39-cd09295875b0`, 2026-11-18
- Canonical branch: `feature/coop-004-ownership`
- Base commit: `0f367ed62af337633941bca0d22d3ff1932129ba`
- Dependencies: COOP-002, COOP-011 (merged shared declarations)
- Last checkpoint: corrected implementation `1e276554`; final target evidence `1958a63a`; final review metadata in this subsequent commit
- Checkpoint location: `.worktrees/COOP-004`; `origin/feature/coop-004-ownership`
- Review/integration: independent COOP-005 full review **PASS** at `1958a63a`; corrected native/target regression passed; ready for integration, not merged
- Pull request: https://github.com/xcntroller/HD2_BaseCoop/pull/9 (ready for integration)

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

Implement the agreed session and soldier ownership model independently of live controllers
and transport. Consumers can resolve a player's soldier and authorize a control transfer
without treating the soldier's actor ID as a connection slot.

## Files and shared contracts

Own new ownership/session implementation files and focused tests at paths agreed in COOP-002.
Consume shared identity and transition contracts. Legacy controller and mission adapters
belong to COOP-008/009; avoid concurrent edits to those files.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own `src/game/campaign_session.{h,cpp}`,
`src/game/campaign_ownership.{h,cpp}`, and `_re/harness/campaign_ownership/`. Follow [the identity, lifecycle and ownership
contracts](../coop_contracts.md#identity-and-session-context), including connection
incarnations, per-soldier control revisions, script suspension and actor-binding invalidation.
Consume COOP-011 shared declarations; coordinate changes with all consumers before
editing that common header. Do not create independent incompatible copies.
No legacy controller/profile layout edits.

## Acceptance criteria

- Enforce one human owner per soldier, at most one soldier per player, and campaign squad limits.
- Handle claims, switching, disconnect, death, temporary scripted control, and mission reset
  according to the contract without changing a soldier's health, inventory, or identity.
- Reject invalid/duplicate/stale requests and ownership races. Verify mission-generation
  and ownership-revision boundaries with behavioral tests.
- Preserve ordinary singleplayer/native co-op behavior; no unfinished UI entry is exposed.
- Provide documented APIs and meaningful model tests. Runtime playability is not this
  module's deliverable; the live adapter and two-player gates remain separate.

## Validation evidence

**Source-only** engine-independent component. No game/menu/transport/controller route is
connected, and no legacy engine, protocol, profile or serialized layout was changed.
Runtime playability is explicitly outside this module's acceptance; controller handoff,
world barrier/replication and host/guest evidence remain COOP-008/009/010.

The [harness README](../../_re/harness/campaign_ownership/README.md) documents complete
API/lifetime, thread/queue, policy, catalog, counter, journal and adapter obligations,
plus prerequisites and exact reproduction commands. The harness compiles the actual
session/ownership implementation, not a replacement fake. Its policy view is an explicitly
permitted development fixture; selection/switching product policy remains unresolved.

Initial checks run on 2026-11-18 at implementation `68bb0bc1` (final corrected-head
evidence is recorded under the review correction below):

- `python3 _re/harness/campaign_ownership/run.py --compiler gcc`: exit **0**;
  GCC **16.2.1 20261029**, C++17, `-Wall -Wextra -Werror -pedantic`.
  Nine suites, **10,618 checks passed**. Exact argv/environment/compile/execute logs:
  `build/campaign_ownership/gcc-9iu_rw4s/`.
- Intended target: MSVC **19.44.35230 x86**, toolset `14.44.35207`, SDK
  `10.0.26100.0`, Windows Python `3.13.9`, Wine `11.21 (Staging)`. Exact command
  from `.worktrees/COOP-004`:

  ```bash
  env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-001\build\tooling\run-python.bat' 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-004\_re\harness\campaign_ownership\run.py' --compiler msvc
  ```

  Exit **0**. `/std:c++17 /EHsc /W4 /WX /permissive- /Zc:__cplusplus /arch:IA32
  /MT /O2`; compile/link and executable execution passed, `_M_IX86`/32-bit pointer
  assertions passed. Nine suites, **10,618 checks passed**, zero compiler warnings/errors.
  Exact argv/environment/compiler version/logs: `build/campaign_ownership/msvc-m2p77ova/`.
  Worktree-private Wine prefix and per-invocation build outputs; no baseline tool/prefix
  modifications and no fixed legacy temporary directory used.
- `git diff --check`: passed. Local documentation link checks and full changed-path/API
  review passed; only the four reserved implementation files, ownership harness and own
  record changed. No game assets, binaries, toolchain or outputs are committed.

Coverage: validated roster (including profile zero), dual transport roles, exclusive
claims and optimistic races; duplicate/invalid IDs, four-human/restricted squads; atomic
switch/release, disconnect/reconnect incarnation, death and no revival; script entry/exit
with owner retention and intervening disconnect; independent actor IDs/zero/slot-use
reuse; stale control/generation/phase rejection and handoff gating; immutable old
publications; cross-family sequence/content conflict/cache eviction; counter exhaustion;
300 deterministic interleavings checking forward/reverse indexes and bounds.

Not run: full game build, original singleplayer/native co-op runtime, real controller
queue drains, transport authentication, loading peer acknowledgments or host/guest
mission behavior. There are no calls into those paths in this component. Tests establish
model behavior only, and do not imply the later adapters or campaign mode are playable.

## Handoff

- Completed: isolated branch/worktree and published claim, session/context + roster,
  atomic ownership/reverse index, lifecycle/control revision invalidation and handoff gate,
  reusable bounded cross-family journal, meaningful model tests and MSVC x86 execution.
- Files: `src/game/campaign_session.{h,cpp}`, `campaign_ownership.{h,cpp}`,
  `_re/harness/campaign_ownership/{model_tests.cpp,run.py,README.md}`, this record.
  Consume merged COOP-011 declarations without modifying that header or legacy code.
- Checkpoint location: `origin/feature/coop-004-ownership`; `.worktrees/COOP-004`.
  Claim acknowledged by coordinator ledger `f960e0c3`; owner remains thread
  `01a1ebf9-60c7-74d0-9d39-cd09295875b0`.
- COOP-005 coordination: exact Begin/Finish signatures agreed through worker threads;
  preparation uses injected callbacks/behavioral fake, no dependency on this unmerged
  header and no competing production journal. Later composition shares one authority
  queue and journal and synchronizes rights/preparation revisions before publication.
  Both optional result generation fields describe the full current session epoch.
  Deployed flags identify the selected preparation squad, before actor construction.
- Remaining: coordinator integration and actual done/merge metadata. Independent full
  review and all component checks passed; no outstanding finding or toolchain blocker.
- Next action: coordinator merges ready PR #9, then this owner records actual merge SHA
  and remaining dependent-task blockers in a reviewed documentation follow-up.
- Decisions: no new product policy. Selection/switching fixture permissions are proposals;
  production policy, peer/world barriers, host-loss and live adapter composition stay explicit.
- Uncommitted work: none at the implementation checkpoint. All build/runtime evidence
  stays ignored under this worktree's `build/`; source-only PR steps reproduce it.

## Review evidence

- Separate full implementation/API review after checkpoint `68bb0bc1`: traced forward/
  reverse ownership commits, all request variants/rejection paths, exact participant
  incarnations, script/disconnect/death, world invalidation and handoff gates; checked
  constructor/catalog bounds, checked counters, full immutable results and journal
  cache/high-water/generation/content/Finish validation. No outstanding finding.
- COOP-005 worker, thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, bounded read-only
  journal consumer review of `68bb0bc1`: **PASS**, no blocker. Confirmed callback signatures,
  exact canonical/family compare, original immutable result replay, generation fences,
  monotonic sequence including rejection, four-player/cache/content bounds and Finish
  matching. This reviewer did not rerun the ownership harness or review the full model.
- Updated main `1b9b307f` merged on this branch as `e86cc8c4`. Only overlap was the
  planning status/handoff for this task; resolved to the active reviewed implementation.
  No implementation source or shared declaration changed during synchronization.
- Full read-only peer review requested from COOP-006; COOP-005 subsequently offered
  full implementation review/integration after its own checkpoint. No review result
  or integration approval is inferred from an offer.
- Environment update after validation: previous coordinator reports COOP-001 worktree
  and borrowed toolchain removed externally. Our native/MSVC logs and fixture remain
  intact in COOP-004 build outputs. The historical target run is valid evidence; a fresh
  target compile needs reprovisioning via the merged build guide or an MSVC x86 developer
  environment. The phase-baseline correction below now requires a fresh target run.

### Full review correction: preserve captured phase baselines

COOP-005 full read-only review and this worker's reciprocal composition review independently
identified that phase-only Freeze had advanced ownership/control revisions after preparation
captured its manifest. StartSimulation likewise advanced the ownership catalog after the
acknowledged world cut. Neither operation transfers ownership or rebinds a controller.
The corrected implementation publishes only the context phase at these two boundaries,
preserving the exact captured/acknowledged catalog and control revisions. Actual world
reconstruction, bindings, transfers, script changes and other invalidations still advance
revisions as before. Both workers agreed this contract interpretation before composition.

Focused regressions retain each frozen/loading cut and assert identical ownership and all
control revisions after Freeze/StartSimulation. Corrected native run:
`python3 _re/harness/campaign_ownership/run.py --compiler gcc`, exit **0**, nine suites,
**10,654 checks**, output `build/campaign_ownership/gcc-3hftx_t9/`.

Corrected checkpoint `1e276554`, intended MSVC **19.44.35230 x86**, same SDK/Python/Wine
versions and `/W4 /WX /arch:IA32 /MT /O2` flags as above: compile/link and executable
execution exit **0**, nine suites, **10,654 checks**, zero compiler warnings/errors.
Logs/version/argv: `build/campaign_ownership/msvc-f4oonfu6/`. Exact final entry command:

```bash
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-007\build\tooling\run-python.bat' 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-004\_re\harness\campaign_ownership\run.py' --compiler msvc
```

COOP-007 explicitly authorized read-only use of the reprovisioned tooling; all outputs
and Wine state stay private to COOP-004. The final source change is now validated on
the intended target. No later C++/fixture change is included in this evidence update.

### Final independent review

COOP-005 coordinator/reviewer, thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`,
reviewed final candidate `1958a63a`: **PASS**, no outstanding findings. Read full session/
ownership/journal source and API, complete eight-path diff, task and harness guide; traced
atomic forward/reverse catalogs, handoff/lifecycle/authentication/generation/journal
boundaries and the corrected phase baseline. Independently reran final native harness
(output `gcc-8df2if3k`): nine suites, **10,654 checks passed**. Inspected final target
execution log `msvc-f4oonfu6`: same **10,654 passed**, with source unchanged after target
validation. Reviewer confirms isolated component scope and downstream runtime limitations.
This subsequent commit adds review metadata only; implementation/fixture unchanged.
