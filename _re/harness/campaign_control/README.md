# COOP-008 controller/input adapter checks

The component composes the merged ownership model and protocol input gate on one
serialized authority thread. No campaign session or live transport hook is enabled.

Run from the repository root with Python and g++:

```
python _re/harness/campaign_control/run.py --compiler gcc
```

For intended x86 target validation, use Windows Python with the x86 MSVC environment
(or `--vcvars` / `HD2_VCVARS32`):

```
python _re/harness/campaign_control/run.py --compiler msvc
```

Each run writes commands, compiler diagnostics and execution results under a fresh
ignored `build/campaign_control/{compiler}-*/` directory. The suite composes real
ownership transitions and codec input validation with an instrumented engine port.
It verifies input at enqueue and apply, all three input kinds, sequence rejection,
failed handoff retries, controller plans on host and guest, scripted suspension,
death, bounded queues, exact-incarnation disconnect and explicit transport mapping.
These checks do not establish actual mission or vehicle gameplay.

`campaign_control_runtime.h` is the dormant native integration seam. Installation
is explicit and non-owning; uninstall before destroying the view/adapter. Host and
guest must consume the same committed catalog; the host adapter additionally closes
the private ownership handoff barrier after engine drain/apply/presentation succeed.
Lifecycle code must call `HandleCampaignDisconnect(PlayerId)` before native removal,
mines cleanup or stats reset; do not reconstruct an incarnation from a reused slot.
The transport must capture that incarnation when queuing departures. Preserve old
bindings long enough to drain their native handlers before world destruction.

`campaign_control_native.h` supplies the engine sink. Its dispatcher callback must
be an audited, bounded decoder installed by COOP-010 after negotiation and world
reconstruction. An input envelope is not a gameplay permission for its target:
validate inventory destinations, healing, interaction range and vehicle seat
ownership independently. The raw native net-object control bits/action queues and
reliable controller routes remain quarantined in campaign mode. COOP-010 must
supply tagged equivalents for outgoing operations and authoritative presentation;
clearing these native queues is deliberately not a playable networking path.

## Native and legacy regressions

`run.py` also compiles the actual `campaign_control_native.cpp` against small
instrumented engine interfaces in `stub/`. It verifies held-input draining,
shared-message reference lifetime, stale physical actor rejection, presentation
refresh and preservation of soldier health/equipment/skills. It composes the actual
sink with the real ownership model to regress unbind-then-drain and verifies that
retained scripts regain their handler contexts. A third executable extracts and
compiles the actual team death/environment methods to check solo, native multiplayer,
campaign host and campaign guest authority, including living AI squad members. It does not substitute
for the real host/guest campaign runtime gate.

The existing human differential suites can be built/run with isolated outputs:

```
python _re/harness/campaign_control/run_legacy.py --suite control \
  --compiler /path/to/i686-w64-mingw32-g++ \
  --original /path/to/hd2_sabresquadron.unpacked.exe \
  --game-dir /path/to/private/reference-dlls --wine-prefix /path/to/private/prefix
```

Repeat with `--suite playernet` and `--suite remote`. Expect `0 failures` and exit
zero. `--cxxflag` can specify local compiler/library search flags; every exact
command and compiler/execution log goes into a unique ignored
`build/campaign_control_legacy/{suite}-*/` directory. Use Linux MinGW x86 and Wine;
the historical Windows batch recipes remain available. `human_globals.cpp` leaves
the campaign boundary explicitly uninstalled for reference comparison. The one-line
standard attribute-order correction allows the historical harness under GCC 16.

Reference prerequisites are local licensed game binaries/DLLs, not repository
artifacts. An isolated reference DLL directory must contain LS3DF, binkw32, rw_data,
vorbisfile, vorbis and ogg. On this system, Wine additionally needed the installed
Steam/Proton game's 32-bit d3d8 copied into that private directory (see the
[baseline guide](../../../docs/build_baseline.md)). Never copy a compiler, game DLL
or reference executable into version control. A run without the DLL prerequisites
can crash while calling unresolved original imports and is not passing evidence.

The tested reference setup reports one unresolved original import; the selected
suites do not call it and complete all comparisons with zero failures. This is
reference-harness evidence, not a claim that the original game was launched.
Optional `run.py --compiler gcc --sanitize` needs working ASan/UBSan runtime libraries.
The initial sanitizer attempt could not link missing system `libasan.so.8.0.0`.
After COOP-009 restored private matching runtime libraries, the complete final
three-executable suite passed ASan/UBSan. Point both `LIBRARY_PATH` and
`LD_LIBRARY_PATH` at the private library directory before invoking the runner;
no system package installation or global environment changes are needed.

Live campaign admission, negotiated payload codecs/dispatch, tagged authoritative
replication, two-instance demonstration and full vehicle/interaction behavior remain
COOP-010 or later gates. See [the adapter audit](../../../docs/coop_control_adapters.md).
