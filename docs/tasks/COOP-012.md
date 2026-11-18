# COOP 012 Campaign session setup and menu entry

## Coordination

- Status: ready
- Owner/session: unassigned
- Canonical branch: `feature/coop-012-session-setup`
- Base commit: not started
- Dependencies: [COOP-010](COOP-010.md) live admission and development session
- Last checkpoint: none
- Checkpoint location: task queue only
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Add the Co-op Campaign choice to the singleplayer mode picker, with a campaign-style
host/join setup and explicit campaign session lifetime. Reuse the live compatibility
handshake while leaving the original campaign mode and native LAN routes intact.
Keep the new path gated to development validation until COOP-013 provides a usable
preparation-to-deployment flow.

## Files and shared contracts

Own `src/game/main_menu.{h,cpp}`, the new setup screen and its menu resources, and
the setup/teardown glue to `campaign_session` and `src/net/campaign_net`. Keep any
`game_app` edits confined to entry and return; COOP-015 owns campaign step advancement.
Publish the admitted-participant and session-lifecycle interface consumed by COOP-013.
Review any changes to shared transport or development-session dispatch with COOP-014.

## Acceptance criteria

- Through the gated menu, actual host and guest instances establish a compatible
  campaign session and reach its setup boundary. No launch arguments are required for
  the menu path. Document the join method supported by the existing LAN transport.
- Failed or cancelled join, incompatible content, full session, guest exit and host
  shutdown return participants to a usable screen without leaking campaign state into
  a second session. Enforce the four-player cap including the host.
- Campaign, Lone Wolf and Carnage still select their original routes and native LAN
  still starts. The normal menu exposes the new path only when COOP-013 completes it.

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
- Next action: Trace the mode picker resource and callbacks, then agree on the admitted-session seam with the COOP-013 scope.
- Blockers: None in scope. Obtain a coordination claim before implementation.
- Decisions: Use the confirmed host-led model and four-player cap. Record setup UX choices separately from open preparation policies.
- Local state: no task worktree or artifacts.
- Review notes: none.
