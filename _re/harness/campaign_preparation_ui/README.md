# COOP-007 preparation component

**Source-only**, isolated native component. This is not connected loadout selection,
a mode-picker entry, a campaign lifecycle adapter, or a new equipment-rule implementation.
The screen consumes the merged `I_CampaignPreparation` contract unchanged. The fixture
implements that same interface with invented soldiers, mission, inventory and policy.

## Build and run behavior checks

From the repository root, Linux Python/GCC C++17:

```sh
python _re/harness/campaign_preparation_ui/run.py --compiler gcc
```

Windows Python, MSVC **x86**, Windows SDK (see `docs/build_baseline.md`):

```bat
set "HD2_VCVARS32=C:\toolchain\vcvars-x86.bat"
python _re\harness\campaign_preparation_ui\run.py --compiler msvc
```

Expected: strict C++17 `/W4 /WX` compilation and the permission/transaction/race/lifecycle/
input PASS line. Every invocation creates a unique ignored `build/campaign_preparation_ui/`
output directory with compiler command, diagnostics and execution log. No engine, game
data or another unmerged task implementation is needed for these behavioral checks.

## Build the native LS3D preview

Build the matching LS3DF and rw_data DLLs using the integrated build guide. Do not use
an original LS3DF DLL against the hand-port interfaces. From the repository root:

```bat
python _re\harness\campaign_preparation_ui\run.py --compiler msvc --native --engine-lib ls3df\build\msvc\LS3DF.lib --data-lib rw_data\build\msvc\rw_data.lib
```

Copy the resulting `preview.exe`, the newly built `LS3DF.dll` and `rw_data.dll` into a
**private runtime copy** of the game data directory. It needs the legally installed
`.dta` archives, original Bink/Ogg/Vorbis DLL dependencies, and loose `Text/` directory.
The preview mounts the campaign archives in the same priority/key order as `app.cpp`.
It never loads a player profile, saves, connects to a server or modifies installed data.
Texture caches and screenshots may be generated in that private runtime directory.
Do not commit it. On Linux, archive symlinks are sufficient; copy loose Text files and runtime DLLs.

The preview compiles the original `C_TextureFont` implementation preceding the explicit
font-manager section marker into a generated, ignored source file. This reuses the actual
campaign font texture/glyph code while excluding the profile-bound global font manager;
no legacy source is edited and no fake game-profile symbols are linked. The build fails
if that extraction marker changes. New panels/portrait markers are original procedural
LS3D quads. The native resource audit opens `items selection`, `selection screen`, and
`description` and logs available anchors; it does not call their mutating menu builders.

Launch from the runtime directory:

```bat
preview.exe 1024 768
```

Expected: muted olive/paper campaign composition, four dossier slots, selected soldier
and committed inventory, shared stock, briefing/essential details, human ownership and
readiness, and explicitly guarded actions. A separate LS3D scene shows an original
campaign soldier model; fixture rifle/medical-kit props follow the committed loadout.
These are **synthetic presentation mappings**, not production face/uniform/attachment
catalog bindings. The latter are supplied by the injected painter at live integration.
No multiplayer equipment screen is used.

Controls:

- Click a squad slot to inspect it; selection never claims. Host edits own/AI, guest
  edits own soldier. Other human loadouts are explicitly read-only.
- Select a stock or inventory row, quantity and destination; Equip/Return queues one
  proposal. Pending display retains committed stock/items until the fake resolves it.
- Tab / Shift+Tab traverse all controls, including disabled controls with reasons.
  Enter/Space activate. Up/Down select list rows; Left/Right page lists. Explicit page
  buttons support mouse navigation. Briefing and essentials/restriction details have
  their own page controls. Skills is an inspect-only toggle.
- Escape opens/cancels local leave confirmation; Leave requires a second explicit action.
  Host and guest confirmation copy is distinct. Leaving closes polling; it does not
  undo an authority transaction or invent a network leave/cancel policy.
- F1 host; F2 guest; F3 initial loading; F4 pending AI edit; F5 scarce-stock rejection;
  F6 restricted one-soldier spectator; F7 Frozen; F8 Disconnected; F9 scripted soldier.
- F10 accepts the pending fake proposal, F11 rejects it as stale. O changes selected
  ownership to the guest and rejects the former editor; D disconnects guest to AI;
  H unassigns the host; R sets fixture participants ready at the committed revision;
  L installs 30 short briefing lines and 30 restriction findings for paging regression.
- F12 writes `coop007-<scenario>-<width>x<height>.tga`. Optional third numeric argument
  limits frames and captures the last frame, e.g. `preview.exe 800 600 60`.

Fixture policy is explicitly proposed: release before another claim, all connected fixture
humans ready, first committed scarce item. Reset advances the generation and preserves the
session/client request-ID allocator. These defaults are not newly approved product rules.

## Automated native input and captures

Use Windows Python from the private runtime directory, with the harness script's full path:

```bat
python C:\repo\_re\harness\campaign_preparation_ui\native_smoke.py --width 1024 --height 768
python C:\repo\_re\harness\campaign_preparation_ui\native_smoke.py --width 800 --height 600
python C:\repo\_re\harness\campaign_preparation_ui\native_smoke.py --width 1280 --height 720
```

The script starts the real preview, focuses its own window by process ID, sends Win32
mouse/keyboard input and asserts observable state traces. It checks host/guest rights,
unchanged pending stock, accepted Equip/Return conservation, keyboard traversal, rejected
edits, ownership loss during a pending edit, disconnect-to-AI, host without a soldier,
readiness invalidation, deployment freeze, nine status fixtures, detail pages, all seven campaign skills, long multiline briefing/restriction paging and leave
confirmation. It exits nonzero on a failed expectation or native process failure.
Run input suites **serially** on a display; multiple foreground automation suites conflict.

Outputs stay in the private runtime directory: `native-<width>x<height>.log`,
`input-evidence-<width>x<height>.txt`, and `captures-<width>x<height>/*.tga`.
Inspect the images as well as the PASS lines; state assertions alone do not establish
readability, clipping or native render behavior. This test uses one process with fake
host/guest snapshots; it makes no connected-host/guest claim.

Tested Wine launch on this workstation (from `build/native-runtime`):

```sh
env -u WAYLAND_DISPLAY DISPLAY=:99 LIBGL_ALWAYS_SOFTWARE=1 \
  WINEPREFIX=/absolute/COOP-007/build/wine-prefix \
  WINEDLLOVERRIDES='d3d8=n,b' WINEDEBUG=-all \
  wine64 cmd /c 'Z:\absolute\COOP-007\build\tooling\run-python.bat' \
  'Z:\absolute\COOP-007\_re\harness\campaign_preparation_ui\native_smoke.py'
```

The existing installed Proton prefix supplies `d3d8.dll`, copied into the private runtime
folder as documented in the baseline guide. The private toolchain/prefix and all generated
artifacts are local prerequisites, not part of this source delivery.

## Live binding contract

- The owner creates one UI-thread adapter attachment, then constructs the screen with
  `I_CampaignPreparation` and presentation resources. Keep both dependencies alive until
  the screen is closed/destroyed. Tick/Render and input are UI-thread operations.
- `GetSnapshot` supplies complete immutable state and the current synchronization status.
  Snapshot notifications/status events are hints: the screen re-reads the current view.
  It never patches stock, merges deltas, or installs a result's older snapshot. Gapped or
  partial transport updates must put the adapter into Resyncing/Loading before publication.
  The protocol/replica owns delta sequencing, not this screen.
- Within the same session/generation, view revisions never decrease. New generations and
  sessions are installed only through the adapter's canonical lifecycle view. The screen
  rejects a backward generation/view and requests resync; a current new generation clears
  pending presentation and stale selections. Readiness-only view updates preserve the
  expected PreparationRevision used in Ready/Deploy/Edit proposals.
- Presentation resolves text, reasons, item descriptions, item/place/skill names and assets from verified
  local catalogs. `COOP_*` labels are symbolic whole messages for localization; this task
  does not allocate numeric text IDs or overlay the legacy language table. Native fixture
  text is English ASCII; the sample font painter supports Latin UTF-8 tokens and uses
  an explicit question-mark fallback for unsupported glyphs. Full production localization and glyph/fit checks remain a live-painter gate.
  The details page labels eligible carriers separately from carried totals and exposes
  selected stock/loadout ammunition, rounds, charges, condition and item descriptions.
  Treat player names as plain text, never markup or format strings. Painter `Preview`
  receives only the selected soldier's committed loadout; copy it if retaining it.
- The painter supplies `Paginate` using exactly the same font metrics, wrapping and line
  height as `Text`. The screen displays those complete pages without byte slicing, so
  multiline blockers and UTF-8 boundaries remain reachable. Skills use independent
  bounded pages, including every catalog entry beyond the base campaign's seven.
- The painter draws native text/quads/assets and performs letterboxing; input uses the
  inverse transform. The component uses the 1024x768 safe area, with equipment/briefing/
  detail pages at every size. The included painter demonstrates 800x600 and widescreen.
- `NextRequestId` must use a session/client allocator shared across all families and all
  screen instances. Reopen cannot restart it. Local queueing is not acceptance; submission
  failures remain explicit, and the adapter enforces any client-wide pending limit.
- Mutating controls re-read status/capabilities on activation, then queue typed requests
  with session/player/request/generation/preparation/control/ownership revisions. One
  outstanding command disables other mutation controls while browsing remains available.
  Each edit has exactly the selected explicit soldier target. The authority still validates
  every proposal; UI capability guards are never an authorization substitute.
- Results must match local token **and** session/player/request/family/generation to clear
  pending presentation. Unrelated results never clear a newer command. GetSnapshot must
  already reflect the result's complete publication (or resync state) before its event.
- Close stops polling and releases the screen snapshot. Then the owner releases the
  attachment. No callback targets the screen and no pending command is undone. A new
  attachment/screen reads current state, while the shared ID allocator prevents old-result
  correlation. Retained immutable snapshot handles remain valid after screen destruction.

Normal mode routing, existing solo equipment panels, campaign globals, engine interfaces,
wire fields, profile layouts and `IsLocalGame` semantics remain unchanged.
