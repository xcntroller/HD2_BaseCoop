# COOP 009 Campaign mission lifecycle adapters

## Coordination

- Status: active
- Owner/session: Codex COOP-009, user assignment 2026-11-18; coordinator COOP-007 acknowledged claim on coord/coop-work-queue, coordination transferred to COOP-008
- Canonical branch: `feature/coop-009-campaign-adapters`
- Base commit: `0fcea78f85d0fe9348c3b8e13d1a0f23f50a482a`
- Dependencies: COOP-001, COOP-004, COOP-005, COOP-006
- Last checkpoint: initial claim (this commit)
- Checkpoint location: `.worktrees/COOP-009`, canonical branch on origin
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

- Completed: legacy start/stop/spawn coupling mapped in the design.
- Remaining: campaign adapters, synchronization hooks, focused regressions and final combined review after COOP-008 merge.
- Next action: implement frozen-manifest/lifecycle barrier and original-point native spawning; agree exact disconnect/context helpers with COOP-008.
- Scope confirmed: sole lifecycle writer in game.cpp/game_app.cpp/game_rounds.cpp/spawn_point.cpp; narrow profile/script/AI adapters, campaign_mission_adapter and lifecycle harness. COOP-008 owns controller/human/actor dispatch files. Native SpawnPoint may gain nonvirtual helpers without layout changes. No live transport attachment; COOP-010 owns activation.
- Coordination: all dependency implementations and completion records are merged; COOP-007 acknowledged claim before substantial work. Its compiler/SDK/Python may be borrowed read-only with private COOP-009 Wine prefix and isolated outputs.
- Blockers: none for the isolated adapter task. COOP-006 protocol merged at
  `59cb8944`; COOP-004 ownership merged at `59a480c8`,
  COOP-005 preparation merged at `06d999d7`, and COOP-001 build/runtime baseline
  merged at `15b572ff`. Prerequisites and commands are in [the guide](../build_baseline.md).
- Decisions: original campaign spawning and mission behavior; multiplayer infrastructure reuse.
- Validation: none for this task.
