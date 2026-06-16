#pragma once
#include "JobQueue.h"
#include "Player.h"
#include "Protocol.h"
#include <atomic>

class Room : public JobQueue
{
public:
    static constexpr uint8 MAX_PLAYERS = 4;

    Room() = default;
    Room(uint32 roomId, const wstring& name) : _roomId(roomId), _name(name) {}

    void Enter(PlayerRef player);
    void Leave(PlayerRef player);
    void Broadcast(SendBufferRef sendBuffer, uint64 exceptPlayerId = 0);

    void HandleMove(PlayerRef player, CS_MOVE_PKT pkt);
    void HandleAnimState(PlayerRef player, CS_ANIM_STATE_PKT pkt);
    void HandleDamage(PlayerRef attacker, CS_DAMAGE_PKT pkt);
    void HandleItemState(PlayerRef player, CS_ITEM_STATE_PKT pkt);
    void HandleThrowBomb(PlayerRef player, CS_THROW_BOMB_PKT pkt);
    void HandleChat(PlayerRef player, CS_CHAT_PKT pkt);
    void HandleItemDrop(PlayerRef player, CS_ITEM_DROP_PKT pkt);
    void HandleItemPickup(PlayerRef player, CS_ITEM_PICKUP_PKT pkt);
    void HandleStartGame(PlayerRef player);
    void HandleCharCustomize(PlayerRef player, CS_CHAR_CUSTOMIZE_PKT pkt);

    // getter
    uint32        GetRoomId()      const { return _roomId; }
    const wstring& GetName()       const { return _name; }
    uint8         GetPlayerCount() const { return _playerCount.load(); }
    bool          IsStarted()      const { return _isStarted.load(); }
    bool          IsFull()         const { return _playerCount.load() >= MAX_PLAYERS; }

private:
    void CheckGameEnd();

    struct DropPos { float x, y; };
    vector<DropPos> _recentDropPositions;

    uint64              _ownerId     = 0;
    uint32              _roomId      = 0;
    wstring             _name;
    std::atomic<uint8>  _playerCount { 0 };
    std::atomic<bool>   _isStarted   { false };

public:
    USE_LOCK;
    map<uint64, PlayerRef> _players;
};

using RoomRef = TSharedPtr<Room>;
