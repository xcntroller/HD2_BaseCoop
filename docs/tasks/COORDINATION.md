# Cooperative campaign coordination ledger

The live copy of this ledger belongs to `coord/coop-work-queue`. The copy on `main` is
the initial template. See the [workflow](../agent_workflow.md#find-the-coordinator) before
creating or updating the operational branch. Do not merge its WIP ledger into `main`.

## Coordinator

- Owner/session: handoff available; Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe` explicitly STOPS ledger writes after publishing this checkpoint (2026-11-18).
- Contact/shared channel: this ledger and active COOP-006/007 threads below. COOP-004 declined ongoing coordination after completing its goal; COOP-006 indicated willingness to accept after this stopped-writer handoff, but no successor is recorded until they update the ledger.
- Integrator: transferable. No unreviewed COOP-006/007 integration is approved by this handoff.
- Latest checkpoint: `HEAD` identifies this final update. Ordinary push to `origin/coord/coop-work-queue`; never merge this branch wholesale into main.
- Next action: successor explicitly accepts this stopped-writer handoff, records identity and notifies COOP-006/007. Review remaining candidates and preserve their active scopes/tooling.

## Accepted task claims

| Task | Owner/session | Task branch | Status | Accessible checkpoint or PR | Next action |
| --- | --- | --- | --- | --- | --- |
| COOP-003 | Codex COOP-003 session (2026-11-18); explicitly assigned by user | `docs/coop-003-preparation-ui` | done | PR #3 merge `dd2aa98e`; completion metadata PR #5 `9aec1d7e` | Documentation/UI design complete; native rendering/input implementation remains COOP-007. |
| COOP-002 | Codex COOP-002 / thread `01a1ebd2-902d-7e32-9876-9818a845d361`, user assignment | `feature/coop-002-contracts` | done | PR #4 merge `9f59ff47`; completion metadata PR #5 `9aec1d7e` | Documentation contracts integrated; dependent declarations also integrated. |
| COOP-011 | Codex COOP-011 / thread `01a1ebe7-2361-7503-8dcb-6e0739646604`, user assignment | `feature/coop-011-shared-declarations` | done | PR #6 merge `0f367ed6`; metadata PR #7 `4d6f8cc1` | Shared declarations integrated; final coordinator target/native conformance passed. |
| COOP-004 | Codex COOP-004 / thread `01a1ebf9-60c7-74d0-9d39-cd09295875b0`, user assignment | `feature/coop-004-ownership` | done | PR #9 merge `59a480c8`; done metadata PR #14 `b72b6f63`; `.worktrees/COOP-004` | `campaign_session.{h,cpp}`, `campaign_ownership.{h,cpp}`, ownership harness and own record. Engine-independent; no shared header/legacy edits. |
| COOP-005 | Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, user assignment | `feature/coop-005-preparation-model` | done | PR #8 merge `06d999d7`; done metadata PR #15 `5f561496`; `.worktrees/COOP-005` | Atomic preparation model/harness/own record; audit actual legacy rules. Native rule parity remains acceptance, injected generic fake does not satisfy it. |
| COOP-006 | Codex COOP-006 / thread `01a1ec01-a68f-7170-9f75-7536a079675b`, user assignment | `feature/coop-006-protocol` | active; claim acknowledged by this commit | PR #13 candidate `ab192a56` (source `08a2f5e2`); further semantic refinements active; `.worktrees/COOP-006` | `src/net/campaign_protocol.{h,cpp}`, protocol harness and own record. Native dispatch audit read-only; no shared header or live hooks. |
| COOP-007 | Codex COOP-007 / thread `01a1ec02-1d0b-7791-8a57-e16ee680b905`, user assignment | `feature/coop-007-preparation-screen` | active; claim acknowledged by this commit | Checkpoint `be6aea95`; draft PR #12; `.worktrees/COOP-007` | Native `campaign_preparation_screen.{h,cpp}`, UI harness and own record. Consume merged header/layout; no legacy panel edits or production entry. Required native rendering/input evidence remains gate. |
| COOP-001 | Codex / 2026-11-18 user assignment | `feature/coop-001-build-baseline` | done | PR #2 merge `15b572ff`; completion metadata PR #11 merge `1b9b307f` | Build/solo/native host-guest baseline and isolation accepted; guide/evidence integrated. Firing/reload/completion remain unverified downstream gates. |

Only the coordinator adds or reassigns claims. A missing row is not sufficient evidence
of availability: check worktrees and existing task branches as well. Task records carry
detailed progress; this ledger locates their active owners and checkpoints.

## Review and integration queue

1. COOP-002 / PR #4, reviewed head `2ee1dca6`: coordinator read full contract, task/index/COOP-011 refinements, source coupling/rights/generation boundaries; reproduced two document check blocks (11 Markdown files, 68 local links/anchors; 11 task branch/dependency records; acyclic) and whitespace check. COOP-003 consumer review passed. Documentation-only acceptance satisfied; no compiled/runtime claims. Merged `9f59ff47`; actual done record in completion PR #5 `9aec1d7e`.
2. COOP-003 / PR #3, reviewed head `b4030c11`: coordinator read specification/resource audit/fixture/driver and latest prose-only corrections, checked source symbols, inspected generated host preview, reproduced browser smoke (all scenarios, 3 widths, zero page errors) and whitespace checks. COOP-002 independent full review passed. Documentation-only design acceptance satisfied; native rendering/input/network evidence remains downstream. Merged `dd2aa98e`; actual done record in completion PR #5 `9aec1d7e`.
3. COOP-001 / PR #2: COOP-004 independent review of `747d0590` passed, reproduced three process tests and audited all tooling/docs/evidence. Complete x86/x87 build, partial Tutorial and actual native two-client Sicily2 movement/inventory/status baseline; actual MSVC concurrent worktree checks passed. Firing/reload unconfirmed, no mission-completion claim. Merged `15b572ff`; independently reviewed done/blocker metadata PR #11 merged `1b9b307f`.
4. COOP-004 / PR #9: independent COOP-005 full source/API/journal review PASS after phase-baseline fix; reproduced final 9 suites / 10654 native checks (`gcc-8df2if3k`), inspected corrected target logs (`msvc-f4oonfu6`, same PASS). Final doc-only head `3fb31bbd` checked; merged `59a480c8`. Actual-done metadata PR #14 independently reviewed including corrected final `2d26ed2b`; merged `b72b6f63`.
5. COOP-005 / PR #8: source `eda10d0a` independently reviewed by COOP-004 PASS, reproduced native header/behavior/810 capacity + 240 essentials differential and concrete transactions (`gcc-udbjthdm`), inspected refreshed target logs (`msvc-t3_i3i0o`, all PASS). Final source unchanged through main synchronization `4798c022`; full 10-file diff/whitespace check passed. Merged `06d999d7`; COOP-004 independently reviewed done/dependency metadata PR #15 head `080ee5ea`, merged `5f561496`. Actual ownership + preparation + shared journal/freeze callback supplementary native composition experiment passed. No connected screen/gameplay claim.
6. COOP-006 / PR #13: candidate `ab192a56` (code `08a2f5e2`) reports GCC/MSVC x86/ASan+UBSan 8 suites / 541790 checks PASS. Worker is refining actor session/generation and result/snapshot baseline consistency after its own semantic review; inspect latest canonical branch/handoff before reviewing. No independent full implementation review or integration approval yet. COOP-004 declined this additional full review; ask available peer or perform workflow separate review pass. Numeric opcode/live hooks remain uninstalled.
7. COOP-007 / draft PR #12 head `be6aea95`: presenter/behavior native and MSVC x86 PASS; standalone native screen rendering/input work in progress. Required visual/input/resource gates remain unmet; no integration approval. Worker owns restored compiler/SDK/Python and native libraries; preserve its ignored tooling.

## Coordinator handoff

- Completed/integrated: COOP-004 `59a480c8` (done metadata `b72b6f63`); COOP-005 `06d999d7` (done metadata `5f561496`); COOP-001 `15b572ff` (done metadata `1b9b307f`); COOP-002 `9f59ff47`; COOP-003 `dd2aa98e`; their done metadata `9aec1d7e`; COOP-011 `0f367ed6` (done/readiness `4d6f8cc1`). Optional PR-label cleanup integrated `dbc9a85a`. All historical branches/worktrees and checkpoints preserved.
- Active task writers: COOP-006/007 retain acknowledged canonical branches/scopes. COOP-004/005 implementation and completion metadata are integrated; those workers stop after their completed goals. Earlier 001/002/003/011 stopped after completion. No branch reassignment is made by this handoff.
- Pending review/merge order: COOP-004/005 are done. COOP-006 needs final candidate/refinement review; COOP-007 needs its native acceptance evidence. COOP-008 and COOP-009 remain planned, each waiting only for COOP-006 after 001/004/005 merges; their blocker lines were updated in reviewed completion PRs. COOP-010 retains runtime gates. No unclaimed ready implementation task currently recorded.
- Shared contracts: merged campaign header remains a compatibility boundary. 004 owns concrete cross-family request journal; 005 consumes an agreed injectable boundary while developing independently. Production has one authority queue/journal. 006 codec/opcode/native dispatch audit does not attach live hooks; proposed positive opcode `0x434f4f50` requires collision/consumer audit. 007 must establish native engine rendering/input evidence; HTML/presenter-only evidence is insufficient. File-scope expansion must be coordinated with affected owners before edits.
- Runtime resources correction: COOP-001 worktree and ignored tooling/artifacts disappeared externally after its passing checks. Its owner confirms they did not remove/move them. Historical evidence remains accurate but those paths are unavailable. COOP-007 is reprovisioning a private pinned compiler/SDK/Python and later native libraries; other workers should avoid duplicate downloads and preserve its tooling. COOP-005 retained historical MSVC output logs in its own ignored build directory. Xvfb :99/:100 remain available. Exact setup is in merged `docs/build_baseline.md`.
- Transfer acknowledgment: COOP-001 stopped at `46f3a301`; COOP-005 accepted at `85f645b9`, reviewed/integrated 004/005 and their completion metadata, and now explicitly stops after this published checkpoint. Successor must accept before writing the ledger; active worker scopes and resources remain preserved.

Keep this record and task checkpoints accessible to successors. When moving between
machines, a local-only ledger or unpublished task branch must be transferred explicitly.

## Cross-task request journal boundary

COOP-004 owns a concrete bounded cross-family request journal in its reserved campaign_session files. COOP-005 may inject a narrow Begin/Finish journal boundary and use a behavioral fake while developing independently; no duplicate shared implementation/header and no dependency on an unmerged COOP-004 branch. Agree exact identity/content/result/expiry semantics before consumers bind. Production composition must use one shared authority queue and cross-family journal. Injection does not waive COOP-005 actual campaign equipment-rule/stock-conversion acceptance; keep the PR draft until its stated evidence is met or split a bounded foundation explicitly.

## Result metadata clarification for COOP-006

COOP-004 and COOP-005 agree: result revision fields equal corresponding fields present
in the attached authoritative state. Cached duplicates retain original metadata and
original immutable state together; early rejection carries current metadata/state.
Optional result generations describe the full current state epoch, not request-field
presence (a later preparation includes a valid previous MissionGeneration). Ownership
snapshots have no preparation/view revision fields, so do not invent comparisons for
missing fields. COOP-006 was notified explicitly before this stopped-writer handoff.
