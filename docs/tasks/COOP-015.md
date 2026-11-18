# COOP 015 Campaign debrief and next mission

## Coordination

- Status: done
- Owner/session: Codex COOP-015, user assignment, thread `01a1eead-4d91-7b52-a8d5-cd1a32cce6d3`
- Canonical branch: `feature/coop-015-campaign-progression`
- Base commit: `ca5ae2f897524cc1905e343b5b7ad3910e56f4c3`
- Dependencies: [COOP-013](COOP-013.md) menu preparation/deployment, [COOP-014](COOP-014.md) authoritative mission outcome
- Last source checkpoint: `b1a92949`
- Checkpoint location: canonical branch, `.worktrees/COOP-015`; claim acknowledged by coordinator at `16963b69`
- Review/integration: PR #31 merged at `09b498a3895f2bd9675b346e82cbec91914bd2b7`. The integrated tree matches reviewed head `c0b22379`.
- Pull request: [#31](https://github.com/xcntroller/HD2_BaseCoop/pull/31)

## Branch authority and completion PR

Follow the [task branch and completion PR workflow](../agent_workflow.md#review-and-integration).
Give reproducible test steps, state the delivery type and record the evidence actually
obtained before requesting review.

## Result and scope

Keep the session alive from a completed Arctic1 mission through the original
debrief and campaign sequence into the next playable mission. Commit campaign
results once on the host and reuse live preparation only at the original equipment
steps. Preserve campaign roster identity when actors and ownership bindings change.

## Files and shared contracts

Own campaign step advancement in `src/game/game_app.{h,cpp}`, campaign state writeback
and debrief routing, and the transition hooks in the mission lifecycle and ownership
adapters. Use the terminal result from COOP-014 and the preparation lifecycle from
COOP-013. Keep disk save/resume and mid-mission admission outside this scope.

## Technical acceptance criteria

- Launch a host/guest session through the game menu and establish an Arctic1 terminal
  result using a reproducible checkpoint or controlled test state. Both peers see
  the original debrief result and reach the original next campaign step in the same
  session. Enter the following playable mission through the original intervening
  steps, including live preparation where required. Label the terminal setup and
  the production transition it exercises.
- The [after-action report](../base_game_coop.md#player-flow) shows all connected
  humans' readiness, including spectators. The host can advance only when all are
  ready for that report. Verify a late Ready, a departure while waiting, and
  repeated advancement requests against the same report.
- Compare squad stats, deaths, inventory and progression against original campaign
  rules. The host commits the result once and guests do not write solo progress.
  A failure follows the original failure path without committing victory.
- Advance the mission generation, invalidate prior input and actor bindings, and
  require a new loading barrier before either player can act. Test a departure
  during the transition and a player with no eligible soldier under the agreed
  policy. Verify the corresponding solo flow and native LAN map change.

## Owner playtest

**Pending owner playtest.** Build the production x86 executable with
`python _re/tools/build_isolated.py game --all --copy <runtime>` using the
[build baseline](../build_baseline.md). Use Hidden & Dangerous 2 Sabre Squadron
1.12.0 and the matching port DLLs. Start `HD2_Port.exe -window` on both peers
without any debug environment variables. Select **Single Player → Co-op
Campaign**, host on one instance, and join its IPv4 address on the other.
Claim separate soldiers, ready and deploy. Use the
[original Arctic1 route](COOP-014.md#owner-playtest) to reach the report.

The original report should show the same objectives, soldier details and
readiness on both peers. Ready the host first and check that Continue stays
disabled until the guest also readies. Select Continue on the host. Watch the
original Arctic2 briefing and press Space at its end. Expect the field
equipment screen and one eligible soldier. Ready the guest as a spectator,
ready and deploy from the host, then wait for both peers to load the original
Arctic2 opening scene. The assigned player controls the soldier after the
scene. The other player follows the view without gaining control. Both peers
remain in the same LAN session.

## Validation evidence

- **Build.** MSVC 14.44.35207 x86 and Windows SDK 10.0.26100.0 under Wine,
  using separate `build/isolated` roots for production and checked sources.
  The full production build compiled 292 units with zero diagnostics. The
  incremental production and checked builds at `b1a92949` linked with zero
  compiler diagnostics. See `build/evidence/build-msvc-r3.log`,
  `build/evidence/build-msvc-r9.log` and `build/evidence/build-checked-r9.log`.
- **Simulated.** Protocol conformance passed 13 suites and 567,598 checks on
  MSVC x86 and GCC with ASan/UBSan. GCC ownership, mission lifecycle,
  preparation authority and original-rule parity, and preparation UI harnesses
  passed. Commands and output are in `build/evidence/` with `protocol-`,
  `ownership-`, `mission-` and `preparation-` log prefixes.
- **Debug-assisted Arctic1 to Arctic2.** The game menu launched both peers.
  The checked host used the wire and casualty fixture and completed the
  terminal conditions through the native objective handlers. The original
  report appeared on both peers with the same objectives, gains and survivor
  presentation. An early host advancement did not commit. The late guest
  Ready opened the gate. Two advancement requests produced one commit and
  three surviving roster records. The same session showed the original
  Arctic2 briefing and field equipment. The guest readied as a spectator.
  In generation 2 the previous input token returned WrongGeneration, host
  simulation stayed at mission time zero until the guest acknowledged
  snapshot 2, and both peers matched the 29,833-byte world with CRC32
  4,207,940,800. The guest received the Arctic2 startup and its cutscene
  after the host sent the 68-event presentation burst in bounded batches.
  A repeat on the final source skipped the next opening scene through normal
  host input and recorded 600 cm of matching soldier movement on both peers.
  Evidence is in `build/evidence/final-success/` and
  `build/evidence/source-final-movement/` with screenshots and driver logs.
- **Campaign writeback.** The host reuses the native debrief result and
  progress functions. In the checked success, the report recorded soldier 4
  as dead, Continue removed the casualty, and the Arctic2 field stock
  included the returned helmet and uniform. The lead kept surviving gear
  and received the required camera if needed. The paired test copies'
  `profile.sav` and `Options.bin` matched their baseline copies byte for
  byte. Full solo disk saves stay with the original solo route.
- **Debug-assisted solo and native LAN.** The solo probe opened original
  Arctic1 inventory, set terminal objectives and called the native fade and
  report path. The ordinary report showed Save and Continue and wrote
  `CAM_0001.sav`. The original Arctic2 briefing and field inventory
  followed, including the original one-soldier selection prompt. Host and
  guest in a native LAN session both loaded Sicily2
  and then Sicily1 following the original map-change command. Captures
  and logs are under `build/evidence/solo-regression/` and
  `build/evidence/native-map/`.
- **Debug-assisted failure and departures.** Three menu clients reached
  the failed Arctic1 report after the checked host induced the guide's native
  death. The second guest departed while the host was waiting. The report
  barrier removed that guest and accepted the first guest's late Ready. The
  host retried Arctic1 without a victory commit. The remaining guest
  disconnected before acknowledging generation 2. The host aborted this
  barrier at mission time zero, reopened preparation in the same session,
  and started Arctic1 on generation 3. No participant acknowledged or
  started generation 2. Logs are in `build/evidence/failure-departures/`.
- **Genuine unmodified gameplay.** The full owner route remains pending.
  The evidence above covers the transition from controlled terminal states.

## PR test instructions and delivery

**Source-only** integrated gameplay. Build with the isolated x86 command above
and follow the [checked probe instructions](../../_re/harness/campaign_arctic1/README.md#campaign-progression-checks)
for the paired, solo and LAN cases. The owner playtest uses the production
executable. Checked fixtures are installed into a detached test checkout.

## Handoff

- Completed: published the claim and implemented mission-scoped report readiness,
  host writeback, synchronized original step dispatch and continued live preparation.
  The x86 builds, protocol and model harnesses, and scoped success and legacy runs pass.
- Remaining: the full owner playtest.
- Next action: run the pending owner playtest. COOP-016 can begin against the
  integrated mission-generation and loading boundary.
- Blockers: none. COOP-013 merged at `73d518a5` and COOP-014 merged at
  `619d6aec`. The host result is available through `GetCampaignMissionResult()`
  while the session is in Ending.
- Decisions: Arctic2 uses the original one-soldier limit. Preserve the host's
  surviving assignment if possible, otherwise choose the first surviving squad
  soldier in roster order and retain that soldier's owner. Additional players
  can ready up as spectators. A departure invalidates a frozen or loading
  world cut. The remaining players return to preparation in the same session.
- Local state: ignored runtime and evidence under `build/`, using private game copies and Wine prefixes.
- Review notes: checked the final diff and merge-tree rehearsal against
  `ca5ae2f8`, report/session fences and one-time commit, host-only writeback,
  spectator ownership, world acknowledgments, departure recovery, native
  regression paths, and recorded checks. The owner route remains pending.
