# Cooperative campaign coordination ledger

The live copy of this ledger belongs to `coord/coop-work-queue`. The copy on `main` is
the initial template. See the [workflow](../agent_workflow.md#find-the-coordinator) before
creating or updating the operational branch. Do not merge its WIP ledger into `main`.

## Coordinator

- Owner/session: Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, accepted 2026-11-18 after explicit stopped-writer handoff `46f3a301`.
- Contact/shared channel: this published ledger and active task threads below; notified all four owners on transfer.
- Integrator: Codex COOP-005; independent review for its own task is requested from COOP-004. Separate candidate review and acceptance gates apply to each task.
- Latest checkpoint: `HEAD` identifies this update. Ordinary push to `origin/coord/coop-work-queue`; never merge this operational branch into main.
- Next action: independently review COOP-004 PR #9; COOP-004 reviews COOP-005 PR #8 while COOP-007 restores removed target tooling.

## Accepted task claims

| Task | Owner/session | Task branch | Status | Accessible checkpoint or PR | Next action |
| --- | --- | --- | --- | --- | --- |
| COOP-003 | Codex COOP-003 session (2026-11-18); explicitly assigned by user | `docs/coop-003-preparation-ui` | done | PR #3 merge `dd2aa98e`; completion metadata PR #5 `9aec1d7e` | Documentation/UI design complete; native rendering/input implementation remains COOP-007. |
| COOP-002 | Codex COOP-002 / thread `01a1ebd2-902d-7e32-9876-9818a845d361`, user assignment | `feature/coop-002-contracts` | done | PR #4 merge `9f59ff47`; completion metadata PR #5 `9aec1d7e` | Documentation contracts integrated; dependent declarations also integrated. |
| COOP-011 | Codex COOP-011 / thread `01a1ebe7-2361-7503-8dcb-6e0739646604`, user assignment | `feature/coop-011-shared-declarations` | done | PR #6 merge `0f367ed6`; metadata PR #7 `4d6f8cc1` | Shared declarations integrated; final coordinator target/native conformance passed. |
| COOP-004 | Codex COOP-004 / thread `01a1ebf9-60c7-74d0-9d39-cd09295875b0`, user assignment | `feature/coop-004-ownership` | active; claim acknowledged by this commit | Checkpoint `9601fcb6`; draft PR #9; `.worktrees/COOP-004` | `campaign_session.{h,cpp}`, `campaign_ownership.{h,cpp}`, ownership harness and own record. Engine-independent; no shared header/legacy edits. |
| COOP-005 | Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, user assignment | `feature/coop-005-preparation-model` | active; claim acknowledged by this commit | Base `4d6f8cc1`; checkpoint `8dc87b0a`; draft PR #8; `.worktrees/COOP-005` | Atomic preparation model/harness/own record; audit actual legacy rules. Native rule parity remains acceptance, injected generic fake does not satisfy it. |
| COOP-006 | Codex COOP-006 / thread `01a1ec01-a68f-7170-9f75-7536a079675b`, user assignment | `feature/coop-006-protocol` | active; claim acknowledged by this commit | Checkpoint `12625f6c`; `.worktrees/COOP-006` | `src/net/campaign_protocol.{h,cpp}`, protocol harness and own record. Native dispatch audit read-only; no shared header or live hooks. |
| COOP-007 | Codex COOP-007 / thread `01a1ec02-1d0b-7791-8a57-e16ee680b905`, user assignment | `feature/coop-007-preparation-screen` | active; claim acknowledged by this commit | Checkpoint `7ea491e0`; `.worktrees/COOP-007` | Native `campaign_preparation_screen.{h,cpp}`, UI harness and own record. Consume merged header/layout; no legacy panel edits or production entry. Required native rendering/input evidence remains gate. |
| COOP-001 | Codex / 2026-11-18 user assignment | `feature/coop-001-build-baseline` | done | PR #2 merge `15b572ff`; completion metadata PR #11 merge `1b9b307f` | Build/solo/native host-guest baseline and isolation accepted; guide/evidence integrated. Firing/reload/completion remain unverified downstream gates. |

Only the coordinator adds or reassigns claims. A missing row is not sufficient evidence
of availability: check worktrees and existing task branches as well. Task records carry
detailed progress; this ledger locates their active owners and checkpoints.

## Review and integration queue

1. COOP-002 / PR #4, reviewed head `2ee1dca6`: coordinator read full contract, task/index/COOP-011 refinements, source coupling/rights/generation boundaries; reproduced two document check blocks (11 Markdown files, 68 local links/anchors; 11 task branch/dependency records; acyclic) and whitespace check. COOP-003 consumer review passed. Documentation-only acceptance satisfied; no compiled/runtime claims. Merged `9f59ff47`; actual done record in completion PR #5 `9aec1d7e`.
2. COOP-003 / PR #3, reviewed head `b4030c11`: coordinator read specification/resource audit/fixture/driver and latest prose-only corrections, checked source symbols, inspected generated host preview, reproduced browser smoke (all scenarios, 3 widths, zero page errors) and whitespace checks. COOP-002 independent full review passed. Documentation-only design acceptance satisfied; native rendering/input/network evidence remains downstream. Merged `dd2aa98e`; actual done record in completion PR #5 `9aec1d7e`.
3. COOP-001 / PR #2: COOP-004 independent review of `747d0590` passed, reproduced three process tests and audited all tooling/docs/evidence. Complete x86/x87 build, partial Tutorial and actual native two-client Sicily2 movement/inventory/status baseline; actual MSVC concurrent worktree checks passed. Firing/reload unconfirmed, no mission-completion claim. Merged `15b572ff`; independently reviewed done/blocker metadata PR #11 merged `1b9b307f`.
4. COOP-004 / draft PR #9 at `68bb0bc1`: worker requests independent behavioral/API review and reproduction of `python3 _re/harness/campaign_ownership/run.py --compiler gcc`. Worker reports native and target x86 MSVC /W4 /WX nine suites, 10618 checks passed. **No coordinator implementation review performed yet; not approved by this ledger.** Worker asked to obtain peer review or perform workflow's separate review pass if no reviewer available, then may integrate once gates pass. No live game behavior claimed for this module.
5. COOP-005 / draft PR #8: implementation checkpoint `8dc87b0a`; native authority/conservation and 810 capacity/backpack + 240 essentials differential checks passed. MSVC x86 passed earlier implementation; final boundary tweaks need rerun. COOP-004 independent review requested. No integration approval yet. COOP-006/007 in progress, no review request recorded. Inspect their canonical branches/latest handoffs before acting.

## Coordinator handoff

- Completed/integrated: COOP-001 `15b572ff` (done metadata `1b9b307f`); COOP-002 `9f59ff47`; COOP-003 `dd2aa98e`; their done metadata `9aec1d7e`; COOP-011 `0f367ed6` (done/readiness `4d6f8cc1`). Optional PR-label cleanup integrated `dbc9a85a`. All historical branches/worktrees and checkpoints preserved.
- Active task writers: COOP-004/005/006/007 retain their acknowledged canonical branches and reserved scopes above. COOP-001 implementation and metadata writers stopped cleanly; COOP-002/003/011 stopped after completion. No branch reassignment is made by this handoff.
- Pending review/merge order: review COOP-004 PR #9 first if ready; COOP-005 PR #8 remains draft/in progress. 004..007 are independent consumers of merged declarations, so integrate individually when their own acceptance passes; do not merge unfinished dependency branches. COOP-008 depends on 001/004/006; COOP-009 on 001/004/005/006; both remain planned. COOP-010 retains combined playable runtime gates after its dependencies. No unclaimed ready task currently recorded.
- Shared contracts: merged campaign header remains a compatibility boundary. 004 owns concrete cross-family request journal; 005 consumes an agreed injectable boundary while developing independently. Production has one authority queue/journal. 006 codec/opcode/native dispatch audit does not attach live hooks; proposed positive opcode `0x434f4f50` requires collision/consumer audit. 007 must establish native engine rendering/input evidence; HTML/presenter-only evidence is insufficient. File-scope expansion must be coordinated with affected owners before edits.
- Runtime resources correction: COOP-001 worktree and ignored tooling/artifacts disappeared externally after its passing checks. Its owner confirms they did not remove/move them. Historical evidence remains accurate but those paths are unavailable. COOP-007 is reprovisioning a private pinned compiler/SDK/Python and later native libraries; other workers should avoid duplicate downloads and preserve its tooling. COOP-005 retained historical MSVC output logs in its own ignored build directory. Xvfb :99/:100 remain available. Exact setup is in merged `docs/build_baseline.md`.
- Transfer acknowledgment: COOP-001 explicitly stopped ledger writes at `46f3a301`; COOP-005 accepts that handoff with this commit and notifies COOP-004/005/006/007. Existing branches/scopes are preserved; this transfer approves no unreviewed implementation.

Keep this record and task checkpoints accessible to successors. When moving between
machines, a local-only ledger or unpublished task branch must be transferred explicitly.

## Cross-task request journal boundary

COOP-004 owns a concrete bounded cross-family request journal in its reserved campaign_session files. COOP-005 may inject a narrow Begin/Finish journal boundary and use a behavioral fake while developing independently; no duplicate shared implementation/header and no dependency on an unmerged COOP-004 branch. Agree exact identity/content/result/expiry semantics before consumers bind. Production composition must use one shared authority queue and cross-family journal. Injection does not waive COOP-005 actual campaign equipment-rule/stock-conversion acceptance; keep the PR draft until its stated evidence is met or split a bounded foundation explicitly.
