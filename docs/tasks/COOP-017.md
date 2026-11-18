# COOP 017 Isolated campaign saves, load and restart

## Coordination

- Status: planned
- Owner/session: unassigned
- Canonical branch: `feature/coop-017-save-load`
- Base commit: not started
- Dependencies: [COOP-016](COOP-016.md) current-world capture and install
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Add host-controlled Co-op Campaign save discovery and resume, quick save/load
and mission restart. Restore all participating instances through a synchronized
load, using the current membership and fresh actor/control bindings. Keep co-op
saves and last-save tracking separate from solo saves.

## Files and shared contracts

Own the co-op save format and namespace, campaign menu resume, quick actions
and restart transaction. Expected integration in `src/game/game_save.cpp`,
`game_app.cpp`, `profile.cpp`, `game_menu.cpp` and the mission lifecycle adapter.
Consume COOP-016's world interface and coordinate shared world/transport edits
with COOP-018.

## Acceptance criteria

- With two actual instances, save after progress, inventory changes and a casualty,
  mutate the world, then load and compare the saved and reconstructed state on
  both instances. Exercise script and vehicle state in a suitable mission fixture.
  Show restart restoring the original mission start and menu resume working after
  closing and reopening the host process.
- Load/restart stops input, advances generation and resolves eligible owners
  against current connections. Old input and saved slots cannot regain authority.
  Both players resume only after the required acknowledgments.
- Failed or interrupted save leaves the previous save usable. Corrupt or incompatible
  saves report a recoverable error, with no partially loaded session continuing.
  Verify solo save discovery, quick save/load and last-save state remain intact.

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
- Next action: Map the solo save paths and menu entry points, then document the separate namespace and host load transaction.
- Blockers: Required results listed above must merge before implementation.
- Decisions: Confirm the proposed host-owned save and shared load policies for the shipping flow.
- Local state: no task worktree or artifacts.
- Review notes: none.
