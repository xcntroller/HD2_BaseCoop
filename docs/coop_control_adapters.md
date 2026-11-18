# Dormant campaign controller and input adapters

COOP-008 supplies a tested inactive component. COOP-010 installs admission, tagged
transport dispatch and the initial-world barrier together with COOP-009. No menu
entry, production context installation or playable campaign session is added here.

## Composition and ownership

On one serialized nonreentrant authority queue, construct
`C_CampaignControlAdapter(ownership, sink)`, where `ownership` is the merged
`C_CampaignOwnership` and `sink` is `C_CampaignNativeControlSink` in the engine.
Install the same ownership view with `InstallCampaignControlRuntime(&ownership,
&adapter)` only at the later explicit integration boundary. Uninstall before
teardown. The default global view and adapter are null; legacy session/transport
predicates and serialized layouts keep their meanings.

After committing ownership/binding/script/death changes, call
`SynchronizeCommitted()` at a safe tick boundary. It disables queued input,
drains changed or retired bindings, installs effective controllers and local
presentation, then acknowledges exact committed control revisions. A failed port
leaves input unavailable and can be retried. It does not recreate unchanged
controllers on presence/phase publications, nor restart an active script on a
scripted soldier's disconnect. Script/death callbacks commit invalidation first;
reconciliation must run after the executing controller's stack returns.

On guests, `C_CampaignReplicaControlAdapter(session, localPlayer, sink)` implements
the same ownership-view interface. `InstallCommitted(snapshot)` validates complete
wire-compatible catalogs, rejects obsolete generations/revisions and conflicting
same-revision catalogs, and derives guest transport/local presentation context.
It permits the model's phase-only Loading-to-Running commit. Failed installations
retain a revision floor and require successful handler/presentation installation
before the caller acknowledges world reconstruction or sends input. Guests never
close the host's private handoff barrier or run authoritative squad AI.

`BindCampaignTransportPlayer(slot, incarnation)` accepts current admitted members
only. It never interprets actor IDs as transport slots, and requires retiring an
old incarnation before reusing a slot. `HandleCampaignDisconnect(PlayerId)` commits
ownership release and reconciles without destroying actors, gear, health or mines.
COOP-009 must call it before native departure destruction; COOP-010 must capture
the authenticated incarnation when enqueuing departures, rather than resolving a
possibly reused slot when a delayed event executes.

## Input and engine boundary

`Enqueue(authenticatedSender, S_Input)` validates the full token through ownership
and the protocol gate; sequence numbers are increasing per token. `ApplyNext()`
checks ownership again immediately before dispatch. The bounded queue owns copied
payloads. Transfer, death, script entry and reconstruction invalidate old queued
control bits, actor actions and vehicle actions. A transfer away and back or reuse
of actor ID/slotUse never restores an old control revision/generation.

The native sink checks the committed ActorRef against the physical actor slot-use
counter. The initial native allocator uses the 32 stable player-role actor slots;
those actor numbers are independent of connection slots. Dynamic compacted actor
IDs lack a native incarnation counter and fail closed at this sink. The logical
model/queue supports independent larger actor IDs, without widening legacy fields.
The sink clears held control arrays, input pointers, reliable queues and net-object
messages. Shared incoming/forwarded messages release exactly one queue reference
at a time. Replacement handlers attach after the old controller detaches.

The native sink's dispatcher is supplied by COOP-010: it must decode a negotiated,
bounded payload and validate gameplay targets separately from controller authority.
Inventory destination, range, item ownership, healing target and vehicle seat are
world-actor checks; do not rewrite target actor IDs as PlayerIds. The raw native
net-object control bits/action queues and reliable actor dispatch are quarantined
in campaign mode. Outgoing native controller queues are discarded until tagged
transport equivalents are attached. This intentional dormant boundary cannot be
used to advertise a working network session. Authoritative presentation likewise
needs a generation-tagged replication path at live integration.

## Legacy coupling audit

| Consumers | Campaign adapter behavior |
| --- | --- |
| `human_control.cpp`, `human_body.cpp`, `human_controller.cpp` | Owner and transport role select local/remote player, host squad AI, scripted or replicated control. Body flags use local ownership. Input wrappers refuse stale/non-player sources. Script invalidation commits before replacement. |
| `player_net.cpp`, `remote_controller.cpp` | Connection activity uses the explicit transport mapping; AI has no fake player. Both class-message paths bypass MP skills/ranks. Raw bit/action/reliable queues fail closed. |
| `human_combat.cpp`, `net_players.cpp`, `player_controller.cpp` | Damage/killer identities stay actor IDs; campaign damage never enters MP slot scoring, ranks or lives. Medic scoring and native class changes are disabled. Original campaign friendship and host progression remain distinct from transport role. |
| `human.cpp`, `human_items.cpp` | Setup skips MP team/VIP/spawn-protection paths; campaign names, seed skills/progress and equipment survive control changes. Campaign progression mutates on the authority host. Equipment snapshots avoid MP class state. |
| `team.cpp`, `interface.cpp`, `first_person_view.cpp` | Full squad membership is retained. Selection follows committed local ownership, with no direct claim from a UI click. Handoff closes inventory and refreshes camera/weapon targets; losing ownership clears the old weapon view. Inventory opens only for the local controlled source. |
| `actors_module.cpp` | Untagged dispatch is rejected before actor/player-range dispatch. Removal never indexes the MP player array using a campaign actor. COOP-009 alone owns departure/spawn/lifecycle call sites. |
| `car_seats.cpp`, `car_hits.cpp`, `car_callbacks.cpp`, `car_update.cpp`, `car_net.cpp` | Local-driver checks use ownership under the explicit boundary and exactly the original slot comparisons outside it. Vehicle action envelopes use the same token/sequence fences; full driving/interaction remains a later runtime gate. |

`C_NetGame::GetLocalHuman()` follows the current owned catalog binding. Interface
and first-person consumers also use the team's committed selection. The reserved
`net_game.cpp::GetPlayerHuman` transport hook remains unmodified; COOP-010 must
resolve/authenticate its connection before attaching a live campaign command route.
No new profile fields, engine vtables, wire fields or actor/player-slot equality
assumptions are introduced. The additional controller overload and friend access
add no object fields or virtual slots.

## Evidence and limits

See [the harness guide](../_re/harness/campaign_control/README.md) and
[task handoff](tasks/COOP-008.md). Component tests use the real ownership/protocol
implementations and an instrumented engine port; the native sink is additionally
compiled/executed against instrumented engine interfaces. Existing human control,
player networking and remote-controller suites compare dormant solo/native paths
with the unpacked original executable. These results establish the isolated adapter
and legacy regressions. Actual two-instance campaign movement/fire/reload/inventory,
AI/equipment persistence after disconnect and vehicle/mission completion remain
COOP-010 and later interaction evidence, as required by the task split.
