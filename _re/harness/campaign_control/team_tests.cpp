// Compile the actual two team rule methods against instrumented game services.
#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <cstdlib>
static bool g_Local, g_Campaign, g_Server;
bool IsLocalGame() { return g_Local; }
bool CampaignControlsActive() { return g_Campaign; }
bool IsServer() { return g_Server; }
using E_HumanRole = int;
constexpr int HUMAN_ROLE_PLAYER=0, HUMAN_ROLE_ENEMY=1, HUMAN_ROLE_NPC_TEAM=3, HUMAN_ROLE_NPC_TEAM_5=5;
constexpr int TEAM_AXIS=1,TEAM_ALLIES=2,TEAM_NONE=3;
constexpr unsigned HUMAN_ENV_DEADLY=1, HUMAN_ENV_RESTRICTED=2;
constexpr int TEXT_SQUAD_RESTRICTED_AREA=99;
struct C_Human {
    bool m_Alive = false;
    unsigned m_Environment = 0;
    E_HumanRole m_role = HUMAN_ROLE_PLAYER;
    int m_netId = 17;
    int GetTeam(bool pending) const;
    void SetPlayerTeam(int team, bool pending);
    std::vector<C_Human *> m_Seen;
    bool IsAlive() const { return m_Alive; }
    unsigned GetEnvironmentFlags() const { return m_Environment; }
    int GetRole() const { return m_role; }
    const char *GetFullName() const { return "fixture"; }
    const std::vector<C_Human *> &GetSeenEnemies() const { return m_Seen; }
};
struct C_MissionDef { bool m_Decided=false; bool AreObjectivesDecided() const { return m_Decided; } };
struct C_Interface { int m_Message=0; void RemoveFromMap(C_Human *) {} void ShowMessage(int text) { m_Message=text; } };
struct C_Game {
    C_MissionDef m_Mission;
    C_Interface m_Interface;
    int m_Failures=0;
    bool m_Deadly=false;
    void FailMission(bool deadly) { ++m_Failures; m_Deadly=deadly; }
    void Print(const char *, const char *) {}
    C_MissionDef *GetMission() { return &m_Mission; }
    C_Interface *GetInterface() { return &m_Interface; }
} g_Game;
C_Game *GetGame() { return &g_Game; }
struct C_Team {
    std::vector<C_Human *> m_members;
    unsigned m_environment=0;
    struct C_Records { void Remove(C_Human *) {} } m_enemyRecords;
    bool IsMember(C_Human *h) { for (auto *v:m_members) if(v==h)return true;return false; }
    int GetMemberCount() const { return static_cast<int>(m_members.size()); }
    C_Human *GetMember(int i) { return m_members[i]; }
    C_Human *GetCommandedMember() { return nullptr; }
    void SetCommandedIndex(int) {}
    void OnMemberDied(C_Human *);
    void CheckEnvironment();
};
#include "team_source.inc"
struct C_Profile { bool m_Coop=false; bool IsCooperativeGame() { return m_Coop; } } g_gameProfile;
struct C_NetGame {
    int m_Reads=0,m_Writes=0;
    int GetPlayerTeam(int actor,bool) { ++m_Reads;return actor+100; }
    void SetPlayerTeam(int,int,bool) { ++m_Writes; }
} g_netGame;
#include "human_team_source.inc"
#define CHECK(x) do { ++checks; if (!(x)) { std::cerr << "FAIL team line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while(false)
int main() {
    int checks=0;
    for (int mode=0;mode<4;++mode) {
        g_Local=mode==0; g_Campaign=mode>=2; g_Server=mode!=3;
        const bool authority=mode==0||mode==2;
        g_Game={};
        g_netGame={};g_gameProfile.m_Coop=false;
        C_Human soldier;
        CHECK(soldier.GetTeam(false)==(mode==1?117:TEAM_ALLIES));
        soldier.SetPlayerTeam(1,false);
        CHECK(g_netGame.m_Writes==(g_Campaign?0:1));
        soldier.m_role=HUMAN_ROLE_ENEMY;
        CHECK(soldier.GetTeam(false)==(mode==1?117:TEAM_AXIS));
        if(g_Campaign)CHECK(g_netGame.m_Reads==0);
        C_Human dead, ai;
        C_Team team;team.m_members={&dead,&ai};
        ai.m_Alive=true; // an unowned squad AI keeps the campaign alive
        team.OnMemberDied(&dead);CHECK(g_Game.m_Failures==0);
        ai.m_Alive=false;
        team.OnMemberDied(&dead);CHECK(g_Game.m_Failures==(authority?1:0));
        g_Game={}; ai.m_Alive=true;ai.m_Environment=HUMAN_ENV_DEADLY;
        team.CheckEnvironment();CHECK(g_Game.m_Failures==(authority?1:0));
        if(authority)CHECK(g_Game.m_Deadly);
        g_Game={};team.m_environment=0;ai.m_Environment=HUMAN_ENV_RESTRICTED;
        team.CheckEnvironment();CHECK(g_Game.m_Interface.m_Message==(authority?TEXT_SQUAD_RESTRICTED_AREA:0));
        g_Game={};team.m_environment=0;g_Game.m_Mission.m_Decided=true;ai.m_Environment=HUMAN_ENV_DEADLY;
        team.CheckEnvironment();CHECK(g_Game.m_Failures==0);
    }
    std::cout << "PASS: actual-source team campaign rules: " << checks << " checks\n";
}
