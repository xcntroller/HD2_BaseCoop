#pragma once
#include <array>
#include <deque>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstddef>
constexpr int HUMAN_CONTROLLER_REMOTE = 6, HUMAN_CONTROLLER_PLAYER = 0,
    HUMAN_CONTROLLER_SQUAD = 1, HUMAN_CONTROLLER_AI = 5;
using E_HumanController = int;
struct C_HumanController {
    int m_Type = HUMAN_CONTROLLER_REMOTE;
    float *m_axes = nullptr, *m_buttons = nullptr;
    unsigned m_Bits = 1;
    std::vector<int> m_Server, m_Clients;
    int GetType() const { return m_Type; }
    void ClearMessagesToServer() { m_Server.clear(); }
    void ClearMessagesToClients() { m_Clients.clear(); }
    void SetControlBits(unsigned bits) { m_Bits = bits; }
};
struct C_PlayerController : C_HumanController {
    std::array<float, 64> m_controlAxes{}, m_controlButtons{};
};
struct C_RemoteController : C_HumanController {
    std::array<float, 64> m_controlAxes{}, m_controlButtons{};
};
class C_Human {
public:
    int m_Id = 0, m_Health = 711, m_Equipment = 42, m_Skills = 83, m_Rebinds = 0;
    bool m_Forced = false;
    struct C_Body { bool m_Local = false; void SetPlayerControlled(bool local) { m_Local = local; } } m_body;
    C_HumanController *m_controller = nullptr;
    int GetNetId() const { return m_Id; }
    void CreateController(E_HumanController type, bool force) {
        ++m_Rebinds; m_Forced = force; m_controller->m_Type = type;
    }
};
struct C_ActorsModule {
    static constexpr int PLAYER_SLOTS = 32;
    C_Human *m_Human = nullptr;
    int m_Use = 0;
    C_Human *GetHuman(int id) { return m_Human && m_Human->m_Id == id ? m_Human : nullptr; }
    int GetPlayerSlotUse(int) const { return m_Use; }
};
inline C_ActorsModule g_actorsModule;
struct C_NetObjectMessage {
    int refCount = 0;
    static inline int m_Destroyed = 0;
    virtual ~C_NetObjectMessage() { ++m_Destroyed; }
};
struct C_NetObject {
    std::deque<C_NetObjectMessage *> m_In, m_Out;
    void *m_StateContext = nullptr, *m_FactoryContext = nullptr;
    C_NetObjectMessage *PopMessage(bool out) {
        auto &queue = out ? m_Out : m_In;
        if (queue.empty()) return nullptr;
        auto *message = queue.front(); queue.pop_front(); --message->refCount; return message;
    }
    template<class T, class U> void SetStateHandlers(T, U, void *context) { m_StateContext = context; }
    template<class T> void SetMessageFactory(T, void *context) { m_FactoryContext = context; }
};
struct C_NetReplication {
    C_NetObject *m_Object = nullptr;
    C_NetObject *FindObject(int, bool) { return m_Object; }
};
inline C_NetReplication g_Replication;
inline C_NetReplication *g_netReplication = &g_Replication;
inline void NetReadControlBits() {}
inline void NetWriteControlBits() {}
inline void NetCreateMessage() {}
struct C_Team {
    C_Human *m_Member = nullptr, *m_Selected = nullptr;
    int m_CameraRefreshes = 0;
    bool IsMember(C_Human *human) { return m_Member == human; }
    void SelectCampaignMember(C_Human *human) { m_Selected = human; }
    void UpdateCameraTarget() { ++m_CameraRefreshes; }
};
struct C_TeamManager {
    C_Team m_Team;
    C_Team *GetPlayerTeam() { return &m_Team; }
};
inline C_TeamManager g_teamManager;
struct C_Interface {
    bool m_Inventory = true;
    int m_Refreshes = 0;
    void ShowInventory(bool shown, void *) { m_Inventory = shown; }
    void RefreshInventory() { ++m_Refreshes; }
};
struct C_FirstPersonView { int m_Refreshes = 0; void Refresh() { ++m_Refreshes; } };
struct C_game {
    C_Interface m_Interface;
    C_FirstPersonView m_View;
    C_Interface *GetInterface() { return &m_Interface; }
    C_FirstPersonView *GetFirstPersonView() { return &m_View; }
};
inline C_game g_Game;
inline C_game *GetGame() { return &g_Game; }
