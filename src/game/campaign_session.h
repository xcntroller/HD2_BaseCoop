#ifndef HD2_CAMPAIGN_SESSION_H
#define HD2_CAMPAIGN_SESSION_H

#include "campaign_contracts.h"
#include <deque>
#include <limits>
#include <map>

// No wrapping: exhaustion requires a new enclosing session, not reuse of a token.
template<class T> std::optional<T> CampaignNextCounter(T current) {
    if (current.Value() == (std::numeric_limits<std::uint64_t>::max)()) return std::nullopt;
    return T(current.Value() + 1);
}

struct S_CampaignRosterEntry {
    int m_ProfileIndex = -1;
    SoldierId m_Soldier;
};
struct S_CampaignSessionSetup {
    SessionId m_Session; // caller allocates a fresh hosting-lifetime token
    CampaignId m_Campaign;
    MissionKey m_Mission;
    S_CampaignRulesDescriptor m_Rules;
    bool m_ListenHost = true;
};

// Host-side context and in-memory campaign roster. Construct once per hosting
// lifetime; retain the catalog across missions. No profile/actor/transport pointers.
class C_CampaignSession {
public:
    static constexpr std::size_t kMaxRoster = 256;
    C_CampaignSession(const S_CampaignSessionSetup &setup, const std::vector<int> &profileIndexes);
    const S_CampaignSessionContext &GetContext() const { return m_Context; }
    const std::vector<S_CampaignRosterEntry> &GetRoster() const { return m_Roster; }
    std::optional<SoldierId> FindSoldier(int profileIndex) const;
    std::optional<int> FindProfile(SoldierId soldier) const;
    PreparationGeneration GetPreparationGeneration() const { return m_PreparationGeneration; }
    static bool UsesCampaignRules(const S_CampaignSessionContext &context);
    static bool HasSimulationAuthority(const S_CampaignSessionContext &context);

    // Trusted authority operations, never client opcode handlers. Ownership owns
    // its own session value and wraps these to publish one coherent catalog cut.
    E_CampaignReason StartPreparation();
    E_CampaignReason Freeze();
    E_CampaignReason BeginMissionGeneration(MissionKey mission);
    E_CampaignReason StartSimulation(); // adapter must first verify the full world/peer barrier
    E_CampaignReason EndMission();
    E_CampaignReason ReturnToSetup(MissionKey mission);
    E_CampaignReason Close();
private:
    S_CampaignSessionContext m_Context;
    PreparationGeneration m_PreparationGeneration;
    std::vector<S_CampaignRosterEntry> m_Roster;
};

// Reusable across ownership/edit/ready/deploy. Bind all consumers to ONE journal
// and ONE serialized, nonreentrant authority queue. Authenticate/admission and
// session/generation/phase fences precede Begin; semantic rejections call Finish.
// canonicalContent is a bounded, unambiguous full request encoding (not a hash).
// currentState supplies the authoritative publication for early rejections.
// Its optional generation fields describe the full CURRENT session epoch across
// all families, populated whenever each counter is valid, not request presence.
class C_CampaignRequestJournal {
public:
    static constexpr std::size_t kMaxContent = 4096;
    explicit C_CampaignRequestJournal(SessionId session, std::size_t cachePerPlayer = 16);
    SessionId GetSession() const { return m_Session; }
    E_CampaignReason Admit(PlayerId player);
    void Forget(PlayerId player); // exact disconnected incarnation; must never be reused
    std::optional<S_CampaignRequestResult> Begin(const S_CampaignRequestContext &context,
        E_CampaignRequestFamily family, const std::string &canonicalContent,
        const S_CampaignRequestResult &currentState);
    void Finish(const S_CampaignRequestContext &context, E_CampaignRequestFamily family,
        const std::string &canonicalContent, const S_CampaignRequestResult &result);
private:
    struct S_Entry {
        S_CampaignRequestContext m_Context;
        E_CampaignRequestFamily m_Family;
        std::string m_Content;
        S_CampaignRequestResult m_Result;
    };
    struct S_Player {
        RequestId m_Highest;
        std::optional<S_Entry> m_Pending;
        std::deque<S_Entry> m_Cache;
    };
    SessionId m_Session;
    std::size_t m_CachePerPlayer;
    std::map<PlayerId, S_Player> m_Players;
};

#endif
