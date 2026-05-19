#pragma once
#include "JobQueue.h"
#include "Player.h"
#include "Protocol.h"

class Room : public JobQueue
{
public:
    void Enter(PlayerRef player);
    void Leave(PlayerRef player);
    void Broadcast(SendBufferRef sendBuffer, uint64 exceptPlayerId = 0);

    void HandleMove(PlayerRef player, CS_MOVE_PKT pkt);
    void HandleAnimState(PlayerRef player, CS_ANIM_STATE_PKT pkt);
    void HandleDamage(PlayerRef attacker, CS_DAMAGE_PKT pkt);
    void HandleChat(PlayerRef player, CS_CHAT_PKT pkt);

public:
    USE_LOCK;
    map<uint64, PlayerRef> _players;
};

using RoomRef = TSharedPtr<Room>;
