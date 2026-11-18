#include <winsock2.h>

#include "net/campaign_net.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <limits>
#include <map>
#include <mutex>

#include "net/dplay8.h"
#include "net/net_game.h"

namespace {

using namespace CampaignProtocol;
using R = E_CampaignReason;
using Clock = std::chrono::steady_clock;
constexpr auto kRejectGraceMs = 1000;
constexpr auto kMaxQueuedPackets = kMaxChunks + kFramesPerSecond;
constexpr auto kMaxQueuedBytes = kMaxWorldBytes + 1024 * 1024;
// DirectPlay client callbacks are bound to a single server connection.
constexpr std::uint32_t kServerConnection = 0;

struct S_Link {
    std::uint32_t m_Connection;
    int m_Slot;
    PlayerId m_Player;
};

enum class E_Event { Connected, Packet, Departed };
struct S_Event {
    E_Event m_Kind;
    S_Link m_Link;
    std::uint64_t m_ReceivedAt;
    T_Bytes m_Packet;
};

struct S_Peer {
    S_Peer(const S_Link &link, const S_CampaignNetConfig &config, bool host, std::uint64_t now)
        : m_Link(link), m_Admission(E_CampaignSessionKind::CampaignCoop, host, config.m_Hello,
                                  config.m_Session, link.m_Player, now)
    {
    }

    S_Link m_Link;
    C_Admission m_Admission;
    bool m_Admitted = false;
    std::uint64_t m_CloseAt = 0;
};

std::atomic<bool> g_enabled{false};
std::atomic<R> g_joinFailure{R::None};
// Protect the provider connection registry and queue. Admission, game callbacks,
// and DirectPlay calls execute on the game thread outside this lock.
std::mutex g_queueLock;
std::map<std::uint32_t, S_Link> g_links;
std::deque<S_Event> g_events;
std::size_t g_queuedPackets = 0, g_queuedBytes = 0;
std::uint64_t g_lastPlayer = 0;
bool g_host = false;
std::uint64_t g_startedAt = 0;
bool g_awaitingServer = false;
S_CampaignNetConfig g_config;
std::map<PlayerId, S_Peer> g_peers;

std::uint64_t NowMs()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count());
}

bool Send(C_NetGame &net, std::uint32_t connection, const T_Message &message)
{
    T_Bytes packet;
    if (EncodeCommandPacket(message, packet) != R::None)
        return false;
    DPN_BUFFER_DESC buffer{static_cast<DWORD>(packet.size()), packet.data()};
    DPNHANDLE operation = 0;
    // DirectPlay owns a copy of the bytes after Send returns.
    // Guaranteed delivery cannot be combined with DPNSEND_NOCOMPLETE.
    HRESULT result = E_FAIL;
    if (g_host && net.m_server)
        result = net.m_server->SendTo(connection, &buffer, 1, 0, nullptr, &operation, DPN_CAMPAIGN_SEND_GUARANTEED);
    else if (!g_host && net.m_client)
        result = net.m_client->Send(&buffer, 1, 0, nullptr, &operation, DPN_CAMPAIGN_SEND_GUARANTEED);
    if (FAILED(result))
        net.m_lastError = result;
    return SUCCEEDED(result);
}

PlayerId AdmittedPlayer(const S_Peer &peer)
{
    return peer.m_Admission.GetAgreement().m_Player;
}

void NotifyDeparture(S_Peer &peer, R reason)
{
    const auto player = peer.m_Admitted ? AdmittedPlayer(peer) : PlayerId{};
    peer.m_Admitted = false;
    peer.m_Admission.Close();
    if (g_config.m_Departed && (player.IsValid() || !g_host))
        g_config.m_Departed(player, reason);
}

void RetireSlot(C_NetGame &net, const S_Link &link)
{
    S_NetPlayerState *players = net.LockPlayers();
    if (link.m_Slot >= 0 && link.m_Slot < net.GetPlayerCount()) {
        auto &player = players[link.m_Slot];
        if (static_cast<std::uint32_t>(player.connection) == link.m_Connection)
            player = S_NetPlayerState{};
    }
    net.UnlockPlayers();
}

void Reject(C_NetGame &net, S_Peer &peer, R reason, std::uint64_t now, bool sendReject)
{
    if (sendReject)
        Send(net, peer.m_Link.m_Connection, S_Reject{reason});
    peer.m_CloseAt = now + kRejectGraceMs;
    NotifyDeparture(peer, reason);
}

void Receive(C_NetGame &net, S_Peer &peer, const T_Bytes &packet, std::uint64_t now)
{
    if (peer.m_CloseAt)
        return;
    T_Bytes frame;
    if (ExtractCommandFrame(packet.data(), packet.size(), frame) != R::None)
        return; // Native DirectPlay payloads never reach the legacy game handlers.
    auto received = peer.m_Admission.Receive(frame, now);
    const bool replied = !received.m_Reply || Send(net, peer.m_Link.m_Connection, *received.m_Reply);
    if (received.m_Reason != R::None || !replied) {
        Reject(net, peer, replied ? received.m_Reason : R::Disconnected, NowMs(), false);
        return;
    }
    if (!peer.m_Admitted && peer.m_Admission.GetState() == E_Admission::Admitted) {
        auto reason = g_config.m_Admitted
            ? g_config.m_Admitted(peer.m_Admission.GetAgreement(), peer.m_Link.m_Slot)
            : R::NotAdmitted;
        if (reason != R::None) {
            Reject(net, peer, reason, NowMs(), true);
            return;
        }
        peer.m_Admitted = true;
        if (!g_host)
            net.m_joinStatus = NET_JOIN_CONNECTED;
    }
    if (received.m_Payload && peer.m_Admitted && g_config.m_Received)
        g_config.m_Received(AdmittedPlayer(peer), std::move(*received.m_Payload));
}

} // namespace

bool BeginCampaignNet(const S_CampaignNetConfig &config, bool host)
{
    if (IsCampaignNet() || (host && (!config.m_Session.IsValid() || !config.m_Host.IsValid())))
        return false;
    const C_Admission probe(E_CampaignSessionKind::CampaignCoop, host, config.m_Hello,
                            config.m_Session, config.m_Host, NowMs());
    if (probe.GetState() == E_Admission::Rejected)
        return false;
    g_config = config;
    g_host = host;
    g_startedAt = NowMs();
    g_awaitingServer = !host;
    g_joinFailure.store(R::None);
    g_lastPlayer = host ? config.m_Host.Value() : 0;
    g_enabled.store(true);
    return true;
}

void EndCampaignNet()
{
    if (!g_enabled.exchange(false))
        return;
    {
        std::lock_guard<std::mutex> lock(g_queueLock);
        g_links.clear();
        g_events.clear();
        g_queuedPackets = g_queuedBytes = 0;
    }
    for (auto &entry : g_peers) {
        if (!entry.second.m_CloseAt)
            NotifyDeparture(entry.second, R::Disconnected);
    }
    g_peers.clear();
    g_config = {};
}

bool IsCampaignNet()
{
    return g_enabled.load();
}

bool QueueCampaignPeer(std::uint32_t connection, int slot)
{
    std::lock_guard<std::mutex> lock(g_queueLock);
    if (!IsCampaignNet() || g_links.size() >= (g_host ? kMaxParticipants - 1 : 1) ||
        g_events.size() >= kMaxQueuedPackets + 32 || g_links.count(connection) ||
        g_lastPlayer == std::numeric_limits<std::uint64_t>::max())
        return false;
    const S_Link link{connection, slot, PlayerId(++g_lastPlayer)};
    g_links.emplace(connection, link);
    g_events.push_back({E_Event::Connected, link, NowMs(), {}});
    return true;
}

void QueueCampaignJoinFailure(E_CampaignReason reason)
{
    g_joinFailure.store(reason);
}

void QueueCampaignDeparture(std::uint32_t connection)
{
    std::lock_guard<std::mutex> lock(g_queueLock);
    const auto entry = g_links.find(connection);
    if (entry == g_links.end())
        return;
    g_events.push_back({E_Event::Departed, entry->second, NowMs(), {}});
    g_links.erase(entry);
}

void QueueCampaignPacket(std::uint32_t connection, int slot, const void *data, std::size_t size)
{
    if (!data || size < kHeaderBytes + kCommandPacketOverhead || size > kMaxFrameBytes + kCommandPacketOverhead)
        return;
    std::lock_guard<std::mutex> lock(g_queueLock);
    const auto entry = g_links.find(connection);
    if (!IsCampaignNet() || entry == g_links.end() || entry->second.m_Slot != slot ||
        g_events.size() >= kMaxQueuedPackets + 32 || g_queuedPackets >= kMaxQueuedPackets ||
        size > kMaxQueuedBytes - g_queuedBytes)
        return;
    const auto *bytes = static_cast<const std::uint8_t *>(data);
    g_events.push_back({E_Event::Packet, entry->second, NowMs(), T_Bytes(bytes, bytes + size)});
    ++g_queuedPackets;
    g_queuedBytes += size;
}

void ProcessCampaignNet(C_NetGame &net)
{
    std::deque<S_Event> events;
    {
        std::lock_guard<std::mutex> lock(g_queueLock);
        events.swap(g_events);
        g_queuedPackets = g_queuedBytes = 0;
    }
    for (const auto &event : events) {
        const auto &link = event.m_Link;
        if (event.m_Kind == E_Event::Connected) {
            auto &peer = g_peers.try_emplace(link.m_Player, link, g_config, g_host, event.m_ReceivedAt).first->second;
            if (auto hello = peer.m_Admission.Start()) {
                g_awaitingServer = false;
                if (!Send(net, kServerConnection, *hello))
                    Reject(net, peer, R::Disconnected, NowMs(), false);
            }
            continue;
        }
        const auto entry = g_peers.find(link.m_Player);
        if (entry == g_peers.end())
            continue;
        auto &peer = entry->second;
        if (event.m_Kind == E_Event::Packet) {
            Receive(net, peer, event.m_Packet, event.m_ReceivedAt);
        } else {
            if (!peer.m_CloseAt)
                NotifyDeparture(peer, R::Disconnected);
            g_peers.erase(entry);
            if (g_host)
                RetireSlot(net, link);
            else
                net.Close(false);
        }
    }
    const auto now = NowMs();
    for (auto &entry : g_peers) {
        auto &peer = entry.second;
        if (peer.m_Admission.Tick(now) == R::Timeout)
            Reject(net, peer, R::Timeout, now, true);
        if (!peer.m_CloseAt || now < peer.m_CloseAt)
            continue;
        peer.m_CloseAt = std::numeric_limits<std::uint64_t>::max();
        if (g_host && net.m_server)
            net.m_server->DestroyClient(peer.m_Link.m_Connection, nullptr, 0, 0);
        else if (!g_host)
            net.Close(false);
    }
    if (g_awaitingServer) {
        auto reason = g_joinFailure.exchange(R::None);
        if (reason == R::None && now - g_startedAt >= kTimeoutMs)
            reason = R::Timeout;
        if (reason != R::None) {
            g_awaitingServer = false;
            if (g_config.m_Departed)
                g_config.m_Departed({}, reason);
            net.Close(false);
        }
    }
}

bool SendCampaignMessage(C_NetGame &net, PlayerId player, const T_Message &message)
{
    if (!IsCampaignNet())
        return false;
    for (const auto &entry : g_peers) {
        const auto &peer = entry.second;
        if (peer.m_Admitted && AdmittedPlayer(peer) == player)
            return Send(net, peer.m_Link.m_Connection, message);
    }
    return false;
}
