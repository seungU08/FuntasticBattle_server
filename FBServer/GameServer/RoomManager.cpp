#include "pch.h"
#include "RoomManager.h"

RoomManager* GRoomManager = nullptr;

RoomManager::RoomManager()
{
    // 기본 방 1개 (roomId = 1)
    _rooms[1] = MakeShared<Room>();
}

RoomRef RoomManager::GetRoom(uint32 roomId)
{
    READ_LOCK;
    auto it = _rooms.find(roomId);
    if (it == _rooms.end())
        return nullptr;
    return it->second;
}
