# COOP 016 Current-world capture and reconstruction

## Coordination

- Status: planned
- Owner/session: unassigned
- Canonical branch: `feature/coop-016-current-world`
- Base commit: not started
- Dependencies: [COOP-015](COOP-015.md) live mission generation transitions and reconstruction lifecycle
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Provide a versioned current-world capture and install path shared by save/load
and Running-session admission. Establish a baseline and ordered updates after that
baseline for admission while simulation continues. Keep the existing initial-world
barrier and make the new path testable without opening a production join route.

## Files and shared contracts

Own the current-world record, validation, transport, reconstruction and bounded
update handoff. Extend `src/game/campaign_dev_world` and `campaign_mission_native`
where appropriate. Audit native serialization in `src/game/game_save.cpp`; define
what the disk and wire consumers share and what stays separate. Agree on identity,
engine/data compatibility, publication order and failure semantics before COOP-017
or COOP-018 builds on this interface.

## Acceptance criteria

- Capture an active host world after changes to objectives and scripts, live and
  dead actors, dynamic items/inventories, and vehicles where applicable. A private
  paired run installs the same cut on a guest and applies post-cut changes in order.
  State comparison covers each supported subsystem and the exact baseline.
- Invalid, incompatible, incomplete or expired cuts and missing update ranges
  cannot advance the recipient to Current. A failed install releases its partial
  state. Test the bounds of capture, transfer and buffered updates.
- World records persist campaign and world identities and state. Current membership,
  saved process pointers and obsolete control tokens cannot be installed as live
  authority. No guest input runs before successful reconstruction and authorization.

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
- Next action: Inventory the state missing from the time-zero cut and reproduce a representative live capture across native serialization seams.
- Blockers: Required results listed above must merge before implementation.
- Decisions: Use a common reconstruction boundary for the later tasks. Scope the first native round trip to a recorded mission and state coverage.
- Local state: no task worktree or artifacts.
- Review notes: none.
