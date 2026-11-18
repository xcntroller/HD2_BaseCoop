# COOP 004 Session and soldier ownership model

## Coordination

- Status: planned (user assigned; coordinator acknowledgment pending)
- Owner/session: Codex COOP-004 / thread `01a1ebf9-60c7-74d0-9d39-cd09295875b0`, 2026-11-18
- Canonical branch: `feature/coop-004-ownership`
- Base commit: `0f367ed62af337633941bca0d22d3ff1932129ba`
- Dependencies: COOP-002, COOP-011 (merged shared declarations)
- Last checkpoint: branch/worktree preparation (this commit)
- Checkpoint location: `.worktrees/COOP-004`; `origin/feature/coop-004-ownership`
- Review/integration: none
- Pull request: none

## Branch authority and completion PR

The assigned agent has full control of this task's branch: editing, committing, rebasing,
resolving conflicts, pushing to the project remote, and creating/updating its PR without
repeated user approval. Preserve other workers' work and shared checkpoints. When the
implementation and required validation are complete, push and create/update a PR targeting
`main`. Include reproducible **How to test** steps, prerequisites, and expected results;
separate checks actually run from proposed reviewer checks. State **Source-only** explicitly
when no runnable artifact is supplied, or **Documentation-only** for documentation tasks.
Record the PR URL in this handoff. Follow the [completion PR requirements](../agent_workflow.md#review-and-integration);
source-only delivery does not waive required runtime acceptance evidence.

## Result and scope

Implement the agreed session and soldier ownership model independently of live controllers
and transport. Consumers can resolve a player's soldier and authorize a control transfer
without treating the soldier's actor ID as a connection slot.

## Files and shared contracts

Own new ownership/session implementation files and focused tests at paths agreed in COOP-002.
Consume shared identity and transition contracts. Legacy controller and mission adapters
belong to COOP-008/009; avoid concurrent edits to those files.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own `src/game/campaign_session.{h,cpp}`,
`src/game/campaign_ownership.{h,cpp}`, and `_re/harness/campaign_ownership/`. Follow [the identity, lifecycle and ownership
contracts](../coop_contracts.md#identity-and-session-context), including connection
incarnations, per-soldier control revisions, script suspension and actor-binding invalidation.
Consume COOP-011 shared declarations; coordinate changes with all consumers before
editing that common header. Do not create independent incompatible copies.
No legacy controller/profile layout edits.

## Acceptance criteria

- Enforce one human owner per soldier, at most one soldier per player, and campaign squad limits.
- Handle claims, switching, disconnect, death, temporary scripted control, and mission reset
  according to the contract without changing a soldier's health, inventory, or identity.
- Reject invalid/duplicate/stale requests and ownership races. Verify mission-generation
  and ownership-revision boundaries with behavioral tests.
- Preserve ordinary singleplayer/native co-op behavior; no unfinished UI entry is exposed.
- Provide documented APIs and meaningful model tests. Runtime playability is not this
  module's deliverable; the live adapter and two-player gates remain separate.

## Handoff

- Completed: read design/workflow/contracts; fetched origin, checked claims/branches/PRs;
  created canonical branch and isolated worktree at merged COOP-011 foundation.
- Remaining: session/catalog/ownership implementation, bounded request journal, model tests,
  intended MSVC x86 compile/execution, separate review and completion PR.
- Next action: obtain coordinator ledger acknowledgment before substantial implementation.
  Requested from coordinator thread `01a1ebd1-3990-7553-920e-7cfee0148527`.
- Dependencies: COOP-002 merged `9f59ff47`; COOP-011 merged `0f367ed6`.
- Coordination: waiting for claim acknowledgment; coordinator ledger remains single-writer.
  COOP-005 agreed an injected request-journal boundary for cross-family sequences;
  shared declarations and legacy adapter files remain untouched.
- Decisions: model follows confirmed campaign behavior and the reviewed contracts.
- Validation: `git diff --check` passed for this preparation checkpoint; no implementation
  build or behavioral checks yet.
