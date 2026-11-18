# COOP 014 Arctic1 mission completion and interaction parity

## Coordination

- Status: active
- Owner/session: Codex COOP-014, user assignment, thread `01a1ed7a-ddd9-7000-bbec-e292bd129d25`
- Canonical branch: `feature/coop-014-arctic1-completion`
- Base commit: `9ec7fe48bb21c38f9d95976b99198c6fb1576e74`
- Dependencies: [COOP-010](COOP-010.md) two-player Arctic1 development path
- Last source checkpoint: `22763e24` native gameplay integrated with COOP-013
- Checkpoint location: `.worktrees/COOP-014` and the canonical branch on origin
- Review/integration: COOP-012 coordinator accepted the claim in ledger `75131f7a`.
- Pull request: [draft #25](https://github.com/xcntroller/HD2_BaseCoop/pull/25)

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Finish Arctic1 in two actual instances through its original objectives and mission
outcome. Start with the guide rendezvous beyond the proven forest-patrol route. Fix
host-authoritative interaction and guest presentation gaps needed for this mission.
Pass a trustworthy mission result to the lifecycle boundary for COOP-015.

## Files and shared contracts

Own the in-mission interaction and presentation path for Arctic1 in the campaign
controller, script, objective and Running replication adapters. Audit original
interactions and scripted events on the route before changing shared native code.
COOP-012 retains setup UI, admission/lifetime, and DirectPlay setup. Coordinate shared
campaign development dispatch, protocol, or launch edits with COOP-012/013;
leave debrief and next-step glue to COOP-015. Record uncovered interaction families
for the later campaign-wide verification task.

## Acceptance criteria

- A solo reference run and a paired campaign run establish the original objective
  sequence and success outcome. Host and guest reach the same terminal outcome and
  objective states without skipping or rewriting objectives.
- Exercise the route's interactions and script/presentation requirements on both
  peers, including the guide and patrols. Verify that AI, damage and script effects
  run once on the host. Document and address guest cutscene, audio, HUD and ambient
  actor gaps that affect following or playing the route.
- Exercise a reproducible failure outcome and verify that it cannot be committed
  as success. Record the terminal squad state, including casualties and loadout changes,
  and repeat the COOP-010 handoff and original Tutorial/Sicily2 smoke checks if affected.

The user owns the full co-op playthrough. Engineering uses focused two-instance
checks and isolated debug fixtures. The full original paired success remains a
playtest gate. The original mission scripts and objective definitions are used
unchanged in the playtest build.

## Validation evidence

Base `9ec7fe48` built through Windows Python 3.13.9 and
`_re/tools/build_isolated.py game --all` with MSVC 14.44.35207 x86 and SDK 10.0.26100.0.
All 287 translation units compiled with zero compiler diagnostics, link returned 0,
and LAA was enabled. Base executable SHA-256
`e7b98b08a3ded24b9ab66b1889986d58de5d1325430e51cf2235f73433888acf`.

Two private Wine instances on displays :102 and :103 ran that build with Sabre Squadron
1.12 data and started generation 1, snapshot 1 after the guest reconstructed the
37,646-byte baseline. Both logged the opening objective activation. Compared the
aircraft intro in both windows. The host rendered the cabin and seated squad while
the guest showed a dark view with the in-mission HUD. The build and process logs are
in ignored `build/evidence/`; per-peer `campaign-dev.log` files are in
`build/runtime/{host,guest}/`. The base solo process reached the original main menu and Single Mission list.
A private profile copy now selects Arctic1 directly via the original Single Mission
menu by setting its best-progress field to 1. This changes menu availability only
and uses the installed mission scripts and assets.

## Original mission audit

Read the archive script assignments and the original Arctic1 scripts from the
local 1.12 installation. Use the official patch copies for the cutscene and
guide dialogue when auditing. The guide (Rebel) is a native AI actor and starts
his route via the original player proximity conditions. His staged routes and
cutscenes use the authoritative script's signals, AI movement and position
conditions. Missing or killing the guide can fail objective 1. The underground
entrance activates through a player range check. A separate script monitors
the original garrison and signals objective 3; its list deliberately excludes
one maintenance worker. When objectives 2 and 3 complete, the original
gathering condition checks all living squad members near the entrance. It
records the squad count and nearby door state for the next mission. All living
squad members surviving is a bonus, as is stopping the vehicle. These are
static findings. Actual solo and paired completion, failure, the native
interactions encountered in play, and final squad evidence are still pending.

The VM and AI currently tick on the host. The guest receives a camera, squad poses
and objective statuses but no final outcome, ambient human motion, or most of the
scene and HUD events used by Arctic1's scripts. The existing native result hook is
unwired and campaign teardown can write soldier state independently of mission
success. COOP-014 will make the original outcome and the captured terminal squad
available at the native lifecycle boundary, with COOP-015 consuming that result
for progression.

### Implementation checkpoint

- The first Running result/presentation build linked in pinned MSVC x86 with zero
  diagnostics. The isolated incremental command was `_re/tools/build_isolated.py game`.
  The test executable SHA-256 was
  `a61d7fe67bb58d5a1d2bed8bbe38cac04183a65be716901f5b66dda6061287b1`.
- GCC protocol (12 suites), mission adapter/native-source, and controller
  composition suites passed. Logs are in `build/evidence/first-component-gcc.log`.
  The protocol fixture update changes only the live mission update frame.
- Running updates now carry original mission objective outcomes, static ambient
  humans, and selected host-evaluated presentation events. The campaign result
  snapshots the original objective statuses and four native squad states before
  allowing the session to leave Running. No branch or profile advancement is
  performed at mission teardown. The result now retains the mission snapshot identity
  and original script save values for COOP-015.
- At `ed38a025`, a clean 287-unit x86 build passed, but the first paired intro
  failed to publish a mission update at sequence 30. The original intro calls
  the speech command with 1,000-meter near and far distances. Increasing the
  protocol bound to admit this range resolved that failure. The next pair
  rendered the aircraft exterior, rain and matching subtitles, with the
  gameplay HUD hidden. Both peers processed 30 ambient humans. The actors
  and subtitle screenshots are in `build/evidence/range-paired/`.
- The full cutscene/door candidate built with 287 units and zero diagnostics.
  GCC and MSVC x86 protocol conformance passed 12 suites and 567,504 checks.
  Cases include the authored speech range, ambient identity, frame pose,
  terminal outcome and door request validation. See
  `build/evidence/parity-protocol-{gcc,msvc}.log`.
- Synchronized the task branch with integrated COOP-012 at `fd5ad4c3`.
  The merged build found a deleted native squad message copy constructor.
  Corrected the ownership at `def38937`, after which incremental link passed
  with zero diagnostics. `build/evidence/merged-paired/` contains the
  subsequent two-process proof of matching aircraft cabin, actor and subtitle
  presentation, plus native host squad signal pages. The run later reached an original squad-wipe failure on the host. The
  guest had already closed, so only the host terminal log was captured.
  The result had success false, objectives 1 through 3 failed, and all
  four soldiers at zero health. Before shutdown the guest’s shot changed
  the authoritative ammo count to four and the guest presented the host event.
- The clean base solo reference used a private 1.12 profile with best original
  campaign progress set to 1 to reveal Arctic1 in Single Mission. In the
  original menu select **Op. Snowball - First Strike**, use automatic squad
  selection and recommended equipment, and deploy. In this trial the active
  soldier bypassed the early patrol on a route along the southern ridge.
  The original guide spoke at the blue rendezvous marker.
  `build/evidence/original-solo/guide-contact.png` captures the dialogue.
  The player then advanced through the original guide dialogue into the swamp.
  Leaving the authored path near the western edge of the swamp killed the active
  soldier. Full solo success is pending.
- Enabled sound in the two private Wine prefixes and recorded each game stream
  independently while the aircraft scene ran. Both 48 kHz stereo streams were
  non-silent, with peak levels of −2.4 dB on the host and −2.7 dB on the guest.
  The pair displayed the aircraft exterior and cabin with the native subtitles.
  Captures and per-process logs are in `build/evidence/audio-failure-paired/`.
- The private host debugger read the living AI follower's native simulation
  position at x=1,145 m while the guest still showed that soldier at the spawn.
  Orders reached the native follower, but its model was disconnected from the
  host-integrated body. The fix uses the authoritative movement path for the
  host's whole squad. On the next pair, both AI soldiers followed within about
  1 m of the host after the first 18 m of travel. Each peer received matching
  health values. The candidate linked with two units compiled and zero diagnostics
  and has SHA-256
  `72ceb8ef72ab4c5578943a6b9b636189ca79a888ad5d301c2a62b851a0656f3b`.
- In that pair, pressing Escape on the guest left the active aircraft cutscene
  playing. Pressing Escape on the host ended the cutscene on both peers and
  activated objective 1 once on each. The guest began its cutscene view at
  mission time 1,200 ms and left at 43,500 ms. The guest Escape screenshot
  and runtime evidence are in `build/evidence/grounded-squad-paired/`.
- In the same run, both controlled soldiers reached the original rendezvous
  with the two AI followers still alive. The guest identified the guide,
  Sygurd Vesaah, through the native crosshair. Two guest rifle shots produced
  two accepted host fire events and two guest presentations. The host rifle
  state reported four rounds after the first shot and three after the second.
  Guide health went from 545 to 55 to 0 on the host.
- The original guide death set objective 1 to critical failure (3) and objectives
  2 and 3 to failure (2) on both peers. The guest applied the terminal Failure
  result at mission time 903,400 ms. The host captured four original squad
  records, with health 515, 585, 545, and 555 and their native equipment slots.
  See `host-running-and-result.log` and `guest-running-and-result.log` in
  `build/evidence/grounded-squad-paired/`. The prior squad-wipe case recorded
  all four health values at zero. The result stayed in Ending for COOP-015.
- Campaign co-op suppresses the legacy multiplayer hit and kill messages
  within the original HUD. The isolated x86 relink for that change compiled
  one unit and reported zero diagnostics.
- The base solo reference used the authored walking points across the swamp
  and showed objective 1 as Success. The original HUD then instructed the
  player to use wire cutters at the fence. Captures include
  `after-swamp-objectives.png` and `cut-option.png` in `build/evidence/original-solo/`.
- Original Arctic1 fence panels can be addressed by stable authored
  frame name. The Running protocol now carries a bounded wire-cut request and
  its committed presentation. MSVC x86 compiled the full candidate and exposed
  a const-qualified frame bound call. The corrected source compiled and linked,
  with no diagnostics in the retry. GCC and MSVC x86 protocol conformance each
  passed 12 suites and 567,510 checks. Live cut validation remains pending.
- On the wire candidate, the host moved a Sten magazine to the backpack and the
  wire cutters to a belt pouch. The guest applied both inventory events. The guest
  lost his soldier at the hut, then remained connected while the host and
  both AI soldiers crossed the swamp. Both peers recorded objective 1 as Success.
  The surviving squad died under fire at the factory fence. Both peers recorded
  Failure and objective states 1 / 2 / 2. The terminal snapshot records all four
  casualties, four Sten magazines in each container, and item 59 (cutters) in the
  host’s belt pouch. Logs are in `build/evidence/wire-candidate-paired/`.
- Repeated the COOP-010 handoff on the same x86 artifact, SHA-256
  `b6fc83cc647e649fa54add94e6d2f4566157d6fdfeaee2099c3f587da7150a5f`.
  After the living guest moved and closed his window, the host preserved native
  actor 1, switched it to the squad controller, matched the native transfer
  state CRC32 `912428408`, and rejected the retired token with `NotAdmitted`.
  Evidence is in `build/evidence/handoff-repeat/result.log`.
- Original Tutorial loaded from the base campaign menu, played the training
  scene, and the selected soldier walked under native WASD input. The original
  Sicily2 LAN host and guest both reached the native Player Setup screen and
  listed both peers preparing. The private prefixes followed the build guide’s
  DirectPlay IPv4 setting. Captures are in `build/evidence/legacy/`.
- The unmodified solo reference completed the original mission using its native
  Single Mission mode, squad orders, weapons and quicksaves. After the guide
  crossed the swamp, the squad approached from the east road, found the
  underground entrance, and cleared the original garrison through the docks.
  `build/evidence/original-solo/garrison-cleared-objectives.png` shows objectives
  1 through 3 as Success and objective 4 open. The three survivors returned to
  the entrance, and the original HUD showed **Mission Successful** and
  **Squad assembled at the entrance** in `mission-successful.png`. The final
  roster and health values were Czakowski 0, Smith 116, Wright 368, Saunby 13.
- In `build/evidence/coordinated-road-paired/`, the guide objective succeeded on
  both peers and all four soldiers regrouped on the east road. Host-owned AI
  killed the three east gate sentries. Both players lost their soldiers inside
  the factory before reaching the underground entrance, and one AI soldier
  survived that attempt. The pair did not reach a terminal result.
- A live guest door request exposed an obsolete input-kind limit in the control
  gate. The gate now shares the codec's limit. The regression checks admit and
  sequence valid door and wire-cut input and reject a repeated sequence and
  an unknown kind. GCC and MSVC x86 each passed 12 suites and 567,516 checks.
  The guest's right mouse button now runs the local native aiming view. The
  ordinary rifle view, iron sights and return to the ordinary view were
  captured in `build/evidence/authorized-actions-paired/`.
- The native door and fence reach check now uses the local hierarchy bounds.
  For the initially open hut door, the guest begins its authored opening
  motion after the initial-world pose check. The candidate linked with no
  compiler diagnostics and has SHA-256
  `4d03fd04e835040f0f1d96b65d040d52803b240062eacd378bbc81e2ed6c55c1`.
  In `build/evidence/doors-running-paired/`, the guest selected Close and Open
  for door 38 using the original action menu. The host accepted each request
  once. Both instances finished closing at angle 0 after the guest stepped
  back, and both started reopening in the subsequent check.

### Integrated and debug-assisted checks

- Integrated current `main` at `be6a81cb`, including COOP-013. The mission
  update, presentation and terminal result use the admitted peer set. The
  clean production x86 build compiled 292 units with zero compiler diagnostics.
  Release executable SHA-256
  `d691fe98f6b01acf9ca38b373ded12c96c3112fffc79b6f2124689053347b9b8`.
  GCC and MSVC x86 protocol checks each passed 12 suites and 567,550 checks.
  The GCC controller, mission lifecycle and legacy source suites also passed.
- **Production deployment.** Two copies of the release entered Arctic1 from
  the standard Co-op Campaign menu. Both players claimed and readied distinct
  soldiers. The host captured a 37,646-byte initial world with CRC32
  `510781437`, the guest reconstructed that same world, and both entered
  Running at generation 1 and snapshot 1. The guest received four squad
  actors and 30 ambient humans and started the shared aircraft scene.
  See `build/evidence/menu-production-*-campaign-dev.log`.
- **Production handoff.** On the integrated release, a living guest moved
  and closed the window. Actor 1 stayed in the world under squad AI with
  native transfer CRC32 `3327485350`. The host rejected the retired token as
  `NotAdmitted`. See `build/evidence/integrated-handoff/result.log`.
- **Debug-assisted wire and success.** A checked host moved the native
  cutters into its pouches and used panel 99 next to a seeded player position.
  The host accepted one request and completed one native cut. An ordinary
  production guest presented the cut once. After the fixture seeded
  objective success and induced one native squad casualty, both peers
  reached the same terminal Success with objectives 1 through 4 complete.
  The host captured four soldiers, including the casualty and the cutters
  still in the host's pouches, and retained the fixture's script save value
  in the mission result. The guest applied the terminal result at
  mission time 13,300 ms. The log verifier passed on
  `build/evidence/debug-wire-success/`.
- **Debug-assisted native failure.** The checked host induced the guide's
  death through the script's native damage entry point. The unchanged
  mission script set objective 1 to critical failure. A later completion
  attempt left it at status 3. Both peers reached Failure with objective
  states 3 / 2 / 2, and the terminal record contained four soldiers.
  The log verifier passed on `build/evidence/debug-guide-failure/`.
- The fixture lives in the isolated harness under `_re/harness/campaign_arctic1/`.
  The production build has no fixture hook. The seeded terminal result is
  technical evidence for synchronization. The original solo completion
  is the genuine playthrough evidence. The original paired completion
  remains for the user's full playtest.

## PR test instructions and delivery

**Source-only** integrated gameplay. Build the normal x86 executable with the
[isolated build guide](../build_baseline.md). Use Sabre Squadron 1.12, matching
engine libraries, and one private game copy per peer on the same IPv4 LAN.
The [controlled-run harness](../../_re/harness/campaign_arctic1/README.md) is
separate from that build.

### User playtest

1. In **Single Player Game → Co-op Campaign**, host and join by IPv4. On the
   host, select **Prepare Mission** and confirm **Op. Snowball - First Strike**.
   Claim distinct soldiers, equip them, and ready both peers. Put the wire
   cutters in a soldier's pouches so the original action menu can use them.
   Deploy with both players connected.
2. Watch the aircraft intro and compare the original camera, subtitles and
   audio. Escape on the guest leaves the scene playing. Escape on the host
   advances both peers. On the default controls, the host can order the
   unclaimed soldiers to follow quietly with numpad 0, 4, 1.
3. Clear or avoid the hut patrol. From just outside the hut door, aim at its
   panel, press Space, and choose the enabled Open or Close action on the
   guest. Give the door room to swing. Both peers should reach the same state.
4. Contact the guide at the rendezvous. Stay near him during the dialogue
   and follow the route he shows. Keep both players on his path in the swamp.
   Confirm both sides mark the guide objective as Success.
5. With the cutters in the pouches, aim at an intact fence panel from within
   reach, press Space and choose **Use wire-cutters**. Wait for the native
   action. The gap should appear on both sides.
6. Locate the underground entrance. Clear the original garrison, including
   the guards along the docks. The east road also provides a route into the
   facility. Return every living squad member to the underground entrance.
   Both HUDs should show **Mission Successful**. Verify the four mandatory
   objectives are complete and the two `campaign-dev.log` files report the
   same final outcome and objectives. The host log records terminal casualties
   and gear. COOP-015 owns the debrief and progression.

## Handoff

- Completed: refreshed origin, verified COOP-010 merge `738b124d` is an ancestor of
  the base, checked for duplicate work, and established the canonical worktree.
- Remaining: original paired success in the user's full playthrough.
- Next action: use the normal build and the user playtest steps above.
- Blockers: none. Serialize live LAN tests with COOP-013 when the fixed provider port is occupied.
- Decisions: Use the existing development launch and game data. Base the interaction scope on what the original Arctic1 route exercises.
- Local state: private build outputs and runtime copies under ignored worktree `build/`.
  Shared pinned MSVC x86 and matching engine builds are available read-only in the
  root checkout at `build/coop-shared-tooling/`.
- Review notes: none.
