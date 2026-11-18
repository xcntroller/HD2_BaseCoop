# COOP 010 Two players in an original campaign mission

## Coordination

- Status: active
- Owner/session: Codex COOP-010, user assignment on 2026-11-18
- Canonical branch: `feature/coop-010-two-player-mission`
- Base commit: `6f1bdd96075b074a6a7b958257af5f56f8374fb5`
- Dependencies: COOP-008, COOP-009 and their transitive merged dependencies
- Last checkpoint: original Arctic1 host Loading with four bound soldiers (this commit)
- Checkpoint location: `.worktrees/COOP-010` and the canonical branch on origin
- Review/integration: claim acknowledged by coordination ledger commit `69cb6361`
- Pull request: [#22](https://github.com/xcntroller/HD2_BaseCoop/pull/22) (draft)

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
- Remaining: live attachment, startup synchronization, host/guest verification and fixes.
- Next action: implement the versioned initial-world encoding and guest
  installation. Audit host and guest static actor creation before defining the
  dynamic actor, inventory UID, script and objective snapshot fields. Use the
  merged world assembler and exact acknowledgment barrier before starting either
  simulation. Then connect ownership-tagged input and replication. Tutorial and
  native Sicily2 are the original-mode regression routes.
- Blockers: no adapter dependency blocker. COOP-008 merged at `d34b6900` and
  COOP-009 at `ce306626`, with their transitive prerequisites integrated. This task
  still requires actual runnable host/guest instances, a complete initial-world
  format/reconstruction path and its stated runtime evidence; dormant components
  do not establish those behaviors.
- Local prerequisites: root-checkout ignored `build/coop-shared-tooling/run-python.bat`
  configures preserved MSVC 14.44.35207 / SDK 10.0.26100.0 / Windows Python 3.13.9.
  Its `engine/` directory retains the matching port import libraries/DLLs; final
  COOP-009 logs/output are in root-checkout ignored `build/coop-009-evidence/`.
  Confirm these local paths exist before borrowing read-only with separate prefixes
  and isolated build outputs. No game installation/data is supplied by Git.
- Decisions: use the agreed four-soldier host-led model and preserve original campaign rules.
- Validation at this transport checkpoint:
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
  - Build and runtime evidence is preserved in the root checkout's ignored
    `build/coop-010-evidence/` directory. Guest initial-world reconstruction,
    synchronized gameplay, in-play AI handoff and the original-mode runtime
    regressions remain pending.
