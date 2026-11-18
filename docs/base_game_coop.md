# Base game cooperative campaign

## Goal and agreed behavior

Add **Co-op Campaign** to the singleplayer mode picker alongside Campaign, Lone Wolf,
and Carnage. Play the base game's campaign with its original squad, missions, scripts,
objectives, equipment, soldier progression, and mission sequence. Other players control
the squad's soldiers.

The host owns the playthrough, runs the simulation, and commits campaign progress. Up to
four humans share the squad, including the host. Unclaimed deployed soldiers use squad AI
when AI is enabled. Guests receive synchronized campaign state. Solo campaign saves and
progress are isolated from co-op playthroughs.

Each human player selects their own soldier's loadout. The host selects their own loadout
and the loadouts of unclaimed soldiers controlled by AI. Hosting does not grant ordinary
editing access to another connected player's loadout.

## Player flow

1. In **Single Player**, select **Co-op Campaign**, then **Join** or **Host**. Join takes
   the host's IP address. A server browser is a future option.
2. Host offers **Resume Playthrough**, **New Playthrough**, and **Edit Playthrough**.
   Resume opens the selected co-op save. New creates a separate playthrough and lets the
   host choose its rules. Edit opens the editable game rules for the selected save.
3. A new playthrough uses the campaign soldier picker. Show each human player's display
   name in the soldier name area and keep the underlying soldier identity available.
   The host determines the legal mission squad, and each player chooses their own
   available soldier. With AI on, the host can fill remaining legal slots with AI
   soldiers. With AI off, leave vacant slots open and let joining players choose their
   own soldier when the campaign permits selection. The host need not reserve a soldier
   for an unknown future guest. Preserve the mission's original squad limits. Connected
   players without an eligible soldier can spectate.
4. Show the original briefing and co-op preparation at the campaign's normal equipment
   steps. Players equip their own soldiers and the host can equip AI soldiers. Freeze
   the committed roster and kits at deployment, then wait for every participating
   client to load before starting play.
5. Play the original mission with human-controlled squad members. Keep the campaign
   objectives, scripts, interactions, and progression. Use the campaign HUD as the
   starting point for co-op presentation. Omit multiplayer match
   timers, hit and kill feeds, rounds, ranks, and map rotation. Provide a co-op
   scoreboard for viewing the squad's mission performance. The scoreboard's fields
   and presentation remain open.
6. The host controls cutscene playback and skipping where the original campaign allows
   it. Guests follow the host's scene transitions. Guest input cannot skip the scene.
   Resolve scripted events exactly once regardless of playback or skip.
7. Show the original after-action report with a readiness control and each player's
   status. Every connected human, including the host and spectators, must be ready
   before the host can advance. Readiness applies to the current report. Departed
   players leave the barrier, and there is no host override. Advance once into the
   original next campaign step.

## Co-op interface design

Use the original campaign's art and period character as a visual reference. Co-op menus
and new HUD elements can use updated layouts, typography, controls, and visual cues where
they make the game easier to read or navigate. Reuse original components where they fit
the task. A co-op screen can rearrange or replace controls that make a shared workflow
hard to follow.

- **Menus and preparation.** Give the player a clear primary action and keep the current
  playthrough, soldier ownership, and readiness easy to find where they are relevant.
  Group related game rules and explain when a setting takes effect. Keep stock conflicts
  and disabled actions next to the control they affect, with a short reason or next step.
- **In-mission HUD.** Give priority to aiming, objectives, subtitles, and interaction
  prompts. Use compact co-op cues for current needs such as a join pause or waiting to
  respawn. Put detailed squad performance in the scoreboard on demand. Check new
  elements alongside campaign overlays at supported resolutions.
- **Readable states.** Use clear labels, legible type, contrast, and visible focus in
  menus. Pair colour with text or a distinct icon for ownership, readiness, and blocking
  states. Check both host and guest views where their actions or waiting states differ.

## Playthrough game rules

Rules belong to the host's co-op save. The following edit boundaries are the initial
recommendation. Edit from the Host menu while the playthrough is idle. Lock roster and
death rules after creation. Apply edits to a fresh mission start or session admission as
indicated, so a mid-mission save resumes with its original simulation rules.

| Rule | Choices | Proposed edit boundary |
| --- | --- | --- |
| Respawn timing | **Timed** after a configurable delay, **Checkpoint** when the squad reaches a designated checkpoint, **Next Mission** at the next legal mission deployment, or **Reinforcement** at the original campaign's reinforcement opportunity. | Choose the mode at creation. Apply timed-delay edits at the next fresh mission start. |
| Permanent soldier death | **On** permanently removes dead soldiers from the playthrough. **Off** keeps a player's chosen soldier assigned through death so that soldier returns at the selected respawn opportunity. | Choose at creation. |
| Retain gear | **On** returns the full carried kit at death with its remaining ammo and condition. **Off** returns the player without that kit. Applies only to Timed and Checkpoint. | Apply at the next fresh mission start. |
| Respawn position | **On Host**, **On Start**, or **On Checkpoint** for in-mission respawns. Next Mission and Reinforcement retain their campaign-defined entry points. | Apply at the next fresh mission start. |
| Squad AI | **On** fills unclaimed deployed slots with AI. **Off** leaves unfilled slots vacant. | Choose at creation. |
| Drop-in joining | **On** allows first-time join requests during a running mission. **Off** limits first-time joins to pre-deployment preparation. Reconnect is handled separately. | Apply when the host next opens the playthrough for a session. |

### Death and returning to play

- With permanent death on, show a new soldier selection screen after death. The player
  can choose a living, available replacement and take control at the next legal entry.
  The host resolves competing claims. The dead soldier stays dead. A player with no available
  replacement waits or spectates. Running out of soldiers is a risk of this rule
  and can end a run under the campaign's failure rules.
- With permanent death off, death keeps the chosen soldier bound to that player.
  The player waits or spectates until that soldier can return under the selected rule.
- Checkpoint respawns use explicit campaign checkpoints, which may be tied to objective
  completion. A checkpoint clears the wait for casualties already waiting when it is
  reached. **On Checkpoint** uses the latest valid checkpoint. Mission start is the
  fallback before the first checkpoint. **On Host** uses a legal position near the
  living host soldier, with the checkpoint or mission start as fallback if the host is
  unavailable or an adjacent spawn is unsuitable. Use the campaign's original start
  point for **On Start**. Defer entry if the current script or world state makes every
  applicable position invalid.
- The host accounts for retained or lost gear as part of the respawn transaction.
  Retaining gear cannot also leave a lootable copy or replenish spent ammunition.
  With gear retention off, normal casualty inventory rules govern the lost kit.
  Next Mission and Reinforcement use the original campaign's equipment and roster
  opportunities. The reinforcement implementation must verify when the original
  campaign offers replacements and how those replacements enter.

### Joining during a mission

After compatibility and admission checks, a new guest synchronizes with the host's
current world before receiving control. They may claim an eligible existing soldier,
such as an AI squadmate with AI on. With AI off, a late guest can take over an eligible
unclaimed soldier already deployed or spectate until the next legal selection opportunity.
Vacant slots do not deploy a new soldier mid-mission. A disconnect preserves the soldier's
health, position, and gear. With AI on, the soldier returns to AI. With AI off, the soldier
remains in the world without autonomous squad control. A returning player may reclaim
their eligible soldier after synchronization.

Start with a host-controlled freeze during the admitted guest's synchronization. Show
joining status to the squad, and release the barrier on success, cancellation, disconnect,
or timeout. A compatibility check or rejected request does not pause play. Continuous
play is acceptable if host/guest verification shows that snapshot catch-up converges
without the freeze and the joining player cannot act on stale state.

### Decisions for later review

- Campaign rules to test and tune first include the initial settings for new saves, the
  timer default and bounds, and checkpoint placement and behavior in constrained missions.
  Validate the outcome when the whole squad is down and a timed respawn is pending.
- Verify whether an original reinforcement opportunity can occur inside a mission,
  and audit how scripted transitions interact with each respawn mode.
- Possible future rules include friendly-fire handling and whether the host can use
  tactical pause. Keep existing campaign behavior until those rules are specified.
- The existing prototype assumptions for host pause, synchronized save/load, host loss,
  and mod compatibility remain applicable. The host controls shared pause and load,
  losing the host ends the session, and peers must pass compatibility checks.

## Existing code and required changes

| Area | Existing behavior | Required campaign co-op behavior |
| --- | --- | --- |
| [Mode picker](../src/game/main_menu.cpp), `SINGLE_MODES`, `OnSingleplayerButton` | Mode controls select campaign rules and clear the network session type. | Add the mode's control and assets, then host/join setup that preserves the campaign route. Audit the HXML/model resources used to build this screen. |
| [Campaign flow](../src/game/game_app.cpp), `StartStep`, `StartNextMap` | Campaign steps and multiplayer map changes use different routes; `StartNextMap` calls `SetupMultiplayerMission`. | Keep the campaign step sequence. Coordinate mission loading and transitions with all guests. |
| [Settings](../src/game/profile.cpp), `IsCooperativeGame`, `GetDifficulty` | Cooperative detection comes from multiplayer settings. Difficulty also branches on local/network play. | Select campaign difficulty and rules independently from the transport role. |
| [Mission lifecycle](../src/game/game.cpp), `C_game::Start`, `Stop` | Only a local game spawns the chosen campaign squad and writes soldier state back to the campaign at exit. | The host performs those campaign operations in a network session. Guests cannot independently commit campaign progress. |
| [Spawn points](../src/game/spawn_point.cpp), `Spawn`, `CreateSoldier` | Local spawns allocate squad actor slots; cooperative network spawns use multiplayer zones and bind a soldier to a player slot. | Spawn the selected campaign squad at the original campaign points, then bind controllers to those actors. |
| [Controllers](../src/game/human_control.cpp), `AssignPlayerController` | Network player control compares the human's net ID with the local player slot. Local play switches between player and squad controllers. | Resolve the controlling player independently of the soldier's identity; retain squad AI for unclaimed soldiers. |
| [Player input and stats](../src/game/player_net.cpp) | Helpers such as `StampActivity` and `ApplySoldierClass` index network players by the human's net ID. | Resolve the connection that controls the soldier. Campaign skills must not be replaced by multiplayer ranks/classes. |
| [Squad selection](../src/game/team.cpp), `SwitchTo`, `ReturnControl` | A single local active soldier can be swapped with another squad member. | Keep local camera selection separate from shared squad membership and host-authorized control transfers. |
| [Campaign equipment screen](../src/game/equipment_screen.cpp), `C_EquipmentMenu` | One local menu edits the mission squad and profile item stocks, including auto-equipping multiple soldiers. | Build a shared preparation model with per-soldier editing rights. Adapt campaign validation and equipment rules without letting a guest or host auto-equip another player's soldier. |
| [Equipment panel](../src/game/equipment_panel.cpp), `C_EquipmentPanel` | The panel selects and mutates one squad soldier, and renders the soldier and inventory controls. | Assess reuse of campaign visuals and inventory components behind the new UI; send validated edits through the shared preparation model instead of mutating replicated state directly. |
| [Script player team](../src/script/script_vm.cpp), `GetPlayerTeamForMode` | Native co-op scripts query `C_CoopTeam`; local scripts query `C_Team`. | Script queries must see the entire deployed campaign squad, including AI members. Audit other player and squad queries as well. |
| [AI replication](../src/game/ai_controller.cpp), `UpdateNetwork` | AI updates are sent for native cooperative network games. | Reuse the replication path with the host running AI. Verify that squad AI supplies all required controller state. |
| [Joining and leaving](../src/game/game_rounds.cpp), `OnPlayerJoined`, `OnPlayerLeft` | A join uses multiplayer spawning; a departure destroys the player's actor and mines. | Claim an existing eligible soldier on join. Release ownership on departure and preserve the soldier and deployed equipment. |
| [Saving](../src/game/game_save.cpp), [application saves](../src/game/game_app.cpp), [profile saves](../src/game/profile.cpp) | Mission snapshots and campaign records exist, but loading and restarting assume the original local/network split. | Add a host-controlled save/load transaction, client reconstruction, and separate co-op save discovery. |
| [HUD and input](../src/game/interface.cpp), [game menu](../src/game/game_menu.cpp) | Several squad, inventory, tactical-map, and switching features are gated on local play. | Restore campaign controls according to local ownership and host authority. |

The menu is only the entry point. Sending a campaign map through the current multiplayer
session flow would retain multiplayer spawning, rounds, disconnect behavior, and progression.

## Campaign preparation and loadouts

The new preparation screen should expose the selected squad, each soldier's human owner
or AI status, and the local player's equipment editor. The host can switch that editor
between their own soldier and unclaimed AI soldiers. Other players' loadouts may be shown
for coordination, but editing remains subject to ownership. Show the original briefing
and campaign equipment information using the co-op interface guidance above. Keep the
player list focused on ownership and readiness.

Separate the shared preparation state from its controls and rendering. This lets the UI,
equipment rules, and transport adapters develop independently. The existing campaign
equipment panel, soldier preview, item definitions, weight rules, and required-equipment
checks are candidates for reuse. Their current direct changes to global squad and stock
objects need adaptation before simultaneous editors can use them safely.

### Loadout transactions and transfer requests

Keep the host authoritative over the shared campaign stock and committed loadouts. Players
select equipment on their own clients; the host validates and commits those requests, so
host authority does not replace the player's choice. Requests identify the preparation
generation, soldier, editor, and expected state revision. Permission checks use the current
soldier owner, with the host permitted to edit their own and unclaimed soldiers only.

Apply each accepted edit as one transaction across the soldier's loadout and campaign
stock. Concurrent requests for the last available item must not duplicate it or overwrite
another edit. Return accepted state or a specific rejection and current state to the
editor, then replicate committed changes to the other participants. Reject duplicate and
stale requests without applying them again. Preserve the existing campaign limits on
equipment, weight, ammunition, and mandatory mission items.

Claiming or releasing a soldier updates editing rights and invalidates outstanding edits
from the previous owner. A joining player receives the existing loadout and can edit it
under the preparation rules. A disconnect preserves the soldier's equipment. Auto-equip
and other bulk operations use the same permission and transaction path as individual
item edits.

At an equipment step, allow a player to request an item from another human's loadout.
The current owner can accept or decline. Only acceptance can move the item. Commit both
inventory changes atomically after validating ownership, item availability, quantity,
and destination. A stale request or a lost race leaves both kits intact and refreshes
the requester's view. Requests leave shared stock available for ordinary first-accepted
edits. In-mission inventory transfers follow the original gameplay rules.

Preparation readiness refers to a committed revision. Require every deployed human to be
ready for that revision as the initial preparation policy. A relevant loadout or squad
change invalidates readiness before deployment. Once deployment begins, freeze preparation
edits and send the same committed squad state to every client.

### Product decisions still open

- Confirm preparation readiness for spectators and host cancellation behavior when
  someone leaves during deployment. The after-action readiness rule applies separately.
- Set the transfer request UI, including quantity selection, request expiry, and how
  players handle several requests for the same item.

## Separate campaign rules from networking

Retain the meaning of `IsLocalGame()`, `IsServer()`, and `IsClient()`. The host needs a real
network session, and guests must not acquire server authority. Marking campaign co-op as
local globally would bypass network updates and allow the wrong simulation paths to run.

Introduce a session context that distinguishes campaign rules, transport role, and local
player ownership. Ordinary singleplayer, existing multiplayer, and the new cooperative
campaign must remain distinguishable. Keep `PLAY_MODE_CAMPAIGN` as the underlying campaign
rules where possible, with an explicit cooperative session kind. A fourth menu entry need
not become an unchecked fourth value in every legacy play-mode array.

Audit each relevant local/network branch by purpose: simulation authority, campaign rules,
or presentation for the local player. Convert branches deliberately. Existing native co-op
checks also need review: some enable useful replication, while others select multiplayer
lives, scoring, or team semantics.

Co-op saves need their own namespace and resume entry, including quick saves and the
profile's last-save tracking. Keep the host's existing solo campaign saves separate.

## Soldier identity and control

Maintain an explicit host-owned mapping between a connection/player slot and a deployed
campaign soldier. The soldier's roster identity persists through campaign progression;
its actor and replication IDs identify its current incarnation in a mission. Rebuild that
mapping after a load or mission change. Never persist process pointers or treat a temporary
connection slot as a campaign identity.

Enforce these invariants at every control transfer:

1. Each deployed soldier has at most one human owner, and each player controls at most
   one soldier at a time.
2. Unowned eligible soldiers run squad AI on the host when enabled. Guests display
   replicated state for soldiers they do not control.
3. Claims target living, available soldiers in the current mission. The host validates
   requests. Death replacements and pre-deployment roster selection follow their
   separate entry rules.
4. A transfer changes control without respawning, resetting skills, replenishing gear,
   reviving, or clearing persistent mission actions.
5. Scripted control can temporarily override player control. Releasing scripted control
   restores the appropriate owner or AI controller.
6. A load, restart, or mission transition invalidates prior input and ownership messages.
   Use a mission generation and control revision so delayed packets cannot control a
   newly created or reassigned actor.

Controller handoff must update both replication and local presentation: the input source,
network object handlers and queued actions, camera target, first-person weapon view,
inventory target, and squad selection. Reusing `CreateController` alone does not establish
that these responsibilities have been transferred correctly.

## Mission synchronization

The host starts the original campaign mission and supplies its mission identity, difficulty,
selected soldiers, and equipment to guests. A loading barrier keeps the simulation stopped
until participating clients have reconstructed the initial state and acknowledged readiness.

The host evaluates authoritative AI, damage, script conditions, objectives, and campaign
results. Audit the script module's existing network behavior before reusing it: presentation
commands, camera sequences, dialog, and scripted player control require appropriate execution
on each client without duplicating authoritative effects.

Initial mission entry can use the existing spawn/state mechanisms after adapting campaign
spawning. Joining an already running mission additionally needs a complete current-world
snapshot, including script progress, dynamic actors, objectives, inventory, and vehicles.
Treat that as a separate acceptance gate; replaying the original spawn message is insufficient.

Loading a save similarly stops input, advances the mission generation, reconstructs the
host world, resets replication, rebuilds guest worlds, restores eligible ownership, and
waits for acknowledgments before resuming. Connection membership remains live session
state rather than being blindly restored from the save file.

## Implementation milestones

### 1 Campaign session and ownership foundation

Define the explicit session context and soldier ownership model, with tests for competing
claims, invalid soldiers, disconnects, switching, death, and stale requests. Map every path
that assumes actor ID equals player slot, including damage attribution and inventory
requests. Establish a reproducible x86 build and baseline singleplayer/native co-op run.

**Acceptance:** ordinary play still follows its original routes, and ownership transitions
are tested before being connected to live controllers.

The preparation model and UI investigation can run alongside this foundation. Agree on
soldier identity, ownership revisions, and preparation state contracts before connecting
loadout transactions to the network. Deliver UI components and equipment transaction logic
as separate tested features so either can be reassigned without depending on an agent's
private context.

### 2 Two players in one original mission

Use a development launch path for an original base campaign mission with four deployed
soldiers. The host controls one soldier, one guest controls another, and the remaining two
use squad AI. Adapt campaign spawn, controller routing, initial state replication, player
team queries, and campaign mission ticking together. Campaign play in this gate bypasses
multiplayer warmup, round scoring, lives, respawns, and map rotation.

**Acceptance:** both players can move, shoot, reload, use inventory, and observe the same
AI squad and objective progress. Disconnecting the guest returns the same soldier to AI
with its state intact. This gate requires two running game instances.

### 3 Campaign interaction and mission completion

Validate squad orders, soldier switching, healing, item transfers, disguise, doors,
explosives, vehicles, scripted sequences, failure, and victory. Confirm soldier deaths and
stats persist in the host's campaign state. Keep all deployed soldiers visible to mission
queries regardless of human ownership.

**Acceptance:** the test mission completes through its original objectives, and the host
reaches the original debrief and following campaign step with the correct squad state.

### 4 Singleplayer menu and campaign sessions

Add the Co-op Campaign control and required menu resources, host/join setup, soldier claims,
and a four-player limit. Implement the new campaign-style preparation UI. Each player
equips their own soldier, and the host equips their own and unclaimed AI soldiers. Connect
briefing, squad selection, validated loadout transactions, and subsequent missions to the
synchronized campaign flow. Handle missions with fewer available soldiers and leaving a
session at every campaign screen. Reuse campaign presentation components where practical.

**Acceptance:** start and continue a base cooperative campaign entirely through the game's
menus, with one through four connected players and AI filling available empty slots. Two
players can edit their own loadouts concurrently without changing each other's soldiers
or duplicating shared stock. The host can equip AI soldiers, and deployment uses the
same committed loadouts on every machine.

### 5 Persistence and joining during play

Implement isolated campaign saves, quick save/load, restart, reconnect, and a current-world
snapshot for joining during a mission. Exercise loading after deaths, equipment changes,
vehicle use, objective changes, and scripted events.

**Acceptance:** connected players converge after loading and reconnecting, and solo saves
remain intact. A new connection cannot gain control before reconstructing the current world.

### 6 Base campaign verification

Track each base mission through completion with the relevant player counts and special
sequences. Compare campaign outcomes and soldier state with ordinary singleplayer. Include
missions with constrained squads, transitions, scripted control, and vehicle sequences.

**Acceptance:** campaign behavior is demonstrated mission by mission. Loading every map
alone does not establish campaign compatibility.

## Implementation handoff

| Work | Scope and acceptance |
| --- | --- |
| [COOP-013](tasks/COOP-013.md) | Continue live preparation and first deployment with AI on. Apply the soldier ownership and stock rules above. Keep the final soldier-picker presentation and cross-player transfer request UI as follow-ups if the native preparation gate does not already require them. |
| [COOP-014](tasks/COOP-014.md) | Continue Arctic1 interaction and completion. Host-directed scenes and guest skip prevention apply to the cutscenes exercised by this gate. Record additional cutscene coverage for campaign-wide verification. |
| [COOP-015](tasks/COOP-015.md) | Deliver the original debrief with all-player readiness and one authoritative progression commit. |
| [COOP-017](tasks/COOP-017.md) | Deliver isolated co-op save discovery, menu resume, and the save boundary for rule persistence. |
| [COOP-018](tasks/COOP-018.md) and [COOP-019](tasks/COOP-019.md) | Keep reconnect distinct from first-time drop-in. Verify admission policy, synchronized handoff, and recovery from interrupted joins. |

The following work can be scoped as independent follow-up tasks when the corresponding
contracts are available.

| Follow-up | Depends on | Acceptance |
| --- | --- | --- |
| Playthrough menu and rule editing | COOP-012 menu and COOP-017 save lifecycle | Create separate saves, resume a selected save, and edit only the supported rules. Verify that saved rules survive reopening and that live play resumes with its saved mission rules. |
| Soldier picker and AI-off deployment | COOP-013 roster and deployment | Show human names, let guests claim available soldiers, and verify that vacant slots stay vacant with AI off. Cover constrained squads and returning guests. |
| Death, replacement, and campaign respawns | COOP-014 mission state, COOP-015 progression, and the playthrough rule model | Exercise all four triggers, roster exhaustion, legal spawn positions, and retained or lost gear. Verify that valid respawns restore control exactly once and follow campaign completion and failure rules. Include save/load coverage when the pending death or respawn state is persisted. |
| Equipment transfer requests | COOP-013 live transactions | Accept or decline a request and verify exclusive ownership, atomic movement, stale-request handling, and simultaneous acceptance attempts for the same item. |
| Co-op scoreboard | Authoritative mission stats and outcome | Display campaign squad performance consistently on host and guests. Keep ordinary campaign HUD and transitions intact. |

## Future gameplay ideas

These are candidates for prototypes and playtests. The campaign flow above is the
baseline for evaluating them.

### Squad coordination and scouting

- **Squad colours and map pings.** Give each human a consistent colour in the soldier
  picker, tactical map, HUD, and scoreboard. Keep it with the player when they change
  soldiers. Pair it with their name or slot number. Try short-lived, player-coloured
  map pings for **Look here**, **Danger**, and **Regroup**.
- **Binocular marks.** Let a player deliberately mark an enemy in view through
  binoculars and share the sighting with the squad. After losing sight, keep a fading
  last-known position on the tactical map. Try about 20 seconds as a starting point.
  A fresh sighting can update the position. Start with binoculars and test rifle
  scopes later if they offer a distinct use. The
  [campaign map](../src/game/interface_map.cpp) already presents seen enemies.
- **Enemy presence zones.** If individual marks clutter the map, group nearby recent
  observations into approximate red zones. Use a deeper shade for more reported
  enemies and fade the zone as reports age. The overlay represents squad observations.
  Unmarked areas may still contain enemies.

**First playtest.** In a two-player patrol encounter, have one player spot and mark
through binoculars while the other moves using that information. Check whether both
can identify each other's pings and tell when a sighting has gone stale.

### Casualties, stealth, and disguise

- **Bleedout and revive.** Try an optional playthrough rule that gives a downed soldier
  a short rescue window. A teammate spends a first-aid item to revive them wounded.
  If the soldier dies, apply the selected permanent-death and respawn rules. Test what
  happens when the whole squad is down under each respawn mode and which scripted
  deaths go directly to the campaign's death path.
- **Stealth feedback.** First make the existing AI reactions easier to read. Try a pause
  or voice line when a guard investigates, with a brief HUD cue when that change would
  be easy to miss. Test whether players can tell the difference between suspicion and
  confirmed detection while watching the scene.
- **Ranked disguises.** First verify the original disguise and
  [script toggle](../src/script/script_vm.cpp) in co-op. The current
  [disguise code](../src/game/human_combat.cpp) gates its updates on local play.
  Then try ordinary and officer uniforms with ordinary guards and officers in one
  mission. A plausible uniform generally passes casual inspection. Officers build
  suspicion during prolonged close observation. An officer uniform gives more leeway
  with ordinary guards, and suspicious behaviour can expose either uniform.

### Mission variants and network quality

- **Two players in Arctic2.** Prototype a mission-specific variant with two players.
  Keep the original scripted lead role on one soldier and give the second player room
  to help. Audit the [single-soldier deployment restriction](../src/game/equipment_screen.cpp)
  and test objective triggers, lead death, failure, and extraction with both players
  before considering other restricted missions or larger squads.
- **Network quality.** Run this work alongside gameplay. Establish a repeatable
  Arctic1 test under simulated latency, jitter, and packet loss. Measure guest input
  delay and visible movement corrections. Compare doors, inventory, and objectives
  on host and guest. Preserve host authority and use the measurements to decide
  whether to improve remote actor interpolation, local input response, or the
  transport. The [current networking baseline](tasks/COOP-010.md#handoff) records
  DirectPlay and sequenced state updates.

## Validation requirements

The upstream [build instructions](../README.md#building) use MSVC x86 and x87 floating-point
behavior for the runnable executable. `_re/tools/build_check.py` provides a fallback g++
syntax check, which does not replace the runnable build or multiplayer verification.

Use focused native tests for ownership and session logic, existing subsystem harnesses for
changed legacy paths, and actual host/guest play for controller handoff, replication, and
mission behavior. Test loadout permissions, simultaneous requests for scarce equipment,
stale and repeated edits, ownership changes during editing, and readiness invalidation.
Keep a small regression set for original singleplayer and native co-op.
Record host and client logs with mission generation, soldier identity, and control revision
so divergence can be traced to a specific handoff or transition.

Playable validation requires a local game installation with Sabre Squadron 1.12 and the
matching port engine libraries. The source repository supplies neither the game's data
nor evidence that arbitrary base missions already synchronize correctly in native co-op.
