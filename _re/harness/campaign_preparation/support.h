#pragma once
#include "../../../src/game/campaign_preparation.h"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

#ifdef _MSC_VER
#ifndef _M_IX86
#error This fixture must use the MSVC x86 target
#endif
static_assert(sizeof(void *) == 4, "x86 target required");
#endif

using R = E_CampaignReason;
using F = E_CampaignRequestFamily;
using O = E_CampaignRequestOutcome;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #x); } while (false)
template<class T, class P> auto Find(T &v, P p) { return std::find_if(v.begin(), v.end(), p); }
S_CampaignPlacement Place(const char *key) { return {PlacementKey(key), 0}; }
struct Ownership : I_CampaignOwnershipView {
    S_CampaignOwnershipSnapshot m_State;
    T_CampaignOwnershipSnapshot GetSnapshot() const override { return std::make_shared<const S_CampaignOwnershipSnapshot>(m_State); }
    void Assign(std::size_t soldier, std::optional<PlayerId> owner) {
        m_State.m_Soldiers[soldier].m_Owner = owner;
        auto &r = m_State.m_Soldiers[soldier].m_ControlRevision; r = ControlRevision(r.Value() + 1);
        m_State.m_Revision = OwnershipRevision(m_State.m_Revision.Value() + 1);
    }
};
// Behavioral fake only; concrete shared production journal belongs to COOP-004.
struct Journal {
    struct Entry { RequestId m_Id; F m_Family; std::string m_Content; S_CampaignRequestResult m_Result; };
    struct Player { RequestId m_High; std::deque<Entry> m_Cache; };
    std::map<PlayerId, Player> m_Players;
    S_CampaignPreparationJournal Bind() {
        return {
            [this](const S_CampaignRequestContext &c, F f, const std::string &content, const S_CampaignRequestResult &current) -> std::optional<S_CampaignRequestResult> {
                auto &p = m_Players[c.m_PlayerEcho];
                for (const auto &e : p.m_Cache) if (e.m_Id == c.m_Request) {
                    if (e.m_Family == f && e.m_Content == content) {
                        auto result = e.m_Result; result.m_OriginalOutcome = result.m_Outcome; result.m_Outcome = O::Duplicate; return result;
                    }
                    auto result = current; result.m_Outcome = O::Rejected; result.m_Reason = R::RequestConflict; return result;
                }
                if (!(p.m_High < c.m_Request)) {
                    auto result = current; result.m_Outcome = O::ResyncRequired; result.m_Reason = R::RequestExpired; return result;
                }
                p.m_High = c.m_Request; return std::nullopt;
            },
            [this](const S_CampaignRequestContext &c, F f, const std::string &content, const S_CampaignRequestResult &result) {
                auto &cache = m_Players[c.m_PlayerEcho].m_Cache;
                cache.push_back({c.m_Request, f, content, result}); if (cache.size() > 2) cache.pop_front();
            }
        };
    }
};
struct Policy : I_CampaignDeploymentPolicy {
    bool m_Resolved = false;
    S_CampaignDeployEvaluation Evaluate(const S_CampaignPreparationSnapshot &s) const override {
        S_CampaignDeployEvaluation result;
        if (!m_Resolved) return result;
        result.m_Reason = R::None;
        // Explicit DEVELOPMENT fixture policy: all currently admitted participants ready.
        for (const auto &p : s.m_Participants) if (p.m_Admitted && p.m_Connected) {
            result.m_RequiredParticipants.push_back(p.m_Player);
            if (p.m_ReadyAt != s.m_PreparationRevision) result.m_Reason = R::PendingTransaction;
        }
        return result;
    }
};
struct Rules : I_CampaignEquipmentRules {
    mutable unsigned m_Plans = 0;
    mutable std::function<void()> m_DuringPlan;
    bool m_ForbiddenPlan = false;
    S_CampaignEquipmentPlan Plan(const S_CampaignPreparationSnapshot &s, const S_CampaignEditRequest &r) const override {
        ++m_Plans;
        if (m_DuringPlan) { auto action = std::move(m_DuringPlan); m_DuringPlan = {}; action(); }
        S_CampaignEquipmentPlan result;
        std::visit([&](const auto &op) {
            using T = std::decay_t<decltype(op)>;
            if constexpr (std::is_same_v<T, S_CampaignAutoEquip> || std::is_same_v<T, S_CampaignApplyPreset>) {
                const bool valid = [&]() {
                    if constexpr (std::is_same_v<T, S_CampaignAutoEquip>) return op.m_Policy == AutoEquipPolicyKey("fixture/ammo");
                    else return op.m_Preset == PresetKey("fixture/ammo");
                }();
                if (!valid) { result.m_Reason = R::InvalidItem; return; }
                for (const auto &target : r.m_Targets) result.m_Steps.push_back(S_CampaignTakeFromStock{target.m_Soldier, StockEntryId(2), Place("belt"), 1});
                if (m_ForbiddenPlan) result.m_Steps.push_back(S_CampaignTakeFromStock{SoldierId(20), StockEntryId(2), Place("belt"), 1});
            } else {
                if constexpr (std::is_same_v<T, S_CampaignTakeFromStock>) {
                    // Fixture single-slot displacement is explicit and transactional.
                    if (op.m_Destination.m_Slot == PlacementKey("hands")) {
                        const auto &load = *Find(s.m_Loadouts, [&](const auto &l) { return l.m_Soldier == op.m_Soldier; });
                        for (const auto &i : load.m_Items) if (i.m_Placement.m_Slot == PlacementKey("hands"))
                            result.m_Steps.push_back(S_CampaignReturnToStock{op.m_Soldier, i.m_Entry, i.m_State.m_Quantity});
                    }
                }
                result.m_Steps.push_back(op);
            }
        }, r.m_Operation);
        return result;
    }
    S_CampaignEquipmentEvaluation Evaluate(const S_CampaignPreparationSnapshot &s) const override {
        S_CampaignEquipmentEvaluation e;
        unsigned carried = 0;
        for (const auto &load : s.m_Loadouts) {
            auto metrics = load; metrics.m_Weight = 0; metrics.m_BeltVolume = 0;
            metrics.m_BackpackVolume = 0; metrics.m_BackpackWeight = 0;
            bool backpack = false; unsigned hands = 0;
            for (const auto &item : load.m_Items) {
                const auto &key = item.m_State.m_Item.Value();
                const auto &slot = item.m_Placement.m_Slot.Value();
                const auto n = item.m_State.m_Quantity;
                float weight = 0, volume = 0;
                if (key == "rifle") { weight = 4; volume = 4; }
                else if (key == "pistol") { weight = 2; volume = 2; }
                else if (key == "ammo") { weight = 0.1f; volume = 1; }
                else if (key == "essential") { weight = 1; volume = 1; carried += n; }
                else if (key == "pack") { weight = 1; backpack = slot == "pack"; }
                else e.m_EditRejection = R::InvalidItem;
                if (slot != "belt" && slot != "backpack" && slot != "hands" && slot != "pack") e.m_EditRejection = R::InvalidPlacement;
                if ((key == "pack") != (slot == "pack")) e.m_EditRejection = R::InvalidPlacement;
                if (slot == "hands") {
                    hands += n;
                    if ((key != "rifle" && key != "pistol") || hands > 1) e.m_EditRejection = R::InvalidPlacement;
                }
                if (item.m_State.m_AmmoType && *item.m_State.m_AmmoType != ItemCatalogKey("ammo")) e.m_EditRejection = R::InvalidItem;
                metrics.m_Weight += weight * static_cast<float>(n);
                if (slot == "belt") metrics.m_BeltVolume += volume * static_cast<float>(n);
                if (slot == "backpack") {
                    metrics.m_BackpackVolume += volume * static_cast<float>(n);
                    metrics.m_BackpackWeight += weight * static_cast<float>(n);
                }
            }
            if (metrics.m_Weight > 10 || metrics.m_BeltVolume > 12 || metrics.m_BackpackVolume > (backpack ? 4 : 0) ||
                metrics.m_BackpackWeight > (backpack ? 3 : 0)) e.m_EditRejection = R::CapacityExceeded;
            metrics.m_WeightCapacity = 10; metrics.m_BeltCapacity = 12;
            metrics.m_BackpackCapacity = backpack ? 4.0f : 0.0f;
            metrics.m_BackpackWeightCapacity = backpack ? 3.0f : 0.0f;
            e.m_LoadoutMetrics.push_back(metrics);
        }
        S_CampaignEssential essential;
        essential.m_Item = ItemCatalogKey("essential"); essential.m_Required = 1; essential.m_Carried = carried;
        essential.m_Deficit = carried ? 0 : 1; e.m_Essentials.push_back(essential);
        return e;
    }
};
inline std::map<std::string, std::uint64_t> Totals(const T_CampaignPreparationSnapshot &s) {
    std::map<std::string, std::uint64_t> totals;
    auto count = [&](const S_CampaignItemState &state) {
        std::ostringstream key;
        key << state.m_Item.Value() << ":" << (state.m_AmmoType ? state.m_AmmoType->Value() : "-");
        key << ":" << (state.m_Rounds ? std::to_string(*state.m_Rounds) : "-");
        key << ":" << (state.m_Charges ? std::to_string(*state.m_Charges) : "-");
        key << ":" << (state.m_Condition ? std::to_string(*state.m_Condition) : "-");
        totals[key.str()] += state.m_Quantity;
    };
    for (const auto &entry : s->m_Stock) count(entry.m_State);
    for (const auto &load : s->m_Loadouts) for (const auto &entry : load.m_Items) count(entry.m_State);
    return totals;
}
struct Fixture {
    Ownership m_Ownership;
    Rules m_Rules;
    Policy m_Policy;
    Journal m_Journal;
    std::unique_ptr<C_CampaignPreparation> m_Model;
    std::uint64_t m_Request = 0;
    explicit Fixture(PreparationRevision prep = PreparationRevision(1), ViewRevision view = ViewRevision(1)) {
        S_CampaignPreparationSnapshot s;
        auto &c = s.m_Context;
        c.m_Kind = E_CampaignSessionKind::CampaignCoop; c.m_Session = SessionId(1); c.m_Campaign = CampaignId(1);
        c.m_Mission = MissionKey("fixture/mission"); c.m_HostPlayer = PlayerId(1); c.m_UsesCampaignRules = true;
        c.m_IsTransportServer = true; c.m_Phase = E_CampaignPhase::Preparation;
        s.m_Generation = PreparationGeneration(1); s.m_PreparationRevision = prep; s.m_ViewRevision = view;
        m_Ownership.m_State.m_Context = c; m_Ownership.m_State.m_Revision = OwnershipRevision(1);
        m_Ownership.m_State.m_Participants = {{PlayerId(1), "Host", true, true, {}}, {PlayerId(2), "Guest", true, true, {}}};
        for (unsigned i = 1; i <= 4; ++i) {
            S_CampaignOwnershipRecord o;
            o.m_Soldier = SoldierId(i * 10); o.m_Eligible = true; o.m_Deployed = true; o.m_Alive = true;
            o.m_ControlRevision = ControlRevision(1); if (i <= 2) o.m_Owner = PlayerId(i);
            m_Ownership.m_State.m_Soldiers.push_back(o);
            S_CampaignSoldierPresentation p; p.m_Soldier = o.m_Soldier; s.m_Soldiers.push_back(p);
            S_CampaignSoldierLoadout l; l.m_Soldier = o.m_Soldier;
            l.m_Skills.push_back({SkillKey("shooting"), 80.25f}); s.m_Loadouts.push_back(l);
            s.m_Slots[i - 1].m_Soldier = o.m_Soldier;
        }
        const std::vector<std::pair<const char *, unsigned>> stock = {{"rifle", 1}, {"ammo", 20}, {"pack", 1}, {"essential", 0}, {"pistol", 1}};
        for (const auto &pair : stock) {
            S_CampaignStockEntry e; e.m_Entry = StockEntryId(s.m_Stock.size() + 1);
            e.m_State.m_Item = ItemCatalogKey(pair.first); e.m_State.m_Quantity = pair.second; s.m_Stock.push_back(e);
        }
        s.m_Stock[0].m_State.m_AmmoType = ItemCatalogKey("ammo");
        s.m_Stock[0].m_State.m_Rounds = 5; s.m_Stock[0].m_State.m_Condition = 0.75f;
        s.m_Loadouts[0].m_Items.push_back({PreparationItemId(1), s.m_Stock[3].m_State, Place("belt")});
        s.m_Loadouts[0].m_Items[0].m_State.m_Quantity = 1;
        m_Model = std::make_unique<C_CampaignPreparation>(s, m_Ownership, m_Rules, m_Policy, m_Journal.Bind());
    }
    T_CampaignPreparationSnapshot Snapshot(unsigned player = 1) const { return m_Model->GetSnapshot(PlayerId(player)); }
    S_CampaignPreparationRequestContext Context(unsigned player = 1) {
        return {{SessionId(1), PlayerId(player), RequestId(++m_Request)}, PreparationGeneration(1), Snapshot()->m_PreparationRevision};
    }
    S_CampaignEditRequest Edit(unsigned player, std::vector<unsigned> targets, T_CampaignEditOperation op) {
        S_CampaignEditRequest r; r.m_Context = Context(player); r.m_Operation = std::move(op);
        for (auto soldier : targets) {
            const auto &s = *Find(m_Ownership.m_State.m_Soldiers, [&](const auto &v) { return v.m_Soldier == SoldierId(soldier); });
            r.m_Targets.push_back({s.m_Soldier, s.m_ControlRevision});
        }
        return r;
    }
    S_CampaignEditRequest Take(unsigned player, unsigned soldier, unsigned stock, const char *place, unsigned count = 1) {
        return Edit(player, {soldier}, S_CampaignTakeFromStock{SoldierId(soldier), StockEntryId(stock), Place(place), count});
    }
    S_CampaignRequestResult Apply(const S_CampaignEditRequest &r) {
        auto totals = Totals(Snapshot());
        auto result = m_Model->ApplyEdit(r.m_Context.m_Request.m_PlayerEcho, r);
        CHECK(Totals(Snapshot()) == totals);
        return result;
    }
    void Ready(unsigned player) { CHECK(m_Model->SetReady(PlayerId(player), {Context(player), true}).m_Reason == R::None); }
};
inline std::string Digest(const T_CampaignPreparationSnapshot &s) {
    std::ostringstream o; o << s->m_PreparationRevision.Value() << ':' << s->m_ViewRevision.Value() << ':' << s->m_OwnershipRevision.Value() << ':' << static_cast<int>(s->m_Context.m_Phase);
    for (const auto &stock : s->m_Stock) o << ";s" << stock.m_Entry.Value() << ':' << stock.m_State.m_Quantity;
    for (const auto &l : s->m_Loadouts) {
        o << ";l" << l.m_Soldier.Value() << ':' << l.m_Weight;
        for (const auto &i : l.m_Items) o << ':' << i.m_Entry.Value() << ':' << i.m_State.m_Item.Value() << ':' << i.m_State.m_Quantity << ':' << i.m_Placement.m_Slot.Value();
        for (const auto &skill : l.m_Skills) o << ':' << skill.m_Value;
    }
    for (const auto &p : s->m_Participants) o << ";p" << p.m_Player.Value() << ':' << (p.m_ReadyAt ? p.m_ReadyAt->Value() : 0);
    return o.str();
}
