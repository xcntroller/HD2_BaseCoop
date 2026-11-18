# COOP 019 First-time join during a running mission

## Coordination

- Status: planned
- Owner/session: unassigned
- Canonical branch: `feature/coop-019-running-join`
- Base commit: not started
- Dependencies: [COOP-018](COOP-018.md) Running admission and ownership handoff
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Admit a new player while a mission is in progress. Install a current-world
baseline and its subsequent updates before granting control of an eligible
existing soldier, or follow the agreed behavior when none is available.

## Files and shared contracts

Own first-time Running admission, join eligibility and synchronized control
transfer in the menu, campaign network dispatch and ownership adapters. Reuse
the COOP-016 world interface and COOP-018 catch-up and input fences.

## Acceptance criteria

- Join an actual Running session for the first time after the host has changed
  objectives, world state and inventory. Verify convergence before any guest input
  is accepted. A claim transfers control of the same actor, inventory and stats.
- Exercise the four-player cap, ineligible or already owned soldiers, simultaneous
  claims for one soldier, and a mission transition during catch-up. Rejected or
  cancelled admission leaves the active mission and its ownership consistent.
- Repeat reconnect and pre-deployment admission smoke checks. Separate content or
  version rejection from lack of an available soldier in the observed results.

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
- Next action: Trace the reconnect admission path, then specify where first-time compatibility, capacity and eligible-soldier checks enter it.
- Blockers: Required results listed above must merge before implementation.
- Decisions: Resolve the no-eligible-soldier behavior and selection policy before exposing the production join flow.
- Local state: no task worktree or artifacts.
- Review notes: none.
