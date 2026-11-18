#include "campaign_session.h"
#include <algorithm>
#include <set>
#include <stdexcept>

C_CampaignSession::C_CampaignSession(const S_CampaignSessionSetup &setup,
    const std::vector<int> &profileIndexes) {
    if (!setup.m_Session.IsValid() || !setup.m_Campaign.IsValid() || !setup.m_Mission.IsValid() ||
        profileIndexes.empty() || profileIndexes.size() > kMaxRoster)
        throw std::invalid_argument("invalid campaign session/catalog");
    std::set<int> unique;
    for (int profile : profileIndexes) {
        if (profile < 0 || !unique.insert(profile).second)
            throw std::invalid_argument("missing or duplicate campaign profile identity");
        // Explicit allocation, independent of profileIndex/squad/actor/player slots.
        m_Roster.push_back({profile, SoldierId(static_cast<std::uint64_t>(m_Roster.size()) + 1)});
    }
    m_Context.m_Kind = E_CampaignSessionKind::CampaignCoop;
    m_Context.m_Session = setup.m_Session;
    m_Context.m_Campaign = setup.m_Campaign;
    m_Context.m_Mission = setup.m_Mission;
    m_Context.m_Phase = E_CampaignPhase::Setup;
    m_Context.m_UsesCampaignRules = true;
    m_Context.m_IsTransportServer = true;
    m_Context.m_IsTransportClient = setup.m_ListenHost;
    m_Context.m_LocalPlayer = PlayerId(1);
    m_Context.m_HostPlayer = PlayerId(1);
    m_Context.m_Rules = setup.m_Rules;
}

std::optional<SoldierId> C_CampaignSession::FindSoldier(int profileIndex) const {
    for (const auto &entry : m_Roster) if (entry.m_ProfileIndex == profileIndex) return entry.m_Soldier;
    return std::nullopt;
}
std::optional<int> C_CampaignSession::FindProfile(SoldierId soldier) const {
    for (const auto &entry : m_Roster) if (entry.m_Soldier == soldier) return entry.m_ProfileIndex;
    return std::nullopt;
}
bool C_CampaignSession::UsesCampaignRules(const S_CampaignSessionContext &context) {
    return context.m_UsesCampaignRules && context.m_Kind != E_CampaignSessionKind::LegacyMultiplayer;
}
bool C_CampaignSession::HasSimulationAuthority(const S_CampaignSessionContext &context) {
    return context.m_Kind == E_CampaignSessionKind::Solo || context.m_IsTransportServer;
}
E_CampaignReason C_CampaignSession::StartPreparation() {
    const auto phase = m_Context.m_Phase;
    if (phase != E_CampaignPhase::Setup && phase != E_CampaignPhase::Frozen &&
        phase != E_CampaignPhase::Loading && phase != E_CampaignPhase::Ending)
        return E_CampaignReason::InvalidPhase;
    auto next = CampaignNextCounter(m_PreparationGeneration);
    if (!next) return E_CampaignReason::WrongGeneration;
    m_PreparationGeneration = *next;
    m_Context.m_Phase = E_CampaignPhase::Preparation;
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignSession::Freeze() {
    if (m_Context.m_Phase != E_CampaignPhase::Preparation) return E_CampaignReason::InvalidPhase;
    m_Context.m_Phase = E_CampaignPhase::Frozen;
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignSession::BeginMissionGeneration(MissionKey mission) {
    const auto phase = m_Context.m_Phase;
    if (phase != E_CampaignPhase::Frozen && phase != E_CampaignPhase::Running &&
        phase != E_CampaignPhase::Setup) return E_CampaignReason::InvalidPhase;
    if (!mission.IsValid()) return E_CampaignReason::Malformed;
    auto next = CampaignNextCounter(m_Context.m_MissionGeneration);
    if (!next) return E_CampaignReason::WrongGeneration;
    m_Context.m_MissionGeneration = *next;
    m_Context.m_Mission = std::move(mission);
    m_Context.m_Phase = E_CampaignPhase::Loading;
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignSession::StartSimulation() {
    if (m_Context.m_Phase != E_CampaignPhase::Loading) return E_CampaignReason::InvalidPhase;
    m_Context.m_Phase = E_CampaignPhase::Running;
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignSession::EndMission() {
    if (m_Context.m_Phase != E_CampaignPhase::Running) return E_CampaignReason::InvalidPhase;
    m_Context.m_Phase = E_CampaignPhase::Ending;
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignSession::ReturnToSetup(MissionKey mission) {
    const auto phase = m_Context.m_Phase;
    if (phase != E_CampaignPhase::Ending && phase != E_CampaignPhase::Frozen &&
        phase != E_CampaignPhase::Loading) return E_CampaignReason::InvalidPhase;
    if (!mission.IsValid()) return E_CampaignReason::Malformed;
    m_Context.m_Mission = std::move(mission);
    m_Context.m_Phase = E_CampaignPhase::Setup;
    return E_CampaignReason::None;
}
E_CampaignReason C_CampaignSession::Close() {
    m_Context.m_Phase = E_CampaignPhase::Closed;
    return E_CampaignReason::None;
}

C_CampaignRequestJournal::C_CampaignRequestJournal(SessionId session, std::size_t cachePerPlayer)
    : m_Session(session), m_CachePerPlayer(cachePerPlayer) {
    if (!session.IsValid() || cachePerPlayer == 0 || cachePerPlayer > 64)
        throw std::invalid_argument("invalid campaign journal bounds");
}
E_CampaignReason C_CampaignRequestJournal::Admit(PlayerId player) {
    if (!player.IsValid()) return E_CampaignReason::NotAdmitted;
    if (m_Players.find(player) != m_Players.end()) return E_CampaignReason::None;
    if (m_Players.size() == 4) return E_CampaignReason::SessionFull;
    m_Players.emplace(player, S_Player{});
    return E_CampaignReason::None;
}
void C_CampaignRequestJournal::Forget(PlayerId player) { m_Players.erase(player); }

std::optional<S_CampaignRequestResult> C_CampaignRequestJournal::Begin(
    const S_CampaignRequestContext &context, E_CampaignRequestFamily family,
    const std::string &canonicalContent, const S_CampaignRequestResult &currentState) {
    auto reject = [&](E_CampaignReason reason, bool resync = false) {
        auto result = currentState;
        result.m_Context = context;
        result.m_Family = family;
        result.m_Outcome = resync ? E_CampaignRequestOutcome::ResyncRequired : E_CampaignRequestOutcome::Rejected;
        result.m_Reason = reason;
        result.m_OriginalOutcome.reset();
        return std::optional<S_CampaignRequestResult>(std::move(result));
    };
    if (context.m_Session != m_Session) return reject(E_CampaignReason::WrongSession);
    auto player = m_Players.find(context.m_PlayerEcho);
    if (player == m_Players.end()) return reject(E_CampaignReason::NotAdmitted);
    if (!context.m_Request.IsValid() || canonicalContent.size() > kMaxContent)
        return reject(E_CampaignReason::Malformed);
    auto &state = player->second;
    if (state.m_Pending) return reject(E_CampaignReason::PendingTransaction);
    for (const auto &entry : state.m_Cache) {
        if (entry.m_Context.m_Request != context.m_Request) continue;
        if (entry.m_Result.m_PreparationGeneration != currentState.m_PreparationGeneration ||
            entry.m_Result.m_MissionGeneration != currentState.m_MissionGeneration)
            return reject(E_CampaignReason::WrongGeneration);
        if (entry.m_Family != family || entry.m_Content != canonicalContent)
            return reject(E_CampaignReason::RequestConflict);
        auto result = entry.m_Result;
        result.m_OriginalOutcome = result.m_Outcome;
        result.m_Outcome = E_CampaignRequestOutcome::Duplicate;
        return result;
    }
    if (!(state.m_Highest < context.m_Request)) return reject(E_CampaignReason::RequestExpired, true);
    state.m_Highest = context.m_Request;
    state.m_Pending = S_Entry{context, family, canonicalContent, currentState};
    return std::nullopt;
}
void C_CampaignRequestJournal::Finish(const S_CampaignRequestContext &context,
    E_CampaignRequestFamily family, const std::string &canonicalContent,
    const S_CampaignRequestResult &result) {
    auto player = m_Players.find(context.m_PlayerEcho);
    if (context.m_Session != m_Session || player == m_Players.end() || !player->second.m_Pending)
        throw std::logic_error("campaign journal Finish without Begin");
    auto &state = player->second;
    const auto &pending = *state.m_Pending;
    if (pending.m_Context.m_Request != context.m_Request || pending.m_Family != family ||
        pending.m_Content != canonicalContent || result.m_Context.m_Session != context.m_Session ||
        result.m_Context.m_PlayerEcho != context.m_PlayerEcho || result.m_Context.m_Request != context.m_Request ||
        result.m_PreparationGeneration != pending.m_Result.m_PreparationGeneration ||
        result.m_MissionGeneration != pending.m_Result.m_MissionGeneration ||
        result.m_Family != family || (result.m_Outcome != E_CampaignRequestOutcome::Accepted &&
        result.m_Outcome != E_CampaignRequestOutcome::Rejected))
        throw std::logic_error("campaign journal Finish mismatch");
    state.m_Cache.push_back({context, family, canonicalContent, result});
    if (state.m_Cache.size() > m_CachePerPlayer) state.m_Cache.pop_front();
    state.m_Pending.reset();
}
