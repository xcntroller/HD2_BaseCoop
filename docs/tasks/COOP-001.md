# COOP 001 Reproducible build and baseline

## Coordination

- Status: active
- Owner/session: Codex / user-assigned COOP-001, 2026-11-18
- Canonical branch: `feature/coop-001-build-baseline`
- Base commit: `7e033adde95881baad06c6877cbbd6eef8197d96`
- Dependencies: none
- Last checkpoint: claim commit (this commit; resolve with `git log -1 -- docs/tasks/COOP-001.md`)
- Checkpoint location: `origin/feature/coop-001-build-baseline`; `.worktrees/COOP-001`
- Review/integration: not ready; baseline acceptance remains required
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
- Remaining: finish engine/game links, solo and real host/guest runtime baseline, documentation and review.
- Next action: finish MSVC/Wine build with the worktree-local wrapper, then launch a copied game installation.
- Blockers: initial Fedora/MSVC gap addressed by local portable MSVC 19.44.35230 + Windows SDK 10.0.26100.0 + Windows Python 3.13.9 in isolated Wine 11.21 prefix. Engine initially failed link because absolute dlltool output names produced overlong import symbols; regenerating with short relative names. Runtime baseline remains untested.
- Decisions: preserve the port's required runnable math/toolchain behavior. Scope expanded in coordinator ledger 6c0421e6 to permit `HD2_VCVARS32` override in engine build script, plus `_re/tools/build_isolated.py` and focused process tests; no engine/gameplay source changes.
- Local state: portable tooling, download manifests/cache, compiler logs and Wine prefix are ignored under `.worktrees/COOP-001/build/`; installed Steam game remains untouched. Build-tool changes are in the next checkpoint.
- Validation: `python _re/tools/test_build_isolated.py -v` passes 3 process-level concurrency/failure/lock tests. `rw_data.dll` built successfully x86. Engine sources compiled; first link failed, being corrected. GCC fallback fails on missing `gnu/stubs-32.h`; native Linux runnable build cannot execute `.bat`. No runtime claims yet.
