# Cooperative campaign coordination ledger

The live copy of this ledger belongs to `coord/coop-work-queue`. The copy on `main` is
the initial template. See the [workflow](../agent_workflow.md#find-the-coordinator) before
creating or updating the operational branch. Do not merge its WIP ledger into `main`.

## Coordinator

- Owner/session: Codex COOP-013, explicit user assignment on 2026-11-18, thread `01a1edba-b5e3-7c73-bfa7-51e0b12fc205`.
- Contact/shared channel: this ledger and the canonical COOP-013 task branch/PR.
- Integrator: Codex COOP-013 for its own task, subject to the review and runtime acceptance gates.
- Latest checkpoint: `HEAD` on `origin/coord/coop-work-queue`.
- Next action: COOP-013 is integrated and has stopped coordination writes. COOP-014 retains its active mission-completion claim.

## Accepted task claims

| Task | Owner/session | Task branch | Status | Accessible checkpoint or PR | Next action |
| --- | --- | --- | --- | --- | --- |
| COOP-003 | Codex COOP-003 session (2026-11-18); explicitly assigned by user | `docs/coop-003-preparation-ui` | done | PR #3 merge `dd2aa98e`; completion metadata PR #5 `9aec1d7e` | Documentation/UI design complete; native rendering/input implementation remains COOP-007. |
| COOP-002 | Codex COOP-002 / thread `01a1ebd2-902d-7e32-9876-9818a845d361`, user assignment | `feature/coop-002-contracts` | done | PR #4 merge `9f59ff47`; completion metadata PR #5 `9aec1d7e` | Documentation contracts integrated; dependent declarations also integrated. |
| COOP-011 | Codex COOP-011 / thread `01a1ebe7-2361-7503-8dcb-6e0739646604`, user assignment | `feature/coop-011-shared-declarations` | done | PR #6 merge `0f367ed6`; metadata PR #7 `4d6f8cc1` | Shared declarations integrated; final coordinator target/native conformance passed. |
| COOP-004 | Codex COOP-004 / thread `01a1ebf9-60c7-74d0-9d39-cd09295875b0`, user assignment | `feature/coop-004-ownership` | done | PR #9 merge `59a480c8`; done metadata PR #14 `b72b6f63`; `.worktrees/COOP-004` | `campaign_session.{h,cpp}`, `campaign_ownership.{h,cpp}`, ownership harness and own record. Engine-independent; no shared header/legacy edits. |
| COOP-005 | Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, user assignment | `feature/coop-005-preparation-model` | done | PR #8 merge `06d999d7`; done metadata PR #15 `5f561496`; `.worktrees/COOP-005` | Atomic preparation model/harness/own record; audit actual legacy rules. Native rule parity remains acceptance, injected generic fake does not satisfy it. |
| COOP-006 | Codex COOP-006 / thread `01a1ec01-a68f-7170-9f75-7536a079675b`, user assignment | `feature/coop-006-protocol` | done | PR #13 merge `59cb8944`; completion PR #16 merge `0fcea78f`; `.worktrees/COOP-006` | `src/net/campaign_protocol.{h,cpp}`, protocol harness and own record. Native dispatch audit read-only; no shared header or live hooks. |
| COOP-007 | Codex COOP-007 / thread `01a1ec02-1d0b-7791-8a57-e16ee680b905`, user assignment | `feature/coop-007-preparation-screen` | done | PR #12 merge `8067b471`; completion PR #18 merge `577c04d8`; `.worktrees/COOP-007` | Native `campaign_preparation_screen.{h,cpp}`, UI harness and own record. Consume merged header/layout; no legacy panel edits or production entry. Required native rendering/input evidence remains gate. |
| COOP-008 | Codex COOP-008 / thread `01a1ec1f-4589-7051-9994-2a236a14b49b`, user assignment | `feature/coop-008-controller-adapters` | done | PR #17 merge `d34b6900`; completion PR #20 merge `934143aa`; canonical branch pushed to `934143aa` | Dormant controller/input/native consumers complete; reviewed by COOP-009, live dispatch/replication remain COOP-010. |
| COOP-009 | Codex COOP-009 / thread `01a1ec1f-ea8d-77f3-b0e9-7c683f367ec3`, user assignment | `feature/coop-009-campaign-adapters` | done | PR #19 merge `ce306626`; completion PR #21 merge `6f1bdd96`; canonical branch pushed to `6f1bdd96` | Dormant lifecycle/world-barrier/native rules complete; COOP-010 owns live attachment and actual two-instance evidence. |
| COOP-001 | Codex / 2026-11-18 user assignment | `feature/coop-001-build-baseline` | done | PR #2 merge `15b572ff`; completion metadata PR #11 merge `1b9b307f` | Build/solo/native host-guest baseline and isolation accepted; guide/evidence integrated. Firing/reload/completion remain unverified downstream gates. |

| COOP-010 | Codex COOP-010 (2026-11-18), user assignment | `feature/coop-010-two-player-mission` | done | PR #22 merged `738b124d`, completion PR #23 merged `dc41c6b7`, `.worktrees/COOP-010` | Arctic1 playable gate and original mode regressions recorded. |
| COOP-012 | Codex COOP-012 (2026-11-18), user assignment | `feature/coop-012-session-setup` | done | PR #26 merged `4ef8b814`; completion PR #27 merged `fd5ad4c3` | Five-instance setup matrix, concurrent admission, route isolation, original-mode regressions and COOP-013 contract recorded. |
| COOP-014 | Codex COOP-014, user assignment, thread `01a1ed7a-ddd9-7000-bbec-e292bd129d25` | `feature/coop-014-arctic1-completion` | active | PR #25 draft, base `9ec7fe48` | Arctic1 solo/dev reference, interactions, objectives and Running replication. |
| COOP-013 | Codex COOP-013, explicit user assignment on 2026-11-18 | `feature/coop-013-live-preparation` | done | PR #28 merged `73d518a5`; completion PR #29 merged `e4344dfc` | First menu deployment and recovery integrated. COOP-015 still awaits COOP-014. |


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

## COOP-007 final integration candidate

- PR #12 exact head `4645f814`, source `d3269c4a`, includes integrated main `0fcea78f`; no unfinished dependency stack. Final diff has only the nine reserved screen/harness/task files; shared contracts, legacy panels/globals, engine ABI, wire and normal menu routing are unchanged. Whitespace/scope/history checks pass.
- Independent COOP-003 full source/API/native fixture/runner/docs/capture review and targeted final rereview PASS. It independently reproduced strict GCC behavior twice. Findings fixed before final approval: painter-measured multiline pagination and paging all seven/additional skills. Eligible carriers and item variant metadata are explicit.
- Strict intended MSVC x86 `/W4 /WX` behavior and native preview builds PASS. Actual Win32 mouse/keyboard LS3DF suites and inspected captures PASS at 1024x768, 800x600, letterboxed1280x720, including long final text/skills, ownership loss, pending stock, rejection, readiness and freeze. Native resource audit succeeds.
- Source-only bounded component; not a shipping menu or connected preparation feature. Procedural panels/portrait markers and unposed synthetic original-model/prop mappings are fixture presentation, not final production face/material/attachment/localization acceptance. These limits are explicit in the README, PR and handoff.
- Final metadata check requested from COOP-003; exact-head integration gates and actual merge still follow. No later live-preparation or playable runtime gate is waived. Preserve all private 007 tooling/resources for 008/009.

## COOP-007 integration outcome / active lifecycle workers

- Independent final COOP-003 metadata check PASS at `4645f814`; source unchanged from approved `d3269c4a`. It checked all three native final logs, preview hash and inspected the 1280x720 final restriction/line30 captures.
- Separate integrator exact-head/diff/scope/history/link/evidence checks PASS; GitHub reported CLEAN with no failing checks. PR #12 merged normally with exact-head match at `8067b4710a5d68f46e37a8cfc5d4a83d4bd33219`. Source-only native component acceptance met; no connected UI or final shipping-look/live localization gate waived. Reviewed completion metadata follow-up is being prepared.
- COOP-009 checkpoint `429ae489` published; worker reports native lifecycle/extracted legacy suites and target compile evidence in progress, not reviewed. Coordinator acknowledges `campaign_mission_native.{h,cpp}` beside its reserved adapter as part of the same lifecycle scope; no other-worker file edit is authorized. 008 API/progress setter and final merge order remain explicit gates.
- COOP-008 remains active in reserved controller scope. Preserve COOP-007 private compiler/SDK/Python/DLL resources while these workers depend on them. Coordinator will hand off stopped-writer state before ending the COOP-007 session.

## COOP-007 final stopped-writer handoff

- Finished: feature PR #12 merged `8067b4710a5d68f46e37a8cfc5d4a83d4bd33219`, exact reviewed head `4645f814` / source `d3269c4a`. Independent COOP-003 full and targeted source/native-capture review PASS; strict GCC/MSVC x86 behavior and actual native LS3DF SendInput/capture suites PASS at 1024x768,800x600,1280x720. No connected preparation or final production-look/localization claim.
- Independently reviewed completion PR #18 merged `577c04d8ecb493ccb643cb7124e85931613c05bc`, reviewed metadata `f3a64440` plus requested exact visual-evidence wording correction `871fc9b4`. Main and canonical 007 branch are clean/updated/pushed to `577c04d8`; done record is integrated. Task/completion worktrees, branches and evidence are preserved. No COOP-007 task work remains.
- Active writers: COOP-008 thread `01a1ec1f-4589-7051-9994-2a236a14b49b`, canonical controller branch, draft PR #17; COOP-009 thread `01a1ec1f-ea8d-77f3-b0e9-7c683f367ec3`, canonical mission branch, checkpoint `429ae489` (latest active branch/handoff remains authoritative). Their reserved scopes and accepted claims remain unchanged. No final implementation review or merge approval has been issued for either.
- Merge sequencing: agree 008 adapter/progress/disconnect API; preserve 009 sole lifecycle-file ownership; final 009 combined integration follows 008 merge and affected checks. Later actual host/guest/controller/mission/COOP-010 gates are not waived by 007 UI evidence. 009's narrow campaign_mission_native files are acknowledged within its lifecycle scope.
- Coordination race correction: 008 appended a kickoff after 007 accepted coordination, then explicitly stopped and declined transfer; 007 retained the role through this completion. Neither 008 nor 009 is automatically the new coordinator. Any stale task prose saying coordination already transferred to 008 must be corrected by that task owner. Next coordinator explicitly accepts this stopped-writer handoff and notifies both workers.
- Required resource preservation: `.worktrees/COOP-007/build/tooling/run-python.bat` and `vcvars-x86.bat` configure pinned MSVC14.44.35207/SDK10.0.26100.0/WinPython3.13.9; native import libraries/DLLs remain under 007 `ls3df/build/msvc` and `rw_data/build/msvc`. Read-only borrowing with each worker's private prefix/output is authorized. Preserve the 007 worktree and ignored tools/runtime/captures while active workers depend on them. No cleanup or global environment changes.
- Transfer: COOP-007 explicitly STOPS all ledger writes after this ordinary push. No successor has accepted yet. User-assigned tasks remain active; future claims/integration require the workflow gates and a confirmed coordinator.

## COOP-009 coordinator acceptance

- COOP-007 explicitly stopped ledger writes at `20310e2a`; COOP-009 accepts the role and notifies COOP-008/007 before this update. No task writer/scopes are reassigned.
- COOP-007 is done at main `577c04d8` (feature `8067b471`); resources and evidence remain preserved. COOP-008 is active in draft PR #17; its latest API checkpoint is `e18835b3`, with final review/validation pending. COOP-009 owns only its acknowledged lifecycle scope, initial published claim `429ae489`; implementation/harness evidence is being prepared.
- No integration approval is implied. Final COOP-009 combined adapter integration follows COOP-008 merge and affected regressions. COOP-010 owns live attachment and actual two-instance campaign acceptance.
- COOP-009 runtime resources: private Xvfb :101 (later :102 guest), own Wine prefixes and build output; preserve other workers displays/resources. Pinned COOP-007 compiler/SDK/Python and engine imports are borrowed read-only, imports copied into ignored 009 directories.

## Resource correction / COOP-009 restoration

- After COOP-007 completed, its entire worktree/toolchain disappeared externally. COOP-007/008/009 each confirmed absence; none reports deleting it. Historical passing checks remain accurate, but old 007 paths no longer exist. Preservation statements above describe the earlier resource state.
- COOP-009 restored a private pinned toolchain using the merged build guide: `.worktrees/COOP-009/build/tooling/run-python.bat`, MSVC14.44.35207/SDK10.0.26100.0/WindowsPython3.13.9, own downloader/cache. Read-only borrowing is authorized for COOP-008 with its own Wine prefix and isolated outputs. Native engine imports/DLLs copied before disappearance remain in ignored 009 build/runtime directories. Preserve these resources until all consumers finish; never infer available paths from historical evidence alone.

## Adapter independent reviews in progress

- COOP-009 draft PR #19 source checkpoint `f552f618` remains inactive. Worker GCC/MSVC x86/ASan+UBSan focused suites pass 343 lifecycle and 100 actual-source native checks; target 9-file compile has zero diagnostics. COOP-008 independently reviewed complete `30ab8434`, reproduced GCC and found guest Running-before-Start bypass. The corrected native predicate explicitly requires reconstructed/matching Start receipt; pre-generation retirement now composes the agreed COOP-008 seam. Final rereview and combined checks after COOP-008 merge remain mandatory.
- COOP-008 PR #17 candidate `6c9f0348` (source `1a992bba`) underwent COOP-009 complete initial-source and final-delta review. Reviewer reproduced initial GCC 1266+147 and original-executable remote comparison (80,002 checks, zero failures) in private detached review output. Findings: old native bindings were not drained after catalog unbind; retained scripted controllers lost handlers; campaign squad death/environment rules remained local-only; original address comments and legacy-runner inputs needed correction. Worker added actual-native/real-model and actual-source team regressions; updated GCC/MSVC/sanitizer 1272+232+22 evidence is being reviewed/reproduced.
- No merge approval yet: COOP-008 identified an additional native team/player-array consumer and is fixing it before exact-head review. Keep both branches/worktrees and private tooling. COOP-009 final progress setter/composition follows actual COOP-008 merge. Later live dispatch/world transfer/host-guest acceptance remains COOP-010.

## COOP-008 integrated / COOP-009 combined gate

- COOP-008 PR #17 merged normally `d34b690098df4d787763bd179b0a1e8d552a06a0`, exact reviewed head `e367aac6`, source `b3f240c2`. COOP-009 independently read the full initial source and every correction, reproduced final GCC 1272+232+36 checks and 80,002 original remote comparisons with zero failures, inspected final MSVC/sanitizer/full compile/affected repeats/280-object trial link evidence. Exact-head scope/history/whitespace/CLEAN/no-failing-check gates passed before merge.
- Independently reviewed one-file actual-done metadata PR #20 merged `934143aa2891721aa29540cb7f43e5cb140541a4`; root main is clean/fast-forwarded. COOP-008 remains available temporarily for final COOP-009 review, with no source scope or ledger ownership reassignment.
- COOP-009 merged only finished main and added the agreed progress setter; current published source `2ff80a94`. COOP-008's complete source review and independent GCC reproduction pass through that source. A new combined real lifecycle/ownership/host+guest control/protocol fixture passes 389 lifecycle and 104 actual-source native checks on GCC/MSVC/ASan+UBSan; final full combined game built 282 units, zero compiler messages, link 0, LAA yes. Final handoff, precise source scope and small harness delta review remain before accepting PR #19.
- No live session is installed by either component. COOP-010 remains planned until COOP-009 actually merges; actual two-instance campaign play, world format/dispatch and later save/join/mission gates remain unchanged.

## COOP-009 final stopped-writer handoff

- Finished: PR #19 merged `ce30662659e4543ddca50ffa3d4f397f65aeb91e`, exact head `7cdb0237`, source `6375ccad` / combined-main synchronization `3a257b90`. Independent COOP-008 complete implementation/final-delta review PASS; it reproduced GCC 409 lifecycle/composition + 104 actual-source native checks and inspected matching target/sanitizer and full game evidence. Final exact-head documentation-only delta, dependencies, 20-file scope, whitespace and ready/CLEAN/no-failing-check gates passed before normal merge.
- Actual-done/dependency metadata PR #21 head `ed8e547f` independently reviewed by COOP-008 PASS: actual merged head/parents/prerequisite ancestry, exactly two task records, unchanged COOP-010 acceptance criteria, links/whitespace and preserved root resources. Merged normally at `6f1bdd96075b074a6a7b958257af5f56f8374fb5`. Root main and origin/main are clean at `6f1bdd96`; canonical 009 branch is fast-forwarded/pushed there so its durable record is done. No uncommitted task source.
- All initial adapter tasks are now done. COOP-010 is ready and unclaimed, with the first actual two-instance campaign world/admission/input/disconnect acceptance still mandatory. No session/menu/live dispatch is installed by 008/009; no save/load, late join, full vehicle behavior or mission-completion gate is waived. Future source fixes remain bounded follow-ups rather than silently changing completed results.
- Resource correction/preservation: the old 007 worktree/toolchain disappeared externally after its completion. Historical checks remain accurate, but old 007 paths are unavailable. COOP-009 restored a private pinned toolchain and additionally copied/reconfigured it OUTSIDE disposable task worktrees: root-checkout ignored `build/coop-shared-tooling/run-python.bat` (MSVC14.44.35207, SDK10.0.26100.0, WindowsPython3.13.9), with matching engine imports/DLLs under `engine/`. A fresh separate-prefix x86 compile through this shared launcher passed one unit with zero errors/warnings. Use it read-only with process-local prefixes and isolated output; preserve it for successors.
- Durable local evidence outside task worktrees: root ignored `build/coop-009-evidence/` holds final full-build/component/sanitizer/portable-tooling probe logs, exact commands/environments and the local built executable. Full combined x86/x87 game: 282 units, zero compiler messages, link0, LAA yes. No game data/compiler/binary artifacts were committed or distributed. No live campaign mission run is claimed.
- Prior 008 worker confirmed completion/canonical done and stopped implementation, remaining briefly for 009 final and metadata reviews. No active source writer or coordinator successor is designated. This agent explicitly STOPS ledger writes after this ordinary push; next coordinator must accept before claims/edits. Branches, worktrees, tooling and artifacts were not cleaned or deleted by this agent.

- Final ledger-map correction: COOP-009 resumes only its own just-published stopped handoff after confirming origin still equals `b0e62402` and no successor writer/claim exists. The accepted-claims row is corrected from its stale active entry to the actual done state documented above; no assignment changes. This correction is the final ledger write, after which COOP-009 explicitly stops again.

## COOP-010 coordinator acceptance

- COOP-009 stopped ledger writes at `3ff21e72`. COOP-010 accepts the transfer and acknowledges the user's assignment. No other active writer or COOP-010 branch, worktree, or PR was found after refreshing origin.
- Main and origin/main agree at `6f1bdd96`; COOP-008 `d34b6900` and COOP-009 `ce306626` are both ancestors. Shared build tooling and previous evidence are available for read-only use.
- COOP-010 owns the development launch, live admission and initial-world reconstruction, and the scoped legacy adapter changes required by the playable gate. Record any scope expansion here before editing shared files.

- Transport scope: COOP-010 owns the new campaign network attachment and the narrow DirectPlay session and callback changes in `net_session.cpp`, `net_messages.cpp`, `net_game.cpp`, `net_game.h` and `dplay8.h`. Campaign traffic uses the provider's reliable ordered channel and its connection identity. Live campaign paths will bypass native UDP, GameSpy and voice; ordinary sessions retain their routes.

- Integration scope: COOP-010 owns the narrow ownership admission change needed to commit the transport-assigned PlayerId after negotiation, with a focused ownership regression. It also owns the original campaign development launcher and any original mission lifecycle integration fixes required by its test fixture.

- Arctic1 fixture scope: COOP-010 owns read-only default preset import in `loadout_presets.{h,cpp}` and the narrow original script-list selection in `script_module.cpp`. The development fixture will negotiate exact local mission content, use the original Arctic1 equipment presets, and keep world simulation behind the existing loading barrier.

- Arctic1 checkpoint: the actual host loaded the original campaign PLAY step with four bound soldiers, original kits and essentials after two-instance admission. An altered guest copy received ContentMismatch. After restoring it, the same host admitted a new PlayerId on the same provider slot. The world-ack deadline aborted at mission time zero as expected. The full x86 build and the nine protocol suites passed. Solo main menu and native LAN two-peer player setup were observed. Guest world reconstruction, in-play ownership transfer and the acceptance gameplay route remain pending. Runtime logs and build evidence are under root ignored `build/coop-010-evidence/`.

- Initial world scope: COOP-010 owns Arctic1 time-zero reconstruction, including narrow Loading-phase startup changes in `game.cpp`, `spawn_point.cpp`, `campaign_mission_native.cpp`, `items_module.cpp`, `actors_module.cpp` and the script modules. The guest rebuilds from negotiated original content and the frozen manifest. A versioned native world comparison must pass before the guest sends its snapshot acknowledgment. Script execution and gameplay effects remain host-owned.

- First-running-frame scope: COOP-010 owns the narrow original Arctic1 melee tracker replica startup and cleanup fix in `melee_tracker.cpp`. The first accepted world transfer exposed an uninitialized presentation movement controller on the guest. The native remote controller remains client-side and the host retains AI authority.


- Two actual instances reconstructed original Arctic1 and acknowledged generation 1 / snapshot 1 before starting. The guest dog animation initialization fix held during the paired run. Source `055daf08`, handoff `28e0d6fd` and draft PR #22 carry this checkpoint.
- Live gameplay scope covers the bounded campaign protocol format and conformance harness, the development launcher/session, ownership and native control adapters, and the necessary player/controller, mission/objective and camera presentation hooks. The guest receives host state and sends typed, ownership-tagged controls. The native multiplayer replication lane stays gated in campaign mode. COOP-010 is the only active writer in these files.


- Running update checkpoint `2cb0509e` with handoff `b316a31c` publishes the host's four squad actor records and original objective statuses. Both instances entered, the guest applied the objective activation and camera state, and live re-admission was rejected after a guest departure. Full playable acceptance and original-mode regressions remain open.

- COOP-010 owns the host-side movement integration in `human_body.cpp`, `human_movement.cpp` and the corresponding `C_Human` predicates. The remote-owned soldier still needs to move through the original body controller even when the host never renders it. The test driver remains outside tracked source.
- COOP-010 owns a narrow squad-controller attach change in `squad_controller.cpp` and `squad_actions.cpp`. The live disconnect after a native inventory swap exposed an equipment change during the original AI controller's attach. The handoff must preserve the committed inventory before normal AI decisions resume.
- COOP-010 also owns the related `human_locomotion.cpp` branches for player collision and terrain contact. Paired movement showed that the native remote-actor ground path cannot stand in for the host's simulation of a guest-owned player.

- COOP-010 owns the original campaign objective information screen in `interface_info.cpp`, the host campaign sight and hearing registration in `human.cpp`, and the guest exit after transport loss. A paired Arctic1 run exposed native deathmatch scoring on the guest, an unregistered host sight observer near the patrol, and an extra guest frame after replication objects had been freed. Native modes keep their original information and actor paths.

- COOP-010 owns the native campaign map and small score overlay selection in `interface_map.cpp` and `interface_overlays.cpp`. The campaign map must remain usable after player ownership ends, and native multiplayer map, scoreboard, and respawn behavior stay under the original multiplayer path.

- Separate integration review: PR #22 head `fd1572a33c15445e7fdd66a0950d9ead305420e1` was reviewed against `main` at `6f1bdd96`. Verified the transport and authority fences, the two-instance start barrier, paired controls, original objectives and forest patrol route, exact changed-kit takeover, ordinary Tutorial and Sicily2 regressions, matching runtime logs, x86 build, and controller/lifecycle/protocol/ownership harnesses. The review added the actor/navigation/sound fingerprint inputs and verified rejection of different navigation data and successful admission after restoration. The merge rehearsal produced the exact feature tree.

- Integration complete: feature PR #22 merged the reviewed head as `738b124d57a85901c167cebf9b07ca079aac2ac9`. Documentation PR #23 recorded done and merged as `dc41c6b7bb920dbaf61522f92ce2cc5cb6670122`. The task worktree and local runtime evidence are retained.

## COOP-012 coordinator acceptance

- The last coordinator's published COOP-010 checkpoint `c6ae9ea2` records completed integration. The user assigned COOP-012. As the sole current worker, COOP-012 acknowledges the claim and takes coordination for this task. Refreshed origin, checked branches/worktrees and open PRs, and verified the COOP-010 integration ancestor of base `9ec7fe48`. No competing writer or COOP-012 branch was found.
- Reserved scope: singleplayer mode picker and setup resources, admitted session lifetime and campaign transport setup/teardown, and scoped application entry/return as needed. COOP-013 will consume the admitted setup session. COOP-014 is unclaimed and retains Running replication, mission interaction and completion. Keep development-world dispatch changes confined to separating the setup entry from the existing dev launch and record the seam before review.

- Accepted parallel user assignment COOP-014 after confirming its worktree, matching base and no competing PR. COOP-012 owns setup UI, admission/lifetime and DirectPlay setup changes. COOP-014 owns Running behavior, interaction/presentation, scripts/objectives and mission outcome. Coordinate any common dispatch/launch edits before writing, with focused commits in each scope. Both workers will use private build output, prefixes and displays; serialize paired runs if shared fixed LAN binding requires it.

## COOP-012 final integration and stopped-writer handoff

- PR #26 merged exact reviewed head `3e47dfdaeb1541ba215d72d6ad7442644e4323bc` as `4ef8b814ee8673168f62a87c95b2286569f35094`. The merged tree matched the rehearsal. Source `994d034e` passed the final runnable x86 route repeats. The five-instance cap/refusal/retry/teardown matrix and controlled overlapping acknowledgments passed at its admission parent. Both native Sicily2 and the original Arctic1 development launch passed on the final executable.
- Completion PR #27 merged as `fd5ad4c3e997be8a6d4a7211e04dd3319677e9a2`; it records actual done and makes COOP-013 ready after its three merged dependencies. Main and the canonical COOP-012 branch point to this completion checkpoint. Durable local evidence and private test scripts are under ignored `build/coop-012-evidence/`.
- COOP-014 retains its active independent claim and draft PR #25. No other task owner is changed. COOP-013 is ready and unclaimed. Its task record points to the admitted setup session and preparation contract. COOP-012 stops task and coordination writes after this ordinary push. A successor can accept this recorded handoff before writing the ledger.

## COOP-013 coordinator acceptance

- COOP-013 accepts the stopped COOP-012 coordination handoff at `204fbbb6` and acknowledges the user's assignment. The current main at `fd5ad4c3` contains the COOP-005, COOP-007 and COOP-012 merges. No competing COOP-013 branch, worktree or PR was found before creating the canonical task worktree.
- COOP-013 owns preparation rule/catalog import, the live presenter and model adapter, setup handoff, preparation-family dispatch and the bounded first deployment/loading transition. COOP-014 retains Running behavior and mission completion. Keep preparation dispatch and first-load changes in separate functions or focused commits where the task touches shared files. Use private build and runtime outputs.
- COOP-014 acknowledged COOP-013 ownership of per-player world send and acknowledgment state, Running recipients and per-actor input buffers. COOP-013 reserves the preparation resync wire tag 21 and feature bit 7. COOP-014 retains Running body semantics. COOP-013's private network namespace isolates its tests from COOP-014's fixed LAN port.


## COOP-013 integration review

- Reviewed PR #28 head `c3f69622` against `main` at `4d4e0fab`. Verified the three merged dependencies, source equivalence to `5632e7b8`, whitespace and the full shared-file diff. The merge rehearsal produced the exact reviewed tree.
- Verified the authority and revision checks, host-only AI editing, negotiated resync, loading fences and ordinary route separation. Matched each world reconstruction's byte count, CRC32 and generation to its host log and checked acknowledgment ordering. Reviewed live kit, briefing, cancel, solo inventory and native LAN evidence, the timeout and redeployment logs, the x86 build and the subsystem and sanitized harness results.
- The acceptance criteria are satisfied for the first Arctic1 deployment with AI filling unclaimed slots. COOP-014 remains active. COOP-015 still depends on both tasks.


## COOP-013 integration outcome and stopped-writer handoff

- PR #28 merged as `73d518a5d4d2f1822c3caa5b995ebd4100c75477`. Its actual merged tree matches the reviewed candidate. Documentation-only PR #29 merged as `e4344dfcc2073c54b253a50633dc15f7701026e0` and records COOP-013 done.
- COOP-014 keeps its active assignment and draft PR #25. COOP-015 remains planned until the mission outcome is integrated. Keep the COOP-013 branch, worktree and private validation evidence for the next integration.
- COOP-013 stops all ledger writes after this ordinary push. A successor can accept the stopped-writer handoff before changing coordination entries.
