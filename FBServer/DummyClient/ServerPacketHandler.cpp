#include "pch.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "MakeSendBuffer.h"

void ServerPacketHandler::HandlePacket(ServerSessionRef session, BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    PacketId packetId = static_cast<PacketId>(header->id);

    switch (packetId)
    {
    case PacketId::SC_LOGIN_RES:    Handle_SC_LOGIN_RES(session, buffer, len);    break;
    case PacketId::SC_ENTER_GAME:   Handle_SC_ENTER_GAME(session, buffer, len);   break;
    case PacketId::SC_PLAYER_ENTER: Handle_SC_PLAYER_ENTER(session, buffer, len); break;
    case PacketId::SC_PLAYER_LEAVE: Handle_SC_PLAYER_LEAVE(session, buffer, len); break;
    case PacketId::SC_PLAYER_MOVE:  Handle_SC_PLAYER_MOVE(session, buffer, len);  break;
    case PacketId::SC_ANIM_STATE:   Handle_SC_ANIM_STATE(session, buffer, len);   break;
    case PacketId::SC_CHAT:         Handle_SC_CHAT(session, buffer, len);         break;
    default:
        cout << "[클라이언트] 알 수 없는 패킷 ID: " << header->id << endl;
        break;
    }
}

void ServerPacketHandler::Handle_SC_LOGIN_RES(ServerSessionRef session, BYTE* buffer, int32 len)
{
    SC_LOGIN_RES_PKT* pkt = reinterpret_cast<SC_LOGIN_RES_PKT*>(buffer);
    if (pkt->success)
    {
        session->myPlayerId = pkt->playerId;
        session->loggedIn   = true;
        cout << "[로그인 성공] playerId=" << pkt->playerId << endl;

        CS_ENTER_ROOM_PKT enterPkt{};
        enterPkt.roomId = 1;
        session->Send(MakeSendBuffer(enterPkt, PacketId::CS_ENTER_ROOM));
    }
}

void ServerPacketHandler::Handle_SC_ENTER_GAME(ServerSessionRef session, BYTE* buffer, int32 len)
{
    SC_ENTER_GAME_PKT* pkt = reinterpret_cast<SC_ENTER_GAME_PKT*>(buffer);
    session->inRoom = true;
    cout << "[방 입장] playerId=" << session->myPlayerId
         << " 기존 플레이어 " << (int)pkt->otherCount << "명" << endl;
}

void ServerPacketHandler::Handle_SC_PLAYER_ENTER(ServerSessionRef session, BYTE* buffer, int32 len)
{
    SC_PLAYER_ENTER_PKT* pkt = reinterpret_cast<SC_PLAYER_ENTER_PKT*>(buffer);
    cout << "[플레이어 입장] id=" << pkt->p.playerId << endl;
}

void ServerPacketHandler::Handle_SC_PLAYER_LEAVE(ServerSessionRef session, BYTE* buffer, int32 len)
{
    SC_PLAYER_LEAVE_PKT* pkt = reinterpret_cast<SC_PLAYER_LEAVE_PKT*>(buffer);
    cout << "[플레이어 퇴장] id=" << pkt->playerId << endl;
}

void ServerPacketHandler::Handle_SC_PLAYER_MOVE(ServerSessionRef session, BYTE* buffer, int32 len)
{
    SC_PLAYER_MOVE_PKT* pkt = reinterpret_cast<SC_PLAYER_MOVE_PKT*>(buffer);
    cout << "[" << session->myPlayerId << "] SC_PLAYER_MOVE from " << pkt->playerId
         << " pos=(" << pkt->x << "," << pkt->y << "," << pkt->z << ")" << endl;
}

void ServerPacketHandler::Handle_SC_ANIM_STATE(ServerSessionRef session, BYTE* buffer, int32 len)
{
    SC_ANIM_STATE_PKT* pkt = reinterpret_cast<SC_ANIM_STATE_PKT*>(buffer);
    cout << "[" << session->myPlayerId << "] SC_ANIM from " << pkt->playerId
         << " state=" << (int)pkt->state << endl;
}

void ServerPacketHandler::Handle_SC_CHAT(ServerSessionRef session, BYTE* buffer, int32 len)
{
    SC_CHAT_PKT* pkt = reinterpret_cast<SC_CHAT_PKT*>(buffer);
    cout << "[채팅] id=" << pkt->playerId << endl;
}
