#include "campaign_control_runtime.h"
#include "campaign_control_adapter.h"
#include <map>
namespace {
const I_CampaignOwnershipView *g_View = nullptr;
std::map<int, PlayerId> g_Players;
C_CampaignControlAdapter *g_Adapter = nullptr;
std::optional<S_CampaignControlToken> g_Dispatch;
}
void InstallCampaignControlView(const I_CampaignOwnershipView *view) {
    InstallCampaignControlRuntime(view, nullptr);
}
void InstallCampaignControlRuntime(const I_CampaignOwnershipView *view, C_CampaignControlAdapter *adapter) {
    g_Dispatch.reset();
    g_View = view;
    g_Adapter = adapter;
    g_Players.clear();
}
std::optional<PlayerId> FindCampaignTransportPlayer(int slot) {
    const auto it = g_Players.find(slot);
    return it == g_Players.end() ? std::nullopt : std::optional<PlayerId>(it->second);
}
E_CampaignReason HandleCampaignDisconnect(PlayerId exactIncarnation) {
    if (!CampaignControlsActive() || !g_Adapter) return E_CampaignReason::InvalidPhase;
    const auto reason = g_Adapter->OnDisconnected(exactIncarnation);
    // Retire transport identity even when engine handoff needs retry; old input
    // already lost authority at the committed ownership event.
    ForgetCampaignTransportPlayer(exactIncarnation);
    return reason;
}
T_CampaignOwnershipSnapshot GetCampaignControlSnapshot() {
    return g_View ? g_View->GetSnapshot() : nullptr;
}
bool CampaignControlsActive() {
    const auto state = GetCampaignControlSnapshot();
    return state && state->m_Context.m_Kind == E_CampaignSessionKind::CampaignCoop;
}
std::optional<S_CampaignOwnershipRecord> FindCampaignActorControl(int actorId) {
    if (actorId < 0) return std::nullopt;
    const auto state = GetCampaignControlSnapshot();
    if (!state || state->m_Context.m_Kind != E_CampaignSessionKind::CampaignCoop) return std::nullopt;
    for (const auto &record : state->m_Soldiers) if (record.m_Actor &&
        record.m_Actor->m_Session == state->m_Context.m_Session &&
        record.m_Actor->m_Generation == state->m_Context.m_MissionGeneration &&
        record.m_Actor->m_Actor.Value() == static_cast<std::uint32_t>(actorId)) return record;
    return std::nullopt;
}
bool IsCampaignLocalActor(int actorId) {
    const auto record = FindCampaignActorControl(actorId);
    const auto state = GetCampaignControlSnapshot();
    return record && state && record->m_Alive && record->m_Eligible && record->m_Deployed &&
        !record->m_Scripted && record->m_Owner && record->m_Owner == state->m_Context.m_LocalPlayer &&
        (state->m_Context.m_Phase == E_CampaignPhase::Loading || state->m_Context.m_Phase == E_CampaignPhase::Running);
}
bool BindCampaignTransportPlayer(int slot, PlayerId incarnation) {
    const auto state = GetCampaignControlSnapshot();
    if (slot < 0 || !state || !CampaignControlsActive()) return false;
    bool admitted = false;
    for (const auto &p : state->m_Participants)
        if (p.m_Player == incarnation && p.m_Admitted && p.m_Connected) admitted = true;
    if (!admitted) return false;
    for (const auto &entry : g_Players)
        if (entry.second == incarnation && entry.first != slot) return false;
    auto old = g_Players.find(slot);
    if (old != g_Players.end() && old->second != incarnation) return false;
    g_Players[slot] = incarnation;
    return true;
}
void ForgetCampaignTransportPlayer(PlayerId incarnation) {
    for (auto it = g_Players.begin(); it != g_Players.end();)
        if (it->second == incarnation) it = g_Players.erase(it); else ++it;
}
std::optional<int> FindCampaignPlayerSlot(int actorId) {
    const auto record = FindCampaignActorControl(actorId);
    const auto state = GetCampaignControlSnapshot();
    if (!record || !record->m_Owner || !state) return std::nullopt;
    bool admitted = false;
    for (const auto &p : state->m_Participants)
        if (p.m_Player == record->m_Owner && p.m_Admitted && p.m_Connected) admitted = true;
    if (!admitted) return std::nullopt;
    for (const auto &entry : g_Players) if (entry.second == *record->m_Owner) return entry.first;
    return std::nullopt;
}

bool CampaignInputAuthorized(int actorId) {
    if (!g_Dispatch) return false;
    const auto record = FindCampaignActorControl(actorId);
    const auto state = GetCampaignControlSnapshot();
    return record && state && state->m_Context.m_Session == g_Dispatch->m_Session &&
        state->m_Context.m_MissionGeneration == g_Dispatch->m_Generation &&
        record->m_Actor == g_Dispatch->m_Actor &&
        record->m_Soldier == g_Dispatch->m_Soldier && record->m_Owner == g_Dispatch->m_Player &&
        record->m_ControlRevision == g_Dispatch->m_ControlRevision &&
        record->m_Alive && record->m_Eligible && record->m_Deployed && !record->m_Scripted &&
        state->m_Context.m_Phase == E_CampaignPhase::Running;
}
void DispatchCampaignInput(const S_CampaignControlToken &token, const std::function<void()> &execute) {
    if (g_Dispatch || !execute) return; // nonreentrant authority queue
    g_Dispatch = token;
    try {
        if (CampaignInputAuthorized(static_cast<int>(token.m_Actor.m_Actor.Value()))) execute();
    } catch (...) { g_Dispatch.reset(); throw; }
    g_Dispatch.reset();
}
