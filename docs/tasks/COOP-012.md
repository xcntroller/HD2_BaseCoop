# COOP 012 Campaign session setup and menu entry

## Coordination

- Status: review
- Owner/session: Codex COOP-012, user assignment 2026-11-18, thread `01a1ed7a-8dcb-7ee1-8de0-23710cb94c81`
- Canonical branch: `feature/coop-012-session-setup`
- Base commit: `9ec7fe48bb21c38f9d95976b99198c6fb1576e74`
- Dependencies: [COOP-010](COOP-010.md) live admission and development session
- Last checkpoint: final tested source `994d034e`; live acceptance and regression evidence recorded
- Checkpoint location: `.worktrees/COOP-012` and canonical remote branch
- Review/integration: claim acknowledged at ledger `63b4ac3f`; parallel scope agreed with COOP-014 at `75131f7a`
- Pull request: [#26](https://github.com/xcntroller/HD2_BaseCoop/pull/26)

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
  64 soldier records. The full local catalog is available for that selection. A preparation state can retain the same setup object and thereby
  own its callbacks and journal, then close it when leaving the campaign.
- Setup uses the existing campaign codec and compatibility handshake through
  DirectPlay. The menu and COOP-010's mission launcher have separate DirectPlay
  application identities. Setup ends before preparation or a world transition. COOP-013 supplies preparation dispatch and the subsequent UI.
- The host's three remote transport slots plus its own participant establish the
  cap. Host-side admission commits PlayerIds in transport order even when
  an acknowledgment arrives early. An earlier peer's departure or the existing
  handshake deadline unblocks later admissions. Guests enter the host's
  IPv4 address and DirectPlay connects on fixed UDP port 31567. Use 127.0.0.1 for
  two processes on the same machine. Discovery, DNS, and port overrides are
  outside this menu entry.

## Validation evidence

Tests used Sabre Squadron 1.12.0 (Steam app 703320, build 2147249), Wine 11.21
Staging, native DirectPlay with IPv4, and private 1024×768 Xvfb game windows.
Local evidence is under the root checkout's ignored `build/coop-012-evidence/`.

| Check | Actual result |
| --- | --- |
| Runnable build | Windows Python 3.13.9, MSVC 19.44.35230 x86 and SDK 10.0.26100.0. Isolated `game --all` at `6bcb163b` compiled 290 units with zero diagnostics. The incremental build at `2c660342` recompiled the sole changed unit and linked with zero diagnostics. The final route fence recompiled two units with zero diagnostics and linked. PE32/i386, x87 and LAA. Final SHA-256 `49cd064d2e92cde95bcde6e55365a405a534512c02f95f97cbe708b2f824128b`. Logs `build-final.log`, `build-ordered-admission.log` and `build-route-fence.log`. |
| Protocol and ownership | `python3 _re/harness/campaign_protocol/run.py --compiler gcc` and `python3 _re/harness/campaign_ownership/run.py --compiler gcc` passed. Both repeated with Windows Python and `--compiler msvc`. Protocol 12 suites/567370 checks, ownership 9 suites/10721 checks on each compiler. |
| Gated UI and join | Five actual no-argument game processes opened Co-op Campaign at `2c660342`. An unhosted join timed out (37), a pending join cancelled, and host/guest reached setup. The host counted four participants, and a fifth got SessionFull (38). One guest left and freed the slot. Logs `*-final-setup.log`, captures `final-timeout.png`, `final-cancelled.png`, `final-session-full.png`, `final-fifth-refused.png` and `final-host-after-departure.png`. The `994d034e` pair also joined by typing a 15-character IPv4 address into the native field and pressing Return. Ending the host returned the guest to setup. Captures and logs `fence-menu-*` and `*-fence-menu.log`. |
| Compatibility and retry | One guest used a private changed `Scripts.dta`, reached live negotiation, received ContentMismatch (32) and returned to usable setup. The same process accepted a new PlayerId in the same hosting lifetime after archive restoration. The host then had four unmodified peers including itself. |
| Overlapping admissions | The actual host ran `2c660342`. A test-only guest delayed its acknowledgment by four seconds. The ordinary guest negotiated PlayerId 3 while PlayerId 2 still held its acknowledgment. The host committed both in order (2, 3) after the delay. Captures `ordered-*`, result `ordered-admission-result.log`, and local fixture build script `build-delayed-ack-fixture.py`. |
| Teardown and new session | Host End returned guests to setup with the host disconnected state. The same host and ordinary guest processes opened and joined a second session. Its SessionId differed, the guest received PlayerId 2 afresh, and ending it closed both sides normally. |
| Gate and original solo modes | With the variable unset, the picker displayed its original controls at `2c660342`. With the variable set, Campaign, Lone Wolf and Carnage retained their native squad selection at `6bcb163b`. Automatic Selection filled one slot in Lone Wolf and four in Campaign/Carnage. Campaign continued to the original Tutorial loading briefing. The later source change was confined to multiplayer admission order. |
| Native LAN | Actual native Sicily2 host and guest at `2c660342` using the build guide's `-cmd` LAN commands. Both reached the original Player Setup screen and showed both peers preparing. Logs/captures `final-native-*`. |
| COOP-010 launch | Actual two-client `-window -campaign-coop host` / `-window -campaign-coop join 127.0.0.1` at `2c660342`. Both loaded original Arctic1, matched 37646 bytes and CRC32 989510138, crossed generation 1/snapshot 1, and exchanged the first live mission state for all four squad actors. The `994d034e` repeat crossed the same barrier and applied the first live state. Logs `host-final-dev.log`, `guest-final-dev.log` and `*-fence-dev.log`. |

The initial five-process menu matrix used normal local LAN loopback. The exact
final menu matrix used an isolated Linux network namespace with a veth pair,
so a separate project test could use the same UDP port. A loopback-only
namespace did not provide a working DirectPlay device for paired admission.
After adding the veth pair, the menu tests completed. The native LAN and
COOP-010 checks on `2c660342` used normal local LAN loopback. The final route
fence repeats used the private veth namespace.

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

- Completed: gated native setup, admitted-participant and lifetime API,
  compatibility handling, four-player cap and original-mode regressions.
- Review: the COOP-014 writer is occupied with its separate mission acceptance.
  The coordinator performed a separate source, lifecycle, runtime-evidence and
  integration pass. Deployment and preparation remain assigned to COOP-013.
- Next action: integrate the verified candidate through PR #26 and record the
  merged commit. Private build outputs and evidence are available under ignored
  `build/`.
