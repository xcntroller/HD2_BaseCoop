# COOP 020 Base campaign verification

## Coordination

- Status: planned
- Owner/session: unassigned
- Canonical branch: `docs/coop-020-campaign-verification`
- Base commit: not started
- Dependencies: [COOP-017](COOP-017.md) save/load, [COOP-019](COOP-019.md) Running admission and the preceding campaign flow
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Prepare a mission-by-mission verification matrix and reproducible routes for the
project owner's full base campaign playtests through the normal menu flow. Compare
recorded results with original solo behavior. Use focused technical checks for
individual interactions and transitions. File bounded follow-ups for discovered
defects and retest their fixes before closing the affected checks.

## Files and shared contracts

Own the verification matrix and evidence index under `docs/`. Record installed
game version, executable build, player counts, campaign steps, tested commits,
exact reproduction steps and evidence locations. Keep logs and captures containing
installed game data outside tracked files. Route implementation fixes through
separately scoped tasks.

## Acceptance criteria

- Document the route and expected results for every base campaign mission and
  transition. Plan coverage for one through four players, including constrained
  squads. Record actual player counts and compare objectives, debrief outcomes
  and persistent soldier state when owner playtest results arrive.
- Exercise relevant scripted control, squad switching and orders, healing,
  transfers, disguise, doors, explosives and vehicles where present. Cover failure,
  restart, save/load and Running admission on representative missions.
- Classify evidence under the
  [campaign testing strategy](../agent_workflow.md#campaign-testing-strategy).
  Record each check as passed, failed, untested, or pending owner playtest. Link
  evidence for passes and follow-ups for failures. Keep outstanding gameplay
  checks open in the matrix as the technical evidence and playtest routes are delivered.
  Repeat relevant ordinary solo and native LAN regressions.

## Validation evidence

Pending. Record the tested commit, toolchain and game version, exact checks, outcomes
and evidence locations. Use the [build guide](../build_baseline.md) for isolated x86
builds and the [campaign design](../base_game_coop.md#validation-requirements) for
runtime evidence and legacy regression requirements.

## PR test instructions and delivery

- Delivery: Documentation-only verification report with runtime evidence.
- Prerequisites and reproduction steps: record build, game setup, player count, actions
  and expected results before review.
- Validation actually run: none for this planned work.

## Handoff

- Completed: task definition.
- Remaining: implementation or verification and the acceptance evidence above.
- Next action: Build the verification matrix from the original campaign sequence and identify the mission and player count needed for each interaction.
- Blockers: Required results listed above must merge before implementation.
- Decisions: Select representative player counts for special cases and record that coverage explicitly in the matrix.
- Local state: no task worktree or artifacts.
- Review notes: none.
