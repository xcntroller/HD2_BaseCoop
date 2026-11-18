# COOP 015 Campaign debrief and next mission

## Coordination

- Status: active
- Owner/session: Codex COOP-015, user assignment, thread `01a1eead-4d91-7b52-a8d5-cd1a32cce6d3`
- Canonical branch: `feature/coop-015-campaign-progression`
- Base commit: `ca5ae2f897524cc1905e343b5b7ad3910e56f4c3`
- Dependencies: [COOP-013](COOP-013.md) menu preparation/deployment, [COOP-014](COOP-014.md) authoritative mission outcome
- Last checkpoint: live progression implementation and first x86 build (this commit)
- Checkpoint location: canonical branch, `.worktrees/COOP-015`; claim acknowledged by coordinator at `16963b69`
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Keep the session alive from a completed Arctic1 mission through the original
debrief and campaign sequence into the next playable mission. Commit campaign
results once on the host and reuse live preparation only at the original equipment
steps. Preserve campaign roster identity when actors and ownership bindings change.

## Files and shared contracts

Own campaign step advancement in `src/game/game_app.{h,cpp}`, campaign state writeback
and debrief routing, and the transition hooks in the mission lifecycle and ownership
adapters. Use the terminal result from COOP-014 and the preparation lifecycle from
COOP-013. Keep disk save/resume and mid-mission admission outside this scope.

## Technical acceptance criteria

- Launch a host/guest session through the game menu and establish an Arctic1 terminal
  result using a reproducible checkpoint or controlled test state. Both peers see
  the original debrief result and reach the original next campaign step in the same
  session. Enter the following playable mission through the original intervening
  steps, including live preparation where required. Label the terminal setup and
  the production transition it exercises.
- The [after-action report](../base_game_coop.md#player-flow) shows all connected
  humans' readiness, including spectators. The host can advance only when all are
  ready for that report. Verify a late Ready, a departure while waiting, and
  repeated advancement requests against the same report.
- Compare squad stats, deaths, inventory and progression against original campaign
  rules. The host commits the result once and guests do not write solo progress.
  A failure follows the original failure path without committing victory.
- Advance the mission generation, invalidate prior input and actor bindings, and
  require a new loading barrier before either player can act. Test a departure
  during the transition and a player with no eligible soldier under the agreed
  policy. Verify the corresponding solo flow and native LAN map change.

## Owner playtest

Hand off the normal menu-to-Arctic1-to-next-mission route with exact launch steps and
expected results. Record any genuine end-to-end evidence separately under the
[campaign testing strategy](../agent_workflow.md#campaign-testing-strategy).

## Validation evidence

- MSVC 14.44.35207 x86 / SDK 10.0.26100.0 under Wine. The isolated
  `_re/tools/build_isolated.py game --all` build compiled 292 translation units
  with zero compiler diagnostics and linked a PE32 executable. Log
  `build/evidence/build-msvc-r2.log`.
- **Simulated:** `python3 _re/harness/campaign_protocol/run.py --compiler gcc`
  passed 13 suites and 567,597 checks. The progression cases cover gated wire
  direction and feature negotiation, report identity, spectators, and malformed
  readiness or step payloads. Log `build/evidence/protocol-gcc.log`.
- Installed Sabre Squadron 1.12 campaign data contains the original Arctic1 report,
  `BriefArctic1b.bik`, Arctic2 field equipment (kind 4), and Arctic2 play in that order.
  This is a data inspection. Paired gameplay evidence is pending.

## PR test instructions and delivery

- Delivery: Integrated gameplay; specify runnable artifact or Source-only at review.
- Prerequisites and reproduction steps: record build, game setup, player count, actions
  and expected results before review.
- Validation actually run: isolated production x86 build and GCC protocol conformance.

## Handoff

- Completed: published the claim and implemented mission-scoped report readiness,
  host writeback, synchronized original step dispatch and continued live preparation.
  The production x86 build and the protocol harness pass.
- Remaining: paired menu-to-Arctic2 checks, failure/retry, departure matrix, and solo/LAN regressions.
- Next action: build an isolated checked host with
  `_re/harness/campaign_arctic1/install_debug.py`. Pair it with the production
  guest, reach the report from the game menus, and verify late readiness,
  a single commit, the original briefing, and the new loading barrier.
- Blockers: none. COOP-013 merged at `73d518a5` and COOP-014 merged at
  `619d6aec`. The host result is available through `GetCampaignMissionResult()`
  while the session is in Ending.
- Decisions: Arctic2 uses the original one-soldier limit. Preserve the host's
  surviving assignment if possible, otherwise choose the first surviving squad
  soldier in roster order and retain that soldier's owner. Additional players
  can ready up as spectators. A departure invalidates a frozen or loading
  world cut. The remaining players return to preparation in the same session.
- Local state: ignored runtime and evidence under `build/`, using private game copies and Wine prefixes.
- Review notes: none.
