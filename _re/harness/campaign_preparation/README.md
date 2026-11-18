# Campaign preparation authority

COOP-005 supplies the synchronous, engine-independent authority component in
`src/game/campaign_preparation.{h,cpp}`. **Source-only**: no executable or game data
is distributed, no game/menu/transport entry point changes, and no connected
preparation screen is claimed. The component uses merged COOP-011 values.

## Run

Native supplementary check (Python 3 and C++17 g++):

```sh
python3 _re/harness/campaign_preparation/run.py --compiler gcc
```

Required target check: Windows Python 3, MSVC x86 C++17 and Windows SDK:

```bat
python _re\harness\campaign_preparation\run.py --compiler msvc
```

Use an x86 developer prompt, `--vcvars C:\toolchain\vcvars32.bat`, or
`HD2_VCVARS32`. Flags are `/std:c++17 /EHsc /W4 /WX /permissive-
/Zc:__cplusplus /arch:IA32 /MT /O2`; fixtures assert `_M_IX86` and four-byte
pointers. Native flags are `-std=c++17 -Wall -Wextra -Werror -pedantic`.
Each invocation creates its own ignored `build/campaign_preparation/{gcc,msvc}-*`
directory, with environment, exact commands, generated legacy bodies and diagnostic
logs. The legacy build's fixed temporary directories are never used.

Local portable compiler setup from COOP-007 replacement tooling (the original COOP-001 worktree was removed),
using a private Wine prefix:

```sh
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-007\build\tooling\run-python.bat' 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-005\_re\harness\campaign_preparation\run.py' --compiler msvc
```

Both behavior executables must exit zero. `legacy-execute.log` must report 810
capacity/backpack cases, 240 mission-essential/carrier cases and successful concrete
transactions; `execute.log` reports permissions, atomic stock, journal, validation,
ownership races, readiness and freeze. No game installation is needed for these
component checks. Two-process game acceptance belongs to the later live adapter.

## Authority and adapter composition

Construct `C_CampaignPreparation` from a complete detached preparation value and
injected ownership/rule/policy views. Import **available** stock once, excluding
already equipped items; register zero-count variants for every equipped state that
may be returned. `C_CampaignEquipmentRules::NormalizeSupply` handles original
campaign ammunition supply flags (`weapon -> ammo item`, same quantity), merges
repeated supply entries with checked counts, and creates zero-count catalog stock.
It does not strip equipped items, refill loaded magazines or repeat supply import
on screen reopen. Existing profile/stock objects stay untouched.

Use a single nonreentrant host authority queue for ownership changes, preparation
requests, membership and deployment. The caller authenticates the sender; the model
checks its player echo/admission. Call `SynchronizeOwnership` as part of the same
queue transaction as an ownership/membership change, before publishing either
model. Full ownership catalogs may include soldiers outside the selected squad;
preparation loadouts/presentations contain the selected maximum four. Any changed
ownership revision/presence invalidates ready values and advances preparation/view
revisions exactly once, without returning equipment. Repeated synchronization is a
no-op. Notify rule/catalog/policy changes using `RevalidateRules` on that queue;
otherwise injected dependencies remain immutable. Trusted roster/supply transitions
require a newly normalized preparation context, not a client loadout proposal.

The `S_CampaignPreparationJournal` callbacks bind to COOP-004's ONE
`C_CampaignRequestJournal::Begin/Finish`, shared with ownership dispatch. No concrete
production journal is duplicated here, and no unmerged ownership implementation is
required to compile this task. The isolated fake deliberately tests the boundary.
The exact signatures match `campaign_session.h`. Request content includes family,
full context/epoch/revision, targets/control revisions and all operation parameters
using length-delimited keys; it is bounded comparison data, never a wire format.
New semantic rejections consume RequestId. Cached duplicates return the original
outcome/snapshot; callers must not install an older snapshot over their current
view. Authentication and generation checks happen before journal lookup. Result
optional generation fields describe the complete current epoch (including a valid
previous mission generation during subsequent preparation), consistently across
families. Codecs must apply their own framing/rate/collection bounds first; model
quantity limit 32767 preserves native signed-short counts, default canonical limit
4096 matches the shared journal. Configure collection bounds consistently with the
transport when composing production adapters.

`ApplyEdit`, `SetReady`, `RequestDeploy` synchronously return authoritative results.
The later `I_CampaignPreparation` transport adapter owns local submission tokens,
queues and UI-thread events. Screen close/reopen only detaches/reattaches that
adapter; it neither cancels authority work nor returns any equipment.
`GetSnapshot(viewer)` returns a detached immutable copy with viewer-specific editing
rights; retained snapshots outlive replacement and authority destruction.

Deployment requires an explicit injected quorum/departure policy and no campaign
blockers. The test policy requiring all admitted participants ready is a named
DEVELOPMENT fixture, not a product decision. Ready toggles affect view revision;
loadout/stock/rights/presence/rules changes invalidate ready at preparation revision.
Freeze captures one immutable manifest and closes edits. When composing with
ownership, bind the optional constructor `freezeOwnership` callback to the trusted
ownership `Freeze` operation on the same queue. The callback must publish only the
phase change, preserving exact ownership/control revisions and participant/rights
values. The model verifies that baseline before publishing its frozen cut and
returns an immutable owning manifest handle. A partial/mismatched cross-model
freeze fails closed; lifecycle must abort into a new generation. The lifecycle adapter
assigns the manifest SnapshotId, reconstructs the world and runs its peer barrier;
this component never starts simulation. Membership changes after freeze invalidate
the manifest and require a fresh preparation generation; an old generation never
thaws. A failed synchronization puts the model in Resyncing with no usable manifest.

## Concrete campaign equipment rules

The trusted detached rule catalog contains original item weights/inventory sizes,
allowed containers, backpack capacities, ammo mapping/ranges, uniform/Axis/diving
classification, mission essentials/restrictions and roster strength. The concrete
rule evaluator truncates strength exactly as original `SetProfile`, keeps belt
volume 12 and backpack limits, validates item/placement/state/stack compatibility,
and preserves fractional skills. Individual fixed-slot replacements return the
displaced variant in the same candidate; a too-small backpack replacement rolls
back both its return and stock withdrawal. Returning a backpack explicitly returns
its contents, matching the campaign double-click operation. Counts and variant
state (ammo, rounds, charges, condition) remain conserved by transfers; no generic
item-count conversion replaces the ammo supply mapping.

Required-item deficits are deployment blockers while intermediate removal is
allowed. Uniform/diving requirements cap at selected squad size, only deployed
soldiers count, and the first Axis carrier is retained across required entries as
in the original screen. An unarmed deployed soldier produces an informational
confirmation finding, matching the original Continue prompt rather than banning
ordinary edits. Mission restrictions can remove rights/placements, never broaden
the owner table. Preset and auto-equip recipe keys are explicit injected catalog
choices, operate only on every named target, and share the same transaction path.
The recipe implementation rejects unsupported auto-equip preferences instead of
silently discarding a human choice. It does not claim to run the original screen's
whole-squad `AssignSlots`/`MatchPreset`/`Replenish` orchestration. A game adapter may
supply an audited `I_CampaignEquipmentRules` planner for those algorithms, while
the authority still enforces permissions, revision, bounds and conservation.

## Evidence and limits

`legacy_source.py` extracts the **current** original function bodies at run time
from `profile_squad.cpp` and `equipment_screen.cpp`. The differential executable
compiles those unchanged bodies with minimal detached collaborators, then compares
capacity, occupied-backpack replacement, required-item and Axis-carrier behavior
against the concrete evaluator. It does not retype expected formulas, compile a
whole original UI/engine, or establish native layout compatibility. No legacy
source or generated reference tier is changed. The behavioral suite independently
checks permissions, explicit bulk targets, last-item contention, stale/conflicting/
repeated/expired requests, identity spoofing, candidate rollback, invalid items,
placements/capacity, stack splitting/move, disconnect idempotence, ownership changing
during planning, readiness invalidation, policy blockers, freeze races and counters.

Native integration must populate the detached catalogs from the validated content
and roster, preserve stock variants/normalization once, bind the shared journal and
single authority queue, and attach the later transport/UI adapter. These are
integration gates, not claims of a connected screen or a playable campaign.
