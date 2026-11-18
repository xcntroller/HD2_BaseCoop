# Cooperative campaign coordination ledger

The live copy of this ledger belongs to `coord/coop-work-queue`. The copy on `main` is
the initial template. See the [workflow](../agent_workflow.md#find-the-coordinator) before
creating or updating the operational branch. Do not merge its WIP ledger into `main`.

## Coordinator

- Owner/session: Codex / COOP-001 session, 2026-11-18; kickoff coordinator, thread `01a1ebd1-3990-7553-920e-7cfee0148527`.
- Contact/shared channel: This task conversation and this published ledger; claims are acknowledged by ledger commits.
- Integrator: Codex; separate review pass required before any integration.
- Latest checkpoint: `HEAD` identifies this ledger update.
- Publication: `origin/coord/coop-work-queue` (ordinary push; no force updates).
- Next action: establish COOP-001 environment/build evidence; preserve other ready tasks for their owners.

## Accepted task claims

| Task | Owner/session | Task branch | Status | Accessible checkpoint or PR | Next action |
| --- | --- | --- | --- | --- | --- |
| COOP-003 | Codex COOP-003 session (2026-11-18); explicitly assigned by user | `docs/coop-003-preparation-ui` | done | PR #3 merge `dd2aa98e`; completion metadata PR #5 `9aec1d7e` | Audit campaign UI and produce specification/resource inventory/original mockup. Scope: `docs/ui/`, `docs/tasks/COOP-003.md`. Coordinate proposed model needs with COOP-002 through durable consumer notes. |
| COOP-002 | Codex COOP-002 / thread `01a1ebd2-902d-7e32-9876-9818a845d361`, user assignment | `feature/coop-002-contracts` | done | PR #4 merge `9f59ff47`; completion metadata PR #5 `9aec1d7e` | Integrated documentation contracts; COOP-011 ready/unclaimed, 004..007 remain planned. |
| COOP-011 | Codex COOP-011 / thread `01a1ebe7-2361-7503-8dcb-6e0739646604`, user assignment | `feature/coop-011-shared-declarations` | done | PR #6 merge `0f367ed6`; metadata PR #7 `4d6f8cc1` | Shared declarations integrated; final coordinator target/native conformance passed. |
| COOP-004 | Codex COOP-004 / thread `01a1ebf9-60c7-74d0-9d39-cd09295875b0`, user assignment | `feature/coop-004-ownership` | active; claim acknowledged by this commit | Base integrated `0f367ed6` or later main; `.worktrees/COOP-004` | `campaign_session.{h,cpp}`, `campaign_ownership.{h,cpp}`, ownership harness and own record. Engine-independent; no shared header/legacy edits. |
| COOP-005 | Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, user assignment | `feature/coop-005-preparation-model` | active; claim acknowledged by this commit | Base `4d6f8cc1`; checkpoint `278723cc`; `.worktrees/COOP-005` | Atomic preparation model/harness/own record; audit actual legacy rules. Native rule parity remains acceptance, injected generic fake does not satisfy it. |
| COOP-006 | Codex COOP-006 / thread `01a1ec01-a68f-7170-9f75-7536a079675b`, user assignment | `feature/coop-006-protocol` | active; claim acknowledged by this commit | Base integrated `dbc9a85a` (002/011 merged); `.worktrees/COOP-006` | `src/net/campaign_protocol.{h,cpp}`, protocol harness and own record. Native dispatch audit read-only; no shared header or live hooks. |
| COOP-001 | Codex / 2026-11-18 user assignment | `feature/coop-001-build-baseline` | review | `origin/feature/coop-001-build-baseline`, checkpoint `747d0590`, draft PR #2 | Bounded full build + solo/native host-guest baseline recorded; COOP-004 asked for final read-only tooling/evidence review. |

Only the coordinator adds or reassigns claims. A missing row is not sufficient evidence
of availability: check worktrees and existing task branches as well. Task records carry
detailed progress; this ledger locates their active owners and checkpoints.

## Review and integration queue

1. COOP-002 / PR #4, reviewed head `2ee1dca6`: coordinator read full contract, task/index/COOP-011 refinements, source coupling/rights/generation boundaries; reproduced two document check blocks (11 Markdown files, 68 local links/anchors; 11 task branch/dependency records; acyclic) and whitespace check. COOP-003 consumer review passed. Documentation-only acceptance satisfied; no compiled/runtime claims. Merged `9f59ff47`; actual done record in completion PR #5 `9aec1d7e`.
2. COOP-003 / PR #3, reviewed head `b4030c11`: coordinator read specification/resource audit/fixture/driver and latest prose-only corrections, checked source symbols, inspected generated host preview, reproduced browser smoke (all scenarios, 3 widths, zero page errors) and whitespace checks. COOP-002 independent full review passed. Documentation-only design acceptance satisfied; native rendering/input/network evidence remains downstream. Merged `dd2aa98e`; actual done record in completion PR #5 `9aec1d7e`.
3. COOP-001 / draft PR #2, final checkpoint `747d0590`: complete x86/x87 build; partial original Tutorial and actual native2-client Sicily2 movement/inventory/status baseline recorded; process/real MSVC worktree isolation verified. Firing/reload synthetic inputs unconfirmed, explicitly not passed; no mission-completion claim. Final read-only review requested from COOP-004 before ready/integration.

## Coordinator handoff

- Completed: fetched origin, checked all branch refs, worktrees and PRs; no prior COOP-001 or coordinator claim exists. Accepted the user-assigned COOP-001 claim.
- Active writers: COOP-001, COOP-004, COOP-005 and COOP-006 in independent worktrees; COOP-011 stopped after completion; COOP-002/003 stopped after integration, branches/worktrees preserved; only COOP-001 coordinator thread writes the ledger. COOP-003 kickoff row from 7f54ee71 is acknowledged and retained; asked its writer to stop ledger edits. Scope: build/run guidance and focused `_re/tools/` fixes; explicit toolchain override in engine build scripts if needed for portable MSVC; inspect all temporary output consumers; no gameplay or engine source changes.
- Blocked tasks/decisions: see individual task records and the campaign design.
- Next coordinator action: Continue COOP-001 native host/guest baseline. COOP-011 merged `0f367ed6`, done/readiness metadata PR #7 merged `4d6f8cc1`; independently repeated GCC/MSVC x86 conformance and negative cases passed. COOP-004/005 accepted user assignments; 006 accepted user assignment; 007 remains ready/unclaimed; other model/UI tasks wait for its merged declarations. PR #5 done metadata independently reviewed by both task workers; integrated `9aec1d7e`. Optional one-line PR label cleanup `da66c7a5` raced the merge and remains only on the preserved docs followup branch.
- Transfer acknowledgment: initial kickoff, no prior writer; do not infer a future handoff from inactivity.

Keep this record and task checkpoints accessible to successors. When moving between
machines, a local-only ledger or unpublished task branch must be transferred explicitly.

## Cross-task request journal boundary

COOP-004 owns a concrete bounded cross-family request journal in its reserved campaign_session files. COOP-005 may inject a narrow Begin/Finish journal boundary and use a behavioral fake while developing independently; no duplicate shared implementation/header and no dependency on an unmerged COOP-004 branch. Agree exact identity/content/result/expiry semantics before consumers bind. Production composition must use one shared authority queue and cross-family journal. Injection does not waive COOP-005 actual campaign equipment-rule/stock-conversion acceptance; keep the PR draft until its stated evidence is met or split a bounded foundation explicitly.
