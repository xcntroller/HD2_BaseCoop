# COOP 009 Campaign mission lifecycle adapters

## Coordination

- Status: review
- Owner/session: Codex COOP-009, user assignment 2026-11-18; coordinator COOP-007 acknowledged claim; COOP-009 accepted its stopped-writer handoff at ledger `9e051608`
- Canonical branch: `feature/coop-009-campaign-adapters`
- Base commit: `0fcea78f85d0fe9348c3b8e13d1a0f23f50a482a`
- Dependencies: COOP-001, COOP-004, COOP-005, COOP-006
- Last checkpoint: source `6375ccad`, integrated-main sync `3a257b90`; final evidence in this documentation commit
- Checkpoint location: `.worktrees/COOP-009`, canonical branch on origin
- Review/integration: combined adapters validated; final COOP-008 independent review/merge gate pending
- Pull request: https://github.com/xcntroller/HD2_BaseCoop/pull/19 (draft)

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

Implement original campaign spawning and lifecycle adapters for a network session,
consuming committed squad/loadout state and preserving campaign rules. Keep them dormant
outside the explicit campaign-session boundary until COOP-010 connects the development run.

## Files and shared contracts

Expected hotspots: `game_app.cpp`, `game.cpp`, `spawn_point.cpp`, `game_rounds.cpp`, profile
settings, and script player-team queries. Coordinate `game_rounds.cpp` and actor/controller
integration with COOP-008. Narrow/split scope from the COOP-002 audit before claiming.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own `src/game/campaign_mission_adapter.{h,cpp}`
and lifecycle changes in `game.cpp`, `game_app.cpp`, `game_rounds.cpp`, `spawn_point.cpp`,
narrow profile-rule adapters, script player-team queries and host AI replication.
Follow [the audit and sequencing](../coop_contracts.md#legacy-coupling-audit-and-adapter-allocation).
You are the sole writer of these overlapping lifecycle files. Call COOP-008's agreed
disconnect/handoff helper before native actor/mine destruction; coordinate its signature
before implementation. Iterate roster bindings, not a contiguous actor-ID/player-slot
assumption. Supply frozen manifest, reconstruction/binding and barrier hooks; COOP-010
owns live dispatch attachment. Final combined adapter integration follows COOP-008 merge
and repeats affected checks; do not merge an unfinished dependency branch.

## Acceptance criteria

- Spawn the selected squad at original campaign points with committed equipment and stats;
  avoid multiplayer zones, warmup, lives, respawns, ranks, and map rotation.
- Distinguish simulation authority from campaign rules without redefining local play.
- Campaign script player-team queries include all deployed human and AI soldiers.
- Prepare explicit initial-world synchronization/loading-barrier and campaign-result hooks;
  define their integration obligations without claiming full save/load or late join.
- Build and run focused lifecycle regressions for ordinary singleplayer/native co-op.
  The completed adapter scope and dormant activation must be reviewable without placeholders.

## Handoff

- Completed: engine-independent frozen manifest/generation/actor binding/loading barrier,
  reconstruction acknowledgment/start hooks, original-point native spawn conversion,
  dormant lifecycle/profile/script/AI guards and source-extracted legacy regressions.
  Native file split `campaign_mission_native.{h,cpp}` was acknowledged by COOP-007.
- Peer review: COOP-008 found an unguarded guest Running publication; native
  ticking now additionally requires an explicitly installed reconstruction receipt
  and matching accepted Start. Reordered Running-before-Start remains gated.
  Adapter copies are disabled and SessionId lifetime is explicit in its header.
  Pre-generation retirement callback drains prior physical controllers before
  advancing ownership generation; failed/missing prior-world retirement does not
  advance the mission generation. Host simulation also requires its own successful
  lifecycle Start receipt for the exact sealed session/generation. Final peer
  rereview is pending; no source fixes remain.
- Remaining: final exact-head independent review and integration of PR #19.
  COOP-008 implementation and actual-done metadata are merged (`d34b6900`, `934143aa`);
  the agreed public progress setter is now called after native mission initialization.
  No live attachment/menu or two-player campaign gameplay claim; COOP-010 owns those.
- Next action: record final independent review and perform exact-head PR #19
  integration gates, then actual-done/dependent-task metadata follow-up.
- Validation run on final combined source `3a257b90` (own source `6375ccad`):
  strict GCC / ASan+UBSan and intended MSVC 19.44.35230 x86 `/W4 /WX` each PASS
  11 lifecycle/model-composition suites / 409 checks and 4 actual-source native
  regression suites / 104 checks. Final directories: `gcc-o6ovgh19` (sanitized),
  `msvc-mz0i94pz`; earlier regular GCC `gcc-y_90vbg1` passes the same suites.
  Exact commands, environment, compiler and execution logs are in those directories.
  Private sanitizer libraries use process-local LD_LIBRARY_PATH and
  `--sanitizer-library-dir`; no machine-global changes.
- Final build: `build/accepted-combined-game.log`: 282 translation units, zero
  compiler messages, link 0, LAA yes, MSVC x86/x87 flags. This includes finished
  COOP-008/007, both pickup guards, the committed progress setter and host/guest
  Start receipts. The focused combined fixture runs real session, ownership,
  host/guest control and protocol modules through world acknowledgment, reordered
  Running/Start, input, disconnect and pre-generation retirement.
- Diff/document checks: `git diff --check`, complete final scope/history review,
  Python runner syntax and local Markdown file links are required before merge.
- Runtime evidence: a private copied 1.12 installation launched intro/main menu on
  Xvfb :101, original campaign flow was attempted but no mission-play acceptance is
  claimed. Focused source regressions establish dormant solo/native settings and
  departure behavior; full live campaign acceptance remains COOP-010.
- Resources: original COOP-007 toolchain/worktree disappeared externally after its
  completion; 007/008/009 confirmed absence; none reports deleting it. Historical
  passing evidence remains accurate. Restored private 009 compiler/SDK/WinPython
  is `build/tooling/run-python.bat` (same 14.44.35207/10.0.26100.0/3.13.9), with own
  Wine prefix and isolated outputs. Read-only borrowing authorized for 008.
  A second preserved, reconfigured launcher is at root-checkout ignored
  `build/coop-shared-tooling/run-python.bat`; matching engine imports/DLLs are in
  its `engine/` directory. Final logs, executable and component outputs are copied
  to root-checkout ignored `build/coop-009-evidence/`. These paths survive loss of
  the task worktree; they remain local prerequisites/artifacts, never Git content.
- API: native callback hooks bind 008 `SynchronizeCommitted`,
  `FindCampaignTransportPlayer(int slot)`, `HandleCampaignDisconnect(PlayerId)`;
  resolve authenticated incarnation before connection cleanup. All world/snapshot
  and input dispatch obligations are in the [harness guide](../../_re/harness/campaign_mission/README.md).
- Scope: sole lifecycle writer; 008 retains all human/controller/actor consumers.
  Narrow scope expansion acknowledged by COOP-008 after its merge: exactly two
  actors_module.cpp pickup-respawn guards now exclude explicit campaign sessions,
  and its composition guide clarifies retaining views until physical retirement.
  No shared DTO/legacy layout/wire changes or generated tier1 edits.
- Prerequisites: 001 `15b572ff`, 004 `59a480c8`, 005 `06d999d7`, 006 `59cb8944` are merged.
  Final 009 source includes only merged dependencies; no draft branch is stacked.
