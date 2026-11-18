#include "campaign_preparation_screen.h"

#include <algorithm>
#include <limits>
#include <sstream>

namespace {
constexpr std::size_t PAGE_SIZE = 5;
std::string Number(float value) { std::ostringstream out; out << value; return out.str(); }
bool SameControl(const S_CampaignPreparationControl &a, const S_CampaignPreparationControl &b) {
    return a.m_Action == b.m_Action && a.m_Key == b.m_Key;
}
}
bool S_CampaignPreparationRect::Contains(float x, float y) const {
    return x >= m_X && y >= m_Y && x < m_X + m_Width && y < m_Y + m_Height;
}
C_CampaignPreparationScreen::C_CampaignPreparationScreen(I_CampaignPreparation &model,
    I_CampaignPreparationPresentation &presentation) : m_Model(model), m_Presentation(presentation) { Tick(); }
C_CampaignPreparationScreen::~C_CampaignPreparationScreen() { Close(); }
void C_CampaignPreparationScreen::Close() { m_Open = false; m_Pending.reset(); m_Snapshot.reset(); m_Controls.clear(); }
std::string C_CampaignPreparationScreen::Label(const char *key) const { return m_Presentation.Label(key); }
bool C_CampaignPreparationScreen::IsCurrent() const {
    return m_Open && m_Snapshot && m_Status == E_CampaignSyncStatus::Current &&
        m_Snapshot->m_Sync == E_CampaignSyncStatus::Current &&
        m_Snapshot->m_Connection == E_CampaignConnectionStatus::Admitted &&
        m_Snapshot->m_Context.m_Phase == E_CampaignPhase::Preparation &&
        m_Snapshot->m_Context.m_LocalPlayer.has_value();
}
const S_CampaignSoldierPresentation *C_CampaignPreparationScreen::Soldier() const {
    if (m_Snapshot && m_Selected)
        for (const auto &s : m_Snapshot->m_Soldiers) if (s.m_Soldier == *m_Selected) return &s;
    return nullptr;
}
const S_CampaignSoldierLoadout *C_CampaignPreparationScreen::Loadout() const {
    const auto *s = Soldier();
    if (s && s->m_CanView.m_Allowed)
        for (const auto &l : m_Snapshot->m_Loadouts) if (l.m_Soldier == s->m_Soldier) return &l;
    return nullptr;
}
bool C_CampaignPreparationScreen::CanEdit() const {
    const auto *s = Soldier();
    if (!IsCurrent() || m_Pending || !s || !s->m_CanView.m_Allowed || !s->m_CanEdit.m_Allowed) return false;
    const auto &o = s->m_Ownership;
    const auto &c = m_Snapshot->m_Context;
    return o.m_Alive && o.m_Eligible && o.m_Deployed && !o.m_Scripted &&
        ((o.m_Owner && o.m_Owner == c.m_LocalPlayer) || (!o.m_Owner && c.m_LocalPlayer == c.m_HostPlayer));
}
bool C_CampaignPreparationScreen::HasLocalSoldier() const {
    if (!m_Snapshot || !m_Snapshot->m_Context.m_LocalPlayer) return false;
    for (const auto &s : m_Snapshot->m_Soldiers)
        if (s.m_Ownership.m_Owner == m_Snapshot->m_Context.m_LocalPlayer && s.m_Ownership.m_Alive &&
            s.m_Ownership.m_Eligible && s.m_Ownership.m_Deployed && !s.m_Ownership.m_Scripted) return true;
    return false;
}
const S_CampaignStockEntry *C_CampaignPreparationScreen::Stock() const {
    if (m_Snapshot && m_Stock) for (const auto &s : m_Snapshot->m_Stock) if (s.m_Entry == *m_Stock) return &s;
    return nullptr;
}
const S_CampaignAllowedDestination *C_CampaignPreparationScreen::Destination() const {
    const auto *stock = Stock();
    if (stock && m_Selected) for (const auto &d : stock->m_Destinations) if (d.m_Soldier == *m_Selected) return &d;
    return nullptr;
}
std::string C_CampaignPreparationScreen::Owner(const S_CampaignSoldierPresentation &s) const {
    if (s.m_Ownership.m_Scripted) return Label("COOP_OWNER_SCRIPTED");
    if (!s.m_Ownership.m_Owner) return Label("COOP_OWNER_AI");
    for (const auto &p : m_Snapshot->m_Participants) if (p.m_Player == *s.m_Ownership.m_Owner)
        return p.m_DisplayName.substr(0, 48); // painter treats display strings as plain text
    return Label("COOP_OWNER_HUMAN");
}
void C_CampaignPreparationScreen::Refresh() {
    const auto view = m_Model.GetSnapshot();
    m_Status = view.m_Status;
    if (!view.m_Snapshot) { m_Snapshot.reset(); return; }
    const auto &next = view.m_Snapshot;
    if (m_Pending && (next->m_Context.m_Session != m_Pending->m_Context.m_Session ||
        next->m_Generation != m_Pending->m_Generation ||
        next->m_Context.m_LocalPlayer != m_Pending->m_Context.m_PlayerEcho)) m_Pending.reset();
    // Only GetSnapshot installs state. Events/results are hints, never deltas or rollback.
    if (m_Snapshot && next->m_Context.m_Session == m_Snapshot->m_Context.m_Session &&
        (next->m_Generation < m_Snapshot->m_Generation || (next->m_Generation == m_Snapshot->m_Generation && next->m_ViewRevision < m_Snapshot->m_ViewRevision))) {
        m_Status = E_CampaignSyncStatus::Resyncing;
        m_Notice = E_CampaignReason::StaleRevision;
        m_Model.RequestResync();
        return;
    }
    if (m_Snapshot && (next->m_Context.m_Session != m_Snapshot->m_Context.m_Session ||
        next->m_Generation != m_Snapshot->m_Generation)) {
        m_Pending.reset(); m_Selected.reset(); m_Stock.reset(); m_Inventory.reset(); m_Destination.reset();
        m_Notice = E_CampaignReason::None;
    }
    const bool wasEditable = CanEdit();
    m_Snapshot = next;
    bool visible = false;
    for (const auto &slot : next->m_Slots) if (slot.m_Soldier && slot.m_Soldier == m_Selected) visible = true;
    if (!visible) {
        m_Selected.reset();
        for (const auto &slot : next->m_Slots) if (slot.m_Soldier) { m_Selected = slot.m_Soldier; break; }
        m_Inventory.reset(); m_InventoryPage = 0;
    }
    if (!Stock()) { m_Stock.reset(); if (!next->m_Stock.empty()) m_Stock = next->m_Stock.front().m_Entry; }
    const auto *d = Destination();
    if (!d || !m_Destination || std::find(d->m_Placements.begin(), d->m_Placements.end(), *m_Destination) == d->m_Placements.end()) {
        m_Destination.reset();
        if (d && !d->m_Placements.empty()) m_Destination = d->m_Placements.front();
    }
    const auto *loadout = Loadout();
    if (m_Inventory && (!loadout || std::none_of(loadout->m_Items.begin(), loadout->m_Items.end(),
        [this](const auto &i) { return i.m_Entry == *m_Inventory; }))) m_Inventory.reset();
    if (wasEditable && !CanEdit()) {
        m_Quantity = 1;
        for (std::size_t i = 0; i < next->m_Slots.size(); ++i) if (next->m_Slots[i].m_Soldier == m_Selected) m_Focus = i;
    }
}
void C_CampaignPreparationScreen::Tick() {
    if (!m_Open) return;
    Refresh();
    // Bound one frame's work even if a transport is continuously producing events.
    for (int i = 0; i < 128; ++i) {
        const auto event = m_Model.PollEvent();
        if (!event) break;
        const auto *r = std::get_if<S_CampaignRequestResultEvent>(&*event);
        if (r && m_Pending) {
            const auto &p = *m_Pending; const auto &v = r->m_Result;
            if (r->m_LocalToken == p.m_Token && v.m_Context.m_Session == p.m_Context.m_Session &&
                v.m_Context.m_PlayerEcho == p.m_Context.m_PlayerEcho && v.m_Context.m_Request == p.m_Context.m_Request &&
                v.m_Family == p.m_Family && v.m_PreparationGeneration == p.m_Generation) {
                m_Notice = v.m_Reason;
                if (v.m_Outcome == E_CampaignRequestOutcome::ResyncRequired ||
                    std::holds_alternative<S_CampaignResyncDescriptor>(v.m_State)) m_Model.RequestResync();
                m_Pending.reset();
            }
        }
        // Polling status is an invalidation hint; an obsolete event cannot end a new session.
        Refresh();
    }
    BuildControls();
}
void C_CampaignPreparationScreen::BuildControls() {
    std::optional<S_CampaignPreparationControl> focused;
    if (m_Focus < m_Controls.size()) focused = m_Controls[m_Focus];
    m_Controls.clear();
    auto add = [this](E_CampaignPreparationAction a, std::uint64_t key, S_CampaignPreparationRect r,
        std::string text, bool enabled = true, E_CampaignReason reason = E_CampaignReason::None) {
        m_Controls.push_back({a, key, r, std::move(text), enabled, reason});
    };
    for (std::size_t i = 0; i < 4; ++i) {
        std::string text = Label("COOP_SLOT_UNAVAILABLE"); bool enabled = false;
        if (m_Snapshot) {
            const auto &slot = m_Snapshot->m_Slots[i];
            if (slot.m_Soldier) { for (const auto &s : m_Snapshot->m_Soldiers) if (s.m_Soldier == *slot.m_Soldier) {
                text = s.m_DisplayName.substr(0, 40) + "\n" + Owner(s) + "\n" +
                    Label(s.m_CanEdit.m_Allowed ? "COOP_OWNER_EDITABLE" : "COOP_OWNER_READ_ONLY"); enabled = true;
            }
            } // no implicit roster creation
            if (!slot.m_Soldier) text += "\n" + m_Presentation.Reason(slot.m_UnavailableReason);
        }
        add(E_CampaignPreparationAction::Slot, i, {40.0f + static_cast<float>(i) * 240, 116, 224, 112}, text, enabled);
    }
    add(E_CampaignPreparationAction::Page, 0, {40, 244, 206, 32}, Label(m_Briefing ? "COOP_EQUIP_PAGE" : "COOP_BRIEFING_PAGE"));
    add(E_CampaignPreparationAction::Skills, 0, {266, 244, 320, 32}, Label(m_Skills ? "COOP_EQUIP_PAGE" : "COOP_SKILLS"), Loadout() != nullptr);
    const auto *loadout = Loadout(); const auto *stock = Stock(); const auto *dest = Destination();
    if (loadout && !m_Briefing && !m_Skills) {
        m_InventoryPage = std::min(m_InventoryPage, loadout->m_Items.empty() ? std::size_t(0) : (loadout->m_Items.size() - 1) / PAGE_SIZE);
        for (std::size_t i = m_InventoryPage * PAGE_SIZE; i < loadout->m_Items.size() && i < (m_InventoryPage + 1) * PAGE_SIZE; ++i) {
            const auto &v = loadout->m_Items[i];
            add(E_CampaignPreparationAction::Inventory, v.m_Entry.Value(), {266, 350 + static_cast<float>(i % PAGE_SIZE) * 32, 320, 30},
                m_Presentation.Item(v.m_State.m_Item) + " x" + std::to_string(v.m_State.m_Quantity) + " / " + m_Presentation.Placement(v.m_Placement.m_Slot));
        }
    }
    if (loadout && !m_Briefing && !m_Skills && loadout->m_Items.size() > PAGE_SIZE)
        add(E_CampaignPreparationAction::InventoryPage, 0, {266, 504, 320, 24}, Label("COOP_PAGE_NEXT"));
    if (m_Briefing) add(E_CampaignPreparationAction::BriefingPage, 0, {610, 554, 358, 32}, Label("COOP_PAGE_NEXT"));
    if (m_Snapshot && !m_Briefing) {
        m_StockPage = std::min(m_StockPage, m_Snapshot->m_Stock.empty() ? std::size_t(0) : (m_Snapshot->m_Stock.size() - 1) / PAGE_SIZE);
        for (std::size_t i = m_StockPage * PAGE_SIZE; i < m_Snapshot->m_Stock.size() && i < (m_StockPage + 1) * PAGE_SIZE; ++i) {
            const auto &v = m_Snapshot->m_Stock[i];
            add(E_CampaignPreparationAction::Stock, v.m_Entry.Value(), {610, 308 + static_cast<float>(i % PAGE_SIZE) * 32, 358, 30},
                m_Presentation.Item(v.m_State.m_Item) + " x" + std::to_string(v.m_State.m_Quantity));
        }
        if (m_Snapshot->m_Stock.size() > PAGE_SIZE)
            add(E_CampaignPreparationAction::StockPage, 0, {610, 470, 358, 24}, Label("COOP_PAGE_NEXT"));
        add(E_CampaignPreparationAction::QuantityDown, 0, {610, 478, 42, 30}, "-", m_Quantity > 1);
        add(E_CampaignPreparationAction::QuantityUp, 0, {716, 478, 42, 30}, "+", m_Quantity < 999);
        add(E_CampaignPreparationAction::Destination, 0, {610, 516, 358, 30}, m_Destination ? m_Presentation.Placement(*m_Destination) : Label("COOP_EQUIP_NO_DESTINATION"), dest && !dest->m_Placements.empty());
        E_CampaignReason denial = !CanEdit() ? (Soldier() ? Soldier()->m_CanEdit.m_Reason : E_CampaignReason::NotOwner) :
            !stock || stock->m_State.m_Quantity < m_Quantity ? E_CampaignReason::StockUnavailable :
            !dest ? E_CampaignReason::InvalidPlacement : dest->m_CanTake.m_Reason;
        const bool take = CanEdit() && stock && stock->m_State.m_Quantity >= m_Quantity && dest && dest->m_CanTake.m_Allowed && m_Destination;
        add(E_CampaignPreparationAction::Equip, 0, {610, 554, 172, 32}, Label("COOP_EQUIP_TAKE"), take, denial);
        add(E_CampaignPreparationAction::Return, 0, {796, 554, 172, 32}, Label("COOP_EQUIP_RETURN"), CanEdit() && m_Inventory.has_value(), E_CampaignReason::NotOwner);
    }
    const auto *s = Soldier();
    add(E_CampaignPreparationAction::Claim, 0, {40, 554, 98, 32}, Label("COOP_SLOT_CLAIM"), IsCurrent() && !m_Pending && s && s->m_CanClaim.m_Allowed,
        s ? s->m_CanClaim.m_Reason : E_CampaignReason::Ineligible);
    add(E_CampaignPreparationAction::Release, 0, {148, 554, 98, 32}, Label("COOP_SLOT_RELEASE"), IsCurrent() && !m_Pending && s && s->m_CanRelease.m_Allowed,
        s ? s->m_CanRelease.m_Reason : E_CampaignReason::Ineligible);
    bool ready = false;
    if (m_Snapshot) for (const auto &p : m_Snapshot->m_Participants) if (p.m_Player == m_Snapshot->m_Context.m_LocalPlayer)
        ready = p.m_ReadyAt == m_Snapshot->m_PreparationRevision;
    add(E_CampaignPreparationAction::Ready, 0, {540, 690, 136, 38}, Label(ready ? "COOP_READY_UNSET" : "COOP_READY_SET"), IsCurrent() && !m_Pending && HasLocalSoldier(), E_CampaignReason::Ineligible);
    if (m_Snapshot && m_Snapshot->m_Context.m_LocalPlayer == m_Snapshot->m_Context.m_HostPlayer && m_Snapshot->m_Context.m_LocalPlayer)
        add(E_CampaignPreparationAction::Deploy, 0, {688, 690, 136, 38}, Label("COOP_READY_DEPLOY"), IsCurrent() && !m_Pending && m_Snapshot->m_CanDeploy.m_Allowed, m_Snapshot->m_CanDeploy.m_Reason);
    add(E_CampaignPreparationAction::Leave, 0, {836, 690, 132, 38}, Label("COOP_READY_LEAVE"));
    if (m_ConfirmLeave) {
        m_Controls.clear();
        add(E_CampaignPreparationAction::ConfirmLeave, 0, {280, 410, 220, 44}, Label("COOP_READY_CONFIRM_LEAVE"));
        add(E_CampaignPreparationAction::CancelLeave, 0, {524, 410, 220, 44}, Label("COOP_READY_CANCEL"));
    }
    m_Focus = std::min(m_Focus, m_Controls.size() - 1);
    if (focused) for (std::size_t i = 0; i < m_Controls.size(); ++i) if (SameControl(*focused, m_Controls[i])) { m_Focus = i; break; }
}
S_CampaignPreparationRequestContext C_CampaignPreparationScreen::Context(RequestId id) const {
    return {{m_Snapshot->m_Context.m_Session, *m_Snapshot->m_Context.m_LocalPlayer, id}, m_Snapshot->m_Generation, m_Snapshot->m_PreparationRevision};
}
void C_CampaignPreparationScreen::Queued(const T_CampaignSubmission &submission, RequestId id, E_CampaignRequestFamily family) {
    if (const auto *q = std::get_if<S_CampaignQueuedSubmission>(&submission)) {
        if (q->m_Token.IsValid()) m_Pending = S_Pending{q->m_Token, Context(id).m_Request, m_Snapshot->m_Generation, family};
        else m_Notice = E_CampaignReason::Malformed;
    } else m_Notice = std::get<S_CampaignLocalFailure>(submission).m_Reason;
}
void C_CampaignPreparationScreen::Activate(std::size_t index) {
    if (index >= m_Controls.size()) return;
    const auto control = m_Controls[index];
    if (!control.m_Enabled) { m_Notice = control.m_Reason; return; }
    using A = E_CampaignPreparationAction;
    switch (control.m_Action) {
    case A::Slot:
        m_Selected = m_Snapshot->m_Slots[static_cast<std::size_t>(control.m_Key)].m_Soldier;
        m_Inventory.reset(); m_Destination.reset(); m_Quantity = 1; m_InventoryPage = 0; Refresh(); break;
    case A::Stock: m_Stock = StockEntryId(control.m_Key); m_Destination.reset(); m_Quantity = 1; Refresh(); break;
    case A::Inventory: m_Inventory = PreparationItemId(control.m_Key); break;
    case A::Page: m_Briefing = !m_Briefing; break;
    case A::InventoryPage: ++m_InventoryPage; if (Loadout() && m_InventoryPage * PAGE_SIZE >= Loadout()->m_Items.size()) m_InventoryPage = 0; break;
    case A::StockPage: ++m_StockPage; if (m_StockPage * PAGE_SIZE >= m_Snapshot->m_Stock.size()) m_StockPage = 0; break;
    case A::BriefingPage: ++m_BriefingPage; if (m_BriefingPage * 1200 >= m_Presentation.Text(m_Snapshot->m_Briefing.m_Body).size()) m_BriefingPage = 0; break;
    case A::Skills: m_Skills = !m_Skills; break;
    case A::QuantityDown: --m_Quantity; break;
    case A::QuantityUp: ++m_Quantity; break;
    case A::Destination: {
        const auto *d = Destination();
        if (d && !d->m_Placements.empty()) {
            auto it = std::find(d->m_Placements.begin(), d->m_Placements.end(), *m_Destination);
            m_Destination = (++it == d->m_Placements.end()) ? d->m_Placements.front() : *it;
        } break;
    }
    case A::Leave: m_ConfirmLeave = true; break;
    case A::CancelLeave: m_ConfirmLeave = false; break;
    case A::ConfirmLeave: m_LeaveRequested = true; Close(); return;
    default: {
        if (!IsCurrent() || m_Pending) return;
        const auto id = m_Presentation.NextRequestId();
        if (!id.IsValid()) { m_Notice = E_CampaignReason::RequestExpired; return; }
        m_Notice = E_CampaignReason::None;
        if (control.m_Action == A::Equip || control.m_Action == A::Return) {
            if (!CanEdit()) return;
            S_CampaignEditRequest request; request.m_Context = Context(id);
            request.m_Targets.push_back({*m_Selected, Soldier()->m_Ownership.m_ControlRevision});
            if (control.m_Action == A::Equip) request.m_Operation = S_CampaignTakeFromStock{*m_Selected, *m_Stock, {*m_Destination, 0}, m_Quantity};
            else request.m_Operation = S_CampaignReturnToStock{*m_Selected, *m_Inventory, m_Quantity};
            Queued(m_Model.SubmitEdit(request), id, E_CampaignRequestFamily::Edit);
        } else if (control.m_Action == A::Claim || control.m_Action == A::Release) {
            S_CampaignOwnershipRequest request;
            request.m_Context = Context(id).m_Request; request.m_PreparationGeneration = m_Snapshot->m_Generation;
            request.m_ExpectedOwnershipRevision = m_Snapshot->m_OwnershipRevision;
            const S_CampaignOwnershipTarget target{*m_Selected, Soldier()->m_Ownership.m_ControlRevision};
            if (control.m_Action == A::Claim) request.m_Operation = S_CampaignClaim{target};
            else request.m_Operation = S_CampaignRelease{target};
            Queued(m_Model.SubmitOwnership(request), id, E_CampaignRequestFamily::Ownership);
        } else if (control.m_Action == A::Ready) {
            bool ready = false;
            for (const auto &p : m_Snapshot->m_Participants) if (p.m_Player == m_Snapshot->m_Context.m_LocalPlayer)
                ready = p.m_ReadyAt == m_Snapshot->m_PreparationRevision;
            Queued(m_Model.SetReady({Context(id), !ready}), id, E_CampaignRequestFamily::Ready);
        } else if (control.m_Action == A::Deploy)
            Queued(m_Model.RequestDeploy({Context(id)}), id, E_CampaignRequestFamily::Deploy);
        break;
    }
    }
    if (m_Open) BuildControls();
}
bool C_CampaignPreparationScreen::Click(float x, float y) {
    Tick(); if (!m_Open) return false;
    for (std::size_t i = 0; i < m_Controls.size(); ++i) if (m_Controls[i].m_Rect.Contains(x, y)) {
        m_Focus = i; Activate(i); return true;
    }
    return false;
}
bool C_CampaignPreparationScreen::Key(E_CampaignPreparationKey key) {
    Tick(); if (!m_Open || m_Controls.empty()) return false;
    using K = E_CampaignPreparationKey;
    if (key == K::Escape) { m_ConfirmLeave = !m_ConfirmLeave; BuildControls(); return true; }
    if (key == K::Enter || key == K::Space) { Activate(m_Focus); return true; }
    if (key == K::Left || key == K::Right || key == K::Up || key == K::Down) {
        auto &c = m_Controls[m_Focus];
        const bool forward = key == K::Right || key == K::Down;
        if (c.m_Action == E_CampaignPreparationAction::Stock && m_Snapshot && m_Snapshot->m_Stock.size() > PAGE_SIZE) {
            const auto pages = (m_Snapshot->m_Stock.size() + PAGE_SIZE - 1) / PAGE_SIZE;
            m_StockPage = forward ? (m_StockPage + 1) % pages : (m_StockPage + pages - 1) % pages;
            BuildControls(); return true;
        }
        if (c.m_Action == E_CampaignPreparationAction::Inventory && Loadout() && Loadout()->m_Items.size() > PAGE_SIZE) {
            const auto pages = (Loadout()->m_Items.size() + PAGE_SIZE - 1) / PAGE_SIZE;
            m_InventoryPage = forward ? (m_InventoryPage + 1) % pages : (m_InventoryPage + pages - 1) % pages;
            BuildControls(); return true;
        }
        if (m_Briefing) { if (forward) ++m_BriefingPage; else if (m_BriefingPage) --m_BriefingPage; return true; }
    }
    const bool back = key == K::BackTab || key == K::Left || key == K::Up;
    m_Focus = back ? (m_Focus + m_Controls.size() - 1) % m_Controls.size() : (m_Focus + 1) % m_Controls.size();
    return true;
}
void C_CampaignPreparationScreen::Render(I_CampaignPreparationPainter &p) {
    Tick(); if (!m_Open) return;
    p.Panel({0, 0, 1024, 768}, false);
    auto text = [&p](float x, float y, float w, float h, const std::string &s, bool muted = false) { p.Text({x,y,w,h}, s, muted); };
    text(40, 24, 928, 24, Label("COOP_PREP_TITLE"));
    text(40, 56, 928, 46, m_Snapshot ? m_Presentation.Text(m_Snapshot->m_Briefing.m_OperationTitle) + " / " +
        m_Presentation.Text(m_Snapshot->m_Briefing.m_MissionTitle) : Label("COOP_PREP_RECEIVING"));
    p.Panel({40, 290, 206, 304}, false); p.Panel({256, 290, 340, 304}, false); p.Panel({600, 290, 378, 304}, false);
    if (m_Snapshot) {
        const auto *s = Soldier(); const auto *l = Loadout();
        text(40, 296, 206, 24, Label("COOP_BRIEFING_TITLE"));
        const auto body = m_Presentation.Text(m_Snapshot->m_Briefing.m_Body);
        if (m_Briefing) {
            const auto pages = std::max(std::size_t(1), (body.size() + 1199) / 1200);
            m_BriefingPage = std::min(m_BriefingPage, pages - 1);
            text(52, 334, 900, 210, body.substr(m_BriefingPage * 1200, 1200));
            text(52, 554, 800, 32, std::to_string(m_BriefingPage + 1) + "/" + std::to_string(pages) + " " + Label("COOP_PAGE_KEYS"));
        } else {
            text(50, 330, 186, 146, body.substr(0, 260));
            if (s) {
                text(266, 296, 320, 24, s->m_DisplayName.substr(0, 40));
                text(266, 322, 320, 26, Label(CanEdit() ? (s->m_Ownership.m_Owner ? "COOP_OWNER_EDIT_OWN" : "COOP_OWNER_EDIT_AI") : "COOP_OWNER_READ_ONLY") + " / " + Owner(*s), !CanEdit());
                if (s->m_CanView.m_Allowed) p.Asset({58, 482, 90, 64}, s->m_Portrait);
                if (l) {
                    if (m_Skills) { float y = 354; for (const auto &skill : l->m_Skills) { if (y > 490) break;
                        text(266, y, 320, 26, m_Presentation.Skill(skill.m_Key) + " " + Number(skill.m_Value)); y += 28; } }
                    text(266, 516, 320, 30, Label("COOP_EQUIP_WEIGHT") + " " + Number(l->m_Weight) + "/" + Number(l->m_WeightCapacity));
                    text(266, 552, 320, 30, Label("COOP_EQUIP_BELT_PACK") + " " + Number(l->m_BeltVolume) + "/" + Number(l->m_BeltCapacity) + " | " + Number(l->m_BackpackVolume) + "/" + Number(l->m_BackpackCapacity));
                } else text(266, 370, 320, 100, m_Presentation.Reason(s->m_CanView.m_Reason));
            }
            text(610, 282, 358, 24, Label("COOP_EQUIP_STOCK"));
            text(658, 478, 58, 30, std::to_string(m_Quantity));
        }
        std::string essentials = Label("COOP_ESSENTIAL_TITLE");
        for (const auto &e : m_Snapshot->m_Essentials) essentials += "  " + m_Presentation.Item(e.m_Item) + " " + std::to_string(e.m_Carried) + "/" + std::to_string(e.m_Required) + (e.m_Deficit ? " !" : "");
        text(40, 602, 928, 34, essentials);
        std::string ready;
        for (const auto &participant : m_Snapshot->m_Participants) if (participant.m_Connected) ready += participant.m_DisplayName.substr(0, 24) + ": " +
            Label(participant.m_ReadyAt == m_Snapshot->m_PreparationRevision ? "COOP_READY_YES" : "COOP_READY_NO") + "  ";
        text(40, 684, 488, 56, ready);
    }
    std::string status;
    if (m_Status != E_CampaignSyncStatus::Current) status = Label(m_Status == E_CampaignSyncStatus::Disconnected ? "COOP_PREP_DISCONNECTED" : "COOP_PREP_RECEIVING");
    else if (m_Snapshot && m_Snapshot->m_Context.m_Phase != E_CampaignPhase::Preparation) status = Label("COOP_PREP_FROZEN");
    else if (m_Pending) status = Label("COOP_PREP_PENDING");
    else if (m_Notice != E_CampaignReason::None) status = m_Presentation.Reason(m_Notice);
    else if (m_Snapshot && !m_Snapshot->m_DeployBlockers.empty()) status = m_Presentation.Reason(m_Snapshot->m_DeployBlockers.front().m_Reason);
    else status = Label("COOP_PREP_COMMITTED");
    text(40, 642, 928, 36, status);
    if (m_ConfirmLeave) {
        p.Panel({240, 316, 544, 160}, true);
        text(260, 336, 504, 62, Label(m_Snapshot && m_Snapshot->m_Context.m_LocalPlayer == m_Snapshot->m_Context.m_HostPlayer ? "COOP_READY_HOST_LEAVE" : "COOP_READY_GUEST_LEAVE"));
    }
    for (std::size_t i = 0; i < m_Controls.size(); ++i) {
        const auto &c = m_Controls[i];
        bool selected = i == m_Focus;
        if (c.m_Action == E_CampaignPreparationAction::Slot && m_Snapshot) selected = selected || m_Snapshot->m_Slots[static_cast<std::size_t>(c.m_Key)].m_Soldier == m_Selected;
        if (c.m_Action == E_CampaignPreparationAction::Stock && m_Stock) selected = selected || c.m_Key == m_Stock->Value();
        if (c.m_Action == E_CampaignPreparationAction::Inventory && m_Inventory) selected = selected || c.m_Key == m_Inventory->Value();
        p.Panel(c.m_Rect, selected); p.Text(c.m_Rect, c.m_Text, !c.m_Enabled);
        if (i == m_Focus) p.Focus(c.m_Rect);
    }
    if (m_Focus < m_Controls.size() && !m_Controls[m_Focus].m_Enabled && m_Controls[m_Focus].m_Reason != E_CampaignReason::None)
        text(40, 740, 928, 26, m_Presentation.Reason(m_Controls[m_Focus].m_Reason));
}
