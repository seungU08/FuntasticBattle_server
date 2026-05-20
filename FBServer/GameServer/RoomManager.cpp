#include "pch.h"
#include "RoomManager.h"
#include "GameSessionManager.h"
#include "MakeSendBuffer.h"

RoomManager* GRoomManager = nullptr;

RoomManager::RoomManager()
{
    // 기본 방 제거: 클라이언트가 동적으로 생성
}

RoomRef RoomManager::GetRoom(uint32 roomId)
{
    READ_LOCK;
    auto it = _rooms.find(roomId);
    if (it == _rooms.end())
        return nullptr;
    return it->second;
}

RoomRef RoomManager::CreateRoom(const wstring& name)
{
    WRITE_LOCK;
    uint32 newId = _nextRoomId++;
    RoomRef room = MakeShared<Room>(newId, name);
    _rooms[newId] = room;
    return room;
}

void RoomManager::RemoveRoom(uint32 roomId)
{
    WRITE_LOCK;
    _rooms.erase(roomId);
}

SendBufferRef RoomManager::MakeRoomListPacket()
{
    SC_ROOM_LIST_PKT pkt{};
    pkt.h.size = sizeof(pkt);
    pkt.h.id   = static_cast<uint16>(PacketId::SC_ROOM_LIST);

    uint8 count = 0;
    {
        READ_LOCK;
        for (auto& [id, room] : _rooms)
        {
            if (count >= 10) break;
            // 꽉 찼거나 시작된 방은 제외
            if (room->IsFull() || room->IsStarted())
                continue;

            RoomEntry& entry = pkt.rooms[count++];
            entry.roomId      = room->GetRoomId();
            entry.playerCount = room->GetPlayerCount();
            entry.isStarted   = room->IsStarted() ? 1 : 0;
            ::wcsncpy_s(entry.name, room->GetName().c_str(), 32);
        }
    }
    pkt.count = count;

    SendBufferRef sb = MakeShared<SendBuffer>(sizeof(pkt));
    ::memcpy(sb->Buffer(), &pkt, sizeof(pkt));
    sb->Close(sizeof(pkt));
    return sb;
}

void RoomManager::BroadcastRoomList()
{
    SendBufferRef sb = MakeRoomListPacket();
    GSessionManager->BroadcastToAll(sb);
}
