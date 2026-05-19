#pragma once

class GameSession;
using GameSessionRef = TSharedPtr<GameSession>;

class Room;
using RoomRef = TSharedPtr<Room>;

class Player : public RefCountable
{
public:
    uint64          playerId    = 0;
    wstring         name;
    float           x = 0, y = 0, z = 0;
    float           yaw = 0;
    float           vx = 0, vy = 0, vz = 0;
    uint8           animState   = 0;
    float           hp          = 100.f;

    RoomRef         room;
    GameSessionRef  ownerSession;
};

using PlayerRef = TSharedPtr<Player>;
