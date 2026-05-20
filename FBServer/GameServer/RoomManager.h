#pragma once
#include "Room.h"

class RoomManager
{
public:
    RoomManager();

    RoomRef GetRoom(uint32 roomId);
    RoomRef CreateRoom(const wstring& name);
    void    RemoveRoom(uint32 roomId);

    void         BroadcastRoomList();
    SendBufferRef MakeRoomListPacket();

private:
    USE_LOCK;
    map<uint32, RoomRef> _rooms;
    uint32               _nextRoomId = 1;
};

extern RoomManager* GRoomManager;
