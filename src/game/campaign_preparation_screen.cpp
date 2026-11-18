#include "campaign_preparation_screen.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {
constexpr std::size_t PAGE_SIZE = 4;
constexpr float LEFT = 32, KIT = 232, STOCK = 624;
constexpr float KIT_INSET = KIT + 12, STOCK_INSET = STOCK + 12;
constexpr float KIT_WIDTH = 356, STOCK_WIDTH = 344;
constexpr float LIST_TOP = 326, ROW_STEP = 46, ROW_HEIGHT = 42;
constexpr S_CampaignPreparationRect DOCUMENT{48, 306, 928, 230};

std::string PageNumber(std::size_t page, std::size_t items)
{
    return std::to_string(page + 1) + " / " + std::to_string((items + PAGE_SIZE - 1) / PAGE_SIZE);
}

std::string Weight(float value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

std::string Name(const std::string &value, std::size_t limit) {
    std::size_t end = std::min(value.size(), limit);
    if (end < value.size()) while (end && (static_cast<unsigned char>(value[end]) & 0xc0) == 0x80) --end;
    auto text = value.substr(0, end);
    for (auto &ch : text) if (static_cast<unsigned char>(ch) < 32 || ch == 127) ch = ' ';
    if (end < value.size()) text += "...";
    return text;
}
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
        m_Snapshot->m_Context.m_Kind == E_CampaignSessionKind::CampaignCoop &&
        m_Snapshot->m_Context.m_Phase == E_CampaignPhase::Preparation &&
        m_Snapshot->m_Context.m_LocalPlayer.has_value() && m_Snapshot->m_Context.m_LocalPlayer->IsValid() &&
        m_Snapshot->m_Context.m_Session.IsValid() && m_Snapshot->m_Generation.IsValid() &&
        m_Snapshot->m_PreparationRevision.IsValid() && m_Snapshot->m_ViewRevision.IsValid();
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
E_CampaignReason C_CampaignPreparationScreen::EditReason() const {
    if (m_Status == E_CampaignSyncStatus::Disconnected) return E_CampaignReason::Disconnected;
    if (m_Status != E_CampaignSyncStatus::Current || !m_Snapshot) return E_CampaignReason::SnapshotUnavailable;
    if (m_Snapshot->m_Context.m_Phase != E_CampaignPhase::Preparation) return E_CampaignReason::Frozen;
    if (m_Pending) return E_CampaignReason::PendingTransaction;
    const auto *s = Soldier();
    if (!s) return E_CampaignReason::InvalidSoldier;
    if (!s->m_CanView.m_Allowed) return s->m_CanView.m_Reason;
    if (!s->m_CanEdit.m_Allowed) return s->m_CanEdit.m_Reason;
    if (s->m_Ownership.m_Scripted) return E_CampaignReason::Scripted;
    return E_CampaignReason::NotOwner;
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
std::string C_CampaignPreparationScreen::ItemState(const S_CampaignItemState &state) const {
    std::string summary = m_Presentation.Item(state.m_Item) + " x" + std::to_string(state.m_Quantity);
    if (state.m_AmmoType) summary += " / " + Label("COOP_EQUIP_AMMO") + " " + m_Presentation.Item(*state.m_AmmoType);
    if (state.m_Rounds) summary += " / " + Label("COOP_EQUIP_ROUNDS") + " " + std::to_string(*state.m_Rounds);
    if (state.m_Charges) summary += " / " + Label("COOP_EQUIP_CHARGES") + " " + Number(*state.m_Charges);
    if (state.m_Condition) summary += " / " + Label("COOP_EQUIP_CONDITION") + " " + Number(*state.m_Condition);
    return summary;
}
std::string C_CampaignPreparationScreen::Owner(const S_CampaignSoldierPresentation &s) const {
    if (s.m_Ownership.m_Scripted) return Label("COOP_OWNER_SCRIPTED");
    if (!s.m_Ownership.m_Owner) return Label("COOP_OWNER_AI");
    for (const auto &p : m_Snapshot->m_Participants) if (p.m_Player == *s.m_Ownership.m_Owner)
        return Name(p.m_DisplayName, 48); // painter treats display strings as plain text
    return Label("COOP_OWNER_HUMAN");
}
void C_CampaignPreparationScreen::Refresh() {
    const auto view = m_Model.GetSnapshot();
    m_Status = view.m_Status;
    if (!view.m_Snapshot) { m_Snapshot.reset(); return; }
    const auto &next = view.m_Snapshot;
    // Only GetSnapshot installs state. Events/results are hints, never deltas or rollback.
    if (m_LastSession.IsValid() && next->m_Context.m_Session == m_LastSession &&
        (next->m_Generation < m_LastGeneration || (next->m_Generation == m_LastGeneration && next->m_ViewRevision < m_LastView))) {
        m_Status = E_CampaignSyncStatus::Resyncing;
        m_Notice = E_CampaignReason::StaleRevision;
        m_Model.RequestResync();
        return;
    }
    if (m_Pending && (next->m_Context.m_Session != m_Pending->m_Context.m_Session ||
        next->m_Generation != m_Pending->m_Generation ||
        next->m_Context.m_LocalPlayer != m_Pending->m_Context.m_PlayerEcho)) m_Pending.reset();
    if (m_LastSession.IsValid() && (next->m_Context.m_Session != m_LastSession ||
        next->m_Generation != m_LastGeneration)) {
        m_Pending.reset(); m_Selected.reset(); m_Stock.reset(); m_Inventory.reset(); m_Destination.reset();
        m_Notice = E_CampaignReason::None;
    }
    const auto *previous = Soldier();
    const bool wasEditable = previous && previous->m_CanEdit.m_Allowed;
    const auto controlRevision = previous ? previous->m_Ownership.m_ControlRevision : ControlRevision();
    m_LastSession = next->m_Context.m_Session; m_LastGeneration = next->m_Generation; m_LastView = next->m_ViewRevision;
    m_Snapshot = next;
    bool visible = false;
    for (const auto &slot : next->m_Slots) if (slot.m_Soldier && slot.m_Soldier == m_Selected) visible = true;
    if (!visible) {
        m_Selected.reset();
        for (const auto &slot : next->m_Slots) if (slot.m_Soldier) { m_Selected = slot.m_Soldier; break; }
        m_Inventory.reset(); m_InventoryPage = 0;
    }
    if (next->m_Stock.empty()) {
        m_Stock.reset();
        m_StockPage = 0;
    } else {
        m_StockPage = std::min(m_StockPage, (next->m_Stock.size() - 1) / PAGE_SIZE);
        const auto start = next->m_Stock.begin() + m_StockPage * PAGE_SIZE;
        const auto end = start + std::min<std::size_t>(PAGE_SIZE, next->m_Stock.end() - start);
        if (std::none_of(start, end, [this](const auto &entry) { return entry.m_Entry == m_Stock; })) {
            m_Stock = start->m_Entry;
            m_Quantity = 1;
        }
    }
    const auto *d = Destination();
    if (!d || !m_Destination || std::find(d->m_Placements.begin(), d->m_Placements.end(), *m_Destination) == d->m_Placements.end()) {
        m_Destination.reset();
        if (d && !d->m_Placements.empty()) m_Destination = d->m_Placements.front();
    }
    const auto *loadout = Loadout();
    if (loadout && !loadout->m_Items.empty()) {
        m_InventoryPage = std::min(m_InventoryPage, (loadout->m_Items.size() - 1) / PAGE_SIZE);
        const auto start = loadout->m_Items.begin() + m_InventoryPage * PAGE_SIZE;
        const auto end = start + std::min<std::size_t>(PAGE_SIZE, loadout->m_Items.end() - start);
        if (m_Inventory && std::none_of(start, end, [this](const auto &item) { return item.m_Entry == *m_Inventory; }))
            m_Inventory.reset();
    } else {
        m_Inventory.reset();
        m_InventoryPage = 0;
    }
    if (wasEditable && (!Soldier() || !Soldier()->m_CanEdit.m_Allowed || Soldier()->m_Ownership.m_ControlRevision != controlRevision)) {
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
    const auto itemCaption = [this](const S_CampaignItemState &item) {
        return m_Presentation.Item(item.m_Item) + " x" + std::to_string(item.m_Quantity);
    };
    const bool isEquipment = !m_Briefing && !m_Findings;
    for (std::size_t i = 0; i < 4; ++i) {
        std::string text = Label("COOP_SLOT_UNAVAILABLE"); bool enabled = false;
        if (m_Snapshot) {
            const auto &slot = m_Snapshot->m_Slots[i];
            if (slot.m_Soldier) { for (const auto &s : m_Snapshot->m_Soldiers) if (s.m_Soldier == *slot.m_Soldier) {
                text = Name(s.m_DisplayName, 40) + "\n" + Owner(s) + "\n" +
                    (s.m_CanEdit.m_Allowed ? Label("COOP_OWNER_EDITABLE") : Label("COOP_OWNER_READ_ONLY"));
                enabled = true;
                if (s.m_Ownership.m_Owner) for (const auto &participant : m_Snapshot->m_Participants)
                    if (participant.m_Player == *s.m_Ownership.m_Owner) text += "\n" + Label(participant.m_ReadyAt == m_Snapshot->m_PreparationRevision ? "COOP_READY_YES" : "COOP_READY_NO");
            }
            } // no implicit roster creation
            if (!slot.m_Soldier) text += "\n" + m_Presentation.Reason(slot.m_UnavailableReason);
        }
        add(E_CampaignPreparationAction::Slot, i, {LEFT + static_cast<float>(i) * 244, 100, 228, 96}, text, enabled);
    }
    add(E_CampaignPreparationAction::Page, 0, {LEFT, 212, 188, 34}, Label(m_Briefing ? "COOP_EQUIP_PAGE" : "COOP_BRIEFING_PAGE"));
    add(E_CampaignPreparationAction::Findings, 0, {STOCK, 212, 368, 34},
        Label(m_Findings ? "COOP_EQUIP_PAGE" : "COOP_ESSENTIAL_DETAILS"));
    add(E_CampaignPreparationAction::Skills, 0, {KIT, 212, 380, 34}, Label(m_Skills ? "COOP_EQUIP_PAGE" : "COOP_SKILLS"),
        Loadout() != nullptr && isEquipment);
    const auto *loadout = Loadout();
    const auto *stock = Stock();
    const auto *dest = Destination();
    if (loadout && isEquipment && !m_Skills) {
        for (std::size_t i = m_InventoryPage * PAGE_SIZE; i < loadout->m_Items.size() && i < (m_InventoryPage + 1) * PAGE_SIZE; ++i) {
            const auto &v = loadout->m_Items[i];
            add(E_CampaignPreparationAction::Inventory, v.m_Entry.Value(),
                {KIT_INSET, LIST_TOP + static_cast<float>(i % PAGE_SIZE) * ROW_STEP, KIT_WIDTH, ROW_HEIGHT},
                itemCaption(v.m_State) + "\n" + m_Presentation.Placement(v.m_Placement.m_Slot));
        }
        if (loadout->m_Items.size() > PAGE_SIZE) {
            add(E_CampaignPreparationAction::InventoryPage, 1, {KIT_INSET, 520, 124, 32},
                Label("COOP_PAGE_PREVIOUS"), m_InventoryPage > 0);
            add(E_CampaignPreparationAction::InventoryPage, 0, {KIT_INSET + 232, 520, 124, 32},
                Label("COOP_PAGE_NEXT"), (m_InventoryPage + 1) * PAGE_SIZE < loadout->m_Items.size());
        }
    }
    if (m_Briefing || m_Findings) {
        add(E_CampaignPreparationAction::BriefingPage, 0, {48, 554, 144, 34}, Label("COOP_PAGE_PREVIOUS"), m_BriefingPage > 0);
        add(E_CampaignPreparationAction::BriefingPage, 1, {832, 554, 144, 34}, Label("COOP_PAGE_NEXT"), m_BriefingPage + 1 < m_TextPages);
    }
    if (loadout && m_Skills && !m_Briefing && !m_Findings) {
        m_SkillPage = std::min(m_SkillPage, loadout->m_Skills.empty() ? std::size_t(0) : (loadout->m_Skills.size()-1)/PAGE_SIZE);
        if (loadout->m_Skills.size() > PAGE_SIZE) {
            add(E_CampaignPreparationAction::SkillsPage, 0, {KIT_INSET, 520, 124, 32}, Label("COOP_PAGE_PREVIOUS"), m_SkillPage > 0);
            add(E_CampaignPreparationAction::SkillsPage, 1, {KIT_INSET + 232, 520, 124, 32}, Label("COOP_PAGE_NEXT"), (m_SkillPage+1)*PAGE_SIZE < loadout->m_Skills.size());
        }
    }
    if (m_Snapshot && isEquipment) {
        for (std::size_t i = m_StockPage * PAGE_SIZE; i < m_Snapshot->m_Stock.size() && i < (m_StockPage + 1) * PAGE_SIZE; ++i) {
            const auto &v = m_Snapshot->m_Stock[i];
            add(E_CampaignPreparationAction::Stock, v.m_Entry.Value(),
                {STOCK_INSET, LIST_TOP + static_cast<float>(i % PAGE_SIZE) * ROW_STEP, STOCK_WIDTH, ROW_HEIGHT},
                itemCaption(v.m_State));
        }
        if (m_Snapshot->m_Stock.size() > PAGE_SIZE) {
            add(E_CampaignPreparationAction::StockPage, 1, {STOCK_INSET, 520, 124, 32},
                Label("COOP_PAGE_PREVIOUS"), m_StockPage > 0);
            add(E_CampaignPreparationAction::StockPage, 0, {STOCK_INSET + 220, 520, 124, 32},
                Label("COOP_PAGE_NEXT"), (m_StockPage + 1) * PAGE_SIZE < m_Snapshot->m_Stock.size());
        }
        add(E_CampaignPreparationAction::QuantityDown, 0, {STOCK_INSET + 204, 574, 32, 34}, "-", m_Quantity > 1);
        add(E_CampaignPreparationAction::QuantityUp, 0, {STOCK_INSET + 268, 574, 32, 34}, "+", m_Quantity < 999);
        add(E_CampaignPreparationAction::Destination, 0, {STOCK_INSET, 574, 194, 34},
            m_Destination ? m_Presentation.Placement(*m_Destination) + "  >" : Label("COOP_EQUIP_NO_DESTINATION"),
            dest && !dest->m_Placements.empty());
        E_CampaignReason denial = !CanEdit() ? EditReason() :
            !stock || stock->m_State.m_Quantity < m_Quantity ? E_CampaignReason::StockUnavailable :
            !dest ? E_CampaignReason::InvalidPlacement : dest->m_CanTake.m_Reason;
        const bool take = CanEdit() && stock && stock->m_State.m_Quantity >= m_Quantity && dest && dest->m_CanTake.m_Allowed && m_Destination;
        const auto countLabel = m_Quantity > 1 ? " x" + std::to_string(m_Quantity) : "";
        add(E_CampaignPreparationAction::Equip, 0, {STOCK_INSET + 102, 613, 242, 36},
            Label("COOP_EQUIP_TAKE") + countLabel, take, denial);
        bool canReturn = false;
        if (loadout && m_Inventory && !m_Skills) for (const auto &entry : loadout->m_Items)
            if (entry.m_Entry == *m_Inventory && entry.m_State.m_Quantity >= m_Quantity) canReturn = true;
        add(E_CampaignPreparationAction::Return, 0, {KIT_INSET + 207, 613, 149, 36},
            Label("COOP_EQUIP_RETURN") + countLabel, CanEdit() && canReturn,
            !CanEdit() ? EditReason() : !m_Inventory || m_Skills ? E_CampaignReason::InvalidItem : E_CampaignReason::InvalidCount);
    }
    const auto *s = Soldier();
    add(E_CampaignPreparationAction::Claim, 0, {LEFT + 12, 613, 78, 36}, Label("COOP_SLOT_CLAIM"), IsCurrent() && !m_Pending && s && s->m_CanClaim.m_Allowed,
        s ? s->m_CanClaim.m_Reason : E_CampaignReason::Ineligible);
    add(E_CampaignPreparationAction::Release, 0, {LEFT + 98, 613, 78, 36}, Label("COOP_SLOT_RELEASE"), IsCurrent() && !m_Pending && s && s->m_CanRelease.m_Allowed,
        s ? s->m_CanRelease.m_Reason : E_CampaignReason::Ineligible);
    bool ready = false;
    if (m_Snapshot) for (const auto &p : m_Snapshot->m_Participants) if (p.m_Player == m_Snapshot->m_Context.m_LocalPlayer)
        ready = p.m_ReadyAt == m_Snapshot->m_PreparationRevision;
    add(E_CampaignPreparationAction::Ready, 0, {552, 714, 132, 38}, Label(ready ? "COOP_READY_UNSET" : "COOP_READY_SET"), IsCurrent() && !m_Pending && HasLocalSoldier(), E_CampaignReason::Ineligible);
    if (m_Snapshot && m_Snapshot->m_Context.m_LocalPlayer == m_Snapshot->m_Context.m_HostPlayer && m_Snapshot->m_Context.m_LocalPlayer)
        add(E_CampaignPreparationAction::Deploy, 0, {696, 714, 152, 38}, Label("COOP_READY_DEPLOY"), IsCurrent() && !m_Pending && m_Snapshot->m_CanDeploy.m_Allowed, m_Snapshot->m_CanDeploy.m_Reason);
    add(E_CampaignPreparationAction::Leave, 0, {860, 714, 132, 38}, Label("COOP_READY_LEAVE"));
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
    m_Notice = E_CampaignReason::None;
    using A = E_CampaignPreparationAction;
    switch (control.m_Action) {
    case A::Slot:
        m_Selected = m_Snapshot->m_Slots[static_cast<std::size_t>(control.m_Key)].m_Soldier;
        m_Inventory.reset(); m_Destination.reset(); m_Quantity = 1; m_InventoryPage = 0; m_SkillPage = 0; Refresh(); break;
    case A::Stock: m_Stock = StockEntryId(control.m_Key); m_Destination.reset(); m_Quantity = 1; Refresh(); break;
    case A::Inventory: m_Inventory = PreparationItemId(control.m_Key); m_Quantity = 1; break;
    case A::Page: m_Briefing = !m_Briefing; m_Findings = false; m_BriefingPage = 0; break;
    case A::Findings: m_Findings = !m_Findings; m_Briefing = false; m_BriefingPage = 0; break;
    case A::InventoryPage:
        if (control.m_Key) --m_InventoryPage;
        else ++m_InventoryPage;
        m_Inventory.reset();
        m_Quantity = 1;
        break;
    case A::StockPage:
        if (control.m_Key) --m_StockPage;
        else ++m_StockPage;
        m_Destination.reset();
        Refresh();
        break;
    case A::BriefingPage: if (control.m_Key) { if (m_BriefingPage+1 < m_TextPages) ++m_BriefingPage; } else if (m_BriefingPage) --m_BriefingPage; break;
    case A::SkillsPage: if (control.m_Key) ++m_SkillPage; else if (m_SkillPage) --m_SkillPage; break;
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
    case A::Leave: m_ConfirmLeave = true; m_Focus = 1; break;
    case A::CancelLeave: m_ConfirmLeave = false; break;
    case A::ConfirmLeave: m_LeaveRequested = true; Close(); return;
    default: {
        if (!IsCurrent() || m_Pending) return;
        const auto id = m_Presentation.NextRequestId();
        if (!id.IsValid()) { m_Notice = E_CampaignReason::RequestExpired; return; }
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
    if (key == K::Escape) {
        m_ConfirmLeave = !m_ConfirmLeave;
        if (m_ConfirmLeave) m_Focus = 1; // Enter keeps the session until the player selects confirmation.
        BuildControls();
        return true;
    }
    if (key == K::Enter || key == K::Space) { Activate(m_Focus); return true; }
    if (key == K::Left || key == K::Right || key == K::Up || key == K::Down) {
        auto &c = m_Controls[m_Focus];
        const bool forward = key == K::Right || key == K::Down;
        const bool inventory = c.m_Action == E_CampaignPreparationAction::Inventory;
        const bool stock = c.m_Action == E_CampaignPreparationAction::Stock;
        if ((inventory && Loadout() && !Loadout()->m_Items.empty()) || (stock && m_Snapshot && !m_Snapshot->m_Stock.empty())) {
            const auto count = inventory ? Loadout()->m_Items.size() : m_Snapshot->m_Stock.size();
            std::size_t position = 0;
            for (std::size_t i = 0; i < count; ++i) {
                const auto id = inventory ? Loadout()->m_Items[i].m_Entry.Value() : m_Snapshot->m_Stock[i].m_Entry.Value();
                if (id == c.m_Key) position = i;
            }
            const auto step = key == K::Left || key == K::Right ? PAGE_SIZE : std::size_t(1);
            position = forward ? (position + step) % count : (position + count - step % count) % count;
            const auto id = inventory ? Loadout()->m_Items[position].m_Entry.Value() : m_Snapshot->m_Stock[position].m_Entry.Value();
            m_Quantity = 1;
            m_Notice = E_CampaignReason::None;
            if (inventory) { m_Inventory = PreparationItemId(id); m_InventoryPage = position / PAGE_SIZE; }
            else { m_Stock = StockEntryId(id); m_StockPage = position / PAGE_SIZE; m_Destination.reset(); Refresh(); }
            BuildControls();
            for (std::size_t i = 0; i < m_Controls.size(); ++i) if (m_Controls[i].m_Key == id &&
                m_Controls[i].m_Action == (inventory ? E_CampaignPreparationAction::Inventory : E_CampaignPreparationAction::Stock)) m_Focus = i;
            return true;
        }
        if (m_Skills && !m_Briefing && !m_Findings && Loadout() && !Loadout()->m_Skills.empty()) {
            const auto pages = (Loadout()->m_Skills.size()+PAGE_SIZE-1)/PAGE_SIZE;
            m_SkillPage = forward ? (m_SkillPage+1)%pages : (m_SkillPage+pages-1)%pages;
            BuildControls(); return true;
        }
        if (m_Briefing || m_Findings) { if (forward) { if (m_BriefingPage+1 < m_TextPages) ++m_BriefingPage; } else if (m_BriefingPage) --m_BriefingPage; return true; }
    }
    const bool back = key == K::BackTab || key == K::Left || key == K::Up;
    m_Focus = back ? (m_Focus + m_Controls.size() - 1) % m_Controls.size() : (m_Focus + 1) % m_Controls.size();
    return true;
}
void C_CampaignPreparationScreen::Render(I_CampaignPreparationPainter &p) {
    Tick(); if (!m_Open) return;
    using A = E_CampaignPreparationAction;
    using Style = E_CampaignPreparationControlStyle;
    p.Panel({0, 0, 1024, 768}, false);
    auto text = [&p](float x, float y, float w, float h, const std::string &line, bool muted = false) {
        p.Text({x, y, w, h}, line, muted);
    };
    p.Heading({LEFT, 18, 960, 36}, Label("COOP_PREP_TITLE"), true);
    p.Heading({LEFT, 58, 960, 28}, m_Snapshot ?
        m_Presentation.Text(m_Snapshot->m_Briefing.m_OperationTitle) + " / " +
        m_Presentation.Text(m_Snapshot->m_Briefing.m_MissionTitle) : Label("COOP_PREP_RECEIVING"), false);
    p.Panel({LEFT, 260, 188, 398}, false);
    p.Panel({KIT, 260, 380, 398}, false);
    p.Panel({STOCK, 260, 368, 398}, false);
    p.Panel({LEFT, 666, 960, 40}, false);
    p.Panel({LEFT, 714, 508, 38}, false);
    if (m_Snapshot) {
        const auto *s = Soldier();
        const auto *l = Loadout();
        const auto body = m_Presentation.Text(m_Snapshot->m_Briefing.m_Body);
        if (m_Briefing || m_Findings) {
            std::string detail = body;
            if (m_Findings) {
                detail = Label("COOP_ESSENTIAL_TITLE") + "\n";
                if (s) detail += Name(s->m_DisplayName, 240) + " / " + Owner(*s) + "\n";
                for (const auto &e : m_Snapshot->m_Essentials) {
                    detail += m_Presentation.Item(e.m_Item) + " " + std::to_string(e.m_Carried) + "/" + std::to_string(e.m_Required) + "\n";
                    detail += Label("COOP_ESSENTIAL_ELIGIBLE") + " ";
                    for (const auto carrier : e.m_EligibleCarriers) for (const auto &candidate : m_Snapshot->m_Soldiers)
                        if (candidate.m_Soldier == carrier) detail += Name(candidate.m_DisplayName, 240) + " (" + Owner(candidate) + ")  ";
                    detail += "\n";
                }
                auto findings = [this, &detail](const auto &list) {
                    for (const auto &f : list) detail += (f.m_Message.IsValid() ? m_Presentation.Text(f.m_Message) : m_Presentation.Reason(f.m_Reason)) + "\n";
                };
                findings(m_Snapshot->m_Findings); findings(m_Snapshot->m_DeployBlockers);
                if (s) findings(s->m_Restrictions);
                if (l) findings(l->m_Findings);
                if (const auto *stock = Stock()) {
                    detail += ItemState(stock->m_State) + "\n" + m_Presentation.ItemDescription(stock->m_State.m_Item) + "\n";
                    for (const auto &constraint : stock->m_Constraints) {detail += m_Presentation.Text(constraint.m_Description) + "\n"; findings(constraint.m_Findings);}
                }
                if (l && m_Inventory) for (const auto &entry : l->m_Items) if (entry.m_Entry == *m_Inventory)
                    detail += ItemState(entry.m_State) + "\n" + m_Presentation.ItemDescription(entry.m_State.m_Item) + "\n";
            }
            const bool showImage = m_Briefing && m_Snapshot->m_Briefing.m_Image.IsValid();
            auto document = DOCUMENT;
            if (showImage)
                document.m_Width = 648;
            auto pages = p.Paginate(document, detail);
            if (pages.empty()) pages.emplace_back();
            m_TextPages = pages.size();
            m_BriefingPage = std::min(m_BriefingPage, m_TextPages - 1);
            p.Panel({LEFT, 260, 960, 398}, false);
            text(48, 274, 928, 26, Label(m_Briefing ? "COOP_BRIEFING_TITLE" : "COOP_ESSENTIAL_DETAILS"));
            p.Text(document, pages[m_BriefingPage]);
            if (showImage)
                p.Asset({712, 306, 260, 195}, m_Snapshot->m_Briefing.m_Image);
            text(478, 554, 100, 34, std::to_string(m_BriefingPage + 1) + " / " + std::to_string(m_TextPages));
            BuildControls();
        } else {
            text(44, 274, 164, 26, Label("COOP_PREP_SOLDIER"));
            text(KIT_INSET, 274, KIT_WIDTH, 26, Label("COOP_EQUIP_KIT"));
            text(STOCK_INSET, 274, STOCK_WIDTH, 26, Label("COOP_EQUIP_STOCK"));
            if (s) {
                text(44, 302, 164, 38, Name(s->m_DisplayName, 40));
                if (s->m_CanView.m_Allowed && l) {
                    p.Panel({44, 350, 164, 247}, true);
                    p.Preview({44, 350, 164, 247}, s->m_Preview, *l);
                }
                text(KIT_INSET, 301, KIT_WIDTH, 24,
                    Label(CanEdit() ? (s->m_Ownership.m_Owner ? "COOP_OWNER_EDIT_OWN" : "COOP_OWNER_EDIT_AI") :
                          "COOP_OWNER_READ_ONLY") + " / " + Owner(*s), !CanEdit());
                if (l) {
                    if (m_Skills) {
                        float y = LIST_TOP;
                        for (std::size_t i = m_SkillPage * PAGE_SIZE; i < l->m_Skills.size() && i < (m_SkillPage + 1) * PAGE_SIZE; ++i) {
                            const auto &skill = l->m_Skills[i];
                            text(KIT_INSET, y, KIT_WIDTH, 30, m_Presentation.Skill(skill.m_Key) + "  " + Number(skill.m_Value));
                            y += ROW_STEP;
                        }
                        if (l->m_Skills.size() > PAGE_SIZE)
                            text(KIT_INSET + 149, 520, 70, 32, PageNumber(m_SkillPage, l->m_Skills.size()));
                    } else if (l->m_Items.size() > PAGE_SIZE) {
                        text(KIT_INSET + 149, 520, 70, 32, PageNumber(m_InventoryPage, l->m_Items.size()));
                    }
                    text(KIT_INSET, 565, KIT_WIDTH, 24, Label("COOP_EQUIP_WEIGHT") + "  " + Weight(l->m_Weight) +
                        " / " + Weight(l->m_WeightCapacity));
                    text(KIT_INSET, 587, KIT_WIDTH, 24, Label("COOP_EQUIP_BELT_PACK") + "  " +
                        Number(l->m_BeltVolume) + "/" + Number(l->m_BeltCapacity) + "    " +
                        Number(l->m_BackpackVolume) + "/" + Number(l->m_BackpackCapacity));
                } else {
                    text(KIT_INSET, LIST_TOP, KIT_WIDTH, 100, m_Presentation.Reason(s->m_CanView.m_Reason));
                }
            }
            if (m_Snapshot->m_Stock.size() > PAGE_SIZE)
                text(STOCK_INSET + 149, 520, 70, 32, PageNumber(m_StockPage, m_Snapshot->m_Stock.size()));
            text(STOCK_INSET, 549, 194, 22, Label("COOP_EQUIP_DESTINATION"));
            text(STOCK_INSET + 204, 549, 116, 22, Label("COOP_EQUIP_QUANTITY"));
            text(STOCK_INSET + 238, 578, 34, 26, std::to_string(m_Quantity));
        }
        std::string essentials = Label("COOP_ESSENTIAL_TITLE");
        for (const auto &e : m_Snapshot->m_Essentials) essentials += "  " + m_Presentation.Item(e.m_Item) + " " + std::to_string(e.m_Carried) + "/" + std::to_string(e.m_Required) + (e.m_Deficit ? " !" : "");
        text(44, 666, 936, 19, essentials);
        std::string ready;
        for (const auto &participant : m_Snapshot->m_Participants) if (participant.m_Connected) ready += Name(participant.m_DisplayName, 24) + ": " +
            Label(participant.m_ReadyAt == m_Snapshot->m_PreparationRevision ? "COOP_READY_YES" : "COOP_READY_NO") + "  ";
        text(44, 714, 484, 38, ready);
    }
    std::string status;
    if (m_Status != E_CampaignSyncStatus::Current) status = Label(m_Status == E_CampaignSyncStatus::Disconnected ? "COOP_PREP_DISCONNECTED" : "COOP_PREP_RECEIVING");
    else if (m_Snapshot && m_Snapshot->m_Context.m_Phase != E_CampaignPhase::Preparation) status = Label("COOP_PREP_FROZEN");
    else if (m_Pending) status = Label("COOP_PREP_PENDING");
    else if (m_Notice != E_CampaignReason::None) status = m_Presentation.Reason(m_Notice);
    else if (m_Focus < m_Controls.size() && !m_Controls[m_Focus].m_Enabled &&
        m_Controls[m_Focus].m_Reason != E_CampaignReason::None)
        status = m_Presentation.Reason(m_Controls[m_Focus].m_Reason);
    else if (m_Snapshot && !HasLocalSoldier()) status = Label("COOP_OWNER_NO_SOLDIER");
    else if (m_Snapshot && !m_Snapshot->m_DeployBlockers.empty()) status = m_Presentation.Reason(m_Snapshot->m_DeployBlockers.front().m_Reason);
    else status = Label("COOP_PREP_COMMITTED");
    text(44, 684, 936, 21, status);
    if (m_ConfirmLeave) {
        p.Panel({240, 316, 544, 160}, false);
        text(260, 336, 504, 62, Label(m_Snapshot && m_Snapshot->m_Context.m_LocalPlayer == m_Snapshot->m_Context.m_HostPlayer ? "COOP_READY_HOST_LEAVE" : "COOP_READY_GUEST_LEAVE"));
    }
    for (std::size_t i = 0; i < m_Controls.size(); ++i) {
        const auto &c = m_Controls[i];
        bool selected = false;
        Style style = Style::Button;
        switch (c.m_Action) {
        case A::Slot:
            style = Style::Card;
            selected = m_Selected && m_Snapshot &&
                m_Snapshot->m_Slots[static_cast<std::size_t>(c.m_Key)].m_Soldier == m_Selected;
            break;
        case A::Stock:
            style = Style::Row;
            selected = m_Stock && c.m_Key == m_Stock->Value();
            break;
        case A::Inventory:
            style = Style::Row;
            selected = m_Inventory && c.m_Key == m_Inventory->Value();
            break;
        case A::Page: selected = m_Briefing; break;
        case A::Findings: selected = m_Findings; break;
        case A::Skills: selected = m_Skills && !m_Briefing && !m_Findings; break;
        case A::Deploy:
        case A::Equip:
        case A::ConfirmLeave:
            style = Style::Primary;
            break;
        default: break;
        }
        p.Control(c.m_Rect, style, selected, c.m_Enabled);
        auto caption = c.m_Rect;
        caption.m_X += 7;
        caption.m_Width -= 12;
        if (c.m_Action == A::Slot && m_Snapshot) {
            caption.m_Y += 5;
            const auto soldier = m_Snapshot->m_Slots[static_cast<std::size_t>(c.m_Key)].m_Soldier;
            for (const auto &candidate : m_Snapshot->m_Soldiers) {
                if (soldier == candidate.m_Soldier && candidate.m_CanView.m_Allowed && candidate.m_Portrait.IsValid()) {
                    p.Asset({caption.m_X + 155, caption.m_Y + 5, 50, 68}, candidate.m_Portrait);
                    caption.m_Width = 148;
                    break;
                }
            }
        } else if (style == Style::Button || style == Style::Primary) {
            caption.m_Y += 5;
            caption.m_Height -= 5;
        }
        if (c.m_Action == A::Inventory) {
            const auto lineBreak = c.m_Text.find('\n');
            p.Text({caption.m_X, caption.m_Y, caption.m_Width, 20}, c.m_Text.substr(0, lineBreak));
            p.Text({caption.m_X, caption.m_Y + 19, caption.m_Width, 21}, c.m_Text.substr(lineBreak + 1), true);
        } else {
            p.Text(caption, c.m_Text, !c.m_Enabled);
        }
        if (i == m_Focus)
            p.Focus(c.m_Rect);
    }
}
