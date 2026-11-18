#include "campaign_preparation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace {
using R = E_CampaignReason;
using F = E_CampaignRequestFamily;
template<class T, class P> auto Find(T &values, P predicate) { return std::find_if(values.begin(), values.end(), predicate); }
template<class T> bool Unique(const std::vector<T> &values) {
    std::set<T> seen;
    for (const auto &v : values) if (!v.IsValid() || !seen.insert(v).second) return false;
    return true;
}
bool SameVariant(const S_CampaignItemState &a, const S_CampaignItemState &b) {
    return a.m_Item == b.m_Item && a.m_AmmoType == b.m_AmmoType && a.m_Rounds == b.m_Rounds &&
        a.m_Charges == b.m_Charges && a.m_Condition == b.m_Condition;
}
bool ValidFloat(const std::optional<float> &value) { return !value || (std::isfinite(*value) && *value >= 0); }
void Number(std::string &out, std::uint64_t value) { out += std::to_string(value) + ";"; }
template<class T> void Token(std::string &out, T value) { Number(out, value.Value()); }
void Key(std::string &out, const std::string &key) { Number(out, key.size()); out += key; }
void Place(std::string &out, const S_CampaignPlacement &p) { Key(out, p.m_Slot.Value()); Number(out, p.m_Position); }
std::string Canonical(const S_CampaignPreparationRequestContext &c, F family) {
    std::string out;
    Number(out, static_cast<std::uint64_t>(family));
    Token(out, c.m_Request.m_Session); Token(out, c.m_Request.m_PlayerEcho); Token(out, c.m_Request.m_Request);
    Token(out, c.m_Generation); Token(out, c.m_ExpectedPreparationRevision);
    return out;
}
std::string Canonical(const S_CampaignEditRequest &r) {
    auto out = Canonical(r.m_Context, F::Edit);
    Number(out, r.m_Targets.size());
    for (const auto &t : r.m_Targets) { Token(out, t.m_Soldier); Token(out, t.m_ExpectedControlRevision); }
    Number(out, r.m_Operation.index());
    std::visit([&](const auto &op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, S_CampaignApplyPreset>) Key(out, op.m_Preset.Value());
        else if constexpr (std::is_same_v<T, S_CampaignAutoEquip>) {
            Key(out, op.m_Policy.Value()); Number(out, op.m_PreferredItems.size());
            for (const auto &key : op.m_PreferredItems) Key(out, key.Value());
        } else {
            Token(out, op.m_Soldier);
            if constexpr (std::is_same_v<T, S_CampaignTakeFromStock>) Token(out, op.m_StockEntry);
            else Token(out, op.m_Entry);
            if constexpr (!std::is_same_v<T, S_CampaignReturnToStock>) Place(out, op.m_Destination);
            Number(out, op.m_Count);
        }
    }, r.m_Operation);
    return out;
}
S_CampaignFinding Blocker(R reason) {
    S_CampaignFinding f; f.m_Reason = reason; f.m_Severity = E_CampaignFindingSeverity::DeploymentBlocker;
    return f;
}
bool SameParticipants(const std::vector<S_CampaignParticipant> &a, const std::vector<S_CampaignParticipant> &b) {
    if (a.size() != b.size()) return false;
    for (const auto &p : a) {
        auto q = Find(b, [&](const auto &v) { return v.m_Player == p.m_Player; });
        if (q == b.end() || p.m_Connected != q->m_Connected || p.m_Admitted != q->m_Admitted ||
            p.m_DisplayName != q->m_DisplayName) return false;
    }
    return true;
}
}

C_CampaignPreparation::C_CampaignPreparation(S_CampaignPreparationSnapshot initial,
    const I_CampaignOwnershipView &ownership, const I_CampaignEquipmentRules &rules,
    const I_CampaignDeploymentPolicy &policy, S_CampaignPreparationJournal journal,
    S_CampaignPreparationLimits limits, std::function<E_CampaignReason()> freezeOwnership)
    : m_State(std::move(initial)), m_Ownership(ownership), m_Rules(rules), m_Policy(policy),
      m_Journal(std::move(journal)), m_Limits(limits), m_FreezeOwnership(std::move(freezeOwnership)) {
    if (!m_Journal.m_Begin || !m_Journal.m_Finish || !limits.m_MaxCount || !limits.m_MaxSteps ||
        !limits.m_MaxEntries || !limits.m_MaxKeyBytes || !limits.m_MaxCanonicalBytes ||
        limits.m_MaxCount > 32767 || limits.m_MaxCanonicalBytes > 4096 || !m_State.m_Context.m_Session.IsValid() ||
        !m_State.m_Context.m_Campaign.IsValid() || !m_State.m_Context.m_Mission.IsValid() ||
        m_State.m_Context.m_Kind != E_CampaignSessionKind::CampaignCoop ||
        !m_State.m_Context.m_IsTransportServer || !m_State.m_Context.m_UsesCampaignRules ||
        m_State.m_Context.m_Phase != E_CampaignPhase::Preparation ||
        !m_State.m_Context.m_HostPlayer || !m_State.m_Context.m_HostPlayer->IsValid() ||
        !m_State.m_Generation.IsValid() || !m_State.m_PreparationRevision.IsValid() || !m_State.m_ViewRevision.IsValid())
        throw std::invalid_argument("invalid preparation authority initialization");
    if (RefreshOwnership(true) != R::None || ValidateState(m_State) != R::None)
        throw std::invalid_argument("invalid normalized preparation/ownership catalog");
    for (const auto &l : m_State.m_Loadouts) for (const auto &i : l.m_Items) {
        if (i.m_Entry.Value() == std::numeric_limits<std::uint64_t>::max())
            throw std::invalid_argument("preparation item identity exhausted");
        m_NextEntry = std::max(m_NextEntry, i.m_Entry.Value() + 1);
    }
    Evaluate(m_State);
    if (std::any_of(m_State.m_Findings.begin(), m_State.m_Findings.end(), [](const auto &f) {
        return f.m_Severity == E_CampaignFindingSeverity::EditRejection;
    })) throw std::invalid_argument("invalid initial campaign equipment");
    m_State.m_Connection = E_CampaignConnectionStatus::Admitted;
    m_State.m_Sync = E_CampaignSyncStatus::Current;
}

bool C_CampaignPreparation::CanAdvance(bool preparation) const {
    const auto max = std::numeric_limits<std::uint64_t>::max();
    return m_State.m_ViewRevision.Value() < max && (!preparation || m_State.m_PreparationRevision.Value() < max);
}
void C_CampaignPreparation::Advance(S_CampaignPreparationSnapshot &state, bool preparation) const {
    state.m_ViewRevision = ViewRevision(state.m_ViewRevision.Value() + 1);
    if (preparation) {
        state.m_PreparationRevision = PreparationRevision(state.m_PreparationRevision.Value() + 1);
        for (auto &p : state.m_Participants) p.m_ReadyAt.reset();
    }
}

R C_CampaignPreparation::RefreshOwnership(bool initializing) {
    auto current = m_Ownership.GetSnapshot();
    if (!current || current->m_Context.m_Session != m_State.m_Context.m_Session ||
        current->m_Context.m_Campaign != m_State.m_Context.m_Campaign ||
        current->m_Context.m_Mission != m_State.m_Context.m_Mission ||
        current->m_Context.m_HostPlayer != m_State.m_Context.m_HostPlayer || !current->m_Revision.IsValid()) return R::SnapshotUnavailable;
    if (current->m_Context.m_MissionGeneration != m_State.m_Context.m_MissionGeneration ||
        (current->m_Context.m_Phase != E_CampaignPhase::Preparation &&
         !(m_State.m_Context.m_Phase == E_CampaignPhase::Frozen && current->m_Context.m_Phase == E_CampaignPhase::Frozen))) return R::InvalidPhase;
    if (!initializing && current->m_Revision == m_State.m_OwnershipRevision &&
        SameParticipants(current->m_Participants, m_State.m_Participants)) return R::None;
    if (!initializing && (current->m_Revision < m_State.m_OwnershipRevision || !CanAdvance(true))) return R::SnapshotUnavailable;
    if (!initializing && m_State.m_Context.m_Phase != E_CampaignPhase::Preparation) return R::InvalidPhase;
    if (current->m_Context.m_Phase != E_CampaignPhase::Preparation || current->m_Participants.size() > 4 ||
        current->m_Soldiers.size() > m_Limits.m_MaxEntries || m_State.m_Loadouts.size() != m_State.m_Soldiers.size()) return R::SnapshotUnavailable;
    std::vector<PlayerId> players, owners;
    for (const auto &p : current->m_Participants) players.push_back(p.m_Player);
    if (!Unique(players)) return R::SnapshotUnavailable;
    auto host = Find(current->m_Participants, [&](const auto &p) { return p.m_Player == *m_State.m_Context.m_HostPlayer; });
    if (host == current->m_Participants.end() || !host->m_Connected || !host->m_Admitted) return R::NotAdmitted;
    std::vector<SoldierId> soldiers;
    for (const auto &s : current->m_Soldiers) {
        soldiers.push_back(s.m_Soldier);
        if (!s.m_ControlRevision.IsValid()) return R::SnapshotUnavailable;
        if (s.m_Owner) {
            auto p = Find(current->m_Participants, [&](const auto &v) { return v.m_Player == *s.m_Owner; });
            if (p == current->m_Participants.end() || !p->m_Admitted || !p->m_Connected) return R::SnapshotUnavailable;
            owners.push_back(*s.m_Owner);
        }
    }
    if (!Unique(soldiers) || !Unique(owners)) return R::SnapshotUnavailable;
    for (const auto &load : m_State.m_Loadouts)
        if (Find(current->m_Soldiers, [&](const auto &v) { return v.m_Soldier == load.m_Soldier; }) == current->m_Soldiers.end()) return R::SnapshotUnavailable;
    for (const auto &presentation : m_State.m_Soldiers)
        if (Find(current->m_Soldiers, [&](const auto &v) { return v.m_Soldier == presentation.m_Soldier; }) == current->m_Soldiers.end()) return R::SnapshotUnavailable;
    auto candidate = m_State;
    candidate.m_OwnershipRevision = current->m_Revision;
    candidate.m_Participants = current->m_Participants;
    for (auto &p : candidate.m_Participants) p.m_ReadyAt.reset();
    for (auto &s : candidate.m_Soldiers)
        s.m_Ownership = *Find(current->m_Soldiers, [&](const auto &v) { return v.m_Soldier == s.m_Soldier; });
    if (!initializing) { Advance(candidate, true); Evaluate(candidate); }
    m_State = std::move(candidate);
    return R::None;
}
R C_CampaignPreparation::SynchronizeOwnership() {
    const auto reason = RefreshOwnership();
    if (reason != R::None) { m_Invalidated = true; m_FrozenManifest.reset(); }
    return reason;
}

R C_CampaignPreparation::RevalidateRules() {
    if (m_Invalidated) return R::SnapshotUnavailable;
    if (m_State.m_Context.m_Phase != E_CampaignPhase::Preparation) return R::Frozen;
    if (!CanAdvance(true)) return R::SnapshotUnavailable;
    auto candidate = m_State;
    Advance(candidate, true); Evaluate(candidate); m_State = std::move(candidate);
    return R::None;
}

R C_CampaignPreparation::CanEdit(PlayerId sender, SoldierId soldier) const {
    if (m_Invalidated) return R::SnapshotUnavailable;
    if (m_State.m_Context.m_Phase == E_CampaignPhase::Frozen) return R::Frozen;
    if (m_State.m_Context.m_Phase != E_CampaignPhase::Preparation) return R::InvalidPhase;
    auto p = Find(m_State.m_Participants, [&](const auto &v) { return v.m_Player == sender; });
    if (p == m_State.m_Participants.end() || !p->m_Connected || !p->m_Admitted) return R::NotAdmitted;
    auto s = Find(m_State.m_Soldiers, [&](const auto &v) { return v.m_Soldier == soldier; });
    if (s == m_State.m_Soldiers.end()) return R::InvalidSoldier;
    const auto &o = s->m_Ownership;
    if (!o.m_Eligible) return R::Ineligible;
    if (!o.m_Deployed) return R::NotDeployed;
    if (!o.m_Alive) return R::Dead;
    if (o.m_Scripted) return R::Scripted;
    if (o.m_Owner == sender || (!o.m_Owner && m_State.m_Context.m_HostPlayer == sender)) return R::None;
    return R::NotOwner;
}

T_CampaignPreparationSnapshot C_CampaignPreparation::GetSnapshot(PlayerId viewer) const {
    auto copy = m_State;
    copy.m_Context.m_LocalPlayer = viewer;
    for (auto &s : copy.m_Soldiers) {
        const auto reason = CanEdit(viewer, s.m_Soldier);
        s.m_CanEdit = {reason == R::None, reason};
        auto p = Find(copy.m_Participants, [&](const auto &v) { return v.m_Player == viewer; });
        const bool admitted = p != copy.m_Participants.end() && p->m_Connected && p->m_Admitted;
        s.m_CanView = {admitted && !m_Invalidated, admitted ? R::None : R::NotAdmitted};
        // Ownership mutation capabilities belong to the ownership policy adapter.
        s.m_CanClaim = {false, R::PolicyRequired}; s.m_CanRelease = {false, R::PolicyRequired};
    }
    if (viewer != copy.m_Context.m_HostPlayer) copy.m_CanDeploy = {false, R::NotHost};
    if (m_Invalidated) {
        copy.m_Sync = E_CampaignSyncStatus::Resyncing;
        copy.m_CanDeploy = {false, R::SnapshotUnavailable};
    }
    return std::make_shared<const S_CampaignPreparationSnapshot>(std::move(copy));
}

R C_CampaignPreparation::Gate(PlayerId sender, const S_CampaignPreparationRequestContext &c) {
    if (!sender.IsValid() || c.m_Request.m_PlayerEcho != sender) return R::NotAdmitted;
    if (c.m_Request.m_Session != m_State.m_Context.m_Session) return R::WrongSession;
    if (c.m_Generation != m_State.m_Generation) return R::WrongGeneration;
    if (!c.m_Request.m_Request.IsValid()) return R::Malformed;
    if (m_Invalidated) return R::SnapshotUnavailable;
    auto sync = SynchronizeOwnership();
    if (sync != R::None) return sync;
    auto p = Find(m_State.m_Participants, [&](const auto &v) { return v.m_Player == sender; });
    return p != m_State.m_Participants.end() && p->m_Admitted && p->m_Connected ? R::None : R::NotAdmitted;
}

R C_CampaignPreparation::ValidateShape(const S_CampaignEditRequest &r) const {
    if (r.m_Targets.empty() || r.m_Targets.size() > m_State.m_Loadouts.size()) return R::InvalidSoldier;
    std::vector<SoldierId> ids;
    for (const auto &t : r.m_Targets) { ids.push_back(t.m_Soldier); if (!t.m_ExpectedControlRevision.IsValid()) return R::Malformed; }
    if (!Unique(ids)) return R::Malformed;
    const auto keyValid = [&](const auto &key) { return key.IsValid() && key.Value().size() <= m_Limits.m_MaxKeyBytes; };
    return std::visit([&](const auto &op) -> R {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, S_CampaignApplyPreset>) return keyValid(op.m_Preset) ? R::None : R::Malformed;
        else if constexpr (std::is_same_v<T, S_CampaignAutoEquip>) {
            if (!keyValid(op.m_Policy) || op.m_PreferredItems.size() > m_Limits.m_MaxSteps) return R::Malformed;
            for (const auto &key : op.m_PreferredItems) if (!keyValid(key)) return R::Malformed;
        } else {
            if (r.m_Targets.size() != 1 || r.m_Targets[0].m_Soldier != op.m_Soldier) return R::InvalidSoldier;
            if (!op.m_Count || op.m_Count > m_Limits.m_MaxCount) return R::InvalidCount;
            if constexpr (std::is_same_v<T, S_CampaignTakeFromStock>) { if (!op.m_StockEntry.IsValid()) return R::InvalidItem; }
            else { if (!op.m_Entry.IsValid()) return R::InvalidItem; }
            if constexpr (!std::is_same_v<T, S_CampaignReturnToStock>) if (!keyValid(op.m_Destination.m_Slot)) return R::InvalidPlacement;
        }
        return R::None;
    }, r.m_Operation);
}
R C_CampaignPreparation::CheckTargets(PlayerId sender, const S_CampaignEditRequest &r) const {
    for (const auto &t : r.m_Targets) {
        auto reason = CanEdit(sender, t.m_Soldier);
        if (reason != R::None) return reason;
        auto s = Find(m_State.m_Soldiers, [&](const auto &v) { return v.m_Soldier == t.m_Soldier; });
        if (s->m_Ownership.m_ControlRevision != t.m_ExpectedControlRevision) return R::StaleRevision;
    }
    return R::None;
}

R C_CampaignPreparation::ValidateState(const S_CampaignPreparationSnapshot &s) const {
    if (s.m_Stock.size() > m_Limits.m_MaxEntries || s.m_Loadouts.size() > 4) return R::InvalidCount;
    auto valid = [&](const S_CampaignItemState &i, bool stock) {
        return i.m_Item.IsValid() && i.m_Item.Value().size() <= m_Limits.m_MaxKeyBytes &&
            (stock || i.m_Quantity) && i.m_Quantity <= m_Limits.m_MaxCount &&
            (!i.m_AmmoType || (i.m_AmmoType->IsValid() && i.m_AmmoType->Value().size() <= m_Limits.m_MaxKeyBytes)) &&
            (!i.m_Rounds || *i.m_Rounds <= m_Limits.m_MaxCount) && ValidFloat(i.m_Charges) && ValidFloat(i.m_Condition);
    };
    std::vector<StockEntryId> stock;
    std::vector<SoldierId> soldiers, presentations, slots;
    std::vector<PreparationItemId> entries;
    for (const auto &i : s.m_Stock) { if (!valid(i.m_State, true)) return R::InvalidItem; stock.push_back(i.m_Entry); }
    for (const auto &l : s.m_Loadouts) {
        soldiers.push_back(l.m_Soldier);
        for (const auto &i : l.m_Items) {
            if (!valid(i.m_State, false)) return R::InvalidItem;
            if (!i.m_Placement.m_Slot.IsValid() || i.m_Placement.m_Slot.Value().size() > m_Limits.m_MaxKeyBytes) return R::InvalidPlacement;
            entries.push_back(i.m_Entry);
        }
        for (const auto &skill : l.m_Skills) if (!skill.m_Key.IsValid() || !std::isfinite(skill.m_Value)) return R::InvalidItem;
    }
    for (const auto &presentation : s.m_Soldiers) presentations.push_back(presentation.m_Soldier);
    for (const auto &slot : s.m_Slots) if (slot.m_Soldier) slots.push_back(*slot.m_Soldier);
    if (entries.size() > m_Limits.m_MaxEntries) return R::InvalidCount;
    if (!Unique(stock) || !Unique(soldiers) || !Unique(entries) || !Unique(presentations) || !Unique(slots)) return R::Malformed;
    std::sort(soldiers.begin(), soldiers.end()); std::sort(presentations.begin(), presentations.end()); std::sort(slots.begin(), slots.end());
    return soldiers == presentations && soldiers == slots ? R::None : R::InvalidSoldier;
}

R C_CampaignPreparation::ApplyStep(S_CampaignPreparationSnapshot &s,
    const T_CampaignPreparationStep &step, std::uint64_t &nextEntry) const {
    return std::visit([&](const auto &op) -> R {
        using T = std::decay_t<decltype(op)>;
        if (!op.m_Count || op.m_Count > m_Limits.m_MaxCount) return R::InvalidCount;
        auto load = Find(s.m_Loadouts, [&](const auto &l) { return l.m_Soldier == op.m_Soldier; });
        if (load == s.m_Loadouts.end()) return R::InvalidSoldier;
        auto add = [&](S_CampaignItemState state, const S_CampaignPlacement &where) -> R {
            if (!where.m_Slot.IsValid() || where.m_Slot.Value().size() > m_Limits.m_MaxKeyBytes) return R::InvalidPlacement;
            if (nextEntry == std::numeric_limits<std::uint64_t>::max()) return R::SnapshotUnavailable;
            state.m_Quantity = op.m_Count;
            load->m_Items.push_back({PreparationItemId(nextEntry++), std::move(state), where});
            return R::None;
        };
        if constexpr (std::is_same_v<T, S_CampaignTakeFromStock>) {
            auto stock = Find(s.m_Stock, [&](const auto &v) { return v.m_Entry == op.m_StockEntry; });
            if (stock == s.m_Stock.end()) return R::InvalidItem;
            if (stock->m_State.m_Quantity < op.m_Count) return R::StockUnavailable;
            auto reason = add(stock->m_State, op.m_Destination);
            if (reason == R::None) stock->m_State.m_Quantity -= op.m_Count;
            return reason;
        } else {
            auto item = Find(load->m_Items, [&](const auto &v) { return v.m_Entry == op.m_Entry; });
            if (item == load->m_Items.end()) return R::InvalidItem;
            if (op.m_Count > item->m_State.m_Quantity) return R::InvalidCount;
            if constexpr (std::is_same_v<T, S_CampaignReturnToStock>) {
                auto stock = Find(s.m_Stock, [&](const auto &v) { return SameVariant(v.m_State, item->m_State); });
                // Normalization supplies even zero-count variants, so returns
                // never manufacture an unchecked stock variant or lose condition.
                if (stock == s.m_Stock.end()) return R::InvalidItem;
                if (op.m_Count > m_Limits.m_MaxCount - stock->m_State.m_Quantity) return R::InvalidCount;
                stock->m_State.m_Quantity += op.m_Count;
                item->m_State.m_Quantity -= op.m_Count;
                if (!item->m_State.m_Quantity) load->m_Items.erase(item);
                return R::None;
            } else {
                if (op.m_Count == item->m_State.m_Quantity) { item->m_Placement = op.m_Destination; return R::None; }
                auto state = item->m_State;
                item->m_State.m_Quantity -= op.m_Count;
                return add(std::move(state), op.m_Destination);
            }
        }
    }, step);
}

void C_CampaignPreparation::Evaluate(S_CampaignPreparationSnapshot &state) const {
    auto evaluation = m_Rules.Evaluate(state);
    state.m_Essentials = std::move(evaluation.m_Essentials);
    state.m_Findings = std::move(evaluation.m_Findings);
    state.m_DeployBlockers = std::move(evaluation.m_DeployBlockers);
    if (evaluation.m_EditRejection != R::None) {
        auto f = Blocker(evaluation.m_EditRejection); f.m_Severity = E_CampaignFindingSeverity::EditRejection;
        state.m_Findings.push_back(f);
    }
    for (auto &load : state.m_Loadouts) {
        auto metrics = Find(evaluation.m_LoadoutMetrics, [&](const auto &v) { return v.m_Soldier == load.m_Soldier; });
        if (metrics != evaluation.m_LoadoutMetrics.end()) {
            auto items = std::move(load.m_Items); auto skills = std::move(load.m_Skills);
            load = *metrics; load.m_Items = std::move(items); load.m_Skills = std::move(skills);
        }
    }
    for (const auto &e : state.m_Essentials) if (e.m_Deficit) state.m_DeployBlockers.push_back(Blocker(R::RequiredItemsMissing));
    for (const auto &f : state.m_Findings) if (f.m_Severity == E_CampaignFindingSeverity::DeploymentBlocker)
        state.m_DeployBlockers.push_back(f);
    if (evaluation.m_EditRejection != R::None) state.m_DeployBlockers.push_back(Blocker(evaluation.m_EditRejection));
    auto policy = m_Policy.Evaluate(state);
    state.m_DeployBlockers.insert(state.m_DeployBlockers.end(), policy.m_Blockers.begin(), policy.m_Blockers.end());
    if (policy.m_Reason != R::None) state.m_DeployBlockers.push_back(Blocker(policy.m_Reason));
    if (state.m_Context.m_Phase == E_CampaignPhase::Frozen) state.m_CanDeploy = {false, R::Frozen};
    else state.m_CanDeploy = {state.m_DeployBlockers.empty(), state.m_DeployBlockers.empty() ? R::None : state.m_DeployBlockers.front().m_Reason};
}

S_CampaignRequestResult C_CampaignPreparation::Result(const S_CampaignPreparationRequestContext &c, F family, R reason) const {
    S_CampaignRequestResult result;
    result.m_Context = c.m_Request; result.m_Family = family;
    result.m_Outcome = reason == R::None ? E_CampaignRequestOutcome::Accepted : E_CampaignRequestOutcome::Rejected;
    result.m_Reason = reason; result.m_PreparationGeneration = m_State.m_Generation;
    if (m_State.m_Context.m_MissionGeneration.IsValid()) result.m_MissionGeneration = m_State.m_Context.m_MissionGeneration;
    result.m_OwnershipRevision = m_State.m_OwnershipRevision;
    result.m_PreparationRevision = m_State.m_PreparationRevision; result.m_ViewRevision = m_State.m_ViewRevision;
    if (m_Invalidated || reason == R::WrongSession || reason == R::NotAdmitted) {
        S_CampaignResyncDescriptor resync;
        resync.m_Session = m_State.m_Context.m_Session; resync.m_PreparationGeneration = m_State.m_Generation;
        if (m_State.m_Context.m_MissionGeneration.IsValid()) resync.m_MissionGeneration = m_State.m_Context.m_MissionGeneration;
        resync.m_OwnershipRevision = m_State.m_OwnershipRevision; resync.m_PreparationRevision = m_State.m_PreparationRevision;
        resync.m_ViewRevision = m_State.m_ViewRevision; resync.m_Reason = reason;
        result.m_State = resync;
    } else result.m_State = GetSnapshot(c.m_Request.m_PlayerEcho);
    result.m_Findings = m_State.m_Findings;
    return result;
}
S_CampaignRequestResult C_CampaignPreparation::Finish(const S_CampaignPreparationRequestContext &c,
    F family, const std::string &content, R reason) {
    auto result = Result(c, family, reason);
    m_Journal.m_Finish(c.m_Request, family, content, result);
    return result;
}

S_CampaignRequestResult C_CampaignPreparation::ApplyEdit(PlayerId sender, const S_CampaignEditRequest &r) {
    auto reason = Gate(sender, r.m_Context);
    if (reason != R::None) return Result(r.m_Context, F::Edit, reason);
    // Bounded shape validation precedes canonicalization/journal storage. The
    // codec/dispatcher must reject oversized frames before materializing DTOs.
    if (r.m_Operation.valueless_by_exception()) return Result(r.m_Context, F::Edit, R::Malformed);
    bool oversized = r.m_Targets.size() > 4;
    std::visit([&](const auto &op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, S_CampaignApplyPreset>) oversized |= op.m_Preset.Value().size() > m_Limits.m_MaxKeyBytes;
        else if constexpr (std::is_same_v<T, S_CampaignAutoEquip>) {
            oversized |= op.m_Policy.Value().size() > m_Limits.m_MaxKeyBytes || op.m_PreferredItems.size() > m_Limits.m_MaxSteps;
            for (const auto &key : op.m_PreferredItems) oversized |= key.Value().size() > m_Limits.m_MaxKeyBytes;
        } else if constexpr (!std::is_same_v<T, S_CampaignReturnToStock>) oversized |= op.m_Destination.m_Slot.Value().size() > m_Limits.m_MaxKeyBytes;
    }, r.m_Operation);
    if (oversized) return Result(r.m_Context, F::Edit, R::Malformed);
    auto content = Canonical(r);
    if (content.size() > m_Limits.m_MaxCanonicalBytes) return Result(r.m_Context, F::Edit, R::Malformed);
    auto replay = m_Journal.m_Begin(r.m_Context.m_Request, F::Edit, content, Result(r.m_Context, F::Edit, R::None));
    if (replay) return *replay;
    reason = ValidateShape(r);
    if (reason == R::None) {
        if (m_State.m_Context.m_Phase == E_CampaignPhase::Frozen) reason = R::Frozen;
        else if (r.m_Context.m_ExpectedPreparationRevision != m_State.m_PreparationRevision) reason = R::StaleRevision;
        else reason = CheckTargets(sender, r);
    }
    if (reason != R::None) return Finish(r.m_Context, F::Edit, content, reason);
    auto plan = m_Rules.Plan(m_State, r);
    if (plan.m_Reason != R::None) return Finish(r.m_Context, F::Edit, content, plan.m_Reason);
    if (plan.m_Steps.size() > m_Limits.m_MaxSteps) return Finish(r.m_Context, F::Edit, content, R::InvalidCount);
    auto candidate = m_State;
    auto nextEntry = m_NextEntry;
    for (const auto &step : plan.m_Steps) {
        const auto soldier = std::visit([](const auto &op) { return op.m_Soldier; }, step);
        if (Find(r.m_Targets, [&](const auto &t) { return t.m_Soldier == soldier; }) == r.m_Targets.end()) { reason = R::NotOwner; break; }
        reason = ApplyStep(candidate, step, nextEntry);
        if (reason != R::None) break;
    }
    if (reason == R::None) reason = ValidateState(candidate);
    if (reason == R::None && !CanAdvance(true)) reason = R::SnapshotUnavailable;
    if (reason == R::None) {
        Advance(candidate, true); Evaluate(candidate);
        auto rejection = Find(candidate.m_Findings, [](const auto &f) { return f.m_Severity == E_CampaignFindingSeverity::EditRejection; });
        if (rejection != candidate.m_Findings.end()) reason = rejection->m_Reason;
    }
    // Trust boundary check again after planning/evaluation; injected callbacks
    // must not reenter authority. A changed external ownership cut aborts.
    auto ownershipNow = m_Ownership.GetSnapshot();
    if (reason == R::None && (!ownershipNow || ownershipNow->m_Revision != m_State.m_OwnershipRevision ||
        !SameParticipants(ownershipNow->m_Participants, m_State.m_Participants))) {
        reason = R::StaleRevision; m_Invalidated = true; m_FrozenManifest.reset();
    }
    if (reason == R::None) {
        m_State = std::move(candidate); m_NextEntry = nextEntry;
    }
    return Finish(r.m_Context, F::Edit, content, reason);
}

S_CampaignRequestResult C_CampaignPreparation::SetReady(PlayerId sender, const S_CampaignReadyRequest &r) {
    auto reason = Gate(sender, r.m_Context);
    if (reason != R::None) return Result(r.m_Context, F::Ready, reason);
    auto content = Canonical(r.m_Context, F::Ready); Number(content, r.m_Ready ? 1 : 0);
    auto replay = m_Journal.m_Begin(r.m_Context.m_Request, F::Ready, content, Result(r.m_Context, F::Ready, R::None));
    if (replay) return *replay;
    if (m_State.m_Context.m_Phase != E_CampaignPhase::Preparation) reason = R::Frozen;
    else if (r.m_Context.m_ExpectedPreparationRevision != m_State.m_PreparationRevision) reason = R::StaleRevision;
    auto participant = Find(m_State.m_Participants, [&](const auto &p) { return p.m_Player == sender; });
    const auto ready = r.m_Ready ? std::make_optional(m_State.m_PreparationRevision) : std::nullopt;
    if (reason == R::None && participant->m_ReadyAt != ready) {
        if (!CanAdvance(false)) reason = R::SnapshotUnavailable;
        else {
            auto candidate = m_State;
            Find(candidate.m_Participants, [&](const auto &p) { return p.m_Player == sender; })->m_ReadyAt = ready;
            Advance(candidate, false); Evaluate(candidate); m_State = std::move(candidate);
        }
    }
    return Finish(r.m_Context, F::Ready, content, reason);
}

S_CampaignRequestResult C_CampaignPreparation::RequestDeploy(PlayerId sender, const S_CampaignDeployRequest &r) {
    auto reason = Gate(sender, r.m_Context);
    if (reason != R::None) return Result(r.m_Context, F::Deploy, reason);
    auto content = Canonical(r.m_Context, F::Deploy);
    auto replay = m_Journal.m_Begin(r.m_Context.m_Request, F::Deploy, content, Result(r.m_Context, F::Deploy, R::None));
    if (replay) return *replay;
    if (m_State.m_Context.m_HostPlayer != sender) reason = R::NotHost;
    else if (m_State.m_Context.m_Phase != E_CampaignPhase::Preparation) reason = R::Frozen;
    else if (r.m_Context.m_ExpectedPreparationRevision != m_State.m_PreparationRevision) reason = R::StaleRevision;
    auto candidate = m_State;
    if (reason == R::None) {
        Evaluate(candidate);
        if (!candidate.m_CanDeploy.m_Allowed) reason = candidate.m_CanDeploy.m_Reason;
    }
    auto policy = m_Policy.Evaluate(candidate);
    if (reason == R::None) {
        if (!Unique(policy.m_RequiredParticipants) || policy.m_RequiredParticipants.empty()) reason = R::PolicyRequired;
        for (const auto &id : policy.m_RequiredParticipants) {
            auto p = Find(candidate.m_Participants, [&](const auto &v) { return v.m_Player == id; });
            if (p == candidate.m_Participants.end() || !p->m_Admitted || !p->m_Connected) reason = R::NotAdmitted;
        }
    }
    auto ownershipNow = m_Ownership.GetSnapshot();
    if (reason == R::None && (!ownershipNow || ownershipNow->m_Revision != candidate.m_OwnershipRevision ||
        !SameParticipants(ownershipNow->m_Participants, candidate.m_Participants))) {
        reason = R::StaleRevision; m_Invalidated = true; m_FrozenManifest.reset();
    }
    if (reason == R::None && !CanAdvance(false)) reason = R::SnapshotUnavailable;
    if (reason == R::None && m_FreezeOwnership) {
        reason = m_FreezeOwnership();
        auto frozen = m_Ownership.GetSnapshot();
        bool valid = reason == R::None && frozen && frozen->m_Context.m_Phase == E_CampaignPhase::Frozen &&
            frozen->m_Context.m_Session == candidate.m_Context.m_Session &&
            frozen->m_Context.m_Campaign == candidate.m_Context.m_Campaign &&
            frozen->m_Context.m_Mission == candidate.m_Context.m_Mission &&
            frozen->m_Context.m_MissionGeneration == candidate.m_Context.m_MissionGeneration &&
            frozen->m_Context.m_HostPlayer == candidate.m_Context.m_HostPlayer &&
            candidate.m_OwnershipRevision == frozen->m_Revision && SameParticipants(frozen->m_Participants, candidate.m_Participants);
        if (valid) for (auto &soldier : candidate.m_Soldiers) {
            auto record = Find(frozen->m_Soldiers, [&](const auto &v) { return v.m_Soldier == soldier.m_Soldier; });
            const auto &old = soldier.m_Ownership;
            if (record == frozen->m_Soldiers.end() || record->m_Eligible != old.m_Eligible || record->m_Deployed != old.m_Deployed ||
                record->m_Alive != old.m_Alive || record->m_Owner != old.m_Owner || record->m_Actor != old.m_Actor ||
                record->m_Scripted != old.m_Scripted || old.m_ControlRevision != record->m_ControlRevision) { valid = false; break; }
            soldier.m_Ownership = *record;
        }
        if (valid) candidate.m_OwnershipRevision = frozen->m_Revision;
        else {
            // An invalid/partial cross-model freeze is never deployable. Abort
            // through lifecycle with a new generation rather than thawing it.
            m_Invalidated = true; m_FrozenManifest.reset();
            if (reason == R::None) reason = R::SnapshotUnavailable;
        }
    }
    if (reason == R::None) {
        Advance(candidate, false); candidate.m_Context.m_Phase = E_CampaignPhase::Frozen;
        candidate.m_CanDeploy = {false, R::Frozen};
        S_CampaignMissionManifest manifest;
        // SnapshotId is assigned by lifecycle/snapshot transfer authority later.
        manifest.m_Context = candidate.m_Context; manifest.m_PreparationGeneration = candidate.m_Generation;
        manifest.m_FrozenPreparationRevision = candidate.m_PreparationRevision; manifest.m_ViewRevision = candidate.m_ViewRevision;
        manifest.m_OwnershipRevision = candidate.m_OwnershipRevision; manifest.m_Soldiers = candidate.m_Soldiers;
        manifest.m_Loadouts = candidate.m_Loadouts; manifest.m_RequiredParticipants = std::move(policy.m_RequiredParticipants);
        m_FrozenManifest = std::make_shared<const S_CampaignMissionManifest>(std::move(manifest)); m_State = std::move(candidate);
    }
    return Finish(r.m_Context, F::Deploy, content, reason);
}
std::shared_ptr<const S_CampaignMissionManifest> C_CampaignPreparation::GetFrozenManifest() const { return m_FrozenManifest; }

namespace {
bool IsContainer(E_CampaignEquipmentContainer c) {
    return c == E_CampaignEquipmentContainer::Belt || c == E_CampaignEquipmentContainer::BackpackContents;
}
}
C_CampaignEquipmentRules::C_CampaignEquipmentRules(S_CampaignEquipmentRuleCatalog catalog) : m_Catalog(std::move(catalog)) {
    std::set<PlacementKey> placements;
    std::set<E_CampaignEquipmentContainer> containers;
    for (const auto &p : m_Catalog.m_Placements)
        if (!p.m_Key.IsValid() || !placements.insert(p.m_Key).second || !containers.insert(p.m_Container).second)
            throw std::invalid_argument("duplicate/invalid equipment placement");
    std::set<ItemCatalogKey> items;
    for (const auto &i : m_Catalog.m_Items) {
        if (!i.m_Item.IsValid() || !items.insert(i.m_Item).second || !std::isfinite(i.m_Weight) || i.m_Weight < 0 ||
            i.m_InventorySize > 16383 || !std::isfinite(i.m_BackpackMaxWeight) || i.m_BackpackMaxWeight < 0 ||
            i.m_BackpackVolume > 32767 || !ValidFloat(i.m_MaxCharges) || i.m_MaxRounds > 32767 ||
            i.m_AllowedPlacements.empty() || (i.m_MaxRounds && !i.m_AmmoItem)) throw std::invalid_argument("invalid equipment item rule");
        for (const auto &p : i.m_AllowedPlacements) if (!placements.count(p)) throw std::invalid_argument("unknown allowed placement");
    }
    for (const auto &i : m_Catalog.m_Items) if (i.m_AmmoItem && !items.count(*i.m_AmmoItem)) throw std::invalid_argument("unknown ammunition catalog key");
    std::set<SoldierId> soldiers;
    for (const auto &s : m_Catalog.m_Soldiers) {
        if (!s.m_Soldier.IsValid() || !soldiers.insert(s.m_Soldier).second || !std::isfinite(s.m_Strength) ||
            s.m_Strength < 1 || s.m_Strength > 32767) throw std::invalid_argument("invalid equipment soldier rule");
        for (const auto &i : s.m_ForbiddenItems) if (!items.count(i)) throw std::invalid_argument("unknown restricted item");
    }
    for (const auto &r : m_Catalog.m_Required)
        if (!items.count(r.m_Item) || !r.m_Count || r.m_Count > 32767) throw std::invalid_argument("invalid essential rule");
    std::set<PresetKey> presets; std::set<AutoEquipPolicyKey> policies;
    for (const auto &r : m_Catalog.m_Recipes) {
        if (bool(r.m_Preset) == bool(r.m_AutoEquip) ||
            (r.m_Preset && (!r.m_Preset->IsValid() || !presets.insert(*r.m_Preset).second)) ||
            (r.m_AutoEquip && (!r.m_AutoEquip->IsValid() || !policies.insert(*r.m_AutoEquip).second))) throw std::invalid_argument("invalid equipment recipe key");
        for (const auto &i : r.m_Items) if (!items.count(i.m_Item) || !placements.count(i.m_Placement.m_Slot) ||
            !i.m_Count || i.m_Count > 32767) throw std::invalid_argument("invalid recipe item");
    }
}

std::vector<S_CampaignStockEntry> C_CampaignEquipmentRules::NormalizeSupply(const std::vector<S_CampaignSupplyItem> &supply) const {
    std::vector<S_CampaignStockEntry> stock;
    // Include zero variants so a committed equipped item always has a return path.
    for (const auto &i : m_Catalog.m_Items) {
        S_CampaignStockEntry entry;
        entry.m_Entry = StockEntryId(stock.size() + 1); entry.m_State.m_Item = i.m_Item;
        stock.push_back(std::move(entry));
    }
    for (const auto &s : supply) {
        auto def = Find(m_Catalog.m_Items, [&](const auto &i) { return i.m_Item == s.m_Item; });
        if (def == m_Catalog.m_Items.end()) throw std::invalid_argument("unknown campaign supply item");
        ItemCatalogKey item = s.m_Item;
        if (s.m_Ammunition) {
            if (!def->m_AmmoItem) throw std::invalid_argument("campaign supply has no ammunition mapping");
            item = *def->m_AmmoItem;
        }
        auto entry = Find(stock, [&](const auto &e) { return e.m_State.m_Item == item; });
        if (s.m_Count > 32767u - entry->m_State.m_Quantity) throw std::invalid_argument("campaign supply count overflow");
        entry->m_State.m_Quantity += s.m_Count;
    }
    return stock;
}

S_CampaignEquipmentPlan C_CampaignEquipmentRules::Plan(const S_CampaignPreparationSnapshot &s, const S_CampaignEditRequest &r) const {
    S_CampaignEquipmentPlan plan;
    auto displacement = [&](SoldierId soldier, const S_CampaignPlacement &destination, std::optional<PreparationItemId> moving) {
        auto placement = Find(m_Catalog.m_Placements, [&](const auto &p) { return p.m_Key == destination.m_Slot; });
        if (placement == m_Catalog.m_Placements.end()) { plan.m_Reason = R::InvalidPlacement; return; }
        if (IsContainer(placement->m_Container)) return;
        auto load = Find(s.m_Loadouts, [&](const auto &l) { return l.m_Soldier == soldier; });
        if (load == s.m_Loadouts.end()) { plan.m_Reason = R::InvalidSoldier; return; }
        for (const auto &i : load->m_Items) if (i.m_Placement.m_Slot == destination.m_Slot && i.m_Entry != moving)
            plan.m_Steps.push_back(S_CampaignReturnToStock{soldier, i.m_Entry, i.m_State.m_Quantity});
    };
    std::visit([&](const auto &op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, S_CampaignTakeFromStock>) {
            displacement(op.m_Soldier, op.m_Destination, std::nullopt); plan.m_Steps.push_back(op);
        } else if constexpr (std::is_same_v<T, S_CampaignMoveWithinLoadout>) {
            displacement(op.m_Soldier, op.m_Destination, op.m_Entry); plan.m_Steps.push_back(op);
        } else if constexpr (std::is_same_v<T, S_CampaignReturnToStock>) {
            auto load = Find(s.m_Loadouts, [&](const auto &l) { return l.m_Soldier == op.m_Soldier; });
            if (load != s.m_Loadouts.end()) {
                auto selected = Find(load->m_Items, [&](const auto &i) { return i.m_Entry == op.m_Entry; });
                if (selected != load->m_Items.end()) {
                    auto placement = Find(m_Catalog.m_Placements, [&](const auto &p) { return p.m_Key == selected->m_Placement.m_Slot; });
                    // Campaign double-click returns the backpack WITH contents.
                    if (placement != m_Catalog.m_Placements.end() && placement->m_Container == E_CampaignEquipmentContainer::Backpack)
                        for (const auto &i : load->m_Items) {
                            auto p = Find(m_Catalog.m_Placements, [&](const auto &v) { return v.m_Key == i.m_Placement.m_Slot; });
                            if (p != m_Catalog.m_Placements.end() && p->m_Container == E_CampaignEquipmentContainer::BackpackContents)
                                plan.m_Steps.push_back(S_CampaignReturnToStock{op.m_Soldier, i.m_Entry, i.m_State.m_Quantity});
                        }
                }
            }
            plan.m_Steps.push_back(op);
        } else {
            auto recipe = Find(m_Catalog.m_Recipes, [&](const auto &v) {
                if constexpr (std::is_same_v<T, S_CampaignApplyPreset>) return v.m_Preset == op.m_Preset;
                else return v.m_AutoEquip == op.m_Policy;
            });
            if (recipe == m_Catalog.m_Recipes.end()) { plan.m_Reason = R::InvalidItem; return; }
            if constexpr (std::is_same_v<T, S_CampaignAutoEquip>) {
                // This explicit recipe policy has no preference parameter. Reject
                // unsupported options instead of quietly ignoring a human choice.
                if (!op.m_PreferredItems.empty()) { plan.m_Reason = R::PolicyRequired; return; }
            }
            for (const auto &target : r.m_Targets) {
                auto load = Find(s.m_Loadouts, [&](const auto &l) { return l.m_Soldier == target.m_Soldier; });
                if (load == s.m_Loadouts.end()) { plan.m_Reason = R::InvalidSoldier; return; }
                for (const auto &i : load->m_Items) plan.m_Steps.push_back(S_CampaignReturnToStock{target.m_Soldier, i.m_Entry, i.m_State.m_Quantity});
            }
            for (const auto &target : r.m_Targets) for (const auto &i : recipe->m_Items) {
                auto stock = Find(s.m_Stock, [&](const auto &e) {
                    return e.m_State.m_Item == i.m_Item && !e.m_State.m_AmmoType && !e.m_State.m_Rounds && !e.m_State.m_Charges && !e.m_State.m_Condition;
                });
                if (stock == s.m_Stock.end()) { plan.m_Reason = R::InvalidItem; return; }
                plan.m_Steps.push_back(S_CampaignTakeFromStock{target.m_Soldier, stock->m_Entry, i.m_Placement, i.m_Count});
            }
        }
    }, r.m_Operation);
    return plan;
}

S_CampaignEquipmentEvaluation C_CampaignEquipmentRules::Evaluate(const S_CampaignPreparationSnapshot &s) const {
    S_CampaignEquipmentEvaluation evaluation;
    auto reject = [&](R reason, std::optional<SoldierId> soldier, const S_CampaignLoadoutItem *item) {
        if (evaluation.m_EditRejection == R::None) evaluation.m_EditRejection = reason;
        S_CampaignFinding f; f.m_Reason = reason; f.m_Severity = E_CampaignFindingSeverity::EditRejection; f.m_Soldier = soldier;
        if (item) { f.m_Item = item->m_State.m_Item; f.m_Entry = item->m_Entry; f.m_Slot = item->m_Placement.m_Slot; }
        evaluation.m_Findings.push_back(std::move(f));
    };
    auto validateItem = [&](const S_CampaignItemState &state) {
        auto def = Find(m_Catalog.m_Items, [&](const auto &i) { return i.m_Item == state.m_Item; });
        if (def == m_Catalog.m_Items.end()) return false;
        return (!state.m_AmmoType || state.m_AmmoType == def->m_AmmoItem) &&
            (!state.m_Rounds || (def->m_AmmoItem && *state.m_Rounds <= def->m_MaxRounds)) &&
            (!state.m_Charges || (def->m_MaxCharges && std::isfinite(*state.m_Charges) && *state.m_Charges >= 0 && *state.m_Charges <= *def->m_MaxCharges)) &&
            (!state.m_Condition || (def->m_HasCondition && std::isfinite(*state.m_Condition) && *state.m_Condition >= 0 && *state.m_Condition <= 1));
    };
    for (const auto &entry : s.m_Stock) if (!validateItem(entry.m_State)) reject(R::InvalidItem, {}, nullptr);
    for (const auto &load : s.m_Loadouts) {
        auto soldier = Find(m_Catalog.m_Soldiers, [&](const auto &v) { return v.m_Soldier == load.m_Soldier; });
        if (soldier == m_Catalog.m_Soldiers.end()) { reject(R::InvalidSoldier, load.m_Soldier, nullptr); continue; }
        S_CampaignSoldierLoadout metrics;
        metrics.m_Soldier = load.m_Soldier; metrics.m_WeightCapacity = std::trunc(soldier->m_Strength); metrics.m_BeltCapacity = 12;
        std::set<PlacementKey> singleSlots;
        std::map<std::pair<PlacementKey, ItemCatalogKey>, std::uint32_t> stacks;
        bool armed = false;
        for (const auto &item : load.m_Items) {
            auto def = Find(m_Catalog.m_Items, [&](const auto &i) { return i.m_Item == item.m_State.m_Item; });
            auto placement = Find(m_Catalog.m_Placements, [&](const auto &p) { return p.m_Key == item.m_Placement.m_Slot; });
            if (!validateItem(item.m_State) || !item.m_State.m_Quantity || item.m_State.m_Quantity > 32767) { reject(R::InvalidItem, load.m_Soldier, &item); continue; }
            if (placement == m_Catalog.m_Placements.end() ||
                Find(def->m_AllowedPlacements, [&](const auto &p) { return p == item.m_Placement.m_Slot; }) == def->m_AllowedPlacements.end() ||
                item.m_Placement.m_Position > 4095) { reject(R::InvalidPlacement, load.m_Soldier, &item); continue; }
            if (Find(soldier->m_ForbiddenItems, [&](const auto &i) { return i == def->m_Item; }) != soldier->m_ForbiddenItems.end()) reject(R::MissionRestriction, load.m_Soldier, &item);
            const auto container = placement->m_Container;
            const auto count = item.m_State.m_Quantity;
            if (!IsContainer(container) && (count != 1 || item.m_Placement.m_Position != 0 || !singleSlots.insert(placement->m_Key).second))
                reject(R::InvalidPlacement, load.m_Soldier, &item);
            auto &stack = stacks[{placement->m_Key, def->m_Item}];
            stack += count;
            if (stack > 32767) reject(R::InvalidCount, load.m_Soldier, &item);
            const float weight = def->m_Weight * static_cast<float>(count);
            const float volume = static_cast<float>(def->m_InventorySize == 0 ? 1 : def->m_InventorySize * 2) * static_cast<float>(count);
            metrics.m_Weight += weight;
            if (container == E_CampaignEquipmentContainer::Backpack) {
                if (!def->m_IsBackpack) reject(R::InvalidItem, load.m_Soldier, &item);
                metrics.m_BackpackWeightCapacity = def->m_BackpackMaxWeight;
                metrics.m_BackpackCapacity = static_cast<float>(def->m_BackpackVolume);
            } else if (container == E_CampaignEquipmentContainer::Belt) { metrics.m_BeltWeight += weight; metrics.m_BeltVolume += volume; }
            else if (container == E_CampaignEquipmentContainer::BackpackContents) { metrics.m_BackpackWeight += weight; metrics.m_BackpackVolume += volume; }
            if (def->m_IsAnimated && container != E_CampaignEquipmentContainer::Head && container != E_CampaignEquipmentContainer::Uniform && container != E_CampaignEquipmentContainer::Backpack) armed = true;
        }
        if (!std::isfinite(metrics.m_Weight) || metrics.m_Weight > metrics.m_WeightCapacity || metrics.m_BeltVolume > 12 ||
            metrics.m_BackpackVolume > metrics.m_BackpackCapacity || metrics.m_BackpackWeight > metrics.m_BackpackWeightCapacity)
            reject(R::CapacityExceeded, load.m_Soldier, nullptr);
        auto ownership = Find(s.m_Soldiers, [&](const auto &v) { return v.m_Soldier == load.m_Soldier; });
        if (!armed && ownership != s.m_Soldiers.end() && ownership->m_Ownership.m_Deployed) {
            // Original Continue asks confirmation. Informational only; deployment
            // policy may request confirmation but ordinary edits stay permitted.
            S_CampaignFinding f; f.m_Reason = R::None; f.m_Severity = E_CampaignFindingSeverity::Information;
            f.m_Message = TextKey("campaign.equipment.unarmed_confirmation"); f.m_Soldier = load.m_Soldier;
            evaluation.m_Findings.push_back(f);
        }
        evaluation.m_LoadoutMetrics.push_back(std::move(metrics));
    }
    std::optional<SoldierId> axisCarrier;
    for (const auto &required : m_Catalog.m_Required) {
        const auto &def = *Find(m_Catalog.m_Items, [&](const auto &i) { return i.m_Item == required.m_Item; });
        S_CampaignEssential essential; essential.m_Item = required.m_Item;
        essential.m_Required = (def.m_IsUniform || def.m_IsDivingGear) ?
            std::min(required.m_Count, static_cast<std::uint32_t>(s.m_Loadouts.size())) : required.m_Count;
        for (const auto &load : s.m_Loadouts) {
            auto soldier = Find(s.m_Soldiers, [&](const auto &v) { return v.m_Soldier == load.m_Soldier; });
            if (soldier == s.m_Soldiers.end() || !soldier->m_Ownership.m_Deployed) continue;
            if (!def.m_IsAxis || !axisCarrier || axisCarrier == load.m_Soldier) essential.m_EligibleCarriers.push_back(load.m_Soldier);
            for (const auto &item : load.m_Items) {
                if (item.m_State.m_Item != required.m_Item || (def.m_IsAxis && axisCarrier && axisCarrier != load.m_Soldier)) continue;
                if (def.m_IsAxis) axisCarrier = load.m_Soldier;
                auto placement = Find(m_Catalog.m_Placements, [&](const auto &p) { return p.m_Key == item.m_Placement.m_Slot; });
                if (placement != m_Catalog.m_Placements.end()) essential.m_Carried += IsContainer(placement->m_Container) ? item.m_State.m_Quantity : 1;
            }
        }
        essential.m_Deficit = essential.m_Required > essential.m_Carried ? essential.m_Required - essential.m_Carried : 0;
        if (essential.m_Deficit) {
            auto f = Blocker(R::RequiredItemsMissing); f.m_Item = required.m_Item;
            f.m_Required = essential.m_Required; f.m_Actual = essential.m_Carried;
            essential.m_Findings.push_back(f); evaluation.m_DeployBlockers.push_back(f);
        }
        evaluation.m_Essentials.push_back(std::move(essential));
    }
    return evaluation;
}
