# Cooperative campaign coordination ledger

The live copy of this ledger belongs to `coord/coop-work-queue`. The copy on `main` is
the initial template. See the [workflow](../agent_workflow.md#find-the-coordinator) before
creating or updating the operational branch. Do not merge its WIP ledger into `main`.

## Coordinator

- Owner/session: Codex COOP-007 / thread `01a1ec02-1d0b-7791-8a57-e16ee680b905`, resumed its own stopped handoff `ee8b441f` after COOP-008 explicitly declined coordinator transfer and confirmed STOPPED. Only COOP-007 writes this ledger.
- Contact/shared channel: this published ledger and active COOP-007 conversation; notifying prior coordinator and available reviewers. Existing claims/scopes remain unchanged.
- Integrator: Codex COOP-007, with independent review before integration. No COOP-007 merge approval yet.
- Latest checkpoint: `HEAD`; ordinary push to `origin/coord/coop-work-queue`.
- Next action: finish/review COOP-007 native component evidence. COOP-008/009 have accepted user claims in the reserved scopes below.

## Accepted task claims

| Task | Owner/session | Task branch | Status | Accessible checkpoint or PR | Next action |
| --- | --- | --- | --- | --- | --- |
| COOP-003 | Codex COOP-003 session (2026-11-18); explicitly assigned by user | `docs/coop-003-preparation-ui` | done | PR #3 merge `dd2aa98e`; completion metadata PR #5 `9aec1d7e` | Documentation/UI design complete; native rendering/input implementation remains COOP-007. |
| COOP-002 | Codex COOP-002 / thread `01a1ebd2-902d-7e32-9876-9818a845d361`, user assignment | `feature/coop-002-contracts` | done | PR #4 merge `9f59ff47`; completion metadata PR #5 `9aec1d7e` | Documentation contracts integrated; dependent declarations also integrated. |
| COOP-011 | Codex COOP-011 / thread `01a1ebe7-2361-7503-8dcb-6e0739646604`, user assignment | `feature/coop-011-shared-declarations` | done | PR #6 merge `0f367ed6`; metadata PR #7 `4d6f8cc1` | Shared declarations integrated; final coordinator target/native conformance passed. |
| COOP-004 | Codex COOP-004 / thread `01a1ebf9-60c7-74d0-9d39-cd09295875b0`, user assignment | `feature/coop-004-ownership` | done | PR #9 merge `59a480c8`; done metadata PR #14 `b72b6f63`; `.worktrees/COOP-004` | `campaign_session.{h,cpp}`, `campaign_ownership.{h,cpp}`, ownership harness and own record. Engine-independent; no shared header/legacy edits. |
| COOP-005 | Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, user assignment | `feature/coop-005-preparation-model` | done | PR #8 merge `06d999d7`; done metadata PR #15 `5f561496`; `.worktrees/COOP-005` | Atomic preparation model/harness/own record; audit actual legacy rules. Native rule parity remains acceptance, injected generic fake does not satisfy it. |
| COOP-006 | Codex COOP-006 / thread `01a1ec01-a68f-7170-9f75-7536a079675b`, user assignment | `feature/coop-006-protocol` | done | PR #13 merge `59cb8944`; completion PR #16 merge `0fcea78f`; `.worktrees/COOP-006` | `src/net/campaign_protocol.{h,cpp}`, protocol harness and own record. Native dispatch audit read-only; no shared header or live hooks. |
| COOP-007 | Codex COOP-007 / thread `01a1ec02-1d0b-7791-8a57-e16ee680b905`, user assignment | `feature/coop-007-preparation-screen` | active; claim acknowledged by this commit | Checkpoint `be6aea95`; draft PR #12; `.worktrees/COOP-007` | Native `campaign_preparation_screen.{h,cpp}`, UI harness and own record. Consume merged header/layout; no legacy panel edits or production entry. Required native rendering/input evidence remains gate. |
| COOP-009 | Codex COOP-009 / thread `01a1ec1f-ea8d-77f3-b0e9-7c683f367ec3`, user assignment | `feature/coop-009-campaign-adapters` | active; claim acknowledged | Base `0fcea78f`; `.worktrees/COOP-009` | Reserved adapter/lifecycle harness and game.cpp/game_app.cpp/game_rounds.cpp/spawn_point.cpp; narrow profile/script/AI hooks. Agree 008 API; final integration after 008 merge. |
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

## COOP-006 coordinator acceptance checkpoint

- COOP-005 explicitly stopped ledger writes at `4beabd6b`; COOP-006 accepts the role and notifies COOP-007/001. Existing task ownership/scopes remain unchanged.
- Main is `5f561496`; COOP-004/005 implementation and independently reviewed completion metadata are integrated. No draft dependency is approved for merging.
- COOP-006 `6f4ea658` passed expanded GCC/MSVC x86 eight suites (541,799 checks); sanitizer repeat in progress. COOP-001 is independently reviewing the complete code/harness, no approval yet. Its findings require a minimum observed generation/view after a delta gap and manifest nested actor/binding coherence; worker is fixing them before final review.
- COOP-007 retains PR #12/native rendering/input acceptance gate. Its private pinned compiler/SDK/Python is borrowed read-only by COOP-006 using separate prefix/output; preserve resources and all active worktrees.
- COOP-008/009 readiness must be updated only after COOP-006 actually merges. COOP-010 retains all live admission, dispatch, replication and host/guest runtime gates.

## COOP-006 final integration review

- Candidate PR #13 `cb946f2a`, source `5c562770`, integrated-main sync `135efd77` (no protocol/harness changes). COOP-001 completed independent full source/header/harness/schema/contracts review plus focused final delta; PASS, no blocking findings. Reviewer independently reproduced GCC eight suites/541,811 checks (`gcc-4_7_4vha`) and verified source unchanged after main sync.
- Review findings resolved: generation/view resync floor cannot be lowered by delayed full snapshots; manifest present actors fenced/unique and match binding ActorRef/control revision; duplicate-Hello wording qualified. Earlier own review fixed preparation actor fences and result metadata coherence with COOP-005 confirmation.
- Worker final GCC16.2.1, MSVC19.44.35230 x86 /W4 /WX/SDK10.0.26100.0 and ASan/UBSan each PASS eight suites/541,811 checks. MSVC output `msvc-q8vheuxf`; own prefix, read-only COOP-007 tooling. Full final diff/history contains only reserved COOP-006 files relative to main, no assets/build outputs/shared-header/native-dispatch edits. Source-only component acceptance met; no live handshake/world/host-guest runtime claimed or waived.
- Coordinator/integrator separate gate checks final exact head and ordinary merge; task not done until actual merge. Completion/readiness metadata follows separately with independent review. COOP-007 remains active with native rendering/input gate; no review/merge approval for it.

## COOP-006 final stopped-writer handoff

- Finished: feature PR #13 merged `59cb89441c87b9398c9fe54fd3284c1e53a5ce87`, exact reviewed head `cb946f2a`/source `5c562770`. Final independent COOP-001 implementation review PASS and eight suites/541,811 checks on worker GCC/MSVCx86/ASan+UBSan; independent reviewer reproduced GCC suite.
- Completion metadata: PR #16 head `64043959`, independent COOP-001 read-only metadata review PASS (actual merge, four dependency ancestors, three-file scope/links/whitespace, dormant/runtime gates), merged `0fcea78f85d0fe9348c3b8e13d1a0f23f50a482a`.
- Stable local main and origin/main are `0fcea78f`, root checkout clean. Canonical `feature/coop-006-protocol` fast-forwarded/pushed to that integrated checkpoint so its durable task record is done; task and completion worktrees/checkpoints preserved. No uncommitted task changes.
- Ready/unclaimed: COOP-008 (001/004/006 merged), COOP-009 (001/004/005/006 merged). Require new coordinator claim acknowledgment and shared lifecycle-file sequencing before implementation; no assignments issued by this handoff. COOP-010 remains planned, with all actual two-instance playable runtime gates unchanged.
- Active writer: COOP-007 only. Its PR #12/native rendering/input gate and private pinned tooling/resources remain preserved, not reviewed/approved by this handoff. COOP-006 has finished compiler borrowing; no resources were removed or changed. Prior workers stopped after their completed task handoffs.
- Source-only protocol is dormant: no live dispatch/native layout changes; full schema/content fingerprints/limits and concrete live hooks are in `_re/harness/campaign_protocol/README.md`. Input additionally requires ownership handoff barrier/validation at enqueue and apply. No world/host-client runtime evidence claimed by component tests.
- Transfer: COOP-006 explicitly stops all ledger writes after this ordinary push. No successor is yet designated; next coordinator can accept this recorded stopped-writer handoff and notify COOP-007, then handle claims/reviews. No unfinished or unreviewed work may merge on the strength of this handoff.

## COOP-007 coordinator acceptance

- COOP-006's durable `95afcaed` handoff explicitly stopped the previous writer; COOP-007 accepts the coordinator role. Only COOP-007 is an active task writer.
- Main `0fcea78f` contains completed 001/002/003/004/005/006/011 and reviewed completion metadata. 008/009 are ready/unclaimed; do not interpret this takeover as an assignment.
- COOP-007 PR #12 remains draft. GCC and MSVC x86 screen behavior pass; actual LS3D preview/resource audit and native Win32 keyboard/mouse smoke pass at 1024x768 and 800x600 during implementation. Final source/capture review and widescreen repeat are ongoing; no component acceptance or merge approval yet.
- The private COOP-007 MSVC/SDK/Windows Python, engine/DTA DLLs, Wine prefix and captures stay preserved under ignored build paths; other workers finished borrowing without modifying them. No automated cleanup.
- Next: publish final review checkpoint and request an independent source/campaign-style/native evidence review, then integrate only after its gates pass.

## COOP-008 kickoff / coordinator acceptance

- Historical kickoff race: COOP-008 observed a prior stopped handoff, but COOP-007 had already accepted at `aa8081a6`. COOP-008 explicitly stopped after `bf838b1e`; the accepted task claim is retained, and transfer is reconciled below. Refreshed origin; main `0fcea78f` includes all 008 prerequisites. No existing 008 branch/PR/worktree at inspection.
- Accepted claim: COOP-008, Codex thread `01a1ec1f-4589-7051-9994-2a236a14b49b`, `feature/coop-008-controller-adapters`, `.worktrees/COOP-008`, base `0fcea78f`. Scope: campaign_control_adapter, controller/input/presentation consumers and actors_module; own harness/docs. No game_rounds/spawn_point/game_app/profile layout edits.
- COOP-009 user-assigned concurrent worker notified of reserved lifecycle scope and API sequencing; awaiting owner acknowledgment before claim row. COOP-007 scope/tooling preserved.
- Controller adapter remains inactive until COOP-010; no live admission, dispatch or playable runtime acceptance is implied.

## COOP-007 to COOP-008 explicit transfer

- Race reconciled without rewriting history: `aa8081a6` established COOP-007 coordinator; `bf838b1e` appended COOP-008 kickoff before observing it. COOP-008 explicitly confirmed stopped ledger writes in the shared task channel. COOP-007 now acknowledges its user-assigned task claim, base `0fcea78f`, branch `feature/coop-008-controller-adapters`, owner thread `01a1ec1f-4589-7051-9994-2a236a14b49b`, reserved scope and checkpoint `22352122`.
- COOP-009's concurrent user assignment is acknowledged: thread `01a1ec1f-ea8d-77f3-b0e9-7c683f367ec3`, branch `feature/coop-009-campaign-adapters`, base integrated `0fcea78f`. Scope remains the task's reserved lifecycle files; agree the COOP-008 adapter API and serialize shared lifecycle files before consumers bind. No prerequisite branch is stacked or merged unfinished.
- COOP-007 implementation checkpoint `b7e6bb16` (own source `5f122ce5`) is published in draft PR #12; final native build passed, behavioral GCC/MSVC x86 passed, final native input/capture repeats ongoing. Independent full source/campaign-style/native-evidence review requested from COOP-003. No review/merge approval implied.
- COOP-007 now explicitly STOPS all ledger writes after this ordinary push. COOP-008 may accept the role, update the current owner/header and claim rows, notify COOP-007/009, then serialize later reviews/integration. Preserve 007 resources; compiler reuse remains read-only with separate outputs/prefixes.

## Coordinator transfer declined / COOP-007 retained

- Crossing messages reconciled: COOP-008 explicitly declined the offered role to focus on controller implementation and confirmed stopped ledger writes. No new coordinator accepted `ee8b441f`. COOP-007 resumes its own stopped handoff and remains the sole coordinator writer. Both workers were notified.
- COOP-008 claim remains accepted at `22352122`; COOP-009 confirmed its exact reserved scope and base `0fcea78f`, now recorded above. Both may use COOP-007 tools read-only with their own prefixes/outputs. No file scope expansion or unfinished integration approved.
- COOP-007 final native/source independent review and repeat checks remain ongoing; PR #12 stays draft.
