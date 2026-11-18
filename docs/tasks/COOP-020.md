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

Produce a mission-by-mission verification record for the base campaign using
the normal menu flow. Compare against original solo behavior and track campaign
outcomes, progression and runtime regressions. File bounded follow-up tasks for
discovered defects and retest their fixes before closing affected coverage.

## Files and shared contracts

Own the verification matrix and evidence index under `docs/`. Record installed
game version, executable build, player counts, campaign steps, tested commits,
exact reproduction steps and evidence locations. Keep logs and captures containing
installed game data outside tracked files. Route implementation fixes through
separately scoped tasks.

## Acceptance criteria

- Complete every base campaign mission and transition in the co-op path. Record
  actual player counts per mission and cover one through four across the matrix,
  including constrained squads. Compare objectives, debrief outcomes and persistent
  soldier state against the original campaign rules.
- Exercise relevant scripted control, squad switching and orders, healing,
  transfers, disguise, doors, explosives and vehicles where present. Cover failure,
  restart, save/load and Running admission on representative missions.
- Record each check as passed, failed or untested, with evidence for passes and
  a linked task for failures. Keep unresolved or untested required cases open.
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
