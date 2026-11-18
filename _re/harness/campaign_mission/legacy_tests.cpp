#include "../../../src/game/campaign_mission_adapter.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <cstring>
#include <stdexcept>
#ifdef _MSC_VER
static_assert(sizeof(void *)==4 && _M_IX86,"x86");
#endif
namespace {
int checks=0;
#define CHECK(x) do {++checks;if(!(x)) throw std::runtime_error(#x);}while(false)
using R=E_CampaignReason;
constexpr int STAT_COUNT=12, STAT_HEALTH=0, STAT_LOCKPICK=8;
constexpr uint8_t SQUAD_SLOT_HEAD=1,SQUAD_SLOT_UNIFORM=2,SQUAD_SLOT_HANDS=4,SQUAD_SLOT_BACKPACK=8,
    SQUAD_SLOT_LONG_GUN=16,SQUAD_SLOT_BELT=32,SQUAD_SLOT_BACKPACK_CONTENTS=64;
struct S_SoldierItem {int16_t uid=-1,typeId=-1,present=0;};
struct S_SquadSoldier {
    int profileIndex=-1,progress=31;
    bool deployed=false,spectating=false,vip=false;
    S_SoldierItem head,uniform,hands,backpack,longGun;
    std::vector<S_SoldierItem> belt,backpackContents;
    void ClearItems() {head={};uniform={};hands={};backpack={};longGun={};belt.clear();backpackContents.clear();}
    void SplitStacks() {
        for(auto *v:{&belt,&backpackContents}) {
            std::vector<S_SoldierItem> split;
            for(auto i:*v) for(int n=0;n<i.present;++n) {auto entry=i;entry.present=1;split.push_back(entry);}
            *v=std::move(split);
        }
    }
    void UpdateLoad() {}
};
struct S_CampaignNativeCatalog {
    std::map<ItemCatalogKey,int> m_Items;
    std::map<PlacementKey,uint8_t> m_Placements;
    std::map<SkillKey,int> m_Skills;
    std::map<SoldierId,S_SquadSoldier> m_Soldiers;
};
struct S_CampaignNativeSpawn {SoldierId m_Soldier;S_SquadSoldier m_State;std::array<int,STAT_COUNT> m_Stats;};
struct S_vector {float x=0,y=0,z=0;S_vector NormalizedGame()const{float len=std::sqrt(x*x+y*y+z*z);return {x/len,y/len,z/len};}};
struct S_vector4 {float x,y,z,w;};
struct S_matrix {float m[4][4]{};S_vector operator()(int n)const{return {m[n][0],m[n][1],m[n][2]};}};
struct PointScript {int uses=0;void OnUse(int){++uses;}};
struct S_FrameData {PointScript *script=nullptr;};
struct PointFrame {S_matrix world;S_FrameData data;const S_matrix &GetMatrix(){return world;}const S_FrameData *GetData(){return &data;}};
struct C_Human {
    int starts=0;C_Human *GetController(){return this;}
    void StartMission(bool){++starts;}
};
struct Team {
    C_Human *watched=nullptr;
    int removed=0;std::vector<C_Human *> members;
    bool IsMember(C_Human *h){return std::find(members.begin(),members.end(),h)!=members.end();}
    void AddMember(C_Human *h){members.push_back(h);}
    void RemoveMember(C_Human *) {++removed;}
    C_Human *GetWatchedHuman(){return watched;}
    void SetWatchedHuman(C_Human *h){watched=h;}
};
using C_TeamBase=Team;
struct Teams {Team player,coop;Team *GetPlayerTeam(){return &player;}Team *GetCoopTeam(){return &coop;}} g_teamManager;
struct Interface {
    C_Human *shown=nullptr;
    C_Human *GetShownSquadMember(){return shown;}
    void ShowSquadMember(C_Human *h){shown=h;}
};
struct S_NetStats {int values[9]{};int &operator[](int i){return values[i];}operator int *(){return values;}};
struct S_NetPlayerState {int connection=0;char name[32]{};bool left=false;C_Human *human=nullptr;S_NetStats stats;bool IsConnected(){return connection!=0;}};
struct Net {
    S_NetPlayerState players[4];int locks=0,process=0;
    S_NetPlayerState *LockPlayers(){++locks;return players;}
    void UnlockPlayers(){--locks;}
    int GetPlayerCount(){return 4;}
    C_Human *GetLocalHuman(){return players[0].human;}
    void Process(int,bool){++process;}
} g_netGame;
struct Temporary {int destroyed=0;void DestroyMinesOf(C_Human *){++destroyed;}} g_temporaryActors;
struct Actors {int destroyed=0,freeSlot=23;int FindFreePlayerSlot(){return freeSlot;}void DestroyActor(C_Human *,bool){++destroyed;}} g_actorsModule;
struct Mp {int coopDifficulty=2;bool select=true;int maps=0;bool SelectMap(const char *){++maps;return select;}} mp,*g_mpSettings=&mp;
struct Player {int GetDifficulty(){return 4;}} fixturePlayer;
bool g_local=true,g_server=true,g_client=true,g_coop=false;
bool IsLocalGame(){return g_local;}bool IsServer(){return g_server;}bool IsClient(){return g_client;}
constexpr int MULTIPLAYER_DIFFICULTY=3;
struct C_GameProfile {Player *m_player=&fixturePlayer;Mp *m_mpSettings=&mp;bool IsCooperativeGame()const{return g_coop;}int GetDifficulty()const;} g_gameProfile;
struct State {int pending=-1;void SetPendingState(int s){pending=s;}};
struct App {State state;State *m_state=&state;int sends=0;void SendGameMessage(int,int,int,int,bool){++sends;}} app;
App *GetGameApp(){return &app;}
int wins[3]{};int &TeamWins(int t){return wins[t];}
constexpr int GAME_MSG_SETTINGS=1,GAME_MSG_SESSION=2,GAME_STATE_NEXT_MAP=3;
struct C_game {
    Interface face;Interface *m_interface=&face;int m_vipSlot=-1,m_spectatedSlot=-1;
    void OnPlayerLeft(int connection);
    static void SendToPlayers(int){}
};
struct Hooks {
    std::function<std::optional<PlayerId>(int)> m_FindTransportPlayer;
    std::function<R(PlayerId)> m_Disconnected;
} g_Hooks;
struct C_SpawnPoint {
    PointFrame *m_frame=nullptr;int m_npc=0;bool m_free=true;C_Human *m_spawned=nullptr;
    C_Human *SpawnCampaign(S_SquadSoldier &state,const int *stats);
    void NotifyCampaignSpawned();
    static inline C_Human created;
    static inline bool npc=false;static inline int slot=0,actor=0,health=0;static inline S_vector4 position{},direction{};
    static C_Human *CreateSoldier(int s,int a,const S_vector4 &p,const S_vector4 &d,S_SquadSoldier *,const int *stats,int uniform,bool isNpc){
        CHECK(uniform==-1);npc=isNpc;slot=s;actor=a;position=p;direction=d;health=stats[0];created={};return &created;
    }
};
#include "legacy_bodies.inc"
struct View:I_CampaignOwnershipView {
    S_CampaignOwnershipSnapshot state;
    T_CampaignOwnershipSnapshot GetSnapshot()const override{return std::make_shared<const S_CampaignOwnershipSnapshot>(state);}
};
void TestDormantLegacy() {
    InstallCampaignMissionView(nullptr);SetCampaignMissionAdapter(nullptr);
    CHECK(g_gameProfile.GetDifficulty()==4);
    g_local=false;g_coop=false;CHECK(g_gameProfile.GetDifficulty()==3);
    g_coop=true;CHECK(g_gameProfile.GetDifficulty()==2);
    CHECK(GetPlayerTeamForMode()==g_teamManager.GetCoopTeam());g_coop=false;
    CHECK(GetPlayerTeamForMode()==g_teamManager.GetPlayerTeam());
    for(int role=0;role<3;++role) {
        g_local=role==0;g_coop=role==1;g_server=true;
        C_Human h;C_game game;auto &p=g_netGame.players[2];p={};p.connection=88;p.human=&h;p.stats[0]=77;
        game.m_vipSlot=2;std::memcpy(p.name,"fixture",8);
        const int actors=g_actorsModule.destroyed,mines=g_temporaryActors.destroyed;
        game.OnPlayerLeft(88);
        CHECK(p.connection==0);CHECK(p.name[0]==0);CHECK(p.left);CHECK(p.human==nullptr);CHECK(p.stats[0]==0);
        CHECK(g_actorsModule.destroyed==actors+1);CHECK(g_temporaryActors.destroyed==mines+1);CHECK(g_netGame.locks==0);
    }
    int before=mp.maps;ChangeMap("original");CHECK(mp.maps==before+1);CHECK(app.state.pending==GAME_STATE_NEXT_MAP);
}
void TestActiveLegacyGuards() {
    View view;view.state.m_Context.m_Kind=E_CampaignSessionKind::CampaignCoop;
    view.state.m_Context.m_UsesCampaignRules=true;view.state.m_Context.m_Rules.m_Difficulty=1;
    InstallCampaignMissionView(&view);
    CHECK(g_gameProfile.GetDifficulty()==1);g_coop=true;CHECK(GetPlayerTeamForMode()==g_teamManager.GetPlayerTeam());
    C_Human h;C_game game;auto &p=g_netGame.players[3];p={};p.connection=77;p.human=&h;p.stats[0]=65;
    const int actors=g_actorsModule.destroyed,mines=g_temporaryActors.destroyed;
    int callbacks=0;
    g_Hooks.m_FindTransportPlayer=[&](int slot)->std::optional<PlayerId>{CHECK(slot==3);return PlayerId(123);};
    g_Hooks.m_Disconnected=[&](PlayerId exact){CHECK(exact==PlayerId(123));CHECK(p.connection==77);CHECK(p.human==&h);CHECK(p.stats[0]==65);CHECK(g_netGame.locks==0);++callbacks;return R::None;};
    game.OnPlayerLeft(77);CHECK(callbacks==1);CHECK(p.connection==0);CHECK(p.human==nullptr);CHECK(p.stats[0]==0);
    game.OnPlayerLeft(77);game.OnPlayerLeft(0);CHECK(callbacks==1);
    CHECK(g_actorsModule.destroyed==actors);CHECK(g_temporaryActors.destroyed==mines);
    int before=mp.maps;ChangeMap("wrong");CHECK(mp.maps==before);
    InstallCampaignMissionView(nullptr);g_netGame.players[3]={};g_Hooks={};
}
void TestOriginalPointSpawn() {
    View view;view.state.m_Context.m_Kind=E_CampaignSessionKind::CampaignCoop;
    view.state.m_Context.m_UsesCampaignRules=true;view.state.m_Context.m_IsTransportServer=true;
    InstallCampaignMissionView(&view);g_server=true;g_client=true;
    PointFrame frame;frame.world.m[3][0]=10;frame.world.m[3][1]=20;frame.world.m[3][2]=30;frame.world.m[2][2]=2;
    PointScript script;frame.data.script=&script;C_SpawnPoint point;point.m_frame=&frame;
    S_SquadSoldier state;std::array<int,STAT_COUNT> stats{};stats[0]=640;
    CHECK(point.SpawnCampaign(state,stats.data())==&C_SpawnPoint::created);
    CHECK(C_SpawnPoint::slot==-1);CHECK(!C_SpawnPoint::npc);CHECK(C_SpawnPoint::actor==23);CHECK(C_SpawnPoint::health==640);
    CHECK(C_SpawnPoint::position.x==10);CHECK(C_SpawnPoint::position.y==20);CHECK(C_SpawnPoint::position.z==30);
    CHECK(C_SpawnPoint::direction.z==1);CHECK(!point.m_free);CHECK(point.m_spawned->starts==2);
    CHECK(g_teamManager.GetPlayerTeam()->IsMember(point.m_spawned));CHECK(script.uses==0);
    point.NotifyCampaignSpawned();CHECK(script.uses==1);
    CHECK(point.SpawnCampaign(state,stats.data())==nullptr);
    point.m_free=true;point.m_npc=1;CHECK(point.SpawnCampaign(state,stats.data())!=nullptr);CHECK(C_SpawnPoint::npc);
    InstallCampaignMissionView(nullptr);point.m_free=true;CHECK(point.SpawnCampaign(state,stats.data())==nullptr);CHECK(point.m_free);
}
void TestConversion() {
    C_CampaignSession session({SessionId(7),CampaignId(2),MissionKey("original"),{},true},{17,0});
    S_CampaignMissionManifest manifest;S_CampaignNativeCatalog catalog;
    catalog.m_Items={{ItemCatalogKey("uniform"),12},{ItemCatalogKey("ammo"),33}};
    catalog.m_Placements={{PlacementKey("uniform"),SQUAD_SLOT_UNIFORM},{PlacementKey("belt"),SQUAD_SLOT_BELT}};
    for(int n=0;n<9;++n)catalog.m_Skills.emplace(SkillKey(std::to_string(n)),n);
    for(auto s:{SoldierId(2),SoldierId(1)}) {
        S_SquadSoldier seed;seed.profileIndex=*session.FindProfile(s);seed.uniform.typeId=99;catalog.m_Soldiers.emplace(s,seed);
        S_CampaignSoldierLoadout load;load.m_Soldier=s;
        for(int n=0;n<9;++n)load.m_Skills.push_back({SkillKey(std::to_string(n)),n==0?640.0f:73.75f});
        load.m_Items={
            {PreparationItemId(1),{ItemCatalogKey("uniform"),1,{},{},{},{}},{PlacementKey("uniform"),0}},
            {PreparationItemId(2),{ItemCatalogKey("ammo"),3,{},{},{},{}},{PlacementKey("belt"),4}}};
        manifest.m_Loadouts.push_back(load);
    }
    std::vector<S_CampaignNativeSpawn> spawns;CHECK(BuildCampaignNativeSpawns(manifest,session,catalog,spawns)==R::None);
    CHECK(spawns.size()==2);CHECK(spawns[0].m_Soldier==SoldierId(2));CHECK(spawns[0].m_State.profileIndex==0);
    CHECK(spawns[0].m_State.uniform.typeId==12);CHECK(spawns[0].m_State.uniform.uid==-1);CHECK(spawns[0].m_State.progress==31);
    CHECK(spawns[0].m_Stats[0]==640);CHECK(spawns[0].m_Stats[4]==73);CHECK(spawns[0].m_State.belt.size()==3);
    for(auto i:spawns[0].m_State.belt){CHECK(i.typeId==33);CHECK(i.present==1);CHECK(i.uid==-1);}
    for(int field=0;field<5;++field) {
        auto bad=manifest;
        switch(field){
        case 0:bad.m_Loadouts[0].m_Items[0].m_State.m_Item=ItemCatalogKey("unknown");break;
        case 1:bad.m_Loadouts[0].m_Skills.pop_back();break;
        case 2:bad.m_Loadouts[0].m_Items[0].m_State.m_Quantity=32768;break;
        case 3:bad.m_Loadouts[0].m_Items[0].m_State.m_Rounds=7;break;
        case 4:bad.m_Loadouts[0].m_Items[0].m_Placement.m_Position=2;break;
        }
        CHECK(BuildCampaignNativeSpawns(bad,session,catalog,spawns)!=R::None);CHECK(spawns[0].m_State.uniform.typeId==12);CHECK(spawns.size()==2);
    }
    CHECK(catalog.m_Soldiers.at(SoldierId(2)).uniform.typeId==99);
}
}
int main(){try{TestDormantLegacy();TestActiveLegacyGuards();TestOriginalPointSpawn();TestConversion();}catch(const std::exception &e){std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}std::cout<<"PASS: 4 native source regression suites / "<<checks<<" checks\n";}
