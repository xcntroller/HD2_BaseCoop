# COOP 012 Campaign session setup and menu entry

## Coordination

- Status: active
- Owner/session: Codex COOP-012, user assignment 2026-11-18, thread `01a1ed7a-8dcb-7ee1-8de0-23710cb94c81`
- Canonical branch: `feature/coop-012-session-setup`
- Base commit: `9ec7fe48bb21c38f9d95976b99198c6fb1576e74`
- Dependencies: [COOP-010](COOP-010.md) live admission and development session
- Last checkpoint: source `6bcb163b`; live setup validation and original-mode repeats in progress
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

## Setup contract for COOP-013

- The development opt-in is the process environment variable
  `HD2_CAMPAIGN_COOP_MENU=1`. The Single Player picker exposes **Co-op Campaign**
  under **H&D2 Original**. The new screen uses the installed singleplayer model,
  native buttons and native edit controls. It targets the original Arctic1
  operation, with Regular difficulty as the setup fixture value.
- `C_CampaignSetupSession` is a game-thread lifetime object. The main menu owns
  it while setup is shown. `GetHostOwnership()` exposes the authoritative roster
  and the admitted participant snapshot, and `GetHostJournal()` exposes the
  same request journal. `GetGuestAgreement()` exposes the negotiated guest
  session identity, compatibility fields and host-allocated PlayerId. Setup
  admits participants without claiming soldiers.
- The host allocates a session and catalog per successful listen lifetime. The
  installed roster contains 73 profiles. COOP-013 needs to select the eligible
  preparation roster when publishing protocol snapshots, which currently support
  64 soldier records. The untouched native roster remains available for that
  selection. A preparation state can retain the same setup object and thereby
  own its callbacks and journal, then close it when leaving the campaign.
- Setup uses the existing campaign codec and compatibility handshake through
  DirectPlay. The menu and COOP-010's mission launcher have separate DirectPlay
  application identities. Setup ends before driving preparation or a world
  transition. COOP-013 supplies preparation dispatch and the subsequent UI.
- The host's three remote transport slots plus its own participant establish the
  cap. The ports stay with the existing LAN transport. Guests enter the host's
  IPv4 address and DirectPlay connects on fixed UDP port 31567. Use 127.0.0.1 for
  two processes on the same machine. Discovery, DNS, and port overrides are
  outside this menu entry.

## Validation evidence

- Isolated MSVC x86/x87 incremental game build linked with zero diagnostics and
  large-address-aware enabled. Final full build at `6bcb163b` in progress.
- Real Wine/DirectPlay runtime on Sabre Squadron 1.12.0 reached the native setup
  screen from the menu on five independent game processes. Four participants
  including the host admitted, a fifth received SessionFull (38), and a guest
  exit returned the slot. An unhosted connect timed out and a second unhosted
  attempt was cancelled. With a private altered script archive, the fifth client
  received ContentMismatch (32). The same process retried successfully after
  restoration and received a new PlayerId in the original session.
- Host end returned admitted guests to a usable setup screen. The same host and
  guest processes established a second session with a new SessionId and shut
  down normally. These runs used the incremental executable SHA-256
  `717ce957b9f3b4f825f86c420bcbaf4c3de69e22a32eda28d02c3ca602795b27`
  immediately before the final source cleanup. Setup logs and screenshots are
  under this worktree's ignored `build/evidence/`.
- GCC 16.2.1 ran `python3 _re/harness/campaign_protocol/run.py --compiler gcc`
  (12 suites, 567370 checks) and
  `python3 _re/harness/campaign_ownership/run.py --compiler gcc`
  (9 suites, 10721 checks). Both passed. Final target build and repeat pending.

## PR test instructions and delivery

- Delivery: **Source-only**. The gated setup is integrated into the native game
  menu and tested with a locally built executable. No executable or game data
  is distributed.
- Build using MSVC x86 Windows Python and the matching port engine DLLs with the
  [isolated build guide](../build_baseline.md). Use an installed Sabre Squadron
  1.12.0 copy per instance. Each local simultaneous instance needs its own Wine
  prefix or a separate Windows machine.
- On host and guests, set `HD2_CAMPAIGN_COOP_MENU=1` in the environment and run
  `HD2_Port.exe` with no arguments. Select **Single Player Game → H&D2
  Original → Co-op Campaign**. The setup screen shows the Arctic1 fixture,
  **Host Campaign**, an editable IPv4 field, **Join by IPv4 Address**, and **Back**.
- With no host, choose **Join** and cancel once using **Cancel Join**. Try again
  and allow the timeout. Setup should stay usable. Start the host and join with
  three guests. The host should show four participants including itself, and a
  fifth process should report a full session. Leave on one admitted guest and
  retry the fifth. It should join in the released slot.
- For content rejection, append a marker to the physical `Scripts.dta` in a
  private guest copy while the host has room. Expect the compatibility error
  and a usable setup screen. Restore that archive byte-for-byte and retry the
  same host. The host should accept a fresh PlayerId. Compare the host and
  guest `campaign-setup.log` files for the admission and rejection results.
- Choose **End Session** on the host. Its guests should return to setup. Host
  again and repeat join within the same processes. The setup logs should show
  a new session identity and clean participant counts.
- Clear the opt-in variable and verify the ordinary Single Player picker. Repeat
  Original Campaign, Lone Wolf and Carnage menu entry, and the native LAN
  Sicily2 host/guest launch from the build guide. With the final build, also
  repeat the COOP-010 Arctic1 development launch from its task record.

## Handoff

- Completed: source and native setup prototype, isolated worktree, published
  claim and scope coordination with the COOP-014 worker.
- Next action: finish the exact-source x86 build and repeat the acceptance
  matrix and original-mode checks before integration review.
- Local state: shared compiler and engine libraries are available. Private
  runtime copies, logs and build outputs stay under ignored `build/`.
