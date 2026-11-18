# COOP 013 Live campaign preparation and first deployment

## Coordination

- Status: done
- Owner/session: Codex COOP-013, user assignment on 2026-11-18, thread `01a1edba-b5e3-7c73-bfa7-51e0b12fc205`
- Canonical branch: `feature/coop-013-live-preparation`
- Base commit: `fd5ad4c3e997be8a6d4a7211e04dd3319677e9a2`
- Dependencies: [COOP-005](COOP-005.md) equipment transactions, [COOP-007](COOP-007.md) preparation screen, [COOP-012](COOP-012.md) session setup
- Last checkpoint: tested source `5632e7b8` includes the default menu route and loading recovery
- Checkpoint location: canonical origin branch and `.worktrees/COOP-013`
- Review/integration: reviewed head `c3f69622` merged as `73d518a5d4d2f1822c3caa5b995ebd4100c75477`. The merged tree matches the review rehearsal.
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

The tested source is `5632e7b8`. The release build used Windows Python 3.13.9,
MSVC 19.44.35230 x86 with x87 floats and Windows SDK 10.0.26100.0. A clean game
build compiled 292 units with zero compiler diagnostics and linked a
large-address-aware executable. The last source change recompiled one unit
with zero diagnostics and linked again. See `build/evidence/build-final.log`
and `build/evidence/build-final-starter.log`.

Runtime checks used Sabre Squadron 1.12.0 from Steam build 2147249 with the
matching LS3DF and rw_data port libraries, Wine 11.21 Staging, private Xvfb
instances and an isolated IPv4 network namespace. The campaign menu runs
launched the normal executable with `HD2_CAMPAIGN_COOP_MENU` unset. Local drivers,
screenshots and logs are under the task worktree's ignored `build/` directory.

| Check | Actual result |
| --- | --- |
| `python3 _re/harness/campaign_protocol/run.py --compiler gcc` | 12 suites and 567,404 checks passed, including resync direction and negotiated feature checks. |
| `python3 _re/harness/campaign_preparation/run.py --compiler gcc` and `python3 _re/harness/campaign_preparation_ui/run.py --compiler gcc` | Preparation behavior, legacy rules, permissions, lifecycle and UI input passed. |
| `python3 _re/harness/campaign_ownership/run.py --compiler gcc`, `python3 _re/harness/campaign_control/run.py --compiler gcc` and `python3 _re/harness/campaign_mission/run.py --compiler gcc` | Ownership, controller composition, mission lifecycle and legacy source checks passed. |
| Live adapter harness command below | Encoded request traffic passed duplicate, stale and unauthorized requests, last-stock contention, ownership and readiness changes, snapshot gap recovery, departure, freeze and reopen checks. The same harness passed with AddressSanitizer and UndefinedBehaviorSanitizer. |
| `unshare --user --map-root-user --net bash build/menu_deploy_mouse_default.sh 1 arctic1-final-mouse` | Mouse claim, Ready and Deploy started Arctic1 with one human and three AI. The host captured 37,646 bytes and started generation 1, snapshot 1. See `final-one-host-initial-world.log` and `arctic1-final-mouse-world.png`. |
| `unshare --user --map-root-user --net bash build/menu_deploy_keys_default.sh 4 arctic1-final-four` | Four actual instances claimed distinct soldiers. All three guests reconstructed 37,646 bytes with CRC32 4167523918. The host accepted three acknowledgments before starting generation 1, snapshot 1. All guests applied the first live update with four actors. See `final-four-*-initial-world.log` and `arctic1-final-four-*-world.png`. |
| `unshare --user --map-root-user --net bash build/menu_two_edits_default.sh 2 arctic1-final-edits` | The host returned a Sten and the guest returned a Springfield from their own kits after concurrent submissions and a stale-revision retry. Both saw the other player's new kit as read-only. The original mission briefing, image, item names and 3D previews rendered. Confirming guest leave returned to campaign setup and restored AI control of the guest's slot on the host. See `arctic1-final-edits-*.png` and `final-edit-*-setup.log`. |
| `unshare --user --map-root-user --net bash build/menu_ai_default.sh 1 arctic1-final-ai` | The host equipped Robert Czakowski from stock while he was AI-controlled. The Sten count fell from two to one, his De Lisle left the hands slot and his kit weight changed. See `arctic1-final-ai-*-equip.png`. |
| `unshare --user --map-root-user --net bash build/menu_retry_default.sh 2 arctic1-final-recovery` | Suspending the guest before Deploy withheld its acknowledgment. The host timed out at mission time zero. Both returned to preparation on the same connection and readied again. Generation 2, snapshot 2 reconstructed 37,646 bytes with CRC32 1139519148 on both, then started and applied the first live update. See `final-recovery-*-initial-world.log`, `final-recovery-*-setup.log` and `arctic1-final-recovery-*.png`. |
| Solo **Single Mission → Op. Snowball - First Strike** | A private test profile with Arctic1 unlocked reached original team selection. Automatic Selection and Recommended Selection filled the native inventory and its original Wire-cutter and Camera requirements. Continue reached the original briefing and aircraft intro. See `final-solo-after-heroes.png`, `final-solo-equipped.png` and `final-solo-running.png`. |
| `unshare --user --map-root-user --net bash build/regress_native.sh` | Connected host and guest both displayed the original Sicily2 Player Setup with native soldier selection, equipment, chat and both players listed. No campaign setup or mission traffic was dispatched. See `final-native-host.png` and `final-native-guest.png`. |
| `unshare --user --map-root-user --net bash build/regress_dev.sh` | The COOP-010 command-line host and guest negotiated, acknowledged the initial world and entered Running. The guest applied the first live update. See `final-dev-*-initial-world.log`. |

Run the live adapter harness with GCC from the worktree root.

```bash
mkdir -p build/evidence
g++ -std=c++17 -Wall -Wextra -Werror -Isrc \
    _re/harness/campaign_preparation/live.cpp \
    src/game/campaign_preparation_live.cpp src/game/campaign_preparation.cpp \
    src/game/campaign_ownership.cpp src/game/campaign_session.cpp \
    src/net/campaign_protocol.cpp -o build/evidence/preparation-live
build/evidence/preparation-live
```

## PR test instructions and delivery

**Source-only** integrated gameplay. The [isolated build guide](../build_baseline.md)
covers the x86 build and copied runtime directories. Use Sabre Squadron 1.12.0,
matching engine libraries and one copy per participant on an IPv4 LAN.
Start `HD2_Port.exe` in each copied game directory.

1. Open **Single Player Game → Co-op Campaign**. Host the campaign and join by
   the host's IPv4 address. On the host, select **Prepare Mission**. Verify
   **Op. Snowball - First Strike** and open the original briefing.
2. In a two-player session, claim different soldiers and return or equip gear
   on both players' kits at the same time. Retry a stale request against the
   refreshed snapshot. Inspect the other player's changed kit and verify it
   is read-only. Select an unclaimed soldier as host and equip their kit.
3. Ready all players, then change a kit. Verify deployment waits for the new
   revision to be readied. Ready both again. Confirm the guest can leave from
   preparation and the host retains that soldier's committed kit under AI.
4. Repeat with one and four participants. Claim different soldiers, mark
   everyone ready, then Deploy. The host log must list each required
   guest acknowledgment before Start. Compare the byte count and CRC32 in
   each guest's reconstruction log against the host's capture.
5. For a loading-recovery check, suspend a private test guest after both
   players are ready, then deploy. Wait for the host's loading timeout and
   resume the guest. Both should return to preparation. Ready and deploy
   again, and check that the new generation and snapshot reach Running.
6. On the original solo route, enter an unlocked Arctic1 Single Mission and
   use Automatic Selection and Recommended Selection. In native LAN co-op,
   host and join Sicily2 and verify the original Player Setup on both peers.

## Handoff

- The menu route uses the first four original profiles and fills unclaimed slots
  with squad AI. Each user claims a soldier and confirms readiness against the
  same preparation revision. The host serializes equipment edits and publishes
  per-player snapshots. Failed loading reopens preparation with retained kits
  and a fresh generation when the connection survives.
- COOP-005 merged at `06d999d7`, COOP-007 at `8067b471` and COOP-012
  at `4ef8b814`. Shared launcher fan-out, per-actor input queues and world
  acknowledgments were coordinated with COOP-014. Preparation resync uses
  wire tag 21 and feature bit 7.
- COOP-014 owns mission interaction and completion. COOP-015 combines the
  preparation flow with later campaign steps after both dependencies merge.
- Local build and runtime evidence stays under `build/evidence/`. The
  copied game data, profiles and binaries stay in ignored directories.
