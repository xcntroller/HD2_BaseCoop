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
| COOP-003 | Codex COOP-003 session (2026-11-18); explicitly assigned by user | `docs/coop-003-preparation-ui` | active; claim acknowledged by this ledger commit | Base `7e033add`; canonical task branch to be created | Audit campaign UI and produce specification/resource inventory/original mockup. Scope: `docs/ui/`, `docs/tasks/COOP-003.md`. Coordinate proposed model needs with COOP-002 through durable consumer notes. |
| COOP-002 | Codex COOP-002 / thread `01a1ebd2-902d-7e32-9876-9818a845d361`, user assignment | `feature/coop-002-contracts` | active; claim acknowledged by this commit | Base `7e033add`; `.worktrees/COOP-002` | Define docs/coop_contracts.md, task record and COOP-004..010 refinements; coordinate with COOP-003; no legacy gameplay edits. |
| COOP-001 | Codex / 2026-11-18 user assignment | `feature/coop-001-build-baseline` | active | Base `7e033adde95881baad06c6877cbbd6eef8197d96`; `.worktrees/COOP-001` | Inventory toolchain/data; build and verify baseline. |

Only the coordinator adds or reassigns claims. A missing row is not sufficient evidence
of availability: check worktrees and existing task branches as well. Task records carry
detailed progress; this ledger locates their active owners and checkpoints.

## Review and integration queue

No candidates yet. Record task, reviewed commit, dependency merges, reviewer/findings,
remaining validation, and proposed merge order. Integrate serially against current `main`.

## Coordinator handoff

- Completed: fetched origin, checked all branch refs, worktrees and PRs; no prior COOP-001 or coordinator claim exists. Accepted the user-assigned COOP-001 claim.
- Active writers: COOP-001, COOP-002 and COOP-003 in independent worktrees; only COOP-001 coordinator thread writes the ledger. COOP-003 kickoff row from 7f54ee71 is acknowledged and retained; asked its writer to stop ledger edits. Scope: build/run guidance and focused `_re/tools/` fixes; explicit toolchain override in engine build scripts if needed for portable MSVC; inspect all temporary output consumers; no gameplay or engine source changes.
- Blocked tasks/decisions: see individual task records and the campaign design.
- Next coordinator action: Record COOP-001 environment checkpoint; receive COOP-002/003 checkpoint and review requests through coordinator thread.
- Transfer acknowledgment: initial kickoff, no prior writer; do not infer a future handoff from inactivity.

Keep this record and task checkpoints accessible to successors. When moving between
machines, a local-only ledger or unpublished task branch must be transferred explicitly.
