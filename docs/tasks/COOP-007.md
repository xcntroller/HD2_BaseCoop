# COOP 007 Campaign preparation screen component

## Coordination

- Status: done
- Owner/session: Codex COOP-007 / thread `01a1ec02-1d0b-7791-8a57-e16ee680b905`, user assignment 2026-11-18
- Canonical branch: `feature/coop-007-preparation-screen`
- Base commit: `dbc9a85a7161d8844f2907dbaf6586c7f3b67d19`
- Dependencies: COOP-002, COOP-003, COOP-011 (merged shared declarations)
- Last checkpoint: source `d3269c4a`; latest handoff/metadata is `HEAD`
- Checkpoint location: `.worktrees/COOP-007`; `origin/feature/coop-007-preparation-screen`
- Review/integration: independent COOP-003 full, targeted final and metadata review PASS; PR #12 merged `8067b4710a5d68f46e37a8cfc5d4a83d4bd33219`
- Pull request: https://github.com/xcntroller/HD2_BaseCoop/pull/12 (merged)

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

- Completed: immutable snapshot-driven campaign preparation component, typed queued
  equipment/ownership/readiness/deployment proposals, local selection, explicit rights
  and synchronization states, input/focus navigation, measured document paging, all
  campaign skills, item-variant inspection, fake model and native LS3D preview.
- Delivery: **Source-only component**. It is not wired into the shipping mode picker,
  campaign flow, or network transport. No connected loadout selection is claimed. The
  fake/native component meets this task's bounded acceptance; live catalog/face/uniform/
  attachment and full localization bindings remain later integration work, not a final
  shipping-look claim. The native fixture explicitly uses original procedural panels/
  portrait markers and an unposed original campaign model with synthetic prop mappings.
- Scope: `campaign_preparation_screen.{h,cpp}`, `_re/harness/campaign_preparation_ui/`,
  and this record only relative to integrated main. No shared header, legacy equipment
  panel, profile/global stock, engine ABI, native network, or menu-routing changes.
- Source checkpoint: `d3269c4a`; merged-main synchronization `b7e6bb16` contains integrated
  main `0fcea78f`, with no stacked unfinished branches. This metadata checkpoint is `HEAD`.
- Independent review: COOP-003 owner/thread `01a1ebd2-c9c0-73a1-9495-337780d904f6`
  reviewed the complete source/API/fake/native harness/runner/docs and native captures.
  It independently compiled/executed strict GCC behavior checks twice. Initial findings
  (unreachable multiline text and skills beyond five) were fixed at `d3269c4a` with measured
  native pagination, skill paging, 30-line UTF-8/finding and 7+11-skill regressions.
  Targeted final rereview **PASS**, no blocking findings. It inspected native briefing
  page 3/3 containing line 30, skill page 2 containing skill 7 and the final details page.
- Native visual/input evidence: final strict target build plus real Win32 SendInput suites
  **PASS** at 1024x768, 800x600 and letterboxed 1280x720. Captures inspected at all three
  sizes; ownership/read-only/pending/rejection/status and page controls are readable,
  footer reachable, final briefing/restriction lines and all skills accessible.
- Product policies: fixture-only all-connected-human readiness, release-before-claim and
  first-committed scarce stock are labeled synthetic proposals. No open production policy
  was silently resolved. Authority still validates every operation.
- Remaining: none for COOP-007. The reviewed component is integrated through PR #12;
  this documentation follow-up records the actual merge. Live preparation/menu/painter
  integration remains later work with the explicit limits above.
- Next action: preserve the task worktree/tooling/captures and hand coordinator state to
  the active lifecycle workers. No further COOP-007 implementation work is required.

## Acceptance evidence

| Criterion | Result / evidence |
| --- | --- |
| Campaign-style layout/navigation through model API | Native LS3D scene/quads, original campaign font renderer and model, four dossier slots, briefing/equipment/details/skills pages, essentials and readiness/footer; same component behind fake/live boundary. |
| Host/guest/AI rights and unambiguous read-only | Snapshot capabilities plus ownership guards; native host/guest/unassigned-host/scripted/restricted captures and input assertions. Selection never submits ownership. |
| Loading/rejection/ownership/scarcity/readiness/input/restrictions | Native nine-state fixture plus ownership-loss pending rejection, guest disconnect-to-AI, scarce stock, ready invalidation, freeze, keyboard/mouse and leave confirmation; behavioral wrong-token/session/player/request/family/generation and close/reopen checks. |
| Validated proposals without global mutations | Only seven model-interface methods used; typed requests bind preparation/control/ownership revisions; committed display remains unchanged pending results. No legacy globals referenced. |
| Runnable preview, visual/input evidence and live contract | Strict MSVC x86 native executable runs with installed resources; repeatable native input/capture driver and detailed README/live binding contract. This is a fake-backed component, not connected selection. |

## Validation actually performed

Environment: Fedora 44, GCC 16.2.1, Python 3.14; Wine 11.21 private prefix;
MSVC 19.44.35230 x86 (toolset 14.44.35207), SDK 10.0.26100.0, Windows Python 3.13.9;
Xvfb :99 at 1280x960. Installed Steam game data is the COOP-001 baseline installation
(app 703320, build 2147249). The component uses a private runtime directory; it never
loads a player profile, campaign save or connected game session.

From this task worktree root:

```sh
python _re/harness/campaign_preparation_ui/run.py --compiler gcc
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all \
  wine64 cmd /c 'build\tooling\run-python.bat' '_re\harness\campaign_preparation_ui\run.py' --compiler msvc
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all \
  wine64 cmd /c 'build\tooling\run-python.bat' '_re\harness\campaign_preparation_ui\run.py' --compiler msvc --native \
  --engine-lib 'ls3df\build\msvc\LS3DF.lib' --data-lib 'rw_data\build\msvc\rw_data.lib'
python -m py_compile _re/harness/campaign_preparation_ui/run.py _re/harness/campaign_preparation_ui/native_smoke.py
git diff --check
git diff origin/main...HEAD --check
```

- GCC and intended MSVC x86 `/W4 /WX`: **PASS**, including complete multiline/UTF-8
  text reachability, seven plus additional skills, item variant metadata, revision-bound
  readiness, stale/generation fences retained across loading, wrong correlations, pending
  stock, accepted/rejected edits, permissions, local selection, paging and closed-screen lifetime.
- Final target outputs: `build/campaign_preparation_ui/msvc-6j44h_k1` (behavior),
  `msvc-iqgq6v8g` (native preview); native C++ source matches `d3269c4a`.
- Engine/DTA prerequisite builds: isolated LS3DF 78 TUs/link **PASS** (existing C4127),
  rw_data **PASS**. They use process-local TEMP/TMP and ignored worktree-specific outputs.
- Native resource audit: `items selection`, `selection screen`, `description`, `mp_en01`,
  `w_enfield`, `l_medkit` **all load successfully**. Expected legacy anchors are logged.
- Native input suite commands (Windows Python from private `build/native-runtime`,
  full script path): `native_smoke.py --width 1024 --height 768`, then `--width 800
  --height 600`, then `--width 1280 --height 720`, serially. All **PASS**. Complete setup,
  Wine invocation and expected results are in the harness README; state traces and captures
  are generated by the executable, not an HTML mock or screenshot recreation.
- Local evidence: `build/native-runtime/final-smoke-{1024,800,1280}.log`,
  `native-<width>x<height>.log`, `input-evidence-<width>x<height>.txt`,
  `captures-<width>x<height>/*.tga` (derived PNGs for inspection). Not committed: game imagery,
  data, toolchain, native artifacts or local build outputs. Reviewer can regenerate them.
- Final local preview SHA-256: `8c96efbfa3a713bbf8d3562700381c575cd393fefcda70b28548f3377f733c76`.
- Not claimed/tested: live host/guest loadout synchronization, shipping menu integration,
  full production face/pose/attachment/material binding, all-language glyph/fit coverage,
  mission completion, save/load or controller/actor behavior. Those belong to later tasks.

## Local resource preservation

COOP-001's ignored worktree/tooling outputs disappeared externally during this task.
The owner confirmed they had not removed/moved them. COOP-007 reconstructed the pinned
compiler/SDK/Windows Python under ignored `build/tooling/`; source setup remains the
integrated build guide/configurator. Its `run-python.bat`/`vcvars-x86.bat` and native DLLs
are preserved and may be borrowed **read-only**, with separate prefixes/outputs. COOP-004,
005 and 006 completed target checks using that route; 008/009 have current authorization.
Do not remove these resources while other active workers depend on them.

## Integration outcome

Actual feature merge: `8067b4710a5d68f46e37a8cfc5d4a83d4bd33219` via PR #12,
exact reviewed head `4645f814` and source `d3269c4a`. COOP-003 independently reviewed
the complete implementation and final fixes, reproduced strict GCC behavior checks twice,
and inspected native captures including the final briefing/restriction lines and skills
page 2 (skill 7); the automated regression checks cover all seven and eleven skills.
It then passed the final read-only metadata check, confirmed all three native input-suite
PASS logs and verified the preview hash. The integrator separately checked the exact PR
head, final diff/history/scope, links and evidence, then merged normally with a head match.
No task work remains. This completion metadata changes documentation only; no component,
resource, build or runtime behavior changed after the reviewed source checkpoint.
