# COOP 010 Two players in an original campaign mission

## Coordination

- Status: done
- Owner/session: Codex COOP-010, user assignment on 2026-11-18
- Canonical branch: `feature/coop-010-two-player-mission`
- Base commit: `6f1bdd96075b074a6a7b958257af5f56f8374fb5`
- Dependencies: COOP-008, COOP-009 and their transitive merged dependencies
- Last checkpoint: Arctic1 playable gate, native mode regressions, and navigation mismatch recovery
- Checkpoint location: `.worktrees/COOP-010` and the canonical branch on origin
- Review/integration: claim acknowledged in coordination ledger `69cb6361`. Separate exact-head review passed in `8455c6a6` against `main` at `6f1bdd96`.
- Pull request: [#22](https://github.com/xcntroller/HD2_BaseCoop/pull/22)
- Merged commit: `738b124d57a85901c167cebf9b07ca079aac2ac9` (reviewed head `fd1572a33c15445e7fdd66a0950d9ead305420e1`)

## Branch authority and completion PR

The assigned agent has full control of this task's branch: editing, committing, rebasing,
resolving conflicts, pushing to the project remote, and creating/updating its PR without
repeated user approval. Preserve other workers' work and shared checkpoints. When the
implementation and required validation are complete, push and create/update a PR targeting
`main`. Include reproducible **How to test** steps, prerequisites, and expected results;
separate checks actually run from proposed reviewer checks. State **Source-only** explicitly
when no runnable artifact is supplied, or **Documentation-only** for documentation tasks.
Record the PR URL in this handoff. Follow the [completion PR requirements](../agent_workflow.md#review-and-integration);
source-only delivery does not waive required runtime acceptance evidence.

## Result and scope

Connect the completed adapters through a development launch path and demonstrate two
players controlling two existing soldiers, with two squad AI, in one original base mission.
This is the first playable gate. Production menu/loadout UI, full campaign transitions,
mid-mission join, and save/load are distinct subsequent deliverables.

## Files and shared contracts

Own the development launch adapter, live protocol/session attachment, initial world-state
integration, and focused fixes discovered during play. Coordinate any legacy file changes
with active owners. Record the chosen mission, initial squad/loadouts, and launch procedure;
the production loadout screen must not be replaced with multiplayer UI for this test.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own development launch, initial-world
integration and live campaign attachment in `src/net/net_game.cpp`, `net_commands.cpp`,
and `net_messages.cpp` after COOP-008/009 merge. Follow [the protocol boundary](../coop_contracts.md#protocol-compatibility-and-snapshot-boundaries):
quarantine native join/spawn/respawn and actor input until campaign admission, reconstruct
the frozen manifest/world and require matching snapshot acknowledgments before simulation.
A Hello codec or preparation snapshot is insufficient. Reject new Running admissions
with `JoinNotSupported` until the later current-world snapshot task. Record fixed development
roster/loadouts/policies as fixtures, not approved production menu behavior.

## Acceptance criteria

- Two actual game instances negotiate compatibility and enter the same original mission
  with identical committed initial state after the loading barrier.
- Host and guest move, shoot, reload, use inventory, and observe two AI squadmates and
  matching objective progress. The host remains authoritative over AI and mission effects.
- Disconnect returns the same guest soldier to AI with health, equipment and position
  preserved; no extra soldier/life appears. Reject input from an obsolete owner/generation.
- Record mission success or a precisely bounded objective route, host/client logs, tested
  commits, and regressions in original singleplayer/native co-op. Claim only the behavior
  actually demonstrated; full mission/campaign completion remains its own later gate.
- No incomplete production menu, fake live UI, scoreboard, or multiplayer respawn flow is
  exposed. Missing runtime evidence keeps this task out of `done`.

## Handoff

- Completed: refreshed origin, verified both adapter merges, accepted the stopped
  coordinator handoff and published the claim. Created the canonical worktree
  from integrated main. Read the protocol, controller and lifecycle composition
  guides. Found the Steam installation and its Proton prefix, plus the shared
  x86 toolchain and engine DLL/import libraries. Added isolated command framing
  and the explicit campaign DirectPlay attachment. It carries exact connection
  incarnations through the callback queue, negotiates before forwarding payloads
  to the launch adapter, and gates legacy game commands, UDP and replication
  when campaign mode is active. Added explicit `-campaign-coop host` and
  `-campaign-coop join <IPv4>` launch paths. The launcher selects the original
  campaign PLAY step for **Arctic1** (mission index 1, seven objectives), builds
  the canonical compatibility descriptors from installed content, and commits
  the same never-reused guest PlayerId assigned by transport. The host imports
  the original Arctic1 kits 23, 22, 21 and 24 for roster profiles 0 through 3.
  It issues the original missing essentials (native item types 59 and 229),
  validates the fixed loadouts and opens a four-soldier Loading generation
  through the original PLAY step. Host and guest are assigned profiles 0 and 1.
  Profiles 2 and 3 use the original campaign squad AI. Assignment, ready
  actions, kit selection and essential distribution are development fixtures.
  Added a per-recipient world sender that schedules chunks for a sealed baseline
  and tested it against the existing assembler through the maximum 8 MiB cut.
  DirectPlay now captures the per-packet monotonic arrival time before enqueueing
  to account for frames received while world loading stalls the game thread.
- Both peers now build the frozen Arctic1 startup state through the original
  PLAY step. The guest reconstructs the original mission actors, spawn points,
  kits, and initial scripts, then validates the typed CIW1 world cut. The
  validation covers the initial objective IDs/status, actor poses, type and
  activation, human native inventory slots and stats, vehicle fuel, temporary
  actors, native item UID/state, and script startup cursors and save values.
  The time-zero actor poses come from the host. The guest sends an exact
  snapshot acknowledgment after controller, actor and world installation.
  The host releases simulation only after both required acknowledgments.
- The Running transport publishes typed, sequenced host mission updates for the
  exact initial snapshot, mission generation and ownership revision. It sends
  four squad actor poses, view and movement state, health and stance, the native
  objective statuses, input availability and the host cutscene camera. The
  guest updates its native actor presentation and objective definitions.
- The guest encodes on-foot held controls, button edges and view direction
  inside its exact ownership token. The host validates the payload and token
  at enqueue and again at execution, expires held input, and applies controls
  through native simulation and aiming from the guest soldier's actual muzzle.
  The guest's native network controller sends no position or authority state.
- The host sends realized fire and reload events with the exact actor and
  inventory identity. The guest uses the committed rounds and consumed magazine
  to update its native inventory and present the event. Both players use the
  native inventory and weapon selector. Requests to move or swap items stay
  within the owned soldier's containers and pass the original capacity and
  placement rules at the host.
- Both controlled soldiers now use the native player locomotion path on the
  host, including terrain contacts when the guest is outside the host view.
  The host updates native model and collision transforms from the simulated
  bodies. The guest renders those published model positions.
- Native squad controller attachment preserves the departing player's current
  equipment through the ownership transfer. The host verifies the same
  native actor, recorded pose, stats and entire inventory at the handoff.
- Both peers use the original campaign information screen for objective
  text and status. Campaign mode leaves the native multiplayer score,
  loadout and respawn routes inactive. Use the window close control to
  leave the development session. Escape still skips the host's intro.
- The recorded objective route starts at the original Arctic1 drop zone,
  activates the opening objectives and ends at the forest patrol. The guide
  rendezvous and mission completion remain unverified. The guest intro
  differs in scripted props, audio and HUD. The live state covers the squad
  and objectives. Ambient patrol movement remains host-side.
- Integration: the reviewed head merged on 2026-11-18. Its tree matches the
  merge rehearsal and the feature tree.
- Blockers: none. COOP-008 merged at `d34b6900` and COOP-009 at `ce306626`.
- Local prerequisites: root-checkout ignored `build/coop-shared-tooling/run-python.bat`
  configures preserved MSVC 14.44.35207 / SDK 10.0.26100.0 / Windows Python 3.13.9.
  Its `engine/` directory retains the matching port import libraries/DLLs; final
  COOP-009 logs/output are in root-checkout ignored `build/coop-009-evidence/`.
  Confirm these local paths exist before borrowing read-only with separate prefixes
  and isolated build outputs. No game installation/data is supplied by Git.
- Decisions: use the agreed four-soldier host-led model and preserve original campaign rules.
- Validation through the transport checkpoint:
  - `python3 _re/harness/campaign_protocol/run.py --compiler gcc` and the same
    runner using Windows Python and `--compiler msvc` passed eight suites with
    562,702 checks each. Logs are under ignored `build/campaign_protocol/`.
  - Pinned MSVC x86 isolated check compiled six changed network units with
    zero diagnostics. Isolated `game --all` built 283 units with zero
    diagnostics, linked successfully and set LAA. Build log is
    `build/transport-game.log`; this executable is local.
  - Ownership suite passed GCC and MSVC x86 with 10,721 checks each, including
    a skipped quarantine ID, exact admission, departure, and refusal to reuse
    or roll back an incarnation. Logs are in `build/campaign_ownership/`.
  - The launcher linked successfully with MSVC x86. Two actual Sabre Squadron
    1.12 game instances ran with private Wine prefixes and game copies on
    displays `:100` and `:101`. Host used `-window -campaign-coop host`.
    Guest used `-window -campaign-coop join 127.0.0.1`. Both logged Arctic1 and
    matching SessionId and PlayerId 2 (provider slot 1) in
    `build/runtime/{host,guest}/campaign-dev.log`.
  - With the host Loading fixture, full isolated `game --all` compiled 285
    units with zero diagnostics, linked and produced an LAA executable
    (SHA-256 `6c617aae18d9384cd26add5fadbe8e5227a29e17f9e4ab16807304e7cad4af98`).
    The two processes negotiated and the host loaded original Arctic1,
    bound soldiers 1–4 to actor slots 0–3 and logged generation 1 at mission
    time 0. Simulation remained stopped. The missing world acknowledgment
    triggered the 10-second timeout at mission time 0, and the guest logged
    session closure. The earlier admission-only run also logged the same
    guest incarnation departing after closing the guest window.
  - `python3 _re/harness/campaign_mission/run.py --compiler gcc` passed 409
    lifecycle and 104 native-source checks. The GCC preparation run passed
    behavior and original rule parity, including 810 capacity and 240
    mission-essential differential cases.
  - The paced world-sender checkpoint passed GCC and MSVC x86 with nine
    conformance suites and 564,472 checks each. Cases include the maximum
    512-chunk transfer, the per-pump and per-window caps, transport send
    failure, and elapsed-time and clock-rollback rejection. MSVC isolated
    check compiled both changed network units with zero diagnostics and the
    incremental game linked successfully.
  - On source checkpoint `33ad68fe`, a private guest copy with one extra
    line in the physical `Scripts.dta` archive reached campaign negotiation
    and received ContentMismatch (reason 32). The host did not load a mission.
    Restoring the file byte-for-byte from the installation and joining the same
    host admitted PlayerId 3 on provider slot 1. The host bound all four
    soldiers and timed out at mission time 0 again as expected.
  - Original-mode smoke on that build reached the solo main menu and the
    native Sicily2 LAN player setup. Both LAN instances listed both peers as
    preparing. Tutorial and native Sicily2 gameplay regressions remain pending.
  - Build and runtime evidence is preserved in the root checkout's ignored
    `build/coop-010-evidence/` directory.
- Initial-world validation at `055daf08`
  - Both real instances started generation 1 / snapshot 1. On the
    `tracker-replica` run the host and guest logged the same 37,646-byte
    committed record with CRC32 `2617437219`. Each rendered Arctic1 and
    remained responsive beyond the first frame. The guest rendered the
    starting terrain with its assigned soldier, while the host played the
    original aircraft intro. This documents the live presentation gap.
  - The preceding run crashed in melee tracker animation on the guest's
    first frame. Initialize the original dog's remote animation controller
    during the Loading phase and release it on guest mission teardown.
    The subsequent run had no recurrence of that crash.
  - The GCC and pinned MSVC x86 mission runners passed 409 lifecycle and
    108 native-source checks per compiler. The source tests cover the guest
    spawn during Loading and its rejection after Running.
  - The pinned MSVC x86 incremental runtime link completed with zero
    diagnostics. Host and guest used the same executable (SHA-256
    `4cca4f6c2fc0a9027e38ef0b821e9d5651a6187a1e841f04a052f37e9c058248`).
    Runtime logs, captures and the preceding crash report are in
    `build/coop-010-evidence/initial-world/` in the root checkout.


- Live mission publication validation at the `2cb0509e` checkpoint
  - The host log in `live-mission-v1` records the first four-actor update at
    mission time 100. The guest applied it and received objectives 2, 3 and 1
    opening. It entered the host's cutscene view at mission time 1,200 and
    returned to its own soldier at 33,000. Captures confirm the original terrain
    on both peers after the intro and a presentation difference during it.
  - In the `tracker-replica` run the host observed PlayerId 2's process
    disconnect. A new connection on provider slot 1 got PlayerId 3 and
    received `JoinNotSupported` (39) while the same host remained Running.
    The concurrent third local connection had timed out (37) before reaching
    campaign admission. The proven rejection used the freed provider slot.
  - Pinned MSVC x86 full game build compiled 286 units with zero diagnostics.
    The runtime build for `live-mission-v1` had SHA-256
    `42871f60945f934eb5936d3b35db2b183025a4a9ed310deca8213f34e95b607e`.
    The final incremental build includes a native track null check and
    terminates the session on a live publication failure. Its two affected
    units compiled with zero diagnostics and linked.
  - The protocol harness passed ten suites and 566,014 checks on GCC and
    MSVC x86. Tests cover the feature negotiation, host-only direction,
    malformed and duplicate actor records, and a new canonical wire fixture.
  - Logs and screenshots are preserved in root ignored
    `build/coop-010-evidence/initial-world/`. Player controls, completed
    objective progress and disconnect state preservation remain unverified.


- Guest input validation at `30b2d519`
  - The actual paired `tagged-controls` run entered Arctic1 and shared
    generation 1 / snapshot 1 and initial world CRC32 `205429481`. Native
    Win32 keyboard and mouse input was delivered to the guest window.
    The host logged that it accepted tagged controls from PlayerId 2 for actor
    1. Displacement, actual rounds used and reload completion still need
    direct state measurements. Guest ammunition and weapon effects currently
    lack the host event path.
  - Full pinned MSVC x86 build compiled 286 units with zero diagnostics and
    linked an LAA executable. Host and guest used SHA-256
    `30a3ef3c3300304f3d960564d06ddc461b2a9eaf5f06cbafddbc32a13b3c04c3`.
  - GCC and MSVC x86 protocol conformance passed ten suites with 566,056
    checks each, including malformed typed control masks, length and version.


- Native weapon, inventory and handoff evidence through `e206fce9`
  - Both instances matched 17 committed fire and reload events in the
    `weapon-events` run. Actor 0 fired the Sten from 32 to 22 and reloaded
    to 32. Actor 1 fired the Springfield from 5 to 0 and reloaded to 5. The
    guest HUD showed the remaining four Springfield magazines. This build
    was source `1edb3770`, with executable SHA-256
    `b4ecf447cde414b7ee60b572bd50563ede4aac1a303cb38659fcbbf40196738f`.
  - The `inventory-swap` run at `e206fce9` used original weapon selector
    inputs on both machines. The host committed items 251 and 271 for actors
    0 and 1, and the guest replayed both transactions. Captures show the
    Grenade No.36 for the host and Enfield .38 for the guest. Both logs
    matched the host displacement samples of 28, 63 and 91 cm and objective
    transitions 2, 3 and 1 to open. Guest displacement still needs a live
    verification. Both processes used SHA-256
    `23033938bd8dea1c4adddb786317a0e927b00f3616bba9e9ffe327db39920655`.
  - The earlier `inventory-handoff` run compared exact before/after native
    state and identity for actor 1. It preserved the same actor, health,
    original inventory and position at squad AI takeover and refused the
    departed PlayerId's control token (NotAdmitted). In the subsequent
    `inventory-swap` run with changed gear, the controller attached but
    state differed. Old-token rejection still passed. Native squad AI chose
    a weapon during controller attach. Retesting the changed kit is pending.
  - GCC and MSVC x86 protocol conformance at `e206fce9` passed 12 suites and
    567,370 checks on each compiler. The full x86 game build compiled 286
    units with zero diagnostics, linked and set LAA. The final single-unit
    trace link also passed. Logs and captures are preserved under
    `build/coop-010-evidence/initial-world/` in the root checkout.

- Terrain movement and changed-kit handoff validation
  - In `terrain-driven`, both actual game instances crossed the loading
    barrier at generation 1 / snapshot 1 with a 37,646-byte world cut and
    CRC32 `2919427009`. After the intro, native input moved actor 0 at least
    1,335 cm and actor 1 at least 1,216 cm in the horizontal plane from their
    starting positions. Both logs contain matching host and guest samples.
    The host's trace records intact health and model and body coordinates
    on the ground for both controlled actors.
  - The guest moved the Enfield .38, item UID 271, into his hands through the
    original equipment selector. The host committed the swap and the guest
    applied it. Terminating the guest process caused the host to attach
    squad AI to that same displaced actor. Before and after ownership
    transfer, the exact serialized native pose, stats and inventory bytes
    matched (CRC32 `2922004808`). The original process's token returned
    `NotAdmitted` (4).
  - The instrumented x86 executable had SHA-256
    `436e122168bfadabd7d1e62f9cd29b1658eb24e0cd2d56b37f61b261be05dbef`.
    The detailed diagnostic was local to this run. Build log, paired logs,
    trace and screenshots are in
    `build/coop-010-evidence/initial-world/terrain-driven/`.
  - The original singleplayer path used source `e206fce9`. The original
    campaign menus reached Tutorial, and native W and D moved Pte. Jenkins
    away from the starting watchtower once the briefing ended. Captures are
    under the `original-solo` evidence directory.

- Native Sicily2 gameplay regression
  - Both original LAN instances used the terrain validation executable.
    Opened the normal player setup, chose roster profiles 0 and 1, added
    British green camouflage Battledress, the Enfield Mk. 4 and its
    magazine, and accepted the loadouts. Both spawned in Sicily2
    with the committed equipment and the original three lives. Host W for
    three seconds moved the host ahead in the guest view. Guest W for six
    seconds moved the guest to the stone wall in the host view. Both opened
    the original inventories. The paired captures and process logs are
    under `build/coop-010-evidence/initial-world/native-sicily2/`.
  - The controller composition harness initially failed to build because
    its instrumented native sink has stubs only for the controller handoff.
    Source `4048303b` separates the live state and item presentation methods
    from that handoff translation unit. Both GCC and x86 MSVC passed the
    controller adapters (1,272 checks), native sink (237), and team rules
    (36). The native checks include the host's remote-owned player body
    and its absence from the host's local view.

- Original objective presentation and patrol follow-up
  - A paired run at `4048303b` exposed the legacy score overlay on the
    guest and a host crash on first patrol contact, at
    `C_SightModule::TestNow`. The host was missing the original sight and
    hearing registration. The guest also tried to render an extra actor
    frame after host loss had freed native replication objects.
  - In the subsequent `patrol-objectives` run, both original objective
    screens showed the guide, underground entrance and complex enemies as
    open. The host capture identifies Pte. Robert Czakowski, and the guest
    capture renders the other squad soldiers at the drop zone. Both peers
    crossed 134 m of horizontal displacement in the direct patrol attempt.
    The host resolved the casualties and both processes stayed running.
    Stopping the host caused the guest to exit without a fault report.
    The tested executable SHA-256 was
    `134903191e4a566bc5665526c41e537c5f47b79735bff9d6fb6f6d15b096cece`.

## Playable gate evidence

The development path is source-only. The following tests used licensed Sabre
Squadron 1.12 data and separate Wine prefixes on one machine. Local process
logs and captures are under `build/coop-010-evidence/initial-world/` in the
root checkout.

1. **Entry and opening objectives.** The host and guest at source `eaee9cc5`
   both started generation 1 / snapshot 1 in `guide-traverse` with CRC32
   `631251518` over the 37,646-byte world cut. Start followed the guest's
   reconstruction acknowledgment. All seven objectives were initially
   inactive. Host and guest recorded the original script opening objectives
   2, 3 and 1 in that order. The paired objective-screen captures from
   `patrol-objectives` show finding the guide, finding the underground
   entrance and clearing the complex all open.
2. **Bounded route.** In `patrol-objectives` the host skipped the aircraft
   intro with Escape. Each player stepped back from the drop zone to see the
   original squad, then held forward toward the tree line. The host reached
   137.04 m of horizontal displacement and the guest reached 134.06 m.
   The encounter ended with the host resolving both controlled soldiers'
   casualties, with the guide objective still open. At source `eaee9cc5`,
   `guide-traverse` repeated the forest approach with forward and right
   input and reproduced the casualty and objective results at 65.24 m and
   71.74 m respectively. Each peer opened the original map after losing its
   soldier. There were no crash reports or replacement soldiers.
3. **Controls and equipment.** In `acceptance-arctic1` at `5269c8bf`, the
   host and guest logged the same 13 realized weapon events. The Sten fired
   from 32 to 26 and reloaded to 32. The Springfield fired from 5 to 0 and
   reloaded to 5. Both native weapon-selector moves were verified in
   `inventory-swap`. At `eaee9cc5` the `crawl-corridor` guest moved 8.91 m
   and swapped to the Enfield .38 (UID 271) through the same native selector.
   Captures show Czakowski and Bennett in the guest scene after the host's
   casualty, when only the two squad AI remained beside the guest. In
   `final-squad` the host backed away from the drop zone and captured the
   guest and both AI soldiers in the same view. Both peers also recorded
   the host and guest inventory swaps (UIDs 251 and 271) in that run.
4. **Ownership and departure.** Closing the `crawl-corridor` guest preserved
   actor 1 through squad AI takeover. The current pose, stats and inventory
   bytes agreed exactly (CRC32 `636762`), and PlayerId 2's token returned
   `NotAdmitted` (4). The same provider slot next got PlayerId 3, which was
   rejected during Running with `JoinNotSupported` (39). Existing controller
   conformance also verified stale generation and revision rejection. The
   `final-squad` guest exited through the native window close message.
   The host again preserved actor 1 and the exact transfer state
   (CRC32 `1402296891`) and rejected the retired token.
5. **Original mode regressions.** The campaign menus loaded original
   Tutorial, where Pte. Jenkins walked. Two original native LAN players
   spawned in Sicily2, saw each other move, and opened their original
   inventories. See the `original-solo` and `native-sicily2` evidence
   directories and the checkpoints above for source and executable hashes.

The final game build compiled 287 units in pinned MSVC x86 with zero compiler
diagnostics and linked with LAA. The last map edit at `eaee9cc5` compiled and
linked with zero compiler diagnostics. Its tested executable SHA-256 was
`658de080fd7c6ba0b803380956ac15155c83b3b1ccaf375174f3eea4ee5a3552`.
GCC and MSVC x86 passed protocol (12 suites, 567,370 checks each), mission
(409 lifecycle and 108 native-source checks each), ownership (10,721 each),
and controller composition (1,272 controller, 237 native sink and 36 team
checks each).

The separate review added the original actor, navigation and sound data
to the mission fingerprint at `5939f0fa`. The altered private guest
navigation file reached negotiation and returned `ContentMismatch` (32)
while the host was still waiting to load. After restoring the original
file lookup, the same host accepted fresh PlayerId 3 on provider slot 1.
Both processes reconstructed 37,646 bytes with CRC32 `2384946426` and
started generation 1 / snapshot 1. The x86 incremental build linked with
zero compiler diagnostics. Its tested executable SHA-256 was
`592b499ca772a13b0f01502079db8359aa3b3ba228a29c9d7602db4f782d0dc4`.
The build, rejection and restored-session logs are in
`build/coop-010-evidence/initial-world/checkpoint-content-mismatch/`.
