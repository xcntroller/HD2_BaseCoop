# COOP 014 Arctic1 mission completion and interaction parity

## Coordination

- Status: active
- Owner/session: Codex COOP-014, user assignment, thread `01a1ed7a-ddd9-7000-bbec-e292bd129d25`
- Canonical branch: `feature/coop-014-arctic1-completion`
- Base commit: `9ec7fe48bb21c38f9d95976b99198c6fb1576e74`
- Dependencies: [COOP-010](COOP-010.md) two-player Arctic1 development path
- Last checkpoint: `def38937` native squad message fix and paired cabin parity
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
  presentation, plus native host squad signal pages. The previous paired route
  attempted the ridge at matching mission state and lost both controlled
  soldiers while the two squad AI survived. It did not produce a terminal result.
- The clean base solo reference used a private 1.12 profile with best original
  campaign progress set to 1 to reveal Arctic1 in Single Mission. In the
  original menu select **Op. Snowball - First Strike**, use automatic squad
  selection and recommended equipment, and deploy. In this trial the active
  soldier bypassed the early patrol on a route along the southern ridge.
  The original guide spoke at the blue rendezvous marker.
  `build/evidence/original-solo/guide-contact.png` captures the dialogue.
  Full solo success is pending.
- Runtime captures so far verify visual subtitles and scene presentation.
  The private prefixes had native sound output disabled. A repeat with sound
  enabled is required to verify audible output.

## PR test instructions and delivery

- Delivery: Integrated gameplay; specify runnable artifact or Source-only at review.
- Prerequisites and reproduction steps: record build, game setup, player count, actions
  and expected results before review.
- Validation actually run: see the base and implementation checkpoints above.

## Handoff

- Completed: refreshed origin, verified COOP-010 merge `738b124d` is an ancestor of
  the base, checked for duplicate work, and established the canonical worktree.
- Remaining: original solo and paired completion, failure and terminal state evidence,
  guest interactions, full cutscene parity and the applicable legacy regressions.
- Next action: continue the original rendezvous on the solo reference and
  repeat the southern approach with both peers. Exercise the host squad orders,
  native door actions and the mission's success and failure boundaries.
- Blockers: none. Serialize live LAN tests with COOP-012 if the fixed provider port
  is occupied.
- Decisions: Use the existing development launch and game data. Base the interaction scope on what the original Arctic1 route exercises.
- Local state: private build outputs and runtime copies under ignored worktree `build/`.
  Shared pinned MSVC x86 and matching engine builds are available read-only in the
  root checkout at `build/coop-shared-tooling/`.
- Review notes: none.
