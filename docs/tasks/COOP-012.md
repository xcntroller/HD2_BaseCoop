# COOP 012 Campaign session setup and menu entry

## Coordination

- Status: active
- Owner/session: Codex COOP-012, user assignment 2026-11-18, thread `01a1ed7a-8dcb-7ee1-8de0-23710cb94c81`
- Canonical branch: `feature/coop-012-session-setup`
- Base commit: `9ec7fe48bb21c38f9d95976b99198c6fb1576e74`
- Dependencies: [COOP-010](COOP-010.md) live admission and development session
- Last checkpoint: claim and implementation plan
- Checkpoint location: `.worktrees/COOP-012` and canonical remote branch
- Review/integration: claim acknowledged at ledger `63b4ac3f`; parallel scope agreed with COOP-014 at `75131f7a`
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

- Completed: COOP-010 integration ancestry, live coordinator ledger, existing
  branches/worktrees and PRs checked. Canonical worktree created from integrated main.
  Traced the native singleplayer picker, DirectPlay callbacks and COOP-010 handshake.
- Scope: append the gated native mode-picker entry and setup screen. Keep setup
  admission, participant and teardown state together for COOP-013. COOP-014 owns
  Running dispatch, mission interactions and outcomes.
- Decisions: opt in with the process environment `HD2_CAMPAIGN_COOP_MENU=1`;
  ordinary game launch needs no additional arguments. Use direct IPv4 on the
  existing fixed LAN port (31567), with an explicit cancel/leave/end action.
- Next action: build the native screen and separate its setup lifetime from the
  fixed two-player development mission. Validate real admission and teardown with
  private Wine game instances alongside the original menu paths.
- Local state: shared compiler and engine libraries are available; private runtime
  copies and prefixes will live under ignored `build/`.
