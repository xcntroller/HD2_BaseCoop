# COOP 010 Two players in an original campaign mission

## Coordination

- Status: active
- Owner/session: Codex COOP-010, user assignment on 2026-11-18
- Canonical branch: `feature/coop-010-two-player-mission`
- Base commit: `6f1bdd96075b074a6a7b958257af5f56f8374fb5`
- Dependencies: COOP-008, COOP-009 and their transitive merged dependencies
- Last checkpoint: branch kickoff and transport integration plan (this commit)
- Checkpoint location: `.worktrees/COOP-010` and the canonical branch on origin
- Review/integration: claim acknowledged by coordination ledger commit `69cb6361`
- Pull request: none

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
  x86 toolchain and engine DLL/import libraries.
- Remaining: live attachment, startup synchronization, host/guest verification and fixes.
- Next action: add bounded framing for the reserved campaign command and attach to
  the DirectPlay session. Use reliable ordered DirectPlay messages to keep campaign
  traffic on the provider-identified connection. Suppress the separate native UDP
  game-data path in campaign mode. Audit the original H&D2 Tutorial spawn and
  startup script as a campaign fixture candidate. Keep native Sicily2 as a
  separate regression route.
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
- Validation: branch/dependency checks passed. No game build or runtime validation has run for this task.
