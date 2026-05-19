#include "pch.h"
#include "ClientPacketHandler.h"
#include "GameSession.h"
#include "Player.h"
#include "Room.h"
#include "RoomManager.h"
#include "MakeSendBuffer.h"

static Atomic<uint64> SNextPlayerId = 1;

void ClientPacketHandler::HandlePacket(GameSessionRef session, BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    PacketId packetId = static_cast<PacketId>(header->id);

    switch (packetId)
    {
    case PacketId::CS_LOGIN:        Handle_CS_LOGIN(session, buffer, len);       break;
    case PacketId::CS_ENTER_ROOM:   Handle_CS_ENTER_ROOM(session, buffer, len);  break;
    case PacketId::CS_LEAVE_ROOM:   Handle_CS_LEAVE_ROOM(session, buffer, len);  break;
    case PacketId::CS_MOVE:         Handle_CS_MOVE(session, buffer, len);        break;
    case PacketId::CS_ANIM_STATE:   Handle_CS_ANIM_STATE(session, buffer, len);  break;
    case PacketId::CS_CHAT:         Handle_CS_CHAT(session, buffer, len);        break;
    default:
        cout << "[서버] 알 수 없는 패킷 ID: " << header->id << endl;
        break;
    }
}

void ClientPacketHandler::Handle_CS_LOGIN(GameSessionRef session, BYTE* buffer, int32 len)
{
    CS_LOGIN_PKT* pkt = reinterpret_cast<CS_LOGIN_PKT*>(buffer);

    PlayerRef player = MakeShared<Player>();
    player->playerId = SNextPlayerId.fetch_add(1);
    player->name = wstring(pkt->name, wcsnlen(pkt->name, 16));
    player->ownerSession = session;
    session->_player = player;

    cout << "[로그인] playerId=" << player->playerId << endl;

    SC_LOGIN_RES_PKT res{};
    res.playerId = player->playerId;
    res.success  = 1;
    session->Send(MakeSendBuffer(res, PacketId::SC_LOGIN_RES));
}

void ClientPacketHandler::Handle_CS_ENTER_ROOM(GameSessionRef session, BYTE* buffer, int32 len)
{
    CS_ENTER_ROOM_PKT* pkt = reinterpret_cast<CS_ENTER_ROOM_PKT*>(buffer);

    if (session->_player == nullptr)
        return;

    RoomRef room = GRoomManager->GetRoom(pkt->roomId);
    if (room == nullptr)
        return;

    PlayerRef player = session->_player;
    room->Push(MakeShared<Job>([room, player]() mutable {
        room->Enter(player);
    }));
}

void ClientPacketHandler::Handle_CS_LEAVE_ROOM(GameSessionRef session, BYTE* buffer, int32 len)
{
    if (session->_player == nullptr)
        return;

    PlayerRef player = session->_player;
    RoomRef room = player->room;
    if (room == nullptr)
        return;

    room->Push(MakeShared<Job>([room, player]() mutable {
        room->Leave(player);
    }));
}

void ClientPacketHandler::Handle_CS_MOVE(GameSessionRef session, BYTE* buffer, int32 len)
{
    CS_MOVE_PKT* pkt = reinterpret_cast<CS_MOVE_PKT*>(buffer);

    if (session->_player == nullptr)
        return;

    PlayerRef player = session->_player;
    RoomRef room = player->room;
    if (room == nullptr)
        return;

    CS_MOVE_PKT pktCopy = *pkt;
    room->Push(MakeShared<Job>([room, player, pktCopy]() mutable {
        room->HandleMove(player, pktCopy);
    }));
}

void ClientPacketHandler::Handle_CS_ANIM_STATE(GameSessionRef session, BYTE* buffer, int32 len)
{
    CS_ANIM_STATE_PKT* pkt = reinterpret_cast<CS_ANIM_STATE_PKT*>(buffer);

    if (session->_player == nullptr)
        return;

    PlayerRef player = session->_player;
    RoomRef room = player->room;
    if (room == nullptr)
        return;

    CS_ANIM_STATE_PKT pktCopy = *pkt;
    room->Push(MakeShared<Job>([room, player, pktCopy]() mutable {
        room->HandleAnimState(player, pktCopy);
    }));
}

void ClientPacketHandler::Handle_CS_CHAT(GameSessionRef session, BYTE* buffer, int32 len)
{
    CS_CHAT_PKT* pkt = reinterpret_cast<CS_CHAT_PKT*>(buffer);

    if (session->_player == nullptr)
        return;

    PlayerRef player = session->_player;
    RoomRef room = player->room;
    if (room == nullptr)
        return;

    CS_CHAT_PKT pktCopy = *pkt;
    room->Push(MakeShared<Job>([room, player, pktCopy]() mutable {
        room->HandleChat(player, pktCopy);
    }));
}
