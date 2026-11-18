# COOP 008 Controller and input adapters

## Coordination

- Status: active
- Owner/session: Codex COOP-008 / thread `01a1ec1f-4589-7051-9994-2a236a14b49b`
- Canonical branch: `feature/coop-008-controller-adapters`
- Base commit: `0fcea78f85d0fe9348c3b8e13d1a0f23f50a482a`
- Dependencies: COOP-001, COOP-004, COOP-006
- Last checkpoint: none
- Checkpoint location: `.worktrees/COOP-008` / canonical branch on origin
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

- Completed: high-level identity coupling identified in the campaign design.
- Remaining: exact file split, adapters, handoff/input tests, legacy regression checks.
- Next action: obtain coordinator claim acknowledgment, review merged dependencies,
  and agree on overlapping-file edits with COOP-009.
- Blockers: none for the isolated adapter task. COOP-006 protocol merged at
  `59cb8944`; COOP-004 session/ownership merged at `59a480c8`;
  COOP-001 build/runtime baseline merged at
  `15b572ff`; prerequisites and commands are in [the guide](../build_baseline.md).
- Decisions: preserve soldier identity and explicit simulation authority.
- Validation: none for this task.

## 2026-11-18 kickoff

- Claim acknowledged on `coord/coop-work-queue`; accepted COOP-006 stopped-writer coordinator handoff and notified COOP-007/009.
- Declared scope follows COOP-002 reservations above. COOP-009 alone edits lifecycle/spawn files and consumes explicit adapter helpers; no live attachment before COOP-010.
- Next checkpoint: engine-independent committed controller plan and input queue boundary, with real ownership/protocol composition regressions; then legacy consumer adapters and target build checks.
