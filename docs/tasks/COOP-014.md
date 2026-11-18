# COOP 014 Arctic1 mission completion and interaction parity

## Coordination

- Status: ready
- Owner/session: unassigned
- Canonical branch: `feature/coop-014-arctic1-completion`
- Base commit: not started
- Dependencies: [COOP-010](COOP-010.md) two-player Arctic1 development path
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Implement and technically verify Arctic1's original interaction, objective, and terminal
result paths. Fix host-authoritative interaction and guest presentation gaps. Pass a
trustworthy mission result to the lifecycle boundary for COOP-015. Follow the
[campaign testing strategy](../agent_workflow.md#campaign-testing-strategy) and prepare
any full-route checks for the project owner's playtest.

## Files and shared contracts

Own the in-mission interaction and presentation path for Arctic1 in the campaign
controller, script, objective and Running replication adapters. Audit original
interactions and scripted events on the route before changing shared native code.
Coordinate edits to common dispatch, protocol or launch code with COOP-012/013;
leave debrief and next-step glue to COOP-015. Record uncovered interaction families
for the later campaign-wide verification task.

## Technical acceptance criteria

- Set up the relevant original objective conditions and verify the native triggers in
  focused host/guest runs. Verify that both peers agree at the objective and terminal
  boundaries. Use captured native saves, reproducible checkpoints, or a debug-only
  harness. Record which native triggers ran and which outcomes were supplied as fixtures.
- Use focused runs to exercise interactions and scripted presentation on both peers,
  including the guide and patrols. Verify that AI, damage and script effects run
  once on the host. Document and address guest cutscene, audio, HUD and ambient
  actor gaps that affect following or playing the route.
- Exercise a reproducible failure outcome and verify that it cannot be committed
  as success. Record the terminal squad state, including casualties and loadout changes,
  and repeat the COOP-010 handoff and original Tutorial/Sicily2 smoke checks if affected.

## Owner playtest

Record the steps and expected results for any remaining genuine Arctic1 route and
mission-completion checks. Track those outcomes separately from the controlled
interaction and terminal-state evidence.

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
- Next action: Use a reproducible checkpoint or controlled state to exercise the next
  unverified interaction or terminal boundary on host and guest.
- Blockers: None in scope. Obtain a coordination claim before implementation.
- Decisions: Use the existing development launch and game data. Base the interaction scope on what the original Arctic1 route exercises.
- Local state: no task worktree or artifacts.
- Review notes: none.
