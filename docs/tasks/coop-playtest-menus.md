# Campaign co-op playtest menus

The Fedora host and Windows guest reported that Esc opened no in-game menu.
The host's preparation capture also showed clipped loadout labels and controls
that blended into their panels.

## Coordination

- Branch `fix/coop-playtest-menus` starts at integrated `ca5ae2f8`. The
  coordinator acknowledged the claim at `0a60a0bd`.
- The changes cover the in-game menu and the preparation presentation. The
  mission exit uses the existing campaign teardown. COOP-015 retains ownership
  of progression, active mission briefing bindings and spectator readiness.

## How to test

Use the [Fedora build guide](../build_baseline.md) to compile the regular x86
game. Stage it in private copies of Sabre Squadron 1.12 with matching port
engine libraries. Start both peers with the same build on an IPv4 LAN.

1. On the host choose **Single Player Game → Co-op Campaign → Host Campaign**.
   Join from the guest and open **Prepare Mission**. Check that carried items
   show their name and placement on separate lines, and that each list has
   distinct selection, focus and page controls.
2. Select the starting helmet on an editable soldier and return it. The
   helmet leaves that soldier's kit and its stock count increases by one.
   Equip the helmet again from stock and check that the count goes back down.
   Use Next and Previous on the stock list. The highlighted item and target
   placement stay on the displayed page.
3. Claim distinct soldiers, ready both peers and deploy Arctic1. Guest Esc
   leaves the aircraft intro running. Host Esc skips the intro for both.
4. In gameplay press Esc on each peer. Resume, Options, and Exit to Main Menu
   appear. Press Esc again or choose Resume to return to play. The mission
   clock advances while the local menu is open.
5. On the guest choose Exit to Main Menu and confirm. The guest returns to
   the Co-op Campaign screen and the host continues. The host receives the departure
   through the normal campaign controller handoff.

## Validation and handoff

- Production x86 source `fffe7677` compiled all 292 units with zero compiler
  messages and linked under the pinned MSVC x86 toolchain. The staged
  executable SHA-256 is
  `254eca640eddd2ae6786ef3e7e6517e54a97444aa50dd55e7c2fea1a17080391`.
- The GCC and MSVC preparation UI harnesses passed, including the regression
  that page changes move the stock target to a visible item. The GCC campaign
  mission lifecycle and native source regression suites passed.
- The PR carries source and tests. Local runnable output is staged under the
  task worktree's ignored `build/runtime/game/`. On Fedora, launch it with
  `./build/run-coop.sh`. On an already staged Windows guest, replace
  `HD2_Port.exe` with the artifact from that directory.
- The production build ran a Fedora Wine host and guest with separate game
  copies, displays, prefixes and a private IPv4 LAN. The host returned and
  re-equipped a helmet, with shared stock changing 3 → 4 → 3. Next and Previous
  kept the stock selection on the visible page. The two players claimed,
  readied, deployed and reconstructed the original Arctic1 mission.
- Both players opened the in-game menu with Esc and resumed with a second
  press. The host received guest movement while the host menu was open. The
  guest opened Options, went back, and confirmed exit. The guest returned to
  the Co-op Campaign screen, and the host logged the guest's departure and
  remained in the mission.
- One sound-enabled final repeat stalled in the guest's aircraft intro.
  The menu and loadout checks above passed on a repeat with sound disabled in
  the two test prefixes. The staged player launcher uses its sound-enabled
  private prefix. Native Windows is an owner follow-up check.
