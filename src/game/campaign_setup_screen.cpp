#include "game/main_menu.h"

#include <cstdio>

#include "game/control.h"
#include "game/controls.h"
#include "game/screen.h"
#include "ls3d/i3d.h"

namespace {

using State = E_CampaignSetupState;
using Reason = E_CampaignReason;
constexpr int kButton = 0x14, kText = 0x13, kAddressBox = 0x1b;
constexpr auto kFrames = ENUMF_WILDMASK | FRAME_MASK_VISUAL | FRAME_MASK_DUMMY;
constexpr std::uint32_t kTitleStyle = 10, kTextStyle = 0xB, kButtonStyle = 0xD;
constexpr std::uint32_t kSetupScreen = MAIN_SCREEN_CAMPAIGN_SETUP << SCREEN_ID_SHIFT;
constexpr std::uint32_t kHost = kSetupScreen | 0x400000;
constexpr std::uint32_t kAddress = kSetupScreen | 0x500000;
constexpr std::uint32_t kJoin = kSetupScreen | 0x600000;
constexpr std::uint32_t kLeave = kSetupScreen | 0x700000;
constexpr std::uint32_t kBack = kSetupScreen | 0x800000;
constexpr std::uint32_t kStatus = kSetupScreen | 0x900000;
constexpr std::uint32_t kPrepare = kSetupScreen | 0xA00000;

I3D_visual *ButtonFace(I3D_frame *button)
{
    return button ? static_cast<I3D_visual *>(button->FindChildFrame("normal*", ENUMF_WILDMASK | FRAME_MASK_VISUAL))
                  : nullptr;
}

float ButtonCenterY(I3D_visual &face)
{
    const auto &bounds = face.GetWorldBound();
    return (bounds.min.y + bounds.max.y) * 0.5f;
}

void MoveInMenu(I3D_frame &frame, const S_vector &offset)
{
    const auto &parent = *frame.GetParent();
    S_matrix inverse;
    if (inverse.Inverse(parent.GetMatrix()))
        frame.SetPos(frame.GetPos() + (parent.GetWorldPos() + offset) * inverse);
}

const char *JoinFailure(Reason reason)
{
    switch (reason) {
    case Reason::None: return "Enter the host's IPv4 address to join.";
    case Reason::ContentMismatch: return "Campaign files differ or could not be read.";
    case Reason::UnsupportedVersion:
    case Reason::ModRequired:
    case Reason::MissingFeature:
    case Reason::RulesMismatch: return "The host uses a different campaign build.";
    case Reason::SessionFull: return "This campaign session is full.";
    case Reason::Timeout: return "The host did not respond.";
    case Reason::Disconnected: return "The connection to the host ended.";
    default: return "Could not join the campaign session.";
    }
}

void SetText(C_MainMenu &menu, std::uint32_t target, const char *text, std::uint32_t style = kTextStyle)
{
    S_ControlMessage message;
    message.target = target;
    message.id = CONTROL_MSG_SET_TEXT;
    message.value = style;
    message.text = C_str(text);
    menu.HandleMessage(&message);
}

void Activate(C_MainMenu &menu, std::uint32_t target, bool active)
{
    S_ControlMessage message;
    message.target = target;
    message.id = active ? CONTROL_MSG_ACTIVATE : CONTROL_MSG_DEACTIVATE;
    menu.HandleMessage(&message);
}

void PostMenuAction(std::uint32_t id, std::uint32_t value = 0)
{
    S_ControlMessage message;
    message.id = id;
    message.value = value;
    PostControlMessage(message);
}

S_ControlDesc *AddText(S_ScreenDesc *desc, int type, const char *name, const char *text, std::uint32_t style)
{
    auto *control = desc->AddControl(type, name);
    AddDescMessage(control, 0, CONTROL_MSG_SET_TEXT, style, C_str(text));
    return control;
}

} // namespace

void C_MainMenu::DescribeCampaignPicker(S_ScreenDesc *desc, I3D_frame *model)
{
    auto *loneWolf = ButtonFace(model->FindChildFrame("blone wolf", kFrames));
    auto *carnage = model->FindChildFrame("carnage", kFrames);
    auto *carnageFace = ButtonFace(carnage);
    if (!loneWolf || !carnageFace)
        return;
    const float rowPitch = ButtonCenterY(*carnageFace) - ButtonCenterY(*loneWolf);
    const S_vector row(0.0f, rowPitch, 0.0f);
    auto *coop = CloneFrame(C_str("bcoop campaign"), carnage->GetParent(), carnage);
    MoveInMenu(*coop, row);
    for (const char *name : {"text_HD2datadisk", "bcampaign01", "blone wolf01", "carnage01",
                             "bsingle mission", "bsingle mission- carnage"}) {
        if (auto *frame = model->FindChildFrame(name, kFrames))
            MoveInMenu(*frame, row);
    }
    auto *button = AddText(desc, kButton, "bcoop campaign", "Co-op Campaign", kButtonStyle);
    const auto self = reinterpret_cast<std::uintptr_t>(this);
    const auto callback = reinterpret_cast<std::uintptr_t>(&OnCampaignPicker);
    AddDescMessage(button, 0, CONTROL_MSG_ADD_CALLBACK, CONTROL_NOTIFY_ENTER, self, callback);
    AddDescMessage(button, 0, CONTROL_MSG_ADD_CALLBACK, CONTROL_NOTIFY_CLICK, self, callback);
}

void C_MainMenu::DescribeCampaignSetup(S_ScreenDesc *desc, I3D_frame *model)
{
    // The original button frames also supply the normal and active faces for
    // the native address edit box.
    for (const char *name : {"carnage01", "bsingle mission",
                             "bsingle mission- carnage", "single side", "text00"}) {
        if (auto *frame = model->FindChildFrame(name, kFrames))
            frame->SetOn(false);
    }
    const auto self = reinterpret_cast<std::uintptr_t>(this);
    const auto screenCallback = reinterpret_cast<std::uintptr_t>(&OnCampaignSetup);
    constexpr std::uint32_t notifications[] = {CONTROL_NOTIFY_SHOWN, CONTROL_NOTIFY_HIDDEN, CONTROL_NOTIFY_SCREEN_TICK};
    for (auto notification : notifications)
        AddScreenMessage(desc, 0, CONTROL_MSG_ADD_CALLBACK, notification, self, screenCallback);
    const auto buttonCallback = reinterpret_cast<std::uintptr_t>(&OnCampaignSetupButton);

    AddText(desc, kText, "text_screen", "Co-op Campaign", kTitleStyle);
    AddDescMessage(desc->AddControl(kText, "text_h&dversion"), 0, CONTROL_MSG_SET_TEXT_ID, 0x28, 0x7D6);
    AddText(desc, kText, "text_HD2original", "H&D2 Original / Arctic1", kButtonStyle);
    auto *host = AddText(desc, kButton, "bcampaign", "Host Campaign", kButtonStyle);
    auto *address = AddText(desc, kAddressBox, "blone wolf", "127.0.0.1", kButtonStyle);
    AddDescMessage(address, 0, CONTROL_MSG_SET_MAX_LENGTH, 15);
    AddDescMessage(address, 0, CONTROL_MSG_ADD_CALLBACK, CONTROL_NOTIFY_RETURN, self, buttonCallback);
    auto *join = AddText(desc, kButton, "carnage", "Join by IPv4 Address", kButtonStyle);
    auto *leave = AddText(desc, kButton, "bcampaign01", "Leave Session", kButtonStyle);
    AddDescMessage(leave, 0, CONTROL_MSG_HIDE, 0);
    auto *back = AddText(desc, kButton, "bback", "Back", kButtonStyle);
    for (auto *button : {host, join, leave, back})
        AddDescMessage(button, 0, CONTROL_MSG_ADD_CALLBACK, CONTROL_NOTIFY_CLICK, self, buttonCallback);
    AddText(desc, kText, "text_HD2datadisk", "", kTextStyle);
    auto *prepare = AddText(desc, kButton, "blone wolf01", "Prepare Mission", kButtonStyle);
    AddDescMessage(prepare, 0, CONTROL_MSG_HIDE, 0);
    AddDescMessage(prepare, 0, CONTROL_MSG_ADD_CALLBACK, CONTROL_NOTIFY_CLICK, self, buttonCallback);
}

bool C_MainMenu::OnCampaignPicker(void *menu, void *, C_Control *, uint32_t notification, uint32_t)
{
    auto &self = *static_cast<C_MainMenu *>(menu);
    if (self.m_screens.size() <= MAIN_SCREEN_CAMPAIGN_SETUP)
        return false;
    if (notification == CONTROL_NOTIFY_ENTER) {
        S_ControlMessage message;
        message.target = (MAIN_SCREEN_SINGLEPLAYER << SCREEN_ID_SHIFT) | 0x200000;
        message.id = CONTROL_MSG_HIDE_FRAME;
        self.HandleMessage(&message);
        SetText(self, (MAIN_SCREEN_SINGLEPLAYER << SCREEN_ID_SHIFT) | 0x300000,
                "Host or join the original campaign on your LAN.", 3);
    } else if (notification == CONTROL_NOTIFY_CLICK) {
        PostMenuAction(MENU_MSG_OPEN_SCREEN, MAIN_SCREEN_CAMPAIGN_SETUP);
    }
    return true;
}

void C_MainMenu::RefreshCampaignSetup()
{
    auto &session = *m_campaignSetup;
    session.Update();
    const auto state = session.GetState();
    if (state != m_campaignSetupState) {
        const bool idle = state == State::Idle;
        Activate(*this, kHost, idle);
        Activate(*this, kAddress, idle);
        Activate(*this, kJoin, idle);
        S_ControlMessage visibility;
        visibility.target = kLeave;
        visibility.id = idle ? CONTROL_MSG_HIDE : CONTROL_MSG_SHOW;
        HandleMessage(&visibility);
        if (!idle)
            SetText(*this, kLeave, state == State::Hosting ? "End Session" :
                    state == State::Joining ? "Cancel Join" : "Leave Session", kButtonStyle);
        visibility.target = kPrepare;
        visibility.id = state == State::Hosting ? CONTROL_MSG_SHOW : CONTROL_MSG_HIDE;
        HandleMessage(&visibility);
        m_campaignSetupState = state;
    }

    const char *status = m_campaignSetupError.IsEmpty()
        ? JoinFailure(session.GetJoinFailure()) : m_campaignSetupError.c_str();
    char hostStatus[100];
    switch (state) {
    case State::Joining:
        status = "Connecting to the campaign host...";
        break;
    case State::Hosting:
        std::snprintf(hostStatus, sizeof hostStatus, "Hosting on LAN. Participants: %zu of %zu.",
                      session.GetHostOwnership()->GetParticipantCount(), CampaignProtocol::kMaxParticipants);
        status = hostStatus;
        break;
    case State::Guest:
        status = "Connected to the campaign host.";
        break;
    case State::Idle:
        break;
    }
    if (!m_campaignSetupError.IsEmpty())
        status = m_campaignSetupError.c_str();
    if (!(m_campaignSetupStatus == status)) {
        SetText(*this, kStatus, status);
        m_campaignSetupStatus = C_str(status);
    }
#ifndef HD2_DEDICATED
    if (session.GetPreparation() &&
        session.GetPreparation()->GetSnapshot().m_Status == E_CampaignSyncStatus::Current)
        OpenCampaignPreparation();
#endif
}

#ifndef HD2_DEDICATED
bool C_MainMenu::OpenCampaignPreparation()
{
    auto *model = m_campaignSetup->GetPreparation();
    if (!model)
        return false;
    if (!m_campaignPreparation) {
        auto screen = std::make_unique<C_CampaignPreparationNativeUI>(*model);
        if (!screen->IsUsable()) {
            m_campaignSetup->Close();
            m_campaignSetupError = C_str("Campaign preparation could not load its font.");
            return false;
        }
        m_campaignPreparation = std::move(screen);
    }
    return true;
}

bool C_MainMenu::HandleInput(float *held, float *pressed)
{
    if (!m_campaignPreparation)
        return C_MenuState::HandleInput(held, pressed);
    if (!m_keyboardOff)
        m_campaignPreparation->KeyInput(pressed);
    return true;
}

void C_MainMenu::Update(int timeDelta)
{
    if (!m_campaignPreparation) {
        C_MenuState::Update(timeDelta);
        return;
    }
    m_campaignSetup->Update();
    if (m_campaignSetup->GetState() != State::Idle) {
        m_campaignPreparation->Update(timeDelta);
        if (!m_keyboardOff && m_cursor)
            m_campaignPreparation->MouseInput(*m_cursor, timeDelta);
        if (!m_campaignPreparation->LeaveRequested()) {
            GetSoundModule()->Update(timeDelta);
            return;
        }
    }
    m_campaignPreparation.reset();
    if (m_campaignSetup->GetState() != State::Idle)
        m_campaignSetup->Close();
    RefreshCampaignSetup();
}

bool C_MainMenu::OnRender()
{
    if (!m_campaignPreparation)
        return false;
    m_campaignPreparation->Render();
    return true;
}

std::unique_ptr<C_CampaignSetupSession> C_MainMenu::TakeCampaignDeployment()
{
    if (!m_campaignSetup || !m_campaignSetup->DeploymentPending())
        return {};
    m_campaignPreparation.reset();
    return std::move(m_campaignSetup);
}

void C_MainMenu::RestoreCampaignPreparation(std::unique_ptr<C_CampaignSetupSession> session, Reason reason)
{
    m_campaignSetup = std::move(session);
    if (reason != Reason::None)
        m_campaignSetupError = C_str("The mission could not start. Check the session and try again.");
    PostMenuAction(MENU_MSG_OPEN_SCREEN, MAIN_SCREEN_SINGLEPLAYER);
    PostMenuAction(MENU_MSG_OPEN_SCREEN, MAIN_SCREEN_CAMPAIGN_SETUP);
}
#endif

bool C_MainMenu::OnCampaignSetup(void *menu, void *, C_Control *, uint32_t notification, uint32_t)
{
    auto &self = *static_cast<C_MainMenu *>(menu);
    if (notification == CONTROL_NOTIFY_HIDDEN) {
#ifndef HD2_DEDICATED
        self.m_campaignPreparation.reset();
#endif
        self.m_campaignSetup.reset();
        return true;
    }
    if (notification == CONTROL_NOTIFY_SHOWN) {
        if (!self.m_campaignSetup) {
            self.m_campaignSetup = std::make_unique<C_CampaignSetupSession>();
            self.m_campaignSetupError = C_str();
        }
        self.m_campaignSetupStatus = C_str();
    }
    if (self.m_campaignSetup)
        self.RefreshCampaignSetup();
    return true;
}

bool C_MainMenu::OnCampaignSetupButton(void *menu, void *, C_Control *control, uint32_t, uint32_t)
{
    auto &self = *static_cast<C_MainMenu *>(menu);
    if (!self.m_campaignSetup || !control)
        return false;
    auto &session = *self.m_campaignSetup;
    const char *error = nullptr;
    switch (control->GetId()) {
    case kHost: {
        const auto reason = session.Host();
        if (reason != Reason::None)
            error = reason == Reason::ContentMismatch ? JoinFailure(reason) : "Could not host a LAN session.";
        break;
    }
    case kAddress:
    case kJoin: {
        S_ControlMessage field;
        field.target = kAddress;
        field.id = CONTROL_MSG_GET_TEXT;
        self.HandleMessage(&field);
        const auto address = ParseCampaignLanAddress(field.text.c_str());
        if (!address) {
            error = "Enter an IPv4 address such as 192.168.1.10.";
        } else if (const auto reason = session.Join(*address); reason != Reason::None) {
            error = JoinFailure(reason);
        }
        break;
    }
    case kLeave:
        session.Close();
        break;
    case kPrepare:
#ifndef HD2_DEDICATED
        if (session.BeginPreparation() != Reason::None)
            error = "The original mission equipment could not be loaded.";
        else if (!self.OpenCampaignPreparation())
            error = "Campaign preparation could not load its font.";
#endif
        break;
    case kBack:
        PostMenuAction(MENU_MSG_BACK);
        return true;
    default:
        return false;
    }
    self.m_campaignSetupError = C_str(error ? error : "");
    self.RefreshCampaignSetup();
    return true;
}
