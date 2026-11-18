# COOP 005 Preparation state and loadout transactions

## Coordination

- Status: review (component checks passed; independent final review pending)
- Owner/session: Codex COOP-005 / thread `01a1ebf9-90e7-7ca2-8a1d-987711a3d8fe`, user assignment 2026-11-18
- Canonical branch: `feature/coop-005-preparation-model`
- Base commit: `4d6f8cc1bb96c50aee09d5c3ac0992ce716a3d39` (integrated `origin/main`)
- Dependencies: COOP-002, COOP-011 (merged shared declarations)
- Last checkpoint: this validated freeze-baseline follow-up; prior implementation `8dc87b0a`
- Checkpoint location: `.worktrees/COOP-005`; `origin/feature/coop-005-preparation-model`
- Review/integration: COOP-004 independent review of `8dc87b0a` passed apart from addressed freeze composition; final follow-up review pending
- Pull request: draft [#8](https://github.com/xcntroller/HD2_BaseCoop/pull/8)

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

- Claim: coordinator acknowledgment `f960e0c3`; canonical branch/worktree retained.
- Completed: atomic candidate transactions, permissions/control fences, explicit bulk
  targets, shared Begin/Finish journal injection agreed with COOP-004, immutable
  viewer snapshots, ownership/presence/rule readiness invalidation, separate revision
  domains, explicit deploy policy, freeze manifest and fail-closed membership changes.
  Concrete detached campaign rule catalog/evaluator/planner handles native capacity,
  placement, ammo/state variants, original essentials/carrier rules and supply mapping.
- Files: only `src/game/campaign_preparation.{h,cpp}`, own harness and this record.
  No shared header, legacy source, UI, index or generated-tier changes.
- Behavioral evidence: GCC C++17 / warnings-as-errors passed, latest output
  `build/campaign_preparation/gcc-wheke2z_`. Harness checks permissions/bulk rejection,
  last-item/stale/replay/conflict/expiry, invalid equipment/capacity, variant preservation,
  per-variant conservation, rollback, ownership races/disconnect, ready/freeze and overflow.
- Original-rule evidence: `legacy_source.py` extracts current original bodies into
  ignored invocation-local output. 810 capacity/backpack and 240 mission-essential/
  Axis-carrier cases compare concrete rules against those compiled source algorithms.
  Concrete transaction checks cover displaced-backpack rollback, backpack+contents
  return, ammunition supply conversion and bulk target permissions. See harness README.
- Target evidence: current implementation MSVC 19.44.35230 x86 /W4 /WX with SDK
  10.0.26100.0 under Wine 11.21 passed standalone header, behavior and original-rule
  parity: `build/campaign_preparation/msvc-t3_i3i0o`. Exact argv/environment/version
  and execute logs retained there. COOP-007 replacement tooling was borrowed read-only;
  all outputs and Wine prefix stayed private to COOP-005. Earlier historical runs
  `msvc-47sttfxj`, `msvc-kbhg9u46` and `msvc-z6ac4646` also passed.
- Commands: `python3 _re/harness/campaign_preparation/run.py --compiler gcc`;
  Windows `python _re\harness\campaign_preparation\run.py --compiler msvc`.
  Exact portable-Wine command, flags, prerequisites, expected output and queue/journal
  composition are in `_re/harness/campaign_preparation/README.md`.
- Environment change resolved: original borrowed COOP-001 tooling disappeared
  externally; COOP-007 reprovisioned pinned private tools. Current README points to
  `.worktrees/COOP-007/build/tooling/run-python.bat`; preserve those borrowed resources.
- Freeze integration: independent review found COOP-004 phase-only transitions
  advanced ownership/control revisions after manifest capture. COOP-004 corrected its
  Freeze/StartSimulation baseline separately. This model's optional trusted freeze
  callback verifies the exact unchanged ownership/control/participant cut before
  publishing an immutable owning manifest. Callback mismatches fail closed/resync;
  no stale manifest is exposed. Regression checks prior mission generation fields,
  full-roster input, host without soldier, scripted/dead AI and both freeze models.
- Independent review: COOP-004 read all initial implementation/header/differential
  harness and independently reran GCC successfully (810+240 parity cases). No other
  blocking finding; final callback/manifest changes are under final peer review.
- Remaining: final independent follow-up review and integration gates. Native
  catalog/transport/UI composition is downstream; this is a source-only component,
  not a connected preparation screen. No full game build/runtime claim is made.
- Decisions: selected records must be eligible/alive/deployed (selected for preparation,
  not actor-spawned); COOP-004 confirmed setup semantics. Production readiness/departure
  policies remain unresolved and return PolicyRequired. The fixture's all-ready quorum
  and explicit recipe auto-equip policy are labeled development inputs. Both optional
  result generation fields describe the complete current session epoch across families.
- Next action: review this published follow-up, then mark PR ready and integrate only
  when independent acceptance passes; record actual merge commit afterward.
- Uncommitted work: none after this checkpoint; build outputs/logs remain ignored.
