# Campaign shared declaration conformance

COOP-011 supplies [campaign_contracts.h](../../../src/game/campaign_contracts.h),
not a production model, protocol, game adapter or screen. **Source-only** delivery;
local fixture executables/objects/logs stay ignored under `build/`. No game data,
engine DLLs, Windows transport objects or profile globals are needed. No gameplay
entry point is changed. See [the logical contract](../../../docs/coop_contracts.md)
and [task evidence/handoff](../../../docs/tasks/COOP-011.md).

## Run the required target check

Prerequisites: Windows Python 3, Visual Studio 2022 MSVC **x86 target** and Windows
SDK with the C++17 standard library and x86 static runtime libraries. Run from the
repository/worktree root in an x86 developer command prompt:

```bat
python _re\harness\campaign_contracts\run.py --compiler msvc
```

Alternatively provide the local environment setup batch explicitly:

```bat
python _re\harness\campaign_contracts\run.py --compiler msvc --vcvars C:\toolchain\vcvars32.bat
```

`HD2_VCVARS32` supplies the same default setup path. A Hostx64 compiler targeting
x86 is supported; x64-target builds fail the fixture's `_M_IX86`/pointer assertions.
The runner uses a private, newly created `build/campaign_contracts/msvc-*` directory
per invocation. `--output-root` can select another private output root. It records
`environment.json`, exact argv in `commands.json`, generated setup/compile batch
files, and each step's diagnostic log. It never uses the legacy game's fixed temp
output paths. No global environment or toolchain files are modified.

The actual target compilation is equivalent to the following in that output
directory (replace the source paths with absolute paths in your checkout):

```bat
cl /nologo /std:c++17 /EHsc /W4 /WX /permissive- /Zc:__cplusplus /arch:IA32 /MT /O2 /Bv /c <root>\_re\harness\campaign_contracts\header_only.cpp /Foheader.obj
cl /nologo /std:c++17 /EHsc /W4 /WX /permissive- /Zc:__cplusplus /arch:IA32 /MT /O2 <root>\_re\harness\campaign_contracts\conformance.cpp /Foconformance.obj /Feconformance.exe /link /MACHINE:X86 /SUBSYSTEM:CONSOLE /INCREMENTAL:NO
conformance.exe
```

The runner also compiles `negative.cpp` six times with the same compile flags plus
`/c /DCAMPAIGN_NEGATIVE_CASE=N /Fonegative-N.obj` for `N=1..6`. Each must fail.
A passing run exits zero and reports standalone header, fake-interface execution,
and six rejected misuse cases. Inspect diagnostics to confirm each intended error.
Compilation alone is not the entire check: the fixture executable must also exit
zero and print `campaign contracts: identity, DTO, fake interface and immutable lifetime checks passed`.

## Portable MSVC/Wine reproduction in this workspace

COOP-001 installed local MSVC 19.44.35230 (toolset directory `14.44.35207`), SDK
`10.0.26100.0`, and Windows Python `3.13.9`. They are local prerequisites, not part
of this branch. From `.worktrees/COOP-011` with a working X display (the shared
baseline display was `:99`), this exact command was run:

```bash
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-001\build\tooling\run-python.bat' 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-011\_re\harness\campaign_contracts\run.py' --compiler msvc
```

That wrapper launches the local Windows Python and supplies the existing
`COOP-001/build/tooling/vcvars-x86.bat` via `HD2_VCVARS32`. Use a separate Wine prefix
and fixture outputs as above; do not modify the shared tooling or baseline prefix.
Wine was `11.21 (Staging)`. Display availability is an environment prerequisite,
not a test of native preparation rendering.

## Supplementary native check

```bash
python3 _re/harness/campaign_contracts/run.py --compiler gcc
```

Requires native `g++` supporting C++17. Uses `-std=c++17 -Wall -Wextra -Werror
-pedantic`, runs the same fixture and six negative cases, and records separate
`gcc-*` outputs. This does **not** replace the MSVC x86 check.

## API and construction notes

- All generated identities/revisions are separate explicit value types; zero is
  invalid. `std::optional` represents absence, not a legitimate zero player/soldier.
  Catalog keys own distinct tagged strings; empty is invalid. Models/codecs must
  still validate catalog membership, scope, finite values, bounds and phase.
- `ActorId` and `ActorSlotUse` are separate coordinates for which zero is valid.
  An optional `ActorRef` carries session and mission generation plus both coordinates.
  It cannot convert to `SoldierId` or `PlayerId`. `S_CampaignControlToken` adds the
  owner, soldier and control revision. Actor reuse does not imply freshness.
- `PreparationItemId` and `StockEntryId` identify entries/variants in a preparation
  catalog/revision. All item proposals are fenced by preparation generation and
  expected revision; they are not world-item UIDs. Bulk operations carry every
  explicit target/control revision; a single-soldier operation names its sole target.
- Published snapshots are detached owning `shared_ptr<const ...>` values with
  nested owned data. Publish with `make_shared<const T>(value)` or move a private
  candidate; retain no mutable aliases. A handle remains valid after replacement
  or screen/adapter destruction. Never cast const away or alias live model storage.
- Snapshot defaults intentionally describe unavailable/invalid state. Only a complete,
  validated publication with `Current` status is usable. Readiness uses preparation
  revision; view revision also sequences readiness/presence/results. Results identify
  full request context/family/generations and either full current state or resync.
  A newer result's snapshot reference is not evidence it was installed.
- Submission returns a local queue token or failure, **not** acceptance. Match token
  plus full request context/family/generation. Request IDs increase across families
  per session/connection. Duplicate results carry the original outcome, never a
  second commit. Poll on the UI thread; stop polling/release attachment on close.
  Reopening reads authoritative state; it never undoes a pending equipment proposal.
- Status events include session/generations/view baseline and are invalidation hints.
  Re-read `GetSnapshot`; stale Loading/Closed events must not overwrite a newer view.
  Loading/Resyncing/Disconnected may have no usable snapshot. The screen cannot use
  an old retained handle to re-enable mutations.
- Capabilities remove rights according to phase/mission rules. Host without a soldier
  can edit eligible AI; another connected human's loadout remains read-only. Unknown
  policies stay `Unresolved` and produce explicit `PolicyRequired` blockers. The
  fixture does not choose a deployment quorum or other shipping product policy.
- Equipment catalog injection provides display/constraint references; ownership and
  policy injection expose immutable values. Implementations own their validators and
  policy evaluation. Fractional skill values are preserved (`soldier.h`/`AddStat`);
  display rounding must not change authoritative profile data. These DTOs define no
  packing, binary layout, enum width, hash encoding, opcode or network limits.

## Coverage and consumer review

| Consumer | Construction/conformance evidence | Deliberately left to consumer |
| --- | --- | --- |
| COOP-004 | Distinct IDs, context, ownership snapshot, claim/switch/release variants, actor-zero/generation-safe token, immutable injected ownership/policy views | Catalog validation, ownership transitions, effective authority and queue handoff |
| COOP-005 | Explicit target/control revisions; all five edit operations; complete loadouts/stock/essentials/findings; ready/deploy requests; two revision domains | Atomic stock conservation, edit/deploy validators, deduplication and policy evaluation |
| COOP-006 | Typed request/result/resync values; authoritative state references; content descriptor, control tuple, mission manifest and snapshot ack | Negotiation DTO/schema, encoding/limits, deltas, dispatch quarantine and transport sender validation |
| COOP-007 | Seven-method fake, detached immutable lifetimes, four optional slots, host-without-soldier AI rights, scripted/read-only guest, policy blockers, pending correlation, resync/status variants, injected display catalog | Actual native rendering/input, event freshness/application algorithm and live binding |

The fixture records proposals and delivers externally supplied fixture publications
and results. It does not implement production permission, transaction, deduplication,
revision application or lifecycle algorithms. Six compile failures cover player-to-
soldier, actor-to-player and view-to-preparation substitution, nested stock/loadout
mutation through a snapshot, and passing an ownership request to `SubmitEdit`.
Pairwise type assertions additionally cover all tagged IDs/keys. The fixture checks
fractional skills and retained snapshots across replacement/destruction.

On 2026-11-18 the coordinator reviewed COOP-004/005/006 compatibility and the
COOP-003 owner reviewed COOP-007 compatibility. Both passed. UI review identified
and resolved fractional skill loss, status-event freshness ambiguity, and reused
sample request IDs. Exact evidence and reviewer sessions are in the task record.
No runtime host/guest test is required or claimed for this declaration component.
