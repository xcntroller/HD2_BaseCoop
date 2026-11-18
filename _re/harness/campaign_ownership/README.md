# Campaign session and ownership model

COOP-004 implements an engine-independent host component using the merged
[shared declarations](../../../src/game/campaign_contracts.h) and
[logical contracts](../../../docs/coop_contracts.md). **Source-only**: no executable,
game data or toolchain is distributed. The fixture compiles and runs the real model;
it does not connect a menu, transport, game actor or controller. Live adapters and
two-player acceptance remain COOP-008/009/010. See the
[task handoff](../../../docs/tasks/COOP-004.md).

## Reproduce the checks

From the worktree root, using native Python 3 and GCC with C++17:

```bash
python3 _re/harness/campaign_ownership/run.py --compiler gcc
```

For the intended target, use Windows Python 3 in an MSVC x86 developer environment,
with Visual Studio 2022 C++17 headers, x86 static runtime and Windows SDK:

```bat
python _re\harness\campaign_ownership\run.py --compiler msvc
```

`--vcvars C:\toolchain\vcvars32.bat` or `HD2_VCVARS32` can supply an environment
setup batch. The compiler can be Hostx64 targeting x86. The fixture asserts `_M_IX86`
and 32-bit pointers. Target flags are `/std:c++17 /EHsc /W4 /WX /permissive-
/Zc:__cplusplus /arch:IA32 /MT /O2`; link flags include `/MACHINE:X86`. Native
flags are `-std=c++17 -Wall -Wextra -Werror -pedantic`. Both compile these three units:

- `_re/harness/campaign_ownership/model_tests.cpp`
- `src/game/campaign_session.cpp`
- `src/game/campaign_ownership.cpp`

Each invocation creates a unique ignored `build/campaign_ownership/{gcc,msvc}-*`
directory containing exact argv, environment, compile diagnostics and execution log.
`--output-root` selects another private output root. There are no legacy fixed
temporary paths, engine libraries or game-data prerequisites. Expected: exit zero,
all steps `PASS`, executable reports nine suites and its check count. A compiler
success alone does not pass: the executable must run successfully too.

The workspace's existing COOP-001 portable toolchain was also used under Wine,
with a private COOP-004 prefix, without changing the baseline prefix/tooling:

```bash
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all wine64 cmd /c 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-001\build\tooling\run-python.bat' 'Z:\home\control\Documents\Projects\HD2SS\HD2_BaseCoop\.worktrees\COOP-004\_re\harness\campaign_ownership\run.py' --compiler msvc
```

That absolute wrapper/toolchain and a working X display are local prerequisites,
not committed files. MSVC 19.44.35230 x86, toolset 14.44.35207, SDK 10.0.26100.0,
Windows Python 3.13.9 and Wine 11.21 (Staging) were used. Native GCC was 16.2.1.
No full game build, controller handoff, network dispatch or gameplay test is claimed.
After these successful runs, the borrowed COOP-001 worktree/toolchain was removed
externally. Its absolute wrapper command above is historical; for a fresh target run,
reprovision MSVC/SDK/Windows Python using [the build guide](../../../docs/build_baseline.md)
or use the Windows developer environment command. COOP-004 execution logs and its
locally built fixture remain under its own ignored output directories.

## Model API and lifetime

[`C_CampaignSession`](../../../src/game/campaign_session.h) takes a fresh caller-issued
SessionId, campaign/mission/rules and the complete campaign roster's `profileIndex`
values. The constructor rejects missing/negative/duplicate profiles, invalid keys
and empty/oversized catalogs. Zero is a valid profile index. It allocates distinct
SoldierIds and supports both lookup directions; these IDs stay fixed across mission
generations within that catalog. They are neither profile indices nor squad positions.
The roster is bounded at 256 entries. A mission-introduced soldier must be explicitly
included in the validated campaign catalog; unknown actors cannot manufacture IDs.
Persistence/ID encoding and save migration remain later work. Do not reconstruct
a catalog from a differently ordered list and treat it as a saved identity mapping.

The session preserves campaign difficulty/content, rules and separate transport flags.
The listen host has both transport flags; client status alone never grants simulation
authority. Narrow `UsesCampaignRules`/`HasSimulationAuthority` helpers do not alter
legacy `IsLocalGame`/server/client functions or serialized structures.

[`C_CampaignOwnership`](../../../src/game/campaign_ownership.h) owns its session value
and cannot be copied. Supply a long-lived `I_CampaignPolicyView` and a journal with
the same session. Initially the host is admitted as PlayerId 1, every roster soldier
is alive but ineligible/undeployed, and all revisions start at 1. Trusted setup
explicitly installs eligibility/deployment with `SetAvailability`. A configured squad
limit 1..4 bounds deployed soldiers; mission restrictions may remove eligibility.
Here deployed means selected mission-squad membership, including preparation before
actors exist; trusted setup sets selected soldiers eligible/deployed before editing.
`Admit` is called only after transport negotiation; it allocates increasing connection
incarnations and caps active humans at four. Display names are presentation only.
Running admission returns `JoinNotSupported`; no reconnect/world snapshot is implied.

Run every operation on **one serialized, nonreentrant authority queue**, including
preparation commits and policy changes. This model does not create a worker thread
or acquire a transport lock. `ApplyRequest(authenticatedSender, request)` compares
the connection-derived sender with the echo, validates admission/session/phase and
generation before the journal, then validates aggregate/control revisions, policy and
the complete proposal. Failed switches change neither soldier. Accepted changes
publish one ownership revision and rebuild the reverse index atomically. Host requests
use the same path. Missing/ambiguous policy returns `PolicyRequired`; fixtures permit
selection/switching explicitly as proposed test policy, not confirmed product behavior.

`GetSnapshot` returns a detached immutable owning value. A retained result or snapshot
does not change after later commits or model destruction. `FindOwner`, `FindSoldier`
and `ResolveActor` never index legacy arrays. Absence is explicit. Actor zero and
slot/use zero are valid, and bindings must match session/generation. Two soldiers
cannot share a live actor slot even with different use counters.

Trusted disconnect/death/eligibility events clear affected owners and invalidate control;
disconnect preserves script suspension and the current actor. Repeated disconnect/death
is a no-op. Script entry retains the assigned human, script exit uses the **current**
owner/AI, and each change advances control revision. There is no revival operation.
The model cannot mutate health, equipment, skills, position, mines or actors: it holds
only ownership/catalog values, with no engine pointers or effect callbacks.

## Lifecycle and adapter obligations

`StartPreparation` advances its generation, including reopening after Frozen/Loading
abort. `Freeze` is a trusted hook after the preparation owner has validated equipment,
policy, expected revision and its immutable manifest. `BeginMissionGeneration` advances
the session-scoped mission generation, clears every actor binding/script override,
revalidates owner eligibility/presence and advances all control revisions. It is
allowed from Frozen, Running (authorized future restart/load) or Setup (explicit
fixed-manifest development entry). No method restores saved connection slots or lives.
`EndMission` suspends input; `ReturnToSetup` clears world bindings without forcing an
equipment screen. `Close` releases mappings and journal participants and is idempotent.

After any owner/actor/eligibility/effective controller change, input remains gated until
`CompleteControlHandoff({soldier, currentControlRevision})`. The adapter first drains
old input/action queues, replaces controller/net-object handlers and updates local
camera/inventory/selection; then acknowledges that exact revision. An obsolete completion
is rejected. Loading requires every living eligible deployed soldier to be bound and
its controller handoff completed before `StartSimulation` succeeds. The caller must
**also** verify the full initial-world snapshot and required-peer acknowledgment barrier;
this module deliberately does not implement it. Membership changes during Frozen/Loading
advance the catalog revision; the adapter must abort/re-evaluate its manifest/barrier.
A departure is never an acknowledgment. Host-loss policy remains external/unresolved.

`GetControlToken` succeeds only for effective Running input. `ValidateInput` checks the
authenticated connection against the full session/generation/player/soldier/actor/control
tuple and handoff gate. Validate both when enqueueing and again when applying movement
or actions. Switch-away/back and bounded actor slot/use reuse cannot restore old authority.
Every generated counter uses checked advancement; exhaustion fails closed with
`WrongGeneration`, requiring a fresh enclosing session rather than wrapping.

## Cross-family journal boundary

`C_CampaignRequestJournal` is the single production deduplication implementation for
this scope. COOP-005 uses injected Begin/Finish callbacks and a behavioral fixture so
it can compile independently. Later production composition binds those callbacks to
the **same** journal/authority queue used by ownership; four independent family caches
would violate the contract. No shared DTO/header was changed to introduce this seam.

1. Authenticate, check admission, session and relevant generation (and the caller's
   phase fence). Build a bounded, unambiguous full canonical request string. The
   ownership encoding uses tagged decimal fields with the classic locale; no hash,
   binary struct comparison or wire ABI is assumed.
   Populate both optional result generation fields from the full current session epoch
   whenever their counters are valid, consistently across families. A preparation result
   after a previous mission still includes that current mission generation; omitting it
   merely because the request does would falsely classify cross-family reuse as a change
   of generation. The trusted production dispatcher must preserve this normalization.
2. Call `Begin(context, family, canonicalContent, currentAuthoritativeResult)`. An empty
   optional reserves the new sequence; otherwise return the supplied result. It rejects
   in-flight work with `PendingTransaction`; callers may not interleave mutations.
3. Perform semantic validation and atomic commit or rejection, then call
   `Finish(context, family, canonicalContent, result)` exactly once, including semantic
   failures. It verifies matching context/family/generation and accepted/rejected outcome.
   Programmer misuse throws `logic_error`; allocation failures are not recoverable model
   rejections. Journal/policy calls must not reenter or throw during authority processing.

The highest consumed RequestId is shared across families per admitted connection.
Cached identical replay returns `Duplicate`, `m_OriginalOutcome` and the original result
including its immutable records. Same ID/different content or family is `RequestConflict`.
Evicted/lower IDs return `ResyncRequired`/`RequestExpired` with current authoritative state.
Generation checks precede cached replay; old payloads cannot mutate a new world. A duplicate
result never rolls back the live model, and consumers must not install its historical
snapshot over a newer view. Semantic rejection consumes a new sequence too. Authentication,
wrong-generation and malformed bounded-syntax failures before reservation do not consume it.

At most four journal participants, 1..64 cached entries each (default 16), and 4096
canonical bytes per request bound storage. `Admit`/`Forget` are trusted lifecycle hooks;
the allocator must never reuse a PlayerId after Forget. A request reaching the maximum
sequence can replay, but the connection must obtain a fresh incarnation to send more;
it must not wrap to zero. Protocol rate/payload/framing limits belong to COOP-006.

## Behavioral coverage

Nine suites exercise catalog/context validity, exclusive claims/races and atomic switches,
four-human and restricted-squad limits, explicit policy and request replay/eviction,
phase/generation fences, script/death/disconnect, controller handoff/actor reuse,
cross-family sequences and 300 deterministic request interleavings. Rejection cases
assert unchanged catalog revisions; interleavings check both ownership indexes, unique
actors/identities, admission and squad bounds after each operation. Tests retain old
publications and explicitly replay accepted historical results without rolling state back.
Health/gear effects and actual controller queues remain adapter/runtime evidence, not
invented model-test claims.
