# COOP 013 Live campaign preparation and first deployment

## Coordination

- Status: active
- Owner/session: Codex COOP-013, explicit user assignment on 2026-11-18
- Canonical branch: `feature/coop-013-live-preparation`
- Base commit: `fd5ad4c3e997be8a6d4a7211e04dd3319677e9a2`
- Dependencies: [COOP-005](COOP-005.md) equipment transactions, [COOP-007](COOP-007.md) preparation screen, [COOP-012](COOP-012.md) session setup
- Last checkpoint: `1b26199e` implements the deployment handoff
- Checkpoint location: canonical origin branch and `.worktrees/COOP-013`
- Review/integration: draft while final runtime and regression evidence is collected
- Pull request: [#28](https://github.com/xcntroller/HD2_BaseCoop/pull/28)

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

Checkpoint tests used Sabre Squadron 1.12.0 from Steam build 2147249, Wine 11.21
Staging, the matching port engine libraries, isolated MSVC 14.44 x86 and private
Xvfb instances. Local scripts and results are under the task worktree's ignored
`build/` directory. The first full MSVC deployment build compiled 292 units
with zero compiler messages and linked a large-address-aware executable.

| Check | Actual result |
| --- | --- |
| `python3 _re/harness/campaign_protocol/run.py --compiler gcc` | 567,404 checks passed, including resync direction and negotiated feature binding. |
| `python3 _re/harness/campaign_preparation/run.py --compiler gcc` | Header, behavior and original-rule parity passed. |
| `python3 _re/harness/campaign_preparation_ui/run.py --compiler gcc` | Screen behavior and input harness passed. |
| `_re/harness/campaign_preparation/live.cpp` with the adapter, preparation, ownership, session and protocol implementations | Encoded host and guest traffic passed ownership, duplicate and stale requests, last-stock contention, snapshot gap, departure, freeze and new-generation reopen checks. |
| `unshare --user --map-root-user --net bash build/menu_deploy_keys.sh 1 arctic1-menu-solo-retry` | One actual menu instance claimed a soldier, marked ready and deployed four original actors. Generation 1 and snapshot 1 started after host capture at mission time zero. The local `build/evidence/solo-menu-initial-world.log` records the barrier. |
| `unshare --user --map-root-user --net bash build/menu_deploy_keys.sh 4 arctic1-menu-four` | Four actual menu instances claimed distinct soldiers and marked ready. All three guests reconstructed exactly 37,646 bytes with CRC32 770313502. The host accepted all three acknowledgments for snapshot 1 before starting simulation. Every guest applied the first live update. `build/evidence/four-menu-*-initial-world.log` and `arctic1-menu-four-*-world.png` capture the run. The preparation screens exposed missing guest item definitions before launch. |

## PR test instructions and delivery

- Delivery: Source-only draft for integrated gameplay. Local executables and game data stay ignored.
- Prerequisites: Use the [isolated build guide](../build_baseline.md), Sabre Squadron
  1.12.0, matching engine libraries and an IPv4 LAN with one game instance per player.
- In **Single Player → Co-op Campaign**, host or join by IPv4. On the host, open
  **Prepare Mission**. Claim different soldiers and mark each player ready. The host
  should be able to edit AI kits. The other players' kits should be read-only.
  Deploy and check host capture and each required guest's acknowledgment before
  the first live mission update. Repeat with one and four participants.

## Handoff

- Completed: retained the admitted connection through preparation and the loading
  barrier. Implemented authoritative preparation with immutable guest publications and
  resync, original campaign equipment import, and per-player world transfer and acknowledgments.
- Remaining: verify the guest equipment catalog fix, paired concurrent kit edits and
  cancellation, failed loading and recovery, final four-instance repeat, and ordinary-mode
  regressions. Enable the default menu route after those checks.
- Next action: finish the updated x86 build and capture paired kit changes in the native menu.
- Dependency status: COOP-005, COOP-007 and COOP-012 are integrated. COOP-012 merged at `4ef8b814` and documents the admitted-session contract.
- Decisions: First Arctic1 uses four original roster entries and AI fills unclaimed
  positions. Players claim their own soldier and every admitted player readies against
  the same preparation revision. The host serializes edits, and a failed loading attempt
  reopens preparation with retained kits and a fresh generation when the connection survives.
  The playthrough soldier picker and additional rules are covered by the updated campaign design.
- Local state: private builds, Wine prefixes and Xvfb displays :125, :126, :128 and :129
  stay in the task worktree. Network namespace isolation separates the local DirectPlay port
  from COOP-014. The guest catalog and briefing fixes are being built for the next checkpoint.
- Review notes: Shared launcher fan-out, per-actor input queues and world acknowledgments
  were coordinated with COOP-014. The preparation resync message reserves wire tag 21 and feature bit 7.
