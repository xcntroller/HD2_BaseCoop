# COOP 009 Campaign mission lifecycle adapters

## Coordination

- Status: active
- Owner/session: Codex COOP-009, user assignment 2026-11-18; coordinator COOP-007 acknowledged claim; COOP-009 accepted its stopped-writer handoff at ledger `9e051608`
- Canonical branch: `feature/coop-009-campaign-adapters`
- Base commit: `0fcea78f85d0fe9348c3b8e13d1a0f23f50a482a`
- Dependencies: COOP-001, COOP-004, COOP-005, COOP-006
- Last checkpoint: lifecycle/native adapter implementation (this commit)
- Checkpoint location: `.worktrees/COOP-009`, canonical branch on origin
- Review/integration: active component; final combined integration waits for COOP-008 merge
- Pull request: https://github.com/xcntroller/HD2_BaseCoop/pull/19 (draft)

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

Implement original campaign spawning and lifecycle adapters for a network session,
consuming committed squad/loadout state and preserving campaign rules. Keep them dormant
outside the explicit campaign-session boundary until COOP-010 connects the development run.

## Files and shared contracts

Expected hotspots: `game_app.cpp`, `game.cpp`, `spawn_point.cpp`, `game_rounds.cpp`, profile
settings, and script player-team queries. Coordinate `game_rounds.cpp` and actor/controller
integration with COOP-008. Narrow/split scope from the COOP-002 audit before claiming.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own `src/game/campaign_mission_adapter.{h,cpp}`
and lifecycle changes in `game.cpp`, `game_app.cpp`, `game_rounds.cpp`, `spawn_point.cpp`,
narrow profile-rule adapters, script player-team queries and host AI replication.
Follow [the audit and sequencing](../coop_contracts.md#legacy-coupling-audit-and-adapter-allocation).
You are the sole writer of these overlapping lifecycle files. Call COOP-008's agreed
disconnect/handoff helper before native actor/mine destruction; coordinate its signature
before implementation. Iterate roster bindings, not a contiguous actor-ID/player-slot
assumption. Supply frozen manifest, reconstruction/binding and barrier hooks; COOP-010
owns live dispatch attachment. Final combined adapter integration follows COOP-008 merge
and repeats affected checks; do not merge an unfinished dependency branch.

## Acceptance criteria

- Spawn the selected squad at original campaign points with committed equipment and stats;
  avoid multiplayer zones, warmup, lives, respawns, ranks, and map rotation.
- Distinguish simulation authority from campaign rules without redefining local play.
- Campaign script player-team queries include all deployed human and AI soldiers.
- Prepare explicit initial-world synchronization/loading-barrier and campaign-result hooks;
  define their integration obligations without claiming full save/load or late join.
- Build and run focused lifecycle regressions for ordinary singleplayer/native co-op.
  The completed adapter scope and dormant activation must be reviewable without placeholders.

## Handoff

- Completed: engine-independent frozen manifest/generation/actor binding/loading barrier,
  reconstruction acknowledgment/start hooks, original-point native spawn conversion,
  dormant lifecycle/profile/script/AI guards and source-extracted legacy regressions.
  Native file split `campaign_mission_native.{h,cpp}` was acknowledged by COOP-007.
- Remaining: final combined adapter check after COOP-008 merges (including its public
  progress setter), exact-head independent review, final source checks and review PR.
  No live attachment/menu or two-player campaign gameplay claim; COOP-010 owns those.
- Next action: repeat target/sanitizer checks on the final source; add the agreed
  COOP-008 progress setter call after its merge and review the combined native paths.
- Validation run: GCC 7 lifecycle suites/274 checks and 3 extracted native source
  regression suites/75 checks PASS; MSVC 19.44.35230 x86 `/W4 /WX` same PASS in
  `build/campaign_mission/msvc-_sn7paat` before tooling disappearance. Source has
  subsequently gained safe-tick reconciliation, atomic profile writeback and bounds;
  refreshed target/sanitizer checks are in progress, not yet claimed.
- Build evidence: full MSVC x86/x87 game build compiled 278 units, 0 compiler messages,
  link 0/LAA yes (`build/full-game-build.log`). Later source refresh (`build/game-restored.log`)
  compiled 1 changed unit and linked successfully after private toolchain restoration.
  The build predates the final source changes; final combined build remains required.
- Runtime evidence: a private copied 1.12 installation launched intro/main menu on
  Xvfb :101, original campaign flow was attempted but no mission-play acceptance is
  claimed. Focused source regressions establish dormant solo/native settings and
  departure behavior; full live campaign acceptance remains COOP-010.
- Resources: original COOP-007 toolchain/worktree disappeared externally after its
  completion;007/008/009 confirmed absence; none reports deleting it. Historical
  passing evidence remains accurate. Restored private 009 compiler/SDK/WinPython
  is `build/tooling/run-python.bat` (same 14.44.35207/10.0.26100.0/3.13.9), with own
  Wine prefix and isolated outputs. Read-only borrowing authorized for 008.
  Native imports/DLLs copied before deletion are preserved in ignored 009 directories.
- API: native callback hooks bind 008 `SynchronizeCommitted`,
  `FindCampaignTransportPlayer(int slot)`, `HandleCampaignDisconnect(PlayerId)`;
  resolve authenticated incarnation before connection cleanup. All world/snapshot
  and input dispatch obligations are in the [harness guide](../../_re/harness/campaign_mission/README.md).
- Scope: sole lifecycle writer; 008 retains all human/controller/actor consumers.
  No shared DTO/legacy layout/wire changes or generated tier1 edits.
- Prerequisites: 001 `15b572ff`, 004 `59a480c8`, 005 `06d999d7`, 006 `59cb8944` are merged.
  Final 009 combined adapter integration follows 008 merge; no draft branch is stacked.
