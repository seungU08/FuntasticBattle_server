#pragma once
#include "Room.h"

class RoomManager
{
public:
    RoomManager();

    RoomRef GetRoom(uint32 roomId);

private:
    USE_LOCK;
    map<uint32, RoomRef> _rooms;
};

extern RoomManager* GRoomManager;
