# COOP 018 Reconnect to a running campaign

## Coordination

- Status: planned
- Owner/session: unassigned
- Canonical branch: `feature/coop-018-reconnect`
- Base commit: not started
- Dependencies: [COOP-016](COOP-016.md) current-world reconstruction and post-cut update handoff
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Let a previously admitted player reconnect to a Running session. The soldier
goes to AI on disconnect. The returning connection rebuilds the current world
and may reclaim an eligible soldier under the agreed policy once synchronized.

## Files and shared contracts

Own reconnect admission and the recovery menu/status, integrating
`src/net/campaign_net`, `campaign_protocol`, session/ownership and control bindings
with COOP-016. Define how the host recognizes a return without trusting a reused
provider slot or player-supplied actor ID. Coordinate with COOP-017 if both touch
lifecycle dispatch; COOP-019 consumes the resulting Running admission path.

## Acceptance criteria

- During an actual two-instance mission, disconnect the guest, let the host and
  AI advance the world, then reconnect. Verify the same existing soldier changes
  controller without resetting state when eligible. The guest first sees the
  current world, then gains input under a new ownership token.
- Exercise an unavailable former soldier, invalid or obsolete reconnect identity,
  interrupted catch-up, and a reused transport slot. Follow the agreed eligibility
  policy without taking a soldier from another player or letting stale input run.
- Verify that clients without an eligible reconnect still receive a bounded
  rejection while Running and that the original departure-to-AI behavior holds.

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
- Next action: Specify the return identity lifetime and eligible-soldier policy, then exercise a reconnect against an advancing host world.
- Blockers: Required results listed above must merge before implementation.
- Decisions: Record choices for return identity lifetime, claim priority and the result when the previous soldier is unavailable.
- Local state: no task worktree or artifacts.
- Review notes: none.
