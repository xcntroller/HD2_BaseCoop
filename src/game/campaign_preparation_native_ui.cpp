#include "game/campaign_preparation_native_ui.h"

#ifndef HD2_DEDICATED

#include <algorithm>
#include <charconv>
#include <string_view>

#include "game/campaign.h"
#include "game/campaign_preparation_live.h"
#include "game/campaign_preparation_screen.h"
#include "game/cursor.h"
#include "game/font.h"
#include "game/game_texts.h"
#include "game/item_defs.h"
#include "game/profile.h"
#include "ls3d/igraph.h"

namespace {

using R = E_CampaignReason;
using Rect = S_CampaignPreparationRect;

constexpr int kOriginalCampaign = 0;
constexpr int kFirstOperation = 0;
constexpr int kArcticMission = 1;
constexpr float kFontHeight = 16, kLinePitch = 18;

int CatalogIndex(std::string_view key, std::string_view prefix)
{
    if (key.substr(0, prefix.size()) != prefix)
        return -1;
    key.remove_prefix(prefix.size());
    int index = -1;
    const auto parsed = std::from_chars(key.data(), key.data() + key.size(), index);
    return parsed.ec == std::errc() && parsed.ptr == key.data() + key.size() && index >= 0 ? index : -1;
}

std::string LocalText(int id)
{
    return id >= 0 && g_texts.Has(id) ? g_texts.Get(id).c_str() : "";
}

constexpr std::pair<std::string_view, const char *> kLabels[] = {
    {"COOP_PREP_TITLE", "CO-OP CAMPAIGN / PREPARATION"},
    {"COOP_PREP_RECEIVING", "Receiving preparation..."},
    {"COOP_PREP_DISCONNECTED", "Connection ended"},
    {"COOP_PREP_FROZEN", "Preparation complete / loading mission"},
    {"COOP_PREP_PENDING", "Waiting for host"},
    {"COOP_PREP_COMMITTED", "Equipment confirmed"},
    {"COOP_OWNER_AI", "AI"}, {"COOP_OWNER_SCRIPTED", "Script controlled"}, {"COOP_OWNER_HUMAN", "Human"},
    {"COOP_OWNER_EDITABLE", "Equipment available"}, {"COOP_OWNER_READ_ONLY", "Read only"},
    {"COOP_OWNER_EDIT_OWN", "Editing your soldier"}, {"COOP_OWNER_EDIT_AI", "Editing AI soldier"},
    {"COOP_SLOT_UNAVAILABLE", "Unavailable"}, {"COOP_SLOT_CLAIM", "Claim"}, {"COOP_SLOT_RELEASE", "Release"},
    {"COOP_OWNER_NO_SOLDIER", "Claim a soldier to take part."},
    {"COOP_BRIEFING_TITLE", "MISSION BRIEFING"}, {"COOP_BRIEFING_PAGE", "Open briefing"}, {"COOP_EQUIP_PAGE", "Equipment"},
    {"COOP_PAGE_PREVIOUS", "Previous page"}, {"COOP_PAGE_NEXT", "Next page"},
    {"COOP_SKILLS", "Skills / equipment"}, {"COOP_PAGE_KEYS", "Arrow keys: page"},
    {"COOP_EQUIP_NO_DESTINATION", "No available placement"}, {"COOP_EQUIP_TAKE", "Equip"},
    {"COOP_EQUIP_RETURN", "Return"}, {"COOP_EQUIP_WEIGHT", "Weight"}, {"COOP_EQUIP_BELT_PACK", "Belt / backpack"},
    {"COOP_EQUIP_STOCK", "SHARED EQUIPMENT"}, {"COOP_EQUIP_AMMO", "Ammo"}, {"COOP_EQUIP_ROUNDS", "Rounds"},
    {"COOP_EQUIP_CHARGES", "Charges"}, {"COOP_EQUIP_CONDITION", "Condition"},
    {"COOP_ESSENTIAL_ELIGIBLE", "Eligible carriers:"}, {"COOP_ESSENTIAL_DETAILS", "Essentials / restrictions"},
    {"COOP_ESSENTIAL_TITLE", "MISSION ESSENTIALS:"}, {"COOP_READY_SET", "Ready"}, {"COOP_READY_UNSET", "Not ready"},
    {"COOP_READY_YES", "Ready"}, {"COOP_READY_NO", "Preparing"}, {"COOP_READY_DEPLOY", "Deploy"},
    {"COOP_READY_LEAVE", "Leave"}, {"COOP_READY_CONFIRM_LEAVE", "Confirm leave"}, {"COOP_READY_CANCEL", "Cancel"},
    {"COOP_READY_HOST_LEAVE", "End the campaign session for everyone?"},
    {"COOP_READY_GUEST_LEAVE", "Leave the campaign session?"}
};

class C_Presentation final : public I_CampaignPreparationPresentation {
public:
    explicit C_Presentation(C_CampaignPreparationLive &model) : m_Model(model) {}

    std::string Text(TextKey key) const override
    {
        if (key.Value() == "original-arctic1/operation")
            return LocalText(g_campaign->GetOperationId(kOriginalCampaign, kFirstOperation));
        const int title = g_campaign->GetMissionText(kOriginalCampaign, kArcticMission, 1);
        if (key.Value() == "original-arctic1/mission")
            return title >= 0 ? LocalText(title + 1) : "Arctic1";
        if (key.Value() == "original-arctic1/briefing" && title >= 0) {
            std::string briefing;
            for (int id = title + 3; id <= title + 9; ++id) {
                const auto line = LocalText(id);
                if (line.empty())
                    break;
                if (!briefing.empty())
                    briefing += "\n";
                briefing += line;
            }
            return briefing;
        }
        if (key.Value() == "campaign.equipment.unarmed_confirmation")
            return "A soldier has no weapon.";
        return {};
    }

    std::string Item(ItemCatalogKey key) const override
    {
        const auto *definition = GetItemDef(CatalogIndex(key.Value(), "original-item/"));
        return definition ? LocalText(definition->GetTextId()) : "Unknown item";
    }

    std::string ItemDescription(ItemCatalogKey key) const override
    {
        const auto *definition = GetItemDef(CatalogIndex(key.Value(), "original-item/"));
        return definition ? LocalText(definition->GetTextId() + 1) : "";
    }

    std::string Placement(PlacementKey key) const override
    {
        static constexpr std::pair<std::string_view, const char *> names[] = {
            {"head", "Head"}, {"uniform", "Uniform"}, {"hands", "Hands"}, {"backpack", "Backpack"},
            {"long-gun", "Long gun"}, {"belt", "Belt"}, {"backpack-contents", "In backpack"}
        };
        for (const auto &name : names)
            if (name.first == key.Value())
                return name.second;
        return "Unknown placement";
    }

    std::string Skill(SkillKey key) const override
    {
        constexpr const char *names[] = {
            "Health", "Strength", "Endurance", "Morale", "Shooting",
            "Skill", "Stealth", "First aid", "Lockpicking"
        };
        const int index = CatalogIndex(key.Value(), "original-stat/");
        return index >= 0 && index < static_cast<int>(std::size(names)) ? names[index] : "Skill";
    }

    std::string Label(const std::string &key) const override
    {
        for (const auto &label : kLabels)
            if (label.first == key)
                return label.second;
        return {};
    }

    std::string Reason(R reason) const override
    {
        switch (reason) {
        case R::None: return {};
        case R::StaleRevision: return "Equipment changed. Select the item again.";
        case R::NotOwner: return "This soldier belongs to another player.";
        case R::AlreadyOwned: return "This soldier is already claimed.";
        case R::AlreadyControlsSoldier: return "Release your current soldier first.";
        case R::StockUnavailable: return "Item no longer available.";
        case R::InvalidPlacement: return "Item does not fit here.";
        case R::CapacityExceeded: return "The selected kit has no room.";
        case R::RequiredItemsMissing: return "The squad needs the mission essentials.";
        case R::NotDeployed: return "Every participant must claim a soldier.";
        case R::PendingTransaction: return "Waiting for every participant to be ready.";
        case R::Frozen: return "The mission is loading.";
        case R::Timeout: return "The host did not answer.";
        case R::Disconnected: return "The connection ended.";
        case R::SnapshotUnavailable:
        case R::SnapshotGap: return "Receiving current equipment...";
        default: return "Action unavailable.";
        }
    }

    RequestId NextRequestId() override { return m_Model.NextRequestId(); }

private:
    C_CampaignPreparationLive &m_Model;
};

class C_Painter final : public I_CampaignPreparationPainter {
public:
    C_Painter() : m_Fonts(AcquireFontManager()), m_Font(m_Fonts->FindFont(FONT_SMALL)) {}
    ~C_Painter() override
    {
        for (const auto &entry : m_Images)
            if (entry.second)
                entry.second->Release();
        m_Fonts->Release();
    }

    bool IsUsable() const { return m_Font != nullptr; }

    void Resize()
    {
        auto *graph = GetIGraph();
        m_Scale = (std::min)(graph->ScrnX() / 1024.0f, graph->ScrnY() / 768.0f);
        m_Left = (graph->ScrnX() - 1024 * m_Scale) / 2;
        m_Top = (graph->ScrnY() - 768 * m_Scale) / 2;
    }

    std::pair<float, float> Logical(float x, float y) const
    {
        return {(x - m_Left) / m_Scale, (y - m_Top) / m_Scale};
    }

    std::vector<std::string> Paginate(const Rect &rect, const std::string &text) override
    {
        const auto lines = Wrap(rect, text);
        const auto pageSize = LineCount(rect);
        std::vector<std::string> pages;
        for (std::size_t first = 0; first < lines.size(); first += pageSize) {
            std::string page;
            for (std::size_t i = first; i < lines.size() && i < first + pageSize; ++i) {
                if (i != first)
                    page += '\n';
                page += lines[i];
            }
            pages.push_back(std::move(page));
        }
        return pages;
    }

    void Panel(const Rect &rect, bool selected) override
    {
        Quad(rect, selected ? 0xffb7a76c : rect.m_Width == 1024 ? 0xff41483a : 0xffd5cdb5);
    }

    void Text(const Rect &rect, const std::string &text, bool muted) override
    {
        const auto lines = Wrap(rect, text);
        const float height = kFontHeight * m_Scale;
        m_Font->ClearVertices();
        m_Font->Select();
        GetIGraph()->SetRenderState(0, 1);
        for (std::size_t row = 0; row < lines.size() && row < LineCount(rect); ++row) {
            float x = m_Left + (rect.m_X + 5) * m_Scale;
            const float y = m_Top + (rect.m_Y + 3 + kFontHeight + row * kLinePitch) * m_Scale;
            for (std::size_t i = 0; i < lines[row].size();) {
                const auto glyph = Glyph(lines[row], i);
                const float width = Width(glyph.first) * m_Scale;
                m_Font->AddQuad(glyph.first, rect.m_Y < 110 ? 0xffe8dfc0 : muted ? 0xff70715f : 0xff20291e,
                    static_cast<int>(width), static_cast<int>(height), static_cast<int>(x), static_cast<int>(y), .02f);
                i += glyph.second;
                x += width;
            }
        }
        m_Font->Flush();
    }

    void Preview(const Rect &rect, AssetKey asset, const S_CampaignSoldierLoadout &loadout) override
    {
        if (const auto *profile = GetSoldierProfile(CatalogIndex(asset.Value(), "original-portrait/"))) {
            if (auto *portrait = Image(profile->faceTexture.c_str()))
                Quad({rect.m_X, rect.m_Y, rect.m_Height, rect.m_Height}, 0xffffffff, portrait);
        }
        for (const auto &item : loadout.m_Items) {
            if (item.m_Placement.m_Slot.Value() != "hands")
                continue;
            auto *definition = GetItemDef(CatalogIndex(item.m_State.m_Item.Value(), "original-item/"));
            if (auto *image = definition ? definition->GetImage() : nullptr)
                Quad({rect.m_X + rect.m_Height + 4, rect.m_Y + 4, rect.m_Width - rect.m_Height - 4, 60},
                    0xffffffff, image);
            break;
        }
    }

    void Focus(const Rect &rect) override
    {
        Quad({rect.m_X, rect.m_Y, rect.m_Width, 2}, 0xfff5eac1);
        Quad({rect.m_X, rect.m_Y + rect.m_Height - 2, rect.m_Width, 2}, 0xff20291e);
    }

    void Asset(const Rect &rect, AssetKey asset) override
    {
        const auto &image = g_campaign->GetMissionImage(kOriginalCampaign, kArcticMission);
        if (asset.Value() == image.c_str())
            if (auto *texture = Image(image.c_str()))
                Quad(rect, 0xffffffff, texture);
    }

private:
    IImage *Image(const char *name)
    {
        if (!name || !*name)
            return nullptr;
        auto result = m_Images.try_emplace(name, nullptr);
        if (result.second)
            GetIGraph()->LoadTexture(name, nullptr, 0x400, &result.first->second);
        return result.first->second;
    }

    void Quad(const Rect &rect, std::uint32_t color, IImage *image = nullptr)
    {
        const float x = m_Left + rect.m_X * m_Scale, y = m_Top + rect.m_Y * m_Scale;
        const float right = x + rect.m_Width * m_Scale, bottom = y + rect.m_Height * m_Scale;
        const S_FontVertex vertices[] = {{x,y,.02f,50,color,0,0,0}, {right,y,.02f,50,color,0,1,0},
            {right,bottom,.02f,50,color,0,1,1}, {x,bottom,.02f,50,color,0,0,1}};
        GetIGraph()->SetRenderState(0, image ? 1 : 0);
        GetIGraph()->SetTexture(image, 0);
        GetIGraph()->DrawPrimitives(4, 2, vertices, 1);
    }

    static std::pair<int, std::size_t> Glyph(const std::string &text, std::size_t start)
    {
        const auto first = static_cast<unsigned char>(text[start]);
        if (first < 0xc2 || first > 0xf4)
            return {first, 1};
        const std::size_t size = first < 0xe0 ? 2 : first < 0xf0 ? 3 : 4;
        if (size > text.size() - start)
            return {first, 1};
        int code = first & ((1 << (7 - size)) - 1);
        for (std::size_t i = 1; i < size; ++i) {
            const auto byte = static_cast<unsigned char>(text[start + i]);
            if ((byte & 0xc0) != 0x80)
                return {first, 1};
            code = (code << 6) | (byte & 0x3f);
        }
        return {code <= 0xffff ? code : '?', size};
    }

    float Width(int glyph) { return (std::max)(0.4f, m_Font->GetGlyph(glyph)->aspect) * kFontHeight; }

    static std::size_t LineCount(const Rect &rect)
    {
        return static_cast<std::size_t>((std::max)(1.0f, (rect.m_Height - 1) / kLinePitch));
    }

    std::vector<std::string> Wrap(const Rect &rect, const std::string &text)
    {
        std::vector<std::string> lines;
        std::string line;
        float width = 0;
        const auto lineBreak = [&] {
            lines.push_back(std::move(line));
            line.clear();
            width = 0;
        };
        const float limit = rect.m_Width - 10;
        for (std::size_t i = 0; i < text.size();) {
            const auto glyph = Glyph(text, i);
            if (glyph.first == '\n')
                lineBreak();
            else if (glyph.first != '\r') {
                const float advance = Width(glyph.first);
                if (width > 0 && glyph.first != ' ' && i > 0 && text[i - 1] == ' ') {
                    float word = 0;
                    for (auto j = i; j < text.size() && text[j] != ' ' && text[j] != '\n';) {
                        const auto part = Glyph(text, j);
                        word += Width(part.first);
                        j += part.second;
                    }
                    if (width + word > limit)
                        lineBreak();
                }
                if (width > 0 && width + advance > limit)
                    lineBreak();
                if (glyph.first != ' ' || width > 0) {
                    line.append(text, i, glyph.second);
                    width += advance;
                }
            }
            i += glyph.second;
        }
        lines.push_back(std::move(line));
        return lines;
    }

    C_FontManager *m_Fonts;
    C_Font *m_Font;
    std::map<std::string, IImage *> m_Images;
    float m_Scale = 1, m_Left = 0, m_Top = 0;
};

} // namespace

struct C_CampaignPreparationNativeUI::S_View {
    explicit S_View(C_CampaignPreparationLive &model) : m_Presentation(model), m_Screen(model, m_Presentation) {}
    C_Presentation m_Presentation;
    C_Painter m_Painter;
    C_CampaignPreparationScreen m_Screen;
};

C_CampaignPreparationNativeUI::C_CampaignPreparationNativeUI(C_CampaignPreparationLive &model)
    : m_View(std::make_unique<S_View>(model)) {}
C_CampaignPreparationNativeUI::~C_CampaignPreparationNativeUI() = default;

bool C_CampaignPreparationNativeUI::IsUsable() const { return m_View->m_Painter.IsUsable(); }
void C_CampaignPreparationNativeUI::Update() { m_View->m_Screen.Tick(); }
bool C_CampaignPreparationNativeUI::LeaveRequested() const { return m_View->m_Screen.LeaveRequested(); }

void C_CampaignPreparationNativeUI::KeyInput(float *pressed)
{
    using K = E_CampaignPreparationKey;
    auto *graph = GetIGraph();
    switch (graph->GetAsciiKey()) {
    case 0x0f: m_View->m_Screen.Key(graph->IsShiftDown() ? K::BackTab : K::Tab); break;
    case 0x1c:
    case 0x9c: m_View->m_Screen.Key(K::Enter); break;
    case 0x39: m_View->m_Screen.Key(K::Space); break;
    case 0xcb: m_View->m_Screen.Key(K::Left); break;
    case 0xcd: m_View->m_Screen.Key(K::Right); break;
    case 0xc8: m_View->m_Screen.Key(K::Up); break;
    case 0xd0: m_View->m_Screen.Key(K::Down); break;
    case 0x01: m_View->m_Screen.Key(K::Escape); break;
    case 0: if (pressed && pressed[46] != 0.0f) m_View->m_Screen.Key(K::Escape); break;
    default: break;
    }
}

void C_CampaignPreparationNativeUI::MouseInput(C_Cursor &cursor, int timeDelta)
{
    if (!cursor.Update(timeDelta) || !cursor.IsClicked())
        return;
    float x = 0, y = 0;
    cursor.GetScreenPosition(&x, &y, nullptr);
    m_View->m_Painter.Resize();
    const auto point = m_View->m_Painter.Logical(x, y);
    m_View->m_Screen.Click(point.first, point.second);
}

void C_CampaignPreparationNativeUI::Render()
{
    auto *graph = GetIGraph();
    if (!graph->BeginScene())
        return;
    graph->SetViewport({0, 0, graph->ScrnX(), graph->ScrnY(), 0, 1});
    m_View->m_Painter.Resize();
    m_View->m_Screen.Render(m_View->m_Painter);
    graph->EndScene();
}

#endif
