#include "game/campaign_preparation_native_ui.h"

#ifndef HD2_DEDICATED

#include <windows.h>

#include <algorithm>
#include <charconv>
#include <string_view>
#include <tuple>

#include "common/config_file.h"
#include "common/os_files.h"
#include "game/campaign.h"
#include "game/campaign_preparation_live.h"
#include "game/campaign_preparation_screen.h"
#include "game/cursor.h"
#include "game/font.h"
#include "game/game_texts.h"
#include "game/item_defs.h"
#include "game/menu_soldier.h"
#include "game/profile.h"
#include "ls3d/igraph.h"

namespace {

using R = E_CampaignReason;
using Rect = S_CampaignPreparationRect;

constexpr int kOriginalCampaign = 0;
constexpr int kArcticMission = 1;
constexpr float kFontHeight = 16;

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

std::string OriginalBriefing()
{
    const auto *mission = g_campaign->GetMissionDefinition(kOriginalCampaign, kArcticMission);
    if (!mission)
        return {};
    std::string briefing;
    const auto append = [&](std::string_view line) {
        if (line.empty())
            return;
        if (!briefing.empty())
            briefing += '\n';
        briefing.append(line);
    };
    const int text = mission->GetText(1);
    if (text >= 0) {
        for (int id = text + 3; id <= text + 9; ++id) {
            const auto line = LocalText(id);
            if (line.empty())
                break;
            append(line);
        }
    }
    const auto language = g_gameProfile.GetLanguage();
    for (int i = 0; i < mission->GetChildCount(); ++i) {
        const auto *step = mission->GetChild(i);
        if (step->GetType() == CAMPAIGN_NODE_PLAY)
            break;
        if (step->GetType() != CAMPAIGN_NODE_VIDEO)
            continue;
        const auto *video = static_cast<const C_CampaignVideo *>(step);
        if (!video->IsBriefing())
            continue;
        std::string name = video->GetFileName();
        const auto extension = name.rfind('.');
        if (extension != std::string::npos)
            name.resize(extension);
        const C_fstr path("%s\\text\\%s\\subtitles\\%s.txt", GetGameDirectory(), language.c_str(), name.c_str());
        C_GameTexts script;
        if (!FileExists(path.c_str()) || !script.Load(path.c_str()))
            continue;
        for (const auto &entry : script.GetTexts()) {
            const std::string_view line = entry.text.c_str();
            const auto open = line.find('<');
            const auto separator = line.find(',', open);
            const auto close = line.find('>', separator);
            if (!line.empty() && line.front() != 'S' && open != std::string_view::npos &&
                separator != std::string_view::npos && close != std::string_view::npos)
                append(line.substr(close + 1));
        }
    }
    return briefing;
}

constexpr std::pair<std::string_view, const char *> kLabels[] = {
    {"COOP_PREP_TITLE", "Campaign preparation"},
    {"COOP_PREP_SOLDIER", "SELECTED SOLDIER"},
    {"COOP_PREP_RECEIVING", "Receiving preparation..."},
    {"COOP_PREP_DISCONNECTED", "Connection ended"},
    {"COOP_PREP_FROZEN", "Preparation complete / loading mission"},
    {"COOP_PREP_PENDING", "Waiting for host"},
    {"COOP_PREP_COMMITTED", "Equipment confirmed"},
    {"COOP_OWNER_AI", "AI"}, {"COOP_OWNER_SCRIPTED", "Script controlled"}, {"COOP_OWNER_HUMAN", "Human"},
    {"COOP_OWNER_EDITABLE", "Can equip"}, {"COOP_OWNER_READ_ONLY", "Read only"},
    {"COOP_OWNER_EDIT_OWN", "Editing your soldier"}, {"COOP_OWNER_EDIT_AI", "Editing AI soldier"},
    {"COOP_SLOT_UNAVAILABLE", "Unavailable"}, {"COOP_SLOT_CLAIM", "Claim"}, {"COOP_SLOT_RELEASE", "Release"},
    {"COOP_OWNER_NO_SOLDIER", "Claim a soldier to take part."},
    {"COOP_BRIEFING_TITLE", "MISSION BRIEFING"}, {"COOP_BRIEFING_PAGE", "Mission briefing"}, {"COOP_EQUIP_PAGE", "Equipment"},
    {"COOP_PAGE_PREVIOUS", "< Previous"}, {"COOP_PAGE_NEXT", "Next >"},
    {"COOP_SKILLS", "Soldier skills"}, {"COOP_PAGE_KEYS", "Arrow keys: page"},
    {"COOP_EQUIP_NO_DESTINATION", "No available placement"}, {"COOP_EQUIP_TAKE", "Equip"},
    {"COOP_EQUIP_RETURN", "Return to stock"}, {"COOP_EQUIP_WEIGHT", "Weight"}, {"COOP_EQUIP_BELT_PACK", "Belt / backpack"},
    {"COOP_EQUIP_STOCK", "SHARED STOCK"}, {"COOP_EQUIP_KIT", "CARRIED EQUIPMENT"},
    {"COOP_EQUIP_DESTINATION", "Move to"}, {"COOP_EQUIP_QUANTITY", "Quantity"}, {"COOP_EQUIP_AMMO", "Ammo"}, {"COOP_EQUIP_ROUNDS", "Rounds"},
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
    explicit C_Presentation(C_CampaignPreparationLive &model)
        : m_Model(model), m_Briefing(OriginalBriefing()) {}

    std::string Text(TextKey key) const override
    {
        if (key.Value() == "original-arctic1/operation")
            return LocalText(g_campaign->GetMissionOperationId(kOriginalCampaign, kArcticMission));
        if (key.Value() == "original-arctic1/mission")
            return LocalText(g_campaign->GetMissionId(kOriginalCampaign, kArcticMission));
        if (key.Value() == "original-arctic1/briefing")
            return m_Briefing;
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
    std::string m_Briefing;
};

class C_Painter final : public I_CampaignPreparationPainter {
public:
    C_Painter() : m_Fonts(AcquireFontManager()), m_Font(m_Fonts->FindFont(FONT_SMALL)) {}
    ~C_Painter() override
    {
        ReleasePreview();
        for (const auto &entry : m_Images)
            if (entry.second)
                entry.second->Release();
        m_Fonts->Release();
    }

    bool IsUsable() const { return m_Font != nullptr; }
    void Tick(int timeDelta)
    {
        if (m_Preview)
            m_Preview->Tick(timeDelta);
    }

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

    void Panel(const Rect &rect, bool recessed) override
    {
        if (rect.m_Width == 1024) {
            Quad(rect, 0xff30392f);
            Quad({0, 0, 1024, 88}, 0xff222b24);
            Quad({32, 87, 960, 1}, 0xff7d896e);
            return;
        }
        Quad(rect, recessed ? 0xffaaa88e : 0xffe3dcc6);
    }

    void Control(const Rect &rect, E_CampaignPreparationControlStyle style, bool selected, bool enabled) override
    {
        using Style = E_CampaignPreparationControlStyle;
        std::uint32_t fill = 0xffeee6cf;
        if (!enabled)
            fill = 0xffc8c4ad;
        else if (selected || style == Style::Primary)
            fill = 0xffd7c28a;
        else if (style == Style::Card)
            fill = 0xffded8bd;
        else if (style == Style::Button)
            fill = 0xffc5c9a8;
        Quad(rect, 0xff6e775e);
        Quad({rect.m_X + 1, rect.m_Y + 1, rect.m_Width - 2, rect.m_Height - 2}, fill);
        if (selected)
            Quad({rect.m_X + 1, rect.m_Y + 1, 4, rect.m_Height - 2}, 0xff746033);
    }

    void Heading(const Rect &rect, const std::string &text, bool primary) override
    {
        const float size = primary ? 26.0f : kFontHeight;
        DrawText(rect, text, size, primary ? 0xfffff1cd : 0xffccd1b5);
    }

    void Text(const Rect &rect, const std::string &text, bool muted) override
    {
        DrawText(rect, text, kFontHeight, muted ? 0xff565d4c : 0xff263026);
    }

    void Preview(const Rect &rect, AssetKey asset, const S_CampaignSoldierLoadout &loadout) override
    {
        const auto *profile = GetSoldierProfile(CatalogIndex(asset.Value(), "original-portrait/"));
        if (!profile) {
            Text(rect, "Soldier preview unavailable", true);
            return;
        }
        static constexpr std::pair<std::string_view, E_MenuSoldierPlace> places[] = {
            {"head", MENU_SOLDIER_HEAD}, {"uniform", MENU_SOLDIER_BODY}, {"hands", MENU_SOLDIER_HANDS},
            {"backpack", MENU_SOLDIER_BACKPACK}, {"long-gun", MENU_SOLDIER_LONG_GUN}, {"belt", MENU_SOLDIER_HOLSTERS}
        };
        std::vector<T_PreviewItem> items;
        for (const auto &item : loadout.m_Items) {
            const auto place = std::find_if(std::begin(places), std::end(places), [&](const auto &entry) {
                return entry.first == item.m_Placement.m_Slot.Value();
            });
            const int type = CatalogIndex(item.m_State.m_Item.Value(), "original-item/");
            if (place != std::end(places) && GetItemDef(type))
                items.emplace_back(place->second, type, item.m_State.m_Quantity);
        }
        const float left = m_Left + rect.m_X * m_Scale, top = m_Top + rect.m_Y * m_Scale;
        const std::array<float, 4> bounds{left, top, left + rect.m_Width * m_Scale, top + rect.m_Height * m_Scale};
        auto *graph = GetIGraph();
        graph->EndScene();
        if (m_PreviewAsset != asset || m_PreviewBounds != bounds || m_PreviewItems != items) {
            ReleasePreview();
            m_PreviewAsset = asset;
            m_PreviewBounds = bounds;
            m_PreviewItems = std::move(items);
            m_Preview = std::make_unique<C_MenuSoldier>();
            m_Preview->Setup(bounds[0], bounds[1], bounds[2], bounds[3]);
            if (m_Preview->GetModel()) {
                auto *camera = m_Preview->GetScene()->GetActiveCamera();
                camera->SetFOV(.58f);
                camera->Update();
                m_Preview->SetFace(profile->faceTexture.c_str());
                for (const auto &[place, type, count] : m_PreviewItems)
                    for (std::uint32_t i = 0; i < count; ++i)
                        m_Preview->AddItem(place, type);
            }
        }
        const bool ready = m_Preview && m_Preview->GetModel();
        if (ready)
            m_Preview->Render();
        graph->BeginScene();
        graph->SetViewport({0, 0, graph->ScrnX(), graph->ScrnY(), 0, 1});
        I3DGetDriver()->SetState(3, 0);
        I3DGetDriver()->SetState(9, 0);
        if (!ready)
            Text(rect, "Soldier preview unavailable", true);
    }

    void Focus(const Rect &rect) override
    {
        Quad({rect.m_X, rect.m_Y, rect.m_Width, 2}, 0xfffff1cd);
        Quad({rect.m_X, rect.m_Y + rect.m_Height - 2, rect.m_Width, 2}, 0xff343c2d);
    }

    void Asset(const Rect &rect, AssetKey asset) override
    {
        const auto &image = g_campaign->GetMissionImage(kOriginalCampaign, kArcticMission);
        if (asset.Value() == image.c_str())
            if (auto *texture = Image(image.c_str()))
                Quad(rect, 0xffffffff, texture);
    }

private:
    using T_PreviewItem = std::tuple<E_MenuSoldierPlace, int, std::uint32_t>;

    void ReleasePreview()
    {
        if (m_Preview) {
            m_Preview->Release();
            m_Preview->ReleaseDivingGear();
            m_Preview.reset();
        }
    }

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

    float Width(int glyph, float height)
    {
        return (std::max)(0.4f, m_Font->GetGlyph(glyph)->aspect) * height;
    }

    void DrawText(const Rect &rect, const std::string &text, float size, std::uint32_t color)
    {
        auto lines = Wrap(rect, text, size);
        const auto count = LineCount(rect, size);
        if (lines.size() > count) {
            lines.resize(count);
            auto &last = lines.back();
            const float limit = rect.m_Width - 10 - 3 * Width('.', size);
            float width = 0;
            std::size_t end = 0;
            while (end < last.size()) {
                const auto glyph = Glyph(last, end);
                width += Width(glyph.first, size);
                if (width > limit)
                    break;
                end += glyph.second;
            }
            last.resize(end);
            last += "...";
        }
        const float height = size * m_Scale;
        m_Font->ClearVertices();
        m_Font->Select();
        GetIGraph()->SetRenderState(0, 1);
        for (std::size_t row = 0; row < lines.size(); ++row) {
            float x = m_Left + (rect.m_X + 5) * m_Scale;
            const float y = m_Top + (rect.m_Y + 3 + size + row * (size + 2)) * m_Scale;
            for (std::size_t i = 0; i < lines[row].size();) {
                const auto glyph = Glyph(lines[row], i);
                const float width = Width(glyph.first, size) * m_Scale;
                m_Font->AddQuad(glyph.first, color, static_cast<int>(width), static_cast<int>(height),
                                static_cast<int>(x), static_cast<int>(y), .02f);
                i += glyph.second;
                x += width;
            }
        }
        m_Font->Flush();
    }

    static std::size_t LineCount(const Rect &rect, float height = kFontHeight)
    {
        return static_cast<std::size_t>((std::max)(1.0f, (rect.m_Height - 1) / (height + 2)));
    }

    std::vector<std::string> Wrap(const Rect &rect, const std::string &text, float height = kFontHeight)
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
                const float advance = Width(glyph.first, height);
                if (width > 0 && glyph.first != ' ' && i > 0 && text[i - 1] == ' ') {
                    float word = 0;
                    for (auto j = i; j < text.size() && text[j] != ' ' && text[j] != '\n';) {
                        const auto part = Glyph(text, j);
                        word += Width(part.first, height);
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
    std::unique_ptr<C_MenuSoldier> m_Preview;
    AssetKey m_PreviewAsset;
    std::array<float, 4> m_PreviewBounds{};
    std::vector<T_PreviewItem> m_PreviewItems;
    float m_Scale = 1, m_Left = 0, m_Top = 0;
};

} // namespace

struct C_CampaignPreparationNativeUI::S_View {
    explicit S_View(C_CampaignPreparationLive &model) : m_Presentation(model), m_Screen(model, m_Presentation) {}
    C_Presentation m_Presentation;
    C_Painter m_Painter;
    C_CampaignPreparationScreen m_Screen;
    bool m_MouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    std::optional<std::pair<E_CampaignPreparationAction, std::uint64_t>> m_PressedControl;
};

C_CampaignPreparationNativeUI::C_CampaignPreparationNativeUI(C_CampaignPreparationLive &model)
    : m_View(std::make_unique<S_View>(model)) {}
C_CampaignPreparationNativeUI::~C_CampaignPreparationNativeUI() = default;

bool C_CampaignPreparationNativeUI::IsUsable() const { return m_View->m_Painter.IsUsable(); }
void C_CampaignPreparationNativeUI::Update(int timeDelta)
{
    m_View->m_Screen.Tick();
    m_View->m_Painter.Tick(timeDelta);
}
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
    cursor.Update(timeDelta);
    const auto root = GetAncestor(GetIGraph()->GetHWND(), GA_ROOT);
    const bool focused = GetAncestor(GetForegroundWindow(), GA_ROOT) == root;
    const int primary = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
    const bool down = cursor.IsButtonDown(1) || (focused && (GetAsyncKeyState(primary) & 0x8000) != 0);
    if (down == m_View->m_MouseDown)
        return;
    m_View->m_MouseDown = down;
    float x = 0, y = 0;
    cursor.GetScreenPosition(&x, &y, nullptr);
    m_View->m_Painter.Resize();
    const auto point = m_View->m_Painter.Logical(x, y);
    const auto &controls = m_View->m_Screen.GetControls();
    const auto control = std::find_if(controls.begin(), controls.end(), [&](const auto &candidate) {
        return candidate.m_Rect.Contains(point.first, point.second);
    });
    if (down) {
        if (control != controls.end())
            m_View->m_PressedControl = std::make_pair(control->m_Action, control->m_Key);
    } else {
        if (control != controls.end() && m_View->m_PressedControl == std::make_pair(control->m_Action, control->m_Key))
            m_View->m_Screen.Click(point.first, point.second);
        m_View->m_PressedControl.reset();
    }
}

void C_CampaignPreparationNativeUI::Render()
{
    auto *graph = GetIGraph();
    auto *driver = I3DGetDriver();
    const auto oldAlpha = graph->GetRenderState(0);
    const auto oldDepth = driver->GetState(3);
    const auto oldFog = driver->GetState(9);
    graph->Clear(0xff292f28, 1.0f, 3);
    graph->BeginScene();
    driver->SetState(3, 0);
    driver->SetState(9, 0);
    graph->SetViewport({0, 0, graph->ScrnX(), graph->ScrnY(), 0, 1});
    m_View->m_Painter.Resize();
    m_View->m_Screen.Render(m_View->m_Painter);
    graph->EndScene();
    driver->SetState(3, oldDepth);
    driver->SetState(9, oldFog);
    graph->SetRenderState(0, oldAlpha);
}

#endif
