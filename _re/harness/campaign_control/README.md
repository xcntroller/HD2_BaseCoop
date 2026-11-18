# COOP-008 controller/input adapters (work in progress)

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

Still required before review: complete guest committed-snapshot reconciliation,
script/death lifecycle events, native handler/queue/presentation regressions,
actor dispatch/removal and damage/inventory/vehicle audit completion, full build
and solo/native regression evidence. COOP-010 owns actual two-instance campaign
activation and runtime demonstration; no such result is claimed here.
