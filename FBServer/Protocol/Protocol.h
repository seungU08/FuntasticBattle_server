#pragma once

// PacketHeader는 ServerCore/PacketSession.h에서 정의됨
// (서버 컨텍스트 외에서 단독 사용 시 아래 주석 해제)
// struct PacketHeader { unsigned short size; unsigned short id; };

#pragma pack(push, 1)

enum class PacketId : unsigned short
{
    // Client -> Server
    CS_LOGIN        = 1,
    CS_ENTER_ROOM   = 2,
    CS_LEAVE_ROOM   = 3,
    CS_MOVE         = 10,
    CS_ANIM_STATE   = 11,
    CS_DAMAGE       = 12,
    CS_CHAT         = 20,

    // Server -> Client
    SC_LOGIN_RES        = 1001,
    SC_ENTER_GAME       = 1002,
    SC_PLAYER_ENTER     = 1003,
    SC_PLAYER_LEAVE     = 1004,
    SC_PLAYER_MOVE      = 1010,
    SC_ANIM_STATE       = 1011,
    SC_HIT              = 1012,
    SC_CHAT             = 1020,
    SC_GAME_END         = 1030,
};

// ------- Client -> Server -------

struct CS_LOGIN_PKT
{
    PacketHeader h;
    wchar_t name[16];
};

struct CS_ENTER_ROOM_PKT
{
    PacketHeader h;
    uint32 roomId;
};

struct CS_LEAVE_ROOM_PKT
{
    PacketHeader h;
};

struct CS_MOVE_PKT
{
    PacketHeader h;
    float x, y, z;
    float yaw;
    float vx, vy, vz;
};

struct CS_ANIM_STATE_PKT
{
    PacketHeader h;
    uint8 state;
};

struct CS_DAMAGE_PKT
{
    PacketHeader h;
    uint64 targetId;
    float  amount;
};

struct CS_CHAT_PKT
{
    PacketHeader h;
    wchar_t msg[128];
};

// ------- Server -> Client -------

struct SC_LOGIN_RES_PKT
{
    PacketHeader h;
    uint64 playerId;
    uint8  success;
};

struct PlayerSnapshot
{
    uint64  playerId;
    float   x, y, z;
    float   yaw;
    wchar_t name[16];
};

struct SC_ENTER_GAME_PKT
{
    PacketHeader h;
    uint64 myPlayerId;
    uint8  otherCount;
    // PlayerSnapshot[otherCount] follows (variable length)
};

struct SC_PLAYER_ENTER_PKT
{
    PacketHeader   h;
    PlayerSnapshot p;
};

struct SC_PLAYER_LEAVE_PKT
{
    PacketHeader h;
    uint64 playerId;
};

struct SC_PLAYER_MOVE_PKT
{
    PacketHeader h;
    uint64 playerId;
    float  x, y, z;
    float  yaw;
    float  vx, vy, vz;
};

struct SC_ANIM_STATE_PKT
{
    PacketHeader h;
    uint64 playerId;
    uint8  state;
};

struct SC_HIT_PKT
{
    PacketHeader h;
    uint64 attackerId;
    uint64 targetId;
    float  amount;
    float  remainHp;
};

struct SC_GAME_END_PKT
{
    PacketHeader h;
    uint64 winnerId; // 0 이면 무승부
};

struct SC_CHAT_PKT
{
    PacketHeader h;
    uint64  playerId;
    wchar_t msg[128];
};

#pragma pack(pop)
