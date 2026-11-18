# COOP 007 Campaign preparation screen component

## Coordination

- Status: active
- Owner/session: Codex COOP-007 / thread `01a1ec02-1d0b-7791-8a57-e16ee680b905`, user assignment 2026-11-18
- Canonical branch: `feature/coop-007-preparation-screen`
- Base commit: `dbc9a85a7161d8844f2907dbaf6586c7f3b67d19`
- Dependencies: COOP-002, COOP-003, COOP-011 (merged shared declarations)
- Last checkpoint: claim `7ea491e0`; current presenter/native harness checkpoint is `HEAD`
- Checkpoint location: `.worktrees/COOP-007`; `origin/feature/coop-007-preparation-screen`
- Review/integration: none
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

Implement the reviewed preparation screen as a component against the shared UI-facing
model contract. Develop with a fake model so work can proceed alongside preparation and
network implementation. Production menu entry and live synchronization are later tasks.

## Files and shared contracts

Own new preparation-screen source/resources and UI harnesses. Adapt reusable campaign
equipment components where appropriate; coordinate shared component edits with COOP-005.
Do not wire a fake or incomplete screen into the normal mode picker. Keep assets original
or generated at install/build time; do not commit extracted game data.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own `src/game/campaign_preparation_screen.{h,cpp}`,
`_re/harness/campaign_preparation_ui/`, and original resources agreed by COOP-003.
The fake and live bindings share [I_CampaignPreparation](../coop_contracts.md#preparation-model-and-ui-boundary):
immutable snapshots, queued proposals/results, explicit synchronization/phase status,
revision-bound readiness and capabilities with denial reasons. Local editor selection
is not an ownership or squad mutation. Consume the merged COOP-011 interface for the
fake; no COOP-005 model implementation dependency is needed. Coordinate any `equipment_panel.*`/
`equipment_frames.*` extraction with COOP-005; keep `equipment_screen.*` on its solo route.

## Acceptance criteria

- Render and navigate the campaign-style layout approved in COOP-003 through the model API.
- Show correct host/guest/AI edit rights and make read-only views unambiguously read-only.
- Exercise loading, rejected edits, changed ownership, scarce stock, readiness changes,
  keyboard/mouse input, and mission restrictions using representative fake states.
- Controls request validated transactions; they do not mutate shared squad/stock globals.
- Provide a runnable component preview/harness with visual/input evidence and document the
  live binding contract. Passing this task does not claim connected loadout selection.

## Handoff

- Completed: dependencies; COOP-002 merged `9f59ff47`, COOP-003 merged `dd2aa98e`,
  COOP-011 merged `0f367ed6`.
- Remaining: native component implementation and UI verification against the merged contract/layout/header.
- Claim acknowledged: coordinator ledger commit `cbe2a01b` (published).
- Next action: implement snapshot-driven presenter and input/transaction guards, then build
  the fake/native LS3D harness. No panel/frame extraction or shipping menu edits planned.
- Blockers: none for the fake-backed component; native visuals/input still require
  this task's runtime component evidence and campaign-style review.
- Decisions: new campaign-style UI, with existing singleplayer components reused when suitable.
- Validation: none for this task.

## Implementation checkpoint (2026-11-18)

- Added snapshot-driven C++ screen/presenter, semantic controls, focus traversal,
  equipment/ownership/ready/deploy queued proposals, explicit loading/frozen/disconnected
  gates, session/generation/token correlation and close/reopen handling. No legacy edits.
- Fake-backed behavior harness passes native GCC and intended MSVC x86 `/W4 /WX`.
  Commands: `python _re/harness/campaign_preparation_ui/run.py --compiler gcc`; Windows
  Python with `HD2_VCVARS32`: same script `--compiler msvc`.
- Native LS3D preview compiles and launches using the port's texture-font methods and
  original procedural panels. First native capture exposed missing alpha state; fixed.
  Visual/input acceptance remains INCOMPLETE; not a playable/connected feature.
- COOP-001 ignored compiler/artifact directories disappeared externally during work.
  Reconstructed MSVC 14.44.35207 / SDK 10.0.26100.0 / Python 3.13.9 under this worktree's
  ignored `build/tooling/`; compiled LS3DF (78 TUs, existing C4127) and rw_data successfully.
  Other workers may borrow the tools read-only with their own prefixes/outputs. Preserve
  these resources while active. Private prefix: `build/wine-prefix`; engine outputs
  isolated below `build/isolated/`. No compiler, binary, or game data committed.
- Next: inspect corrected native glyph output, complete preview/assets and keyboard/mouse
  scenario evidence, review full diff, update detailed reproduction/handoff and request review.
