// Native LS3D component preview. No game loop, profile, stock or shipping menu entry.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "fake.h"
#include "game/font.h"
#include "ls3d/igraph.h"
#include "ls3d/rw_data.h"
#include <cstdio>
#include <cmath>
#include <memory>

namespace {
void Error(std::uint32_t code, int severity, const char *text, std::uint32_t) {
    std::fprintf(stderr, "LS3D %u/%d: %s\n", code, severity, text);
}
class C_NativePainter final : public I_CampaignPreparationPainter {
public:
    C_TextureFont m_Font;
    float m_Scale = 1, m_Left = 0, m_Top = 0;
    bool Audit() {
        bool ok = true;
        for (const auto *name : {"items selection", "selection screen", "description"}) {
            auto *model = static_cast<I3D_model *>(I3DGetDriver()->CreateFrame(FRAME_MODEL));
            if (!model) return false;
            const auto result = model->Open((std::string("Models\\")+name+".4ds").c_str(),0,nullptr,nullptr);
            std::printf("campaign-resource %s HRESULT=%ld", name, static_cast<long>(result));
            if (SUCCEEDED(result)) for (const auto *anchor : {"equippanel", "photo03", "description"})
                std::printf(" %s=%d", anchor, model->FindChildFrame(anchor,FRAME_MASK_ANY)!=nullptr);
            std::printf("\n"); std::fflush(stdout); ok = ok && SUCCEEDED(result); model->Release();
        }
        return ok;
    }
    bool Init() {
        // Direct presentation reuse; never call global-mutating equipment builders.
        for (const char *path : {"Text\\Fonts\\Ansi\\Set3\\SMALL", "Text\\English\\Fonts\\Set3\\SMALL", "Text\\Fonts\\Set3\\SMALL"})
            if (m_Font.Load((std::string(path)+".tga").c_str(), (std::string(path)+".tbl").c_str())) return true;
        return false;
    }
    void Resize() {
        m_Scale = std::min(GetIGraph()->ScrnX()/1024.0f, GetIGraph()->ScrnY()/768.0f);
        m_Left = (GetIGraph()->ScrnX() - 1024*m_Scale)/2; m_Top = (GetIGraph()->ScrnY()-768*m_Scale)/2;
    }
    void Quad(S_CampaignPreparationRect r, std::uint32_t color) {
        const float x = m_Left+r.m_X*m_Scale, y = m_Top+r.m_Y*m_Scale;
        const float right = x+r.m_Width*m_Scale, bottom = y+r.m_Height*m_Scale;
        const S_FontVertex v[] = {{x,bottom,.02f,50,color,0,0,0},{x,y,.02f,50,color,0,0,0},{right,y,.02f,50,color,0,0,0},
            {right,bottom,.02f,50,color,0,0,0},{x,bottom,.02f,50,color,0,0,0},{right,y,.02f,50,color,0,0,0}};
        GetIGraph()->SetRenderState(0,0); GetIGraph()->SetTexture(nullptr,0); GetIGraph()->DrawPrimitives(3,2,v,1);
    }
    void Panel(const S_CampaignPreparationRect &r, bool selected) override {
        Quad(r, selected ? 0xffb7a76c : r.m_Width == 1024 ? 0xff41483a : 0xffd5cdb5);
        if (r.m_Width != 1024) { auto line=r; line.m_Height=1; Quad(line,0xff6c7057); }
    }
    void Text(const S_CampaignPreparationRect &r, const std::string &text, bool muted) override {
        // Bounded word wrap and clipping using the reused campaign texture font.
        // This fixture's copy is ASCII; localized UTF-8 uses the game's text-box adapter later.
        const float height = 16*m_Scale;
        const float left = m_Left+(r.m_X+5)*m_Scale, top=m_Top+(r.m_Y+3)*m_Scale;
        const float right=m_Left+(r.m_X+r.m_Width-5)*m_Scale, bottom=m_Top+(r.m_Y+r.m_Height)*m_Scale;
        float x=left, y=top+height;
        m_Font.ClearVertices();
        for (std::size_t i=0; i<text.size(); ++i) {
            const auto ch=static_cast<unsigned char>(text[i]);
            if (ch != ' ' && ch != '\n' && (i == 0 || text[i-1] == ' ')) {
                float word = 0;
                for (std::size_t j=i; j<text.size() && text[j]!=' ' && text[j]!='\n'; ++j) word += std::max(.4f,m_Font.GetGlyph(static_cast<unsigned char>(text[j]))->aspect)*height;
                if(x>left && x+word>right) {x=left; y+=height+2*m_Scale;}
            }
            float width=std::max(0.4f,m_Font.GetGlyph(ch)->aspect)*height;
            if (ch=='\n' || x+width>right) { x=left; y+=height+2*m_Scale; if(ch=='\n') continue; }
            if (y>bottom || right<=left) break;
            m_Font.AddQuad(ch, r.m_Y<110?0xffe8dfc0:muted?0xff70715f:0xff20291e, static_cast<int>(width),static_cast<int>(height),static_cast<int>(x),static_cast<int>(y),.02f);
            x+=width;
        }
        GetIGraph()->SetRenderState(0,1);
        m_Font.Flush();
    }
    void Focus(const S_CampaignPreparationRect &r) override {
        Quad({r.m_X,r.m_Y,r.m_Width,2},0xfff5eac1);
        Quad({r.m_X,r.m_Y+r.m_Height-2,r.m_Width,2},0xff20291e);
        Quad({r.m_X,r.m_Y,2,r.m_Height},0xff20291e);
        Quad({r.m_X+r.m_Width-2,r.m_Y,2,r.m_Height},0xff20291e);
    }
    I3D_scene *m_Preview = nullptr;
    I3D_model *m_Soldier = nullptr;
    std::array<I3D_model *, 2> m_Props{};
    ~C_NativePainter() override { if (m_Preview) m_Preview->Release(); }
    void Preview(const S_CampaignPreparationRect &r, AssetKey, const S_CampaignSoldierLoadout &loadout) override {
        Panel(r, false);
        if (!m_Preview) {
            auto *driver = I3DGetDriver();
            m_Preview = reinterpret_cast<I3D_scene *>(driver->CreateFrame(FRAME_SCENE));
            if (m_Preview) m_Preview->SetUnknown210(S_vector(.32f,.34f,.27f));
            auto *camera = static_cast<I3D_camera *>(driver->CreateFrame(FRAME_CAMERA));
            auto *light = static_cast<I3D_light *>(driver->CreateFrame(FRAME_LIGHT));
            auto *model = static_cast<I3D_model *>(driver->CreateFrame(FRAME_MODEL));
            if (!m_Preview || !camera || !light || !model) {
                if(camera) camera->Release(); if(light) light->Release(); if(model) model->Release();
                return;
            }
            camera->SetPos(S_vector(0, 1.1f, -6)); camera->SetWorldDir(S_vector(0, 0, 1), 0);
            camera->SetFOV(.33f); camera->SetOrthogonal(false); camera->SetRange(.1f, 20);
            m_Preview->AddFrame(camera); m_Preview->SetActiveCamera(camera); camera->Update(); camera->Release();
            light->SetLightType(I3DLIGHT_AMBIENT); light->SetColorRGB(1,1,1); light->SetPower(1);
            m_Preview->AddFrame(light); m_Preview->GetPrimarySector()->AddLight(light); light->Release();
            const auto result = model->Open("Models\\mp_en01.4ds", 0, nullptr, nullptr);
            std::printf("preview-model HRESULT=%ld\n", static_cast<long>(result)); std::fflush(stdout);
            if (SUCCEEDED(result)) {
                model->SetDir(S_vector(0,0,-1),0); model->SetPos(S_vector(0,0,0)); model->Update();
                m_Preview->AddFrame(model); m_Preview->UpdateFrameSector(model, model->GetWorldPos()); m_Soldier=model;
            }
            model->Release();
            for (std::size_t i = 0; i < m_Props.size(); ++i) {
                auto *prop = static_cast<I3D_model *>(driver->CreateFrame(FRAME_MODEL));
                if (!prop) continue;
                const auto path = i == 0 ? "Models\\w_enfield.4ds" : "Models\\l_medkit.4ds";
                const auto loaded = prop->Open(path,0,nullptr,nullptr);
                std::printf("fixture-prop %s HRESULT=%ld\n",path,static_cast<long>(loaded)); std::fflush(stdout);
                if (SUCCEEDED(loaded)) {
                    prop->SetPos(S_vector(i == 0 ? -.5f : .6f,.8f,0)); prop->Update();
                    m_Preview->AddFrame(prop); m_Preview->UpdateFrameSector(prop,prop->GetWorldPos()); m_Props[i]=prop;
                }
                prop->Release();
            }
        }
        // Synthetic fixture props illustrate committed values; production attachment/face
        // resolution belongs to the injected painter/catalog, never the shared UI model.
        for (std::size_t i = 0; i < m_Props.size(); ++i) if(m_Props[i]) {
            const ItemCatalogKey item(i == 0 ? "1" : "2");
            const bool present = std::any_of(loadout.m_Items.begin(),loadout.m_Items.end(),[&item](const auto &v){return v.m_State.m_Item==item && v.m_State.m_Quantity;});
            m_Props[i]->SetOn(present);
        }
        if (!m_Soldier) { Text(r, "Soldier preview asset unavailable", true); return; }
        const int left = static_cast<int>(m_Left+r.m_X*m_Scale), top = static_cast<int>(m_Top+r.m_Y*m_Scale);
        m_Preview->SetViewport(left,top,left+static_cast<int>(r.m_Width*m_Scale),top+static_cast<int>(r.m_Height*m_Scale));
        m_Preview->GetActiveCamera()->SetAspectRatio(r.m_Width/r.m_Height);
        m_Soldier->SetDir(S_vector(std::sin(static_cast<float>(GetTickCount())*.00025f),0,-std::cos(static_cast<float>(GetTickCount())*.00025f)),0); m_Soldier->Update();
        // The component calls inside its 2D pass; scene rendering owns Begin/End.
        GetIGraph()->EndScene(); m_Preview->Render(0); GetIGraph()->BeginScene();
        GetIGraph()->SetViewport({0,0,GetIGraph()->ScrnX(),GetIGraph()->ScrnY(),0,1});
        I3DGetDriver()->SetState(3,0);
    }
    void Asset(const S_CampaignPreparationRect &r, AssetKey) override {
        Panel(r,false);
        Quad({r.m_X+r.m_Width*.32f,r.m_Y+6,r.m_Width*.36f,18},0xff92957d);
        Quad({r.m_X+6,r.m_Y+28,r.m_Width-12,r.m_Height-34},0xff747b62);
    }
};
void Click(C_CampaignPreparationScreen &ui, E_CampaignPreparationAction a, std::uint64_t key=0) {
    ui.Tick(); for(const auto &c:ui.GetControls()) if(c.m_Action==a && c.m_Key==key) {ui.Click(c.m_Rect.m_X+2,c.m_Rect.m_Y+2);return;}
}
void Scenario(C_FakePreparation &fake, C_CampaignPreparationScreen &ui, int scenario) {
    fake.Reset();
    using A=E_CampaignPreparationAction;
    if(scenario==2) {fake.m_State.m_Context.m_LocalPlayer=PlayerId(2);fake.Publish();ui.Tick();Click(ui,A::Slot,0);}
    if(scenario==3) {fake.m_Status=E_CampaignSyncStatus::Loading;}
    if(scenario==4) {Click(ui,A::Slot,2);Click(ui,A::Equip);}
    if(scenario==5) {Click(ui,A::Slot,2);Click(ui,A::Equip);fake.Resolve(E_CampaignReason::StockUnavailable);}
    if(scenario==6) {fake.m_State.m_Context.m_LocalPlayer=PlayerId(7);for(std::size_t i=1;i<4;++i) fake.m_State.m_Slots[i]={{},E_CampaignReason::MissionRestriction,{}};fake.Publish();}
    if(scenario==7) {fake.m_State.m_Context.m_Phase=E_CampaignPhase::Frozen;fake.Publish();}
    if(scenario==8) {fake.m_Status=E_CampaignSyncStatus::Disconnected;}
    if(scenario==9) {fake.m_State.m_Soldiers[0].m_Ownership.m_Scripted=true;fake.Publish();}
    ui.Tick();
}
}
int main(int argc, char **argv) {
    static_assert(sizeof(void*)==4,"native component preview targets MSVC x86");
    // Run from a private runtime directory containing data links/copies and matching DLLs.
    struct S_Archive { const char *m_Name; std::uint32_t m_Low,m_High; };
    const S_Archive archives[]={{"PatchX03.dta",0xa0a0a0a4,0xa0a0a0a0},{"PatchX02.dta",0xa0a0a0a3,0xa0a0a0a0},
        {"PatchX01.dta",0xa0a0a0a2,0xa0a0a0a0},{"SabreSquadron.dta",0xa0a0a0a1,0xa0a0a0a0},
        {"LangEnglish.dta",0xa0a0a0a0,0xa0a0a0a0},{"Patch.dta",0x5d805270,0x10acb252},
        {"Models.dta",0x5d805259,0x10acb252},{"Maps.dta",0xb438d0a5,0xf26527fa},
        {"Maps_U.dta",0xb038d1a1,0xf26520fa},{"Others.dta",0xea85902a,0x65f7ab23}};
    std::vector<C_DtaArchive*> mounted;
    for(const auto &a:archives) if(auto *archive=dtaCreate(a.m_Name)) { if(!archive->SetKey(a.m_Low,a.m_High)) return 2;mounted.push_back(archive); }
    S_IGraphInit init={};init.instance=GetModuleHandleA(nullptr);init.width=argc>1?std::atoi(argv[1]):1024;init.height=argc>2?std::atoi(argv[2]):768;
    init.bitDepth=32;init.errorCallback=Error;
    auto *graph=GetIGraph();
    if(FAILED(graph->Init(init)) || FAILED(I3DGetDriver()->Init(0))) return 3;
    graph->SetAppName("COOP-007 native preparation fixture - F1..F9 scenarios / F10 resolve / F11 reject / F12 shot");
    graph->InitInput(0); graph->AddDir("Maps", 1); graph->AddDir("Maps_U", 1);
    int result=0;
    {
        C_NativePainter painter;
        if(!painter.Init()) {std::fprintf(stderr,"Required campaign font unavailable\n"); I3DGetDriver()->Close(); graph->Close(); for(auto *archive:mounted) archive->Release(); return 4;}
        if (!painter.Audit()) { std::fprintf(stderr,"Campaign resource audit failed\n"); result=6; }
        C_FakePreparation fake;C_CampaignPreparationScreen ui(fake,fake);
        std::string previousTrace;
        HWND window=graph->GetHWND(); bool held[256]={}; bool mouse=false;
        int scenario=1; const int frames=argc>3?std::atoi(argv[3]):0;
        for(int frame=0;ui.IsOpen() && (!frames || frame<frames);++frame) {
            graph->ProcessWinMessages(); painter.Resize();
            auto press=[&held](int vk){const bool down=(GetAsyncKeyState(vk)&0x8000)!=0; const bool hit=down&&!held[vk];held[vk]=down;return hit;};
            for(int i=0;i<9;++i) if(press(VK_F1+i)) {scenario=i+1;Scenario(fake,ui,scenario);}
            if (press('O')) {
                if (ui.GetSelection()) {
                    auto &soldier = fake.m_State.m_Soldiers.at(static_cast<std::size_t>(ui.GetSelection()->Value()-1));
                    soldier.m_Ownership.m_Owner = PlayerId(2);
                    soldier.m_Ownership.m_ControlRevision = ControlRevision(soldier.m_Ownership.m_ControlRevision.Value()+1);
                    fake.m_State.m_OwnershipRevision = OwnershipRevision(fake.m_State.m_OwnershipRevision.Value()+1); fake.Publish(true);
                    fake.Resolve(E_CampaignReason::NotOwner);
                }
            }
            if (press('D')) {fake.m_State.m_Soldiers[1].m_Ownership.m_Owner.reset();fake.m_State.m_Participants[1].m_Connected=false;fake.Publish(true);}
            if (press('H')) {fake.m_State.m_Soldiers[0].m_Ownership.m_Owner.reset();fake.Publish(true);}
            if (press('R')) {for(auto &p:fake.m_State.m_Participants) p.m_ReadyAt=fake.m_State.m_PreparationRevision;fake.Publish();}
            if(press(VK_F10)) fake.Resolve(); if(press(VK_F11)) fake.Resolve(E_CampaignReason::StaleRevision);
            if(press(VK_TAB)) ui.Key((GetAsyncKeyState(VK_SHIFT)&0x8000)?E_CampaignPreparationKey::BackTab:E_CampaignPreparationKey::Tab);
            for(const auto &pair:std::array<std::pair<int,E_CampaignPreparationKey>,7>{{{VK_RETURN,E_CampaignPreparationKey::Enter},{VK_SPACE,E_CampaignPreparationKey::Space},{VK_ESCAPE,E_CampaignPreparationKey::Escape},{VK_LEFT,E_CampaignPreparationKey::Left},{VK_RIGHT,E_CampaignPreparationKey::Right},{VK_UP,E_CampaignPreparationKey::Up},{VK_DOWN,E_CampaignPreparationKey::Down}}})
                if(press(pair.first)) ui.Key(pair.second);
            const bool down=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
            if(down&&!mouse) {POINT point;GetCursorPos(&point);ScreenToClient(window,&point);ui.Click((point.x-painter.m_Left)/painter.m_Scale,(point.y-painter.m_Top)/painter.m_Scale);}
            mouse=down;
            graph->Clear(0xff292f28,1,3);
            graph->BeginScene(); I3DGetDriver()->SetState(3,0); I3DGetDriver()->SetState(9,0); ui.Render(painter); graph->EndScene();
            const auto snapshot = ui.GetSnapshot();
            int readyCount = 0, equip = 0;
            if (snapshot) for (const auto &participant : snapshot->m_Participants)
                if (participant.m_ReadyAt == snapshot->m_PreparationRevision) ++readyCount;
            for (const auto &control : ui.GetControls()) if (control.m_Action == E_CampaignPreparationAction::Equip) equip = control.m_Enabled ? 1 : 0;
            const std::string trace = "STATE selected=" + std::to_string(ui.GetSelection() ? ui.GetSelection()->Value() : 0) +
                " focus=" + std::to_string(ui.GetFocus()) + " pending=" + std::to_string(ui.IsPending()) +
                " stock=" + std::to_string(snapshot && !snapshot->m_Stock.empty() ? snapshot->m_Stock[0].m_State.m_Quantity : 9999) +
                " generation=" + std::to_string(snapshot ? snapshot->m_Generation.Value() : 0) +
                " revision=" + std::to_string(snapshot ? snapshot->m_PreparationRevision.Value() : 0) +
                " notice=" + std::to_string(static_cast<int>(ui.GetNotice())) + " scenario=" + std::to_string(scenario) + " ready=" + std::to_string(readyCount) + " equip=" + std::to_string(equip) + " phase=" + std::to_string(snapshot ? static_cast<int>(snapshot->m_Context.m_Phase) : 0);
            if(trace != previousTrace) {std::puts(trace.c_str());std::fflush(stdout);previousTrace=trace;}
            if(press(VK_F12) || (frames && frame==frames-1)) {
                char path[80];std::snprintf(path,sizeof(path),"coop007-%d-%dx%d.tga",scenario,init.width,init.height);
                if(!graph->SaveShot(path,0,0,0)) {std::fprintf(stderr,"Screenshot failed\n");result=5;}
            }
            graph->Flip();Sleep(16);
        }
    }
    I3DGetDriver()->Close();graph->Close(); for(auto *archive:mounted) archive->Release();
    return result;
}
