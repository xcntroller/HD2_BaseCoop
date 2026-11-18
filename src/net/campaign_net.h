#ifndef HD2_CAMPAIGN_NET_H
#define HD2_CAMPAIGN_NET_H

#include "net/campaign_protocol.h"

#include <functional>

class C_NetGame;

enum class E_CampaignLanRoute { DevelopmentMission, CampaignSession };

// The launch adapter owns these callbacks and installs them before opening a
// campaign connection. The game thread invokes them serially. An admission
// callback commits the player before returning success. For guest callbacks,
// PlayerId denotes the local participant assigned by the host.
struct S_CampaignNetConfig {
    E_CampaignLanRoute m_Route = E_CampaignLanRoute::DevelopmentMission;
    CampaignProtocol::S_Hello m_Hello;
    SessionId m_Session;
    PlayerId m_Host;
    std::function<E_CampaignReason(const CampaignProtocol::S_Accept &, int)> m_Admitted;
    std::function<void(PlayerId, CampaignProtocol::T_Message)> m_Received;
    std::function<void(PlayerId, E_CampaignReason)> m_Departed;
};

bool BeginCampaignNet(const S_CampaignNetConfig &config, bool host);
void EndCampaignNet();
bool IsCampaignNet();
bool IsCampaignNet(E_CampaignLanRoute route); // game thread
bool QueueCampaignPeer(std::uint32_t connection, int slot);
void QueueCampaignJoinFailure(E_CampaignReason reason);
void QueueCampaignDeparture(std::uint32_t connection);
void QueueCampaignPacket(std::uint32_t connection, int slot, const void *data, std::size_t size);
void ProcessCampaignNet(C_NetGame &net);
bool SendCampaignMessage(C_NetGame &net, PlayerId player, const CampaignProtocol::T_Message &message);

#endif
