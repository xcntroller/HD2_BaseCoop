#ifndef HD2_CAMPAIGN_PREPARATION_SCREEN_H
#define HD2_CAMPAIGN_PREPARATION_SCREEN_H

#include "campaign_contracts.h"

// New component, with no legacy ABI/layout or global campaign references.
// Coordinates use the 1024x768 campaign safe area; the renderer applies letterboxing.
struct S_CampaignPreparationRect {
    float m_X, m_Y, m_Width, m_Height;
    bool Contains(float x, float y) const;
};
class I_CampaignPreparationPainter {
public:
    virtual ~I_CampaignPreparationPainter() = default;
    // Return complete pages measured with exactly the same font/wrap rules as Text.
    // Preserve UTF-8 boundaries; every line must be reachable at this rectangle size.
    virtual std::vector<std::string> Paginate(const S_CampaignPreparationRect &rect, const std::string &text) = 0;
    virtual void Panel(const S_CampaignPreparationRect &rect, bool selected) = 0;
    virtual void Text(const S_CampaignPreparationRect &rect, const std::string &text,
                      bool muted = false) = 0;
    // Missing catalog assets must be explicit placeholders, never unchecked frames.
    virtual void Preview(const S_CampaignPreparationRect &rect, AssetKey asset,
        const S_CampaignSoldierLoadout &loadout) = 0;
    virtual void Focus(const S_CampaignPreparationRect &rect) = 0;
    virtual void Asset(const S_CampaignPreparationRect &rect, AssetKey asset) = 0;
};
class I_CampaignPreparationPresentation {
public:
    virtual ~I_CampaignPreparationPresentation() = default;
    virtual std::string Text(TextKey key) const = 0;
    virtual std::string Item(ItemCatalogKey key) const = 0;
    virtual std::string ItemDescription(ItemCatalogKey key) const = 0;
    virtual std::string Placement(PlacementKey key) const = 0;
    virtual std::string Skill(SkillKey key) const = 0;
    // Symbolic COOP_* labels: live binding localizes whole messages here.
    virtual std::string Label(const std::string &key) const = 0;
    virtual std::string Reason(E_CampaignReason reason) const = 0;
    // One monotonic session/client allocator shared by every screen/request family.
    // Reopening a screen MUST NOT restart request IDs. Invalid ID disables submission.
    virtual RequestId NextRequestId() = 0;
};
enum class E_CampaignPreparationKey { Tab, BackTab, Enter, Space, Left, Right, Up, Down, Escape };
enum class E_CampaignPreparationAction {
    Slot, Page, Findings, Inventory, Stock, InventoryPage, StockPage, BriefingPage, Destination, QuantityDown, QuantityUp,
    Equip, Return, Skills, SkillsPage, Claim, Release, Ready, Deploy, Leave, ConfirmLeave, CancelLeave
};
struct S_CampaignPreparationControl {
    E_CampaignPreparationAction m_Action;
    std::uint64_t m_Key = 0;
    S_CampaignPreparationRect m_Rect;
    std::string m_Text;
    bool m_Enabled = true;
    E_CampaignReason m_Reason = E_CampaignReason::None;
};

class C_CampaignPreparationScreen {
public:
    C_CampaignPreparationScreen(I_CampaignPreparation &model,
        I_CampaignPreparationPresentation &presentation, bool allowSpectators = false);
    ~C_CampaignPreparationScreen();
    C_CampaignPreparationScreen(const C_CampaignPreparationScreen &) = delete;
    C_CampaignPreparationScreen &operator=(const C_CampaignPreparationScreen &) = delete;
    void Tick();
    void Render(I_CampaignPreparationPainter &painter);
    bool Key(E_CampaignPreparationKey key);
    bool Click(float x, float y); // logical coordinates after inverse letterboxing
    void Close(); // stops polling; caller releases its adapter attachment after close
    bool IsOpen() const { return m_Open; }
    bool LeaveRequested() const { return m_LeaveRequested; }
    bool IsPending() const { return m_Pending.has_value(); }
    std::optional<SoldierId> GetSelection() const { return m_Selected; }
    const std::vector<S_CampaignPreparationControl> &GetControls() const { return m_Controls; }
    std::size_t GetDocumentPage() const { return m_BriefingPage; }
    std::size_t GetDocumentPageCount() const { return m_TextPages; }
    std::size_t GetSkillPage() const { return m_SkillPage; }
    std::size_t GetFocus() const { return m_Focus; }
    E_CampaignReason GetNotice() const { return m_Notice; }
    T_CampaignPreparationSnapshot GetSnapshot() const { return m_Snapshot; }
private:
    struct S_Pending {
        LocalSubmissionId m_Token;
        S_CampaignRequestContext m_Context;
        PreparationGeneration m_Generation;
        E_CampaignRequestFamily m_Family;
    };
    bool IsCurrent() const;
    bool CanEdit() const;
    E_CampaignReason EditReason() const;
    const S_CampaignSoldierPresentation *Soldier() const;
    const S_CampaignSoldierLoadout *Loadout() const;
    const S_CampaignStockEntry *Stock() const;
    const S_CampaignAllowedDestination *Destination() const;
    bool HasLocalSoldier() const;
    std::string ItemState(const S_CampaignItemState &state) const;
    std::string Owner(const S_CampaignSoldierPresentation &soldier) const;
    void Refresh();
    void BuildControls();
    void Activate(std::size_t index);
    S_CampaignPreparationRequestContext Context(RequestId id) const;
    void Queued(const T_CampaignSubmission &submission, RequestId request, E_CampaignRequestFamily family);
    std::string Label(const char *key) const;
    I_CampaignPreparation &m_Model;
    I_CampaignPreparationPresentation &m_Presentation;
    bool m_AllowSpectators;
    T_CampaignPreparationSnapshot m_Snapshot;
    SessionId m_LastSession;
    PreparationGeneration m_LastGeneration;
    ViewRevision m_LastView;
    E_CampaignSyncStatus m_Status = E_CampaignSyncStatus::Loading;
    std::optional<SoldierId> m_Selected;
    std::optional<StockEntryId> m_Stock;
    std::optional<PreparationItemId> m_Inventory;
    std::optional<PlacementKey> m_Destination;
    std::optional<S_Pending> m_Pending;
    std::vector<S_CampaignPreparationControl> m_Controls;
    std::size_t m_Focus = 0;
    std::size_t m_StockPage = 0, m_InventoryPage = 0, m_BriefingPage = 0, m_TextPages = 1, m_SkillPage = 0;
    std::uint32_t m_Quantity = 1;
    E_CampaignReason m_Notice = E_CampaignReason::None;
    bool m_Open = true, m_LeaveRequested = false, m_ConfirmLeave = false;
    bool m_Briefing = false, m_Skills = false, m_Findings = false;
};
#endif
