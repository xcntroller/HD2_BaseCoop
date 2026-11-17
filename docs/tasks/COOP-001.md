# COOP 001 Reproducible build and baseline

## Coordination

- Status: active
- Owner/session: Codex / user-assigned COOP-001, 2026-11-18
- Canonical branch: `feature/coop-001-build-baseline`
- Base commit: `7e033adde95881baad06c6877cbbd6eef8197d96`
- Dependencies: none
- Last checkpoint: `ca28d52c` (build isolation and portable MSVC selection); next handoff commit contains this update
- Checkpoint location: `origin/feature/coop-001-build-baseline`; `.worktrees/COOP-001`
- Review/integration: not ready; baseline acceptance remains required
- Pull request: https://github.com/xcntroller/HD2_BaseCoop/pull/2 (draft)

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

## Handoff

- Completed: read design/workflow/index; fetched origin and checked branches, worktrees and PRs. No existing task branch or handoff. Kickoff coordinator accepted the claim on `coord/coop-work-queue`. Created canonical branch/worktree from integrated main.
- Remaining: finish real host/guest in-mission actions, documentation and review; build and solo training run passed.
- Next action: equip/accept both native co-op participants on Sicily2, spawn and verify replicated movement/actions, then finish baseline guide and review.
- Blockers: none currently. MSVC/Wine and game data available. Native Wine DirectPlay server is insufficient; copied the already installed native DirectPlay components into private test prefixes and selected IPv4. `port` changes domain to internet, so tested LAN command orders `port 11001 domain local`; otherwise key validation fails. Guest has now connected to native host player setup.
- Decisions: preserve the port's required runnable math/toolchain behavior. Scope expanded in coordinator ledger 6c0421e6 to permit `HD2_VCVARS32` override in engine build script, plus `_re/tools/build_isolated.py` and focused process tests; no engine/gameplay source changes.
- Local state: portable tooling, download manifests/cache, compiler logs and Wine prefix are ignored under `.worktrees/COOP-001/build/`; installed Steam game remains untouched. Build-tool changes are in the next checkpoint.
- Validation: Linux and clean embedded Windows Python/Wine pass 3 process-level isolation/lock/failure tests. Two real MSVC c_str.cpp checks ran concurrently in distinct worktrees and passed independently. All 78 LS3DF and 272 game translation units compiled; matching x86 DLL/executable links passed (game `/arch:IA32`, zero game diagnostics). Native x64 dlltool needs `-m i386 -f --32` to make every import archive member i386. Solo original Campaign/Tutorial: automatic squad/active soldier, loading/scripted scene, W movement, I inventory and normal return/exit passed. Native co-op host and guest now connected on Sicily2; mission actions pending. Detailed exact commands/evidence are being consolidated in docs/build_baseline.md.
