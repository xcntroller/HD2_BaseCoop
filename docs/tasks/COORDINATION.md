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
| COOP-011 | Codex COOP-011 / thread `01a1ebe7-2361-7503-8dcb-6e0739646604`, user assignment | `feature/coop-011-shared-declarations` | active; claim acknowledged by this commit | Base `9aec1d7e`; `.worktrees/COOP-011` | Shared `src/game/campaign_contracts.h`, `_re/harness/campaign_contracts/`, own task record; standalone MSVC x86 conformance, no engine/model/codec implementations. |
| COOP-001 | Codex / 2026-11-18 user assignment | `feature/coop-001-build-baseline` | active | Base `7e033adde95881baad06c6877cbbd6eef8197d96`; `.worktrees/COOP-001` | Inventory toolchain/data; build and verify baseline. |

Only the coordinator adds or reassigns claims. A missing row is not sufficient evidence
of availability: check worktrees and existing task branches as well. Task records carry
detailed progress; this ledger locates their active owners and checkpoints.

## Review and integration queue

1. COOP-002 / PR #4, reviewed head `2ee1dca6`: coordinator read full contract, task/index/COOP-011 refinements, source coupling/rights/generation boundaries; reproduced two document check blocks (11 Markdown files, 68 local links/anchors; 11 task branch/dependency records; acyclic) and whitespace check. COOP-003 consumer review passed. Documentation-only acceptance satisfied; no compiled/runtime claims. Merged `9f59ff47`; actual done record in completion PR #5 `9aec1d7e`.
2. COOP-003 / PR #3, reviewed head `b4030c11`: coordinator read specification/resource audit/fixture/driver and latest prose-only corrections, checked source symbols, inspected generated host preview, reproduced browser smoke (all scenarios, 3 widths, zero page errors) and whitespace checks. COOP-002 independent full review passed. Documentation-only design acceptance satisfied; native rendering/input/network evidence remains downstream. Merged `dd2aa98e`; actual done record in completion PR #5 `9aec1d7e`.
3. COOP-001 / draft PR #2, checkpoint `ca28d52c`: x86 executable and matching engine/DTA DLLs built with MSVC/Wine; runtime tests in progress. Not ready to merge.

## Coordinator handoff

- Completed: fetched origin, checked all branch refs, worktrees and PRs; no prior COOP-001 or coordinator claim exists. Accepted the user-assigned COOP-001 claim.
- Active writers: COOP-001 and COOP-011 in independent worktrees; COOP-002/003 stopped after integration, branches/worktrees preserved; only COOP-001 coordinator thread writes the ledger. COOP-003 kickoff row from 7f54ee71 is acknowledged and retained; asked its writer to stop ledger edits. Scope: build/run guidance and focused `_re/tools/` fixes; explicit toolchain override in engine build scripts if needed for portable MSVC; inspect all temporary output consumers; no gameplay or engine source changes.
- Blocked tasks/decisions: see individual task records and the campaign design.
- Next coordinator action: Continue COOP-001 native host/guest baseline. COOP-011 is claimed by the user-assigned worker; other model/UI tasks wait for its merged declarations. PR #5 done metadata independently reviewed by both task workers; integrated `9aec1d7e`. Optional one-line PR label cleanup `da66c7a5` raced the merge and remains only on the preserved docs followup branch.
- Transfer acknowledgment: initial kickoff, no prior writer; do not infer a future handoff from inactivity.

Keep this record and task checkpoints accessible to successors. When moving between
machines, a local-only ledger or unpublished task branch must be transferred explicitly.
