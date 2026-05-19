#include "pch.h"
#include "Room.h"
#include "GameSession.h"
#include "MakeSendBuffer.h"

void Room::Enter(PlayerRef player)
{
    // 입장한 플레이어에게 기존 플레이어 목록 전송
    {
        SC_ENTER_GAME_PKT enterPkt;
        enterPkt.myPlayerId = player->playerId;
        enterPkt.otherCount = static_cast<uint8>(_players.size());
        auto sb = MakeSendBuffer(enterPkt, PacketId::SC_ENTER_GAME);
        player->ownerSession->Send(sb);
    }

    // 기존 플레이어들에게 개별 스냅샷 전송
    for (auto& [id, other] : _players)
    {
        SC_PLAYER_ENTER_PKT snapPkt{};
        snapPkt.p.playerId = other->playerId;
        snapPkt.p.x = other->x; snapPkt.p.y = other->y; snapPkt.p.z = other->z;
        snapPkt.p.yaw = other->yaw;
        ::wcsncpy_s(snapPkt.p.name, other->name.c_str(), 16);
        auto sb = MakeSendBuffer(snapPkt, PacketId::SC_PLAYER_ENTER);
        player->ownerSession->Send(sb);
    }

    // 기존 모든 플레이어에게 새 플레이어 입장 알림
    {
        SC_PLAYER_ENTER_PKT newPkt{};
        newPkt.p.playerId = player->playerId;
        newPkt.p.x = player->x; newPkt.p.y = player->y; newPkt.p.z = player->z;
        newPkt.p.yaw = player->yaw;
        ::wcsncpy_s(newPkt.p.name, player->name.c_str(), 16);
        auto sb = MakeSendBuffer(newPkt, PacketId::SC_PLAYER_ENTER);
        Broadcast(sb, player->playerId);
    }

    _players[player->playerId] = player;
    player->room = TSharedPtr<Room>(this);
}

void Room::Leave(PlayerRef player)
{
    _players.erase(player->playerId);
    player->room = nullptr;

    SC_PLAYER_LEAVE_PKT leavePkt{};
    leavePkt.playerId = player->playerId;
    auto sb = MakeSendBuffer(leavePkt, PacketId::SC_PLAYER_LEAVE);
    Broadcast(sb);
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptPlayerId)
{
    for (auto& [id, player] : _players)
    {
        if (id == exceptPlayerId)
            continue;
        player->ownerSession->Send(sendBuffer);
    }
}

void Room::HandleMove(PlayerRef player, CS_MOVE_PKT pkt)
{
    player->x = pkt.x; player->y = pkt.y; player->z = pkt.z;
    player->yaw = pkt.yaw;
    player->vx = pkt.vx; player->vy = pkt.vy; player->vz = pkt.vz;

    SC_PLAYER_MOVE_PKT movePkt{};
    movePkt.playerId = player->playerId;
    movePkt.x = pkt.x; movePkt.y = pkt.y; movePkt.z = pkt.z;
    movePkt.yaw = pkt.yaw;
    movePkt.vx = pkt.vx; movePkt.vy = pkt.vy; movePkt.vz = pkt.vz;
    auto sb = MakeSendBuffer(movePkt, PacketId::SC_PLAYER_MOVE);
    Broadcast(sb, player->playerId);
}

void Room::HandleAnimState(PlayerRef player, CS_ANIM_STATE_PKT pkt)
{
    player->animState = pkt.state;

    SC_ANIM_STATE_PKT animPkt{};
    animPkt.playerId = player->playerId;
    animPkt.state = pkt.state;
    auto sb = MakeSendBuffer(animPkt, PacketId::SC_ANIM_STATE);
    Broadcast(sb, player->playerId);
}

void Room::HandleDamage(PlayerRef attacker, CS_DAMAGE_PKT pkt)
{
    auto it = _players.find(pkt.targetId);
    if (it == _players.end()) return;

    PlayerRef target = it->second;
    if (target->hp <= 0.f) return;

    target->hp = max(0.f, target->hp - pkt.amount);

    SC_HIT_PKT hitPkt{};
    hitPkt.attackerId = attacker->playerId;
    hitPkt.targetId   = target->playerId;
    hitPkt.amount     = pkt.amount;
    hitPkt.remainHp   = target->hp;
    auto sb = MakeSendBuffer(hitPkt, PacketId::SC_HIT);
    Broadcast(sb);

    cout << "[데미지] attacker=" << attacker->playerId
         << " target=" << target->playerId
         << " amount=" << pkt.amount
         << " remainHp=" << target->hp << endl;
}

void Room::HandleChat(PlayerRef player, CS_CHAT_PKT pkt)
{
    SC_CHAT_PKT chatPkt{};
    chatPkt.playerId = player->playerId;
    ::wcsncpy_s(chatPkt.msg, pkt.msg, 128);
    auto sb = MakeSendBuffer(chatPkt, PacketId::SC_CHAT);
    Broadcast(sb);
}
