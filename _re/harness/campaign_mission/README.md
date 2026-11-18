# Campaign mission lifecycle adapters (COOP-009)

**Source-only component.** No binary, game data, proprietary libraries or toolchain
is distributed. The native hooks compile into the game but are dormant until an
explicit campaign view/adapter and native catalog/hooks are installed. COOP-010 owns
live transport admission, dispatch, world transfer and the first playable run.
Full save/load, reconnect, late join and complete campaign transitions remain later
work. A manifest is not a world snapshot.

## Reproduce component and legacy regressions

From this worktree root, native Python 3 plus GCC C++17:

```bash
python3 _re/harness/campaign_mission/run.py --compiler gcc
python3 _re/harness/campaign_mission/run.py --compiler gcc --sanitize
```

Windows Python 3 in an MSVC x86/Windows SDK developer environment:

```bat
python _re\harness\campaign_mission\run.py --compiler msvc
```

`--vcvars C:\toolchain\vcvars32.bat` or `HD2_VCVARS32` selects an explicit setup.
Each invocation creates private ignored `build/campaign_mission/{gcc,msvc}-*`
outputs, exact commands, compiler/version/environment logs and execution logs.
Expected: both executable suites exit zero and all steps report PASS. MSVC requires
32-bit pointers/_M_IX86, `/W4 /WX /std:c++17 /arch:IA32 /MT`; GCC uses strict warnings.
The sanitizers run both executables, including extracted checked-out source bodies.

The lifecycle executable uses real session/ownership/lifecycle implementation:
frozen-state validation, noncontiguous actors, loading versus simulation, old/wrong
ack tuples, duplicate/missing acks, departure, abort, generations/immutable retained
manifests, failed native reconstruction and binding verification. The second
executable extracts the actual current `OnPlayerLeft`, `ChangeMap`, difficulty,
script-team and native spawn-conversion bodies. Detached services verify legacy
solo/native co-op destruction and settings paths are preserved when dormant, while
campaign departure bypasses actor/mine destruction and map rotation. It also
checks committed equipment/stat conversion, roster/profile identity, stock/seed
isolation, bounded quantities and lossless rejection of unsupported variants.
These are focused regressions, not a claim of real host/guest campaign play.

For a runnable game follow [the baseline guide](../../../docs/build_baseline.md),
including installed Sabre Squadron 1.12 data, matching port engine DLLs and MSVC x86
x87 flags. Build in isolated outputs:

```bat
python _re\tools\build_isolated.py game --all --copy C:\HD2Coop009Test
```

Run the copied game with `HD2_Port.exe -window`; the existing solo and native LAN
flows remain dormant-adapter regressions. Suggested manual checks are the Tutorial
squad/gear/movement and a native Sicily2 host/guest join/departure using the exact
setup in the baseline guide. These suggested checks do not establish activation
of this component. See the task handoff for checks actually performed.

## Authority and initial-world sequence

All model operations, catalog imports, controller reconciliation and publication
run on **one serialized, nonreentrant authority queue**. The adapter is retained
for the whole hosting session so SnapshotIds never reset within a session. Catalog
lifetime and attachment lifetimes must exceed consumers; uninstall before teardown.

1. Preparation validates policy/equipment and freezes ownership on the same queue.
   Pass `GetFrozenManifest()` into `C_CampaignMissionAdapter::Begin`. It checks the
   exact context, rules, generations, rights, quorum and deployed loadout set before
   advancing the mission generation. No new soldier or connection is manufactured.
2. Install the SAME ownership view in mission and COOP-008 control runtime. On the
   host also attach the mission adapter and native catalog. All installation is
   explicit; no production menu, console command or dispatch installs it here.
   Native hooks bind `m_SynchronizeControllers` to COOP-008
   `SynchronizeCommitted`, `m_FindTransportPlayer` to
   `FindCampaignTransportPlayer`, and `m_Disconnected` to
   `HandleCampaignDisconnect(PlayerId)`. A missing/failed host hook fails closed.
3. Start the original campaign mission through the campaign PLAY step (never
   `StartNextMap` or native multiplayer preparation). The host consumes detached
   committed spawn plans at original team-zero points, allocates independent actor
   slots, records actor/use/generation bindings and reconciles controllers. All
   deployed humans and AI remain in `PlayerTeam`, which campaign scripts query.
   No multiplayer zone, spawn packet, rank, warmup, respawn, life or map rotation is
   selected. Stats come from committed campaign skills, not a network player class.
4. **Remain Loading.** Capture/serialize the complete initial world: roster bindings,
   transforms, health, inventories and item UIDs, controllers/AI, vehicles/dynamic
   actors, scripts/objectives and replication baseline. Freeze world mutations
   during capture/transfer or buffer and replay ordered post-baseline changes.
   Native script startup must be complete at this cut. Calling `SealInitialWorld`
   is the trusted integration assertion that these operations succeeded; the
   method validates binding/catalog consistency and publishes an immutable
   manifest. It deliberately does not implement an engine world save format.
5. Send that exact manifest and bounded complete world using COOP-006 transfer
   framing/assembler. CRC is integrity, not authentication. Only negotiated host
   messages enter guest reconstruction; no native spawn/control/round message
   may reach its legacy handler during campaign loading.
6. On a guest, keep one `C_CampaignMissionReconstruction` per session/player. Its
   callback reconstructs ALL world categories and installs the exact ownership
   view/native bindings/controllers while input remains suspended. The hook checks
   installed catalog/rules/actors before exposing `GetAcknowledgment`. A failed
   callback, partial body, missing binding or changed catalog never acknowledges.
   Observed generation/snapshot floors reject delayed older reconstruction. A
   retry uses a fresh host-issued SnapshotId; do not reset the replica to retry.
7. Authenticate each peer and pass its ack to host `Acknowledge`. The full tuple
   must match session, player incarnation, mission/preparation generations,
   snapshot and all baseline revisions. Duplicate exact acks are idempotent;
   a departure is never an ack and changes invalidate the barrier. Abort/reopen
   preparation at a fresh generation rather than lowering the quorum implicitly.
8. `Start` additionally requires ownership's completed controller handoff barrier.
   Only then broadcast its exact `S_CampaignStartSimulation`. The guest calls
   `AcceptStart`, then atomically publishes Running on its installed view; its
   input dispatcher must require that reconstruction/start result in addition to
   COOP-008's token validation at enqueue AND apply. Bytes arriving alone never
   authorize input. Keep a Loading deadline in live dispatch; timeout calls Abort.
9. At safe host ticks, reconciliation processes deferred script/death changes
   before simulation. Capture exact authenticated PlayerId when enqueueing a
   departure; legacy `OnPlayerLeft(connection)` is only an immediate serialized
   bridge and cannot fence a delayed reused native connection by itself. It calls
   the disconnect/handoff hook BEFORE retiring native connection fields, without
   removing squad actors, deployed equipment or mines. COOP-010 must attach the
   incarnation-tagged event path, not rely on a reused slot/connection number.
10. Campaign result callback is host-only; consumers decide authorized next steps
    without map/rank reset. Host Stop iterates roster bindings/profile identities,
    captures all soldier results before committing, and strips world-local UIDs.
    Guests do not write profile progress or destroy the shared preparation squad.
    Loading failures never commit partial campaign results. Persistence policy and
    a network debrief/transition flow are not supplied by this hook.

## Native import contract

`S_CampaignNativeCatalog` maps validated content keys to original item IDs,
placement bits and stat indices, plus detached original soldier metadata keyed by
SoldierId. Bootstrap must import it from the negotiated original campaign content,
not parse numeric IDs from network strings. Seed profile indices must match the
session roster. Seed equipment is discarded; committed items and skills replace it.
The converter is atomic with respect to its output and does not mutate profile stock.
It requires the original nine profile stats, keeps original integer stat conversion,
and preserves seed progress/speech metadata. Inventory vectors follow committed
placement ordering; one-slot items require quantity one and position zero. Counts
respect signed-short native fields and a bounded 4096 expanded items per soldier.

The original preparation record stores only item type/count/UID. Optional live
ammo/rounds/charges/condition variants cannot be represented by that six-byte record
and are rejected explicitly rather than discarded. The production preparation
catalog must therefore expose only representable original preparation items here;
live inventory variants belong in the initial-world inventory reconstruction.
An explicit unsupported-variant rejection is not permission to drop committed
metadata or silently substitute default equipment. Missing uniforms are rejected
before effects; import the original point's uniform into preparation before freeze
when the original mission supplies one. Startup failure aborts the loading attempt;
caller tears down the partial world without profile/stock writeback.
