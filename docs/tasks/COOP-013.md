# COOP 013 Live campaign preparation and first deployment

## Coordination

- Status: planned
- Owner/session: unassigned
- Canonical branch: `feature/coop-013-live-preparation`
- Base commit: not started
- Dependencies: [COOP-005](COOP-005.md) equipment transactions, [COOP-007](COOP-007.md) preparation screen, [COOP-012](COOP-012.md) session setup
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Connect the native preparation screen to the authoritative ownership, stock and
loadout model over the campaign connection. Use the original campaign briefing and
equipment context to prepare and deploy Arctic1 through the campaign entry. Replace the fixed
development roster/loadout inputs on the menu path with the committed preparation
state. Enable normal menu entry when the full flow is playable.

## Files and shared contracts

Own the live `I_CampaignPreparation` adapter, the integration of
`src/game/campaign_preparation_screen.{h,cpp}` and `campaign_preparation.{h,cpp}`,
and preparation-family dispatch and resync in `src/net/campaign_protocol` and
`campaign_net`. Reuse the initial-world transfer and loading barrier. Coordinate
changes to shared dispatch, launch or world code with COOP-014. COOP-015 owns use
of this flow on later campaign steps.

## Acceptance criteria

- Actual sessions start from the game menu with one and four connected players,
  including a four-instance run. In a two-player run, both players edit their own kits
  concurrently, see each other's committed choices, and cannot edit each other's soldiers.
  The host can equip AI soldiers, and AI fills available unclaimed slots.
- A race for the last stock item commits at most once. Repeated, stale or unauthorized
  requests do not change stock or loadouts. Loss of ownership, participant departure
  and snapshot gaps revalidate permissions and readiness against committed state.
- Arctic1 deploys from one frozen roster, stock and loadout revision.
  Each participant reconstructs the same initial world before input resumes. A failed
  loading attempt tears down the partial world and uses a fresh preparation generation
  when returning to preparation is possible.
- The native screen displays real briefing, equipment, ownership and readiness data.
  Cancellation returns to the appropriate campaign menu. Solo equipment and native LAN
  preparation retain their original behavior.

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
- Next action: Use the admitted-session interface to trace a real equipment step from briefing through freeze. Resolve any open policy that blocks the player flow before implementing it.
- Blockers: Required results listed above must merge before implementation.
- Decisions: Roster allocation, readiness quorum, scarce stock UX and deployment cancellation remain subject to the open design decisions. Preserve per-player edit permissions and the original equipment rules.
- Local state: no task worktree or artifacts.
- Review notes: none.
