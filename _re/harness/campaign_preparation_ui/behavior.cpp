#include "fake.h"
#include <iostream>

void Check(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
void Activate(C_CampaignPreparationScreen &ui, E_CampaignPreparationAction action, std::uint64_t key = 0) {
    ui.Tick();
    for (const auto &c : ui.GetControls()) if (c.m_Action == action && c.m_Key == key) {
        ui.Click(c.m_Rect.m_X + 2, c.m_Rect.m_Y + 2); return;
    }
    throw std::runtime_error("missing control");
}
bool Enabled(C_CampaignPreparationScreen &ui, E_CampaignPreparationAction action) {
    ui.Tick(); for (const auto &c : ui.GetControls()) if (c.m_Action == action) return c.m_Enabled;
    return false;
}
int main() {
    using A = E_CampaignPreparationAction;
    C_FakePreparation fake;
    {
        C_CampaignPreparationScreen ui(fake, fake);
        Check(Enabled(ui, A::Equip), "host own edit");
        Activate(ui, A::Slot, 1); Check(!Enabled(ui, A::Equip), "host cannot edit guest");
        Activate(ui, A::Equip); Check(fake.m_Submissions == 0, "disabled shortcut must not submit");
        Activate(ui, A::Slot, 2); Check(Enabled(ui, A::Equip), "host AI edit");
        Check(fake.m_Submissions == 0, "browsing does not mutate ownership");
        Activate(ui, A::Equip); Check(ui.IsPending(), "queue creates pending");
        Check(ui.GetSnapshot()->m_Stock[0].m_State.m_Quantity == 1, "pending no optimistic stock debit");
        Check(fake.m_Edit->m_Targets[0].m_Soldier == SoldierId(3), "selected AI target");
        Check(fake.m_Edit->m_Targets[0].m_ExpectedControlRevision == ControlRevision(1), "control fence");
        auto wrong = fake.Result(); wrong.m_Result.m_Context.m_Request = RequestId(1000);
        fake.m_Events.push_back(wrong); ui.Tick(); Check(ui.IsPending(), "wrong request cannot clear pending");
        wrong = fake.Result(); wrong.m_LocalToken = LocalSubmissionId(999);
        fake.m_Events.push_back(wrong); ui.Tick(); Check(ui.IsPending(), "wrong token cannot clear pending");
        wrong = fake.Result(); wrong.m_Result.m_Family = E_CampaignRequestFamily::Ownership;
        fake.m_Events.push_back(wrong); ui.Tick(); Check(ui.IsPending(), "wrong family cannot clear pending");
        fake.Resolve(); ui.Tick(); Check(!ui.IsPending(), "matching result clears pending");
        Check(ui.GetSnapshot()->m_Stock[0].m_State.m_Quantity == 0 && ui.GetSnapshot()->m_Loadouts[2].m_Items.size() == 2, "atomic display");
        Check(!Enabled(ui, A::Equip), "last item exhausted");
        Activate(ui, A::Inventory, ui.GetSnapshot()->m_Loadouts[2].m_Items.back().m_Entry.Value());
        Activate(ui, A::Return); fake.Resolve(); ui.Tick(); Check(ui.GetSnapshot()->m_Stock[0].m_State.m_Quantity == 1, "return stock");
        Activate(ui, A::Equip); fake.m_State.m_Soldiers[2].m_Ownership.m_Owner = PlayerId(2);
        fake.m_State.m_Soldiers[2].m_Ownership.m_ControlRevision = ControlRevision(2); fake.Publish(true);
        ui.Tick(); Check(!Enabled(ui, A::Equip), "claim invalidates host rights immediately");
        fake.Resolve(E_CampaignReason::NotOwner); ui.Tick(); Check(ui.GetNotice() == E_CampaignReason::NotOwner, "rejection notice");
        Check(ui.GetSelection() == SoldierId(3), "persistent identity selection");
        fake.m_State.m_Context.m_LocalPlayer = PlayerId(2); fake.Publish(); ui.Tick();
        Check(Enabled(ui, A::Equip), "guest own edit"); Activate(ui, A::Slot, 3); Check(!Enabled(ui, A::Equip), "guest AI readonly");
        Activate(ui, A::Slot, 0); Check(!Enabled(ui, A::Equip), "guest other human readonly");
        fake.m_Status = E_CampaignSyncStatus::Loading; ui.Tick(); Check(!Enabled(ui, A::Ready) && !Enabled(ui, A::Equip), "loading gates");
        Check(!ui.GetSnapshot(), "no partial loading display"); fake.m_Status = E_CampaignSyncStatus::Current; ui.Tick();
        fake.m_State.m_Context.m_Phase = E_CampaignPhase::Frozen; fake.Publish(); ui.Tick();
        Check(!Enabled(ui, A::Equip) && !Enabled(ui, A::Ready) && !Enabled(ui, A::Claim), "frozen gates");
        fake.m_State.m_Context.m_Phase = E_CampaignPhase::Preparation; fake.Publish();
        fake.m_Status = E_CampaignSyncStatus::Disconnected; ui.Tick(); Check(!Enabled(ui, A::Ready), "disconnect gates");
        fake.m_Status = E_CampaignSyncStatus::Current;
        const int polls = fake.m_Polls; ui.Close(); ui.Tick(); Check(fake.m_Polls == polls, "closed UI never polls");
    }
    fake.Reset();
    {
        C_CampaignPreparationScreen ui(fake, fake);
        Activate(ui, A::Equip); const auto old = fake.Result();
        ui.Close(); fake.Resolve();
        C_CampaignPreparationScreen reopened(fake, fake);
        Check(reopened.GetSnapshot()->m_Stock[0].m_State.m_Quantity == 0, "reopen gets committed state");
        Activate(reopened, A::Ready); Check(reopened.IsPending(), "new screen ready request");
        Check(fake.m_Ready->m_Context.m_Request.m_Request != old.m_Result.m_Context.m_Request, "request IDs survive reopen");
        fake.m_Events.push_back(old); reopened.Tick(); Check(reopened.IsPending(), "old screen result cannot clear new request");
        fake.Resolve(); reopened.Tick();
        Check(reopened.GetSnapshot()->m_Participants[0].m_ReadyAt == reopened.GetSnapshot()->m_PreparationRevision, "readiness committed revision");
        const auto revision = fake.m_State.m_PreparationRevision;
        Activate(reopened, A::Slot, 2); Check(fake.m_State.m_PreparationRevision == revision, "selection preserves readiness");
        fake.m_State.m_Slots[0].m_Soldier = SoldierId(3); fake.m_State.m_Slots[2].m_Soldier = SoldierId(1); fake.Publish(); reopened.Tick();
        Check(reopened.GetSelection() == SoldierId(3), "selection survives reorder");
        fake.m_Events.push_back(S_CampaignStatusChanged{}); reopened.Tick();
        Check(Enabled(reopened, A::Ready), "obsolete closed event cannot override current snapshot");
        fake.m_State.m_ViewRevision = ViewRevision(1); reopened.Tick(); Check(fake.m_Resyncs > 0 && !Enabled(reopened, A::Ready), "rollback resync");
    }
    fake.Reset();
    {
        fake.m_State.m_Context.m_LocalPlayer = PlayerId(7); fake.Rights();
        for (std::size_t i = 1; i < 4; ++i) fake.m_State.m_Slots[i] = {{}, E_CampaignReason::MissionRestriction, {}};
        C_CampaignPreparationScreen ui(fake, fake); Check(!Enabled(ui, A::Ready), "restricted spectator no ready");
        Check(!Enabled(ui, A::Equip), "restricted guest no edit");
        fake.m_State.m_Context.m_LocalPlayer = PlayerId(1); fake.m_State.m_Soldiers[0].m_Ownership.m_Owner.reset(); fake.Publish();
        Check(Enabled(ui, A::Equip) && !Enabled(ui, A::Ready), "host without soldier edits AI but cannot ready");
        fake.m_State.m_Soldiers[0].m_Ownership.m_Scripted = true; fake.Publish();
        Check(!Enabled(ui, A::Equip), "scripted control suspends edit");
        ui.Key(E_CampaignPreparationKey::Escape); ui.Key(E_CampaignPreparationKey::Escape); Check(ui.IsOpen(), "leave cancellation");
        Activate(ui, A::Leave); Activate(ui, A::ConfirmLeave); Check(ui.LeaveRequested() && !ui.IsOpen(), "confirmed leave");
    }
    fake.Reset();
    {
        C_CampaignPreparationScreen ui(fake, fake);
        Activate(ui,A::Equip);
        for (int field = 0; field < 3; ++field) {
            auto wrong = fake.Result();
            if (field == 0) wrong.m_Result.m_Context.m_Session = SessionId(999);
            if (field == 1) wrong.m_Result.m_Context.m_PlayerEcho = PlayerId(999);
            if (field == 2) wrong.m_Result.m_PreparationGeneration = PreparationGeneration(999);
            fake.m_Events.push_back(wrong); ui.Tick(); Check(ui.IsPending(), "full result scope correlation");
        }
        auto retained = ui.GetSnapshot();
        fake.Resolve(E_CampaignReason::CapacityExceeded); ui.Tick();
        Check(ui.GetNotice() == E_CampaignReason::CapacityExceeded && ui.GetSnapshot()->m_Stock[0].m_State.m_Quantity == 1, "capacity rejection preserves display");
        const auto focus = ui.GetFocus(); fake.Publish(); ui.Tick(); Check(ui.GetFocus() == focus, "remote stock refresh preserves focus");
        fake.m_State.m_Soldiers[0].m_CanView = {false,E_CampaignReason::Ineligible};
        ui.Tick(); Check(!Enabled(ui,A::Skills) && !Enabled(ui,A::Equip), "visibility denial cannot expose inventory controls");
        fake.m_State.m_Soldiers[0].m_CanView = {true,E_CampaignReason::None};
        // Readiness-only view changes must not be sent as preparation revisions.
        fake.Publish(); Activate(ui,A::Ready);
        Check(fake.m_Ready->m_Context.m_ExpectedPreparationRevision == fake.m_State.m_PreparationRevision, "ready binds preparation revision");
        Check(fake.m_Ready->m_Context.m_ExpectedPreparationRevision.Value() != fake.m_State.m_ViewRevision.Value(), "view and preparation revision separated");
        fake.Resolve(); ui.Tick();
        ui.Close(); Check(retained->m_Stock[0].m_State.m_Quantity == 1, "retained snapshot survives screen close");
    }
    fake.Reset();
    {
        for (std::uint64_t id = 2; id <= 10; ++id) { auto stock = fake.m_State.m_Stock[0]; stock.m_Entry = StockEntryId(id); fake.m_State.m_Stock.push_back(stock); }
        C_CampaignPreparationScreen ui(fake,fake);
        Activate(ui,A::Stock,1); ui.Key(E_CampaignPreparationKey::Down);
        Activate(ui,A::Equip); Check(std::get<S_CampaignTakeFromStock>(fake.m_Edit->m_Operation).m_StockEntry == StockEntryId(2), "arrow selects stock identity");
        fake.Resolve(E_CampaignReason::MissionRestriction); ui.Tick();
        Activate(ui,A::StockPage); Activate(ui,A::Stock,5);
        Activate(ui,A::Equip); Check(std::get<S_CampaignTakeFromStock>(fake.m_Edit->m_Operation).m_StockEntry == StockEntryId(5), "mouse stock paging");
        fake.Resolve(E_CampaignReason::MissionRestriction); ui.Tick();
        Activate(ui,A::Findings); Check(!Enabled(ui,A::Equip), "restriction detail page has no equipment mutations");
        Activate(ui,A::Findings); Activate(ui,A::Equip);
        const auto old = fake.Result();
        fake.Reset(); ui.Tick(); Check(!ui.IsPending(), "new generation discards pending presentation");
        fake.m_Events.push_back(old); ui.Tick(); Check(ui.GetSnapshot()->m_Generation == fake.m_State.m_Generation, "obsolete result cannot install old generation");
        fake.m_Status = E_CampaignSyncStatus::Resyncing; ui.Tick();
        Check(!Enabled(ui,A::Equip) && !Enabled(ui,A::Ready), "snapshot-gap resync gates mutation");
        const auto generation = fake.m_State.m_Generation;
        fake.m_Status = E_CampaignSyncStatus::Loading; ui.Tick();
        fake.m_State.m_Generation = PreparationGeneration(generation.Value()-1); fake.m_Status = E_CampaignSyncStatus::Current;
        ui.Tick(); Check(fake.m_Resyncs > 0 && !ui.GetSnapshot(), "loading cannot erase generation freshness fence");
    }
    std::cout << "PASS: campaign preparation component permissions, transactions, races, lifecycle and input\n";
}
