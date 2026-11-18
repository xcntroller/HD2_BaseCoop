# COOP 005 Preparation state and loadout transactions

## Coordination

- Status: active (coordinator claim acknowledgment requested; no substantial implementation yet)
- Owner/session: Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, user assignment 2026-11-18
- Canonical branch: `feature/coop-005-preparation-model`
- Base commit: `4d6f8cc1bb96c50aee09d5c3ac0992ce716a3d39` (integrated `origin/main`)
- Dependencies: COOP-002, COOP-011 (merged shared declarations)
- Last checkpoint: this initial claim/audit checkpoint; implementation has not started
- Checkpoint location: `.worktrees/COOP-005`; `origin/feature/coop-005-preparation-model`
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

Implement the preparation model behind both the new UI and network adapters: ownership
permissions, committed soldier loadouts, and atomic changes to campaign stock. Preserve
singleplayer equipment rules while allowing simultaneous human editors.

## Files and shared contracts

Own new preparation implementation and focused tests. Consume COOP-002 request/result and
stock contracts. Audit existing item/soldier/equipment validation for reuse; coordinate any
changes to `equipment_screen.*` or `equipment_panel.*` with COOP-007. Prefer adapters over
an unrelated rewrite of the original singleplayer screen.

### COOP-002 scope refinement

Contract implementation scope from COOP-002: own `src/game/campaign_preparation.{h,cpp}`
and `_re/harness/campaign_preparation/`. Implement [the preparation boundary](../coop_contracts.md#preparation-model-and-ui-boundary)
with one aggregate preparation commit revision, a separate view revision for readiness,
atomic stock/loadout candidate commits, explicit bulk targets, and commit-time ownership
checks. Use the common RequestId deduplication rules. Consume the merged COOP-011
identity/DTO/interface declarations and inject an ownership catalog view; no dependency on the COOP-004 model implementation is required.
Reuse campaign edit/deploy validation semantics; direct panel/profile mutation is not the
new model API. Any legacy validator extraction requires coordinator scope acknowledgment
and COOP-007 notification before editing shared equipment files.

## Acceptance criteria

- Each human edits their own soldier; host edits their own and unclaimed AI soldiers only.
- Commit each edit atomically across loadout and stock. Concurrent requests for the last
  item cannot duplicate it or overwrite another soldier's state.
- Test stale/repeated requests, rejected/invalid equipment, bulk auto-equip permissions,
  ownership changes while editing, and disconnect without double returns to stock.
- Preserve equipment capacity and mission-essential checks. Implement revision/freeze and
  readiness semantics only as agreed in the contract; flag unresolved deployment policy.
- Expose a UI/transport-independent API and tests. No claim of a connected preparation
  screen is made until its later live synchronization gate passes.

## Validator audit (2026-11-18)

- `profile_squad.cpp::PutItem`/`UpdateLoad` own soldier weight, belt volume
  (12), backpack weight/volume and replacement checks. A replacement backpack must
  accommodate existing contents; single slots displace equipment and container
  additions may fit only partially. The transaction must commit only a fully
  validated candidate, including all displacement/returns.
- `equipment_panel.cpp::PutItemChecked` checks item/container compatibility and
  occupied single slots; `DropItem` adds partial-count/forced placement behavior.
  These methods mutate live squad/preview state and cannot be called on proposals.
- `equipment_screen.cpp::HasRequiredItems`/`CountCarried` are Continue checks:
  deployed carriers only, Axis items on one selected carrier, and required uniform/
  diving-gear counts capped by squad size. `IsSquadWithinWeight` blocks Continue;
  `IsSquadArmed` asks for confirmation rather than unconditionally rejecting edits.
- Campaign supply normalizes ammunition through `GetAmmoItemId` in `OnStart`.
  `SetupStore`/`TakeSquadItems`, preset remapping, `DistributeRequired` and
  `Replenish` mutate global stock/soldiers. Screen binding must never call these
  as model initialization or repeat normalization. Fractional stats remain floats.
- Plan: isolated atomic authority core with explicit injected equipment rules and
  deploy policy. A native validator/planner adapter requires a subsequent scoped
  extraction and parity evidence; fake rules do not establish native parity.

## Handoff

- Completed: fetched origin, checked no existing COOP-005 branch/worktree; created
  canonical branch/worktree from merged COOP-002/011. Read design, workflow,
  task index, preparation contract and shared declarations; audited validators above.
- Remaining: coordinator acknowledgment, atomic model, injected rules and shared
  request-journal boundary, behavioral harness, MSVC x86 validation, publication/PR.
- Next action: obtain coordinator acknowledgment, agree cross-family journal boundary
  with COOP-004, then implement isolated authority and immutable publications.
- Blockers: no product decision blocks isolated tests. Native adapter/parity and
  production deploy policies remain explicit gates; source-only draft until proven.
- Decisions: exact ownership permissions and aggregate/view revision semantics follow
  merged contract. No shared equipment/header/index/ledger edits are planned.
- Validation: read-only source/branch/dependency audit; no compile/runtime checks yet.
- Uncommitted work: none after this checkpoint.
