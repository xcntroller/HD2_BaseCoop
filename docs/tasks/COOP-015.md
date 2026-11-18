# COOP 015 Campaign debrief and next mission

## Coordination

- Status: planned
- Owner/session: unassigned
- Canonical branch: `feature/coop-015-campaign-progression`
- Base commit: not started
- Dependencies: [COOP-013](COOP-013.md) menu preparation/deployment, [COOP-014](COOP-014.md) authoritative mission outcome
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Keep the session alive from a completed Arctic1 mission through the original
debrief and campaign sequence into the next playable mission. Commit campaign
results once on the host and reuse live preparation only at the original equipment
steps. Preserve campaign roster identity when actors and ownership bindings change.

## Files and shared contracts

Own campaign step advancement in `src/game/game_app.{h,cpp}`, campaign state writeback
and debrief routing, and the transition hooks in the mission lifecycle and ownership
adapters. Use the terminal result from COOP-014 and the preparation lifecycle from
COOP-013. Keep disk save/resume and mid-mission admission outside this scope.

## Acceptance criteria

- Start from the game menu and complete Arctic1 as host and guest. Both see the
  original debrief result and reach the original next campaign step in the same
  session. Enter the following playable mission through the original intervening
  steps, including live preparation where required.
- The [after-action report](../base_game_coop.md#player-flow) shows all connected
  humans' readiness, including spectators. The host can advance only when all are
  ready for that report. Verify a late Ready, a departure while waiting, and
  repeated advancement requests against the same report.
- Compare squad stats, deaths, inventory and progression against original campaign
  rules. The host commits the result once and guests do not write solo progress.
  A failure follows the original failure path without committing victory.
- Advance the mission generation, invalidate prior input and actor bindings, and
  require a new loading barrier before either player can act. Test a departure
  during the transition and a player with no eligible soldier under the agreed
  policy. Verify the corresponding solo flow and native LAN map change.

## Validation evidence

Pending. Record the tested commit, toolchain and game version, exact checks, outcomes
and evidence locations. Use the [build guide](../build_baseline.md) for isolated x86
builds and the [campaign design](../base_game_coop.md#validation-requirements) for
runtime evidence and legacy regression requirements.

## PR test instructions and delivery

- Delivery: Integrated gameplay; specify runnable artifact or Source-only at review.
- Prerequisites and reproduction steps: record build, game setup, player count, actions
  and expected results before review.
- Validation actually run: none for this planned work.

## Handoff

- Completed: task definition.
- Remaining: implementation or verification and the acceptance evidence above.
- Next action: Trace the original campaign nodes from Arctic1 mission exit to the next playable step and identify the host writeback boundary.
- Blockers: Required results listed above must merge before implementation.
- Decisions: Preserve the original step sequence and record any necessary eligibility or transition departure decision before exposing that policy.
- Local state: no task worktree or artifacts.
- Review notes: none.
