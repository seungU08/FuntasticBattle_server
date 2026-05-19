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
    if (_players.find(player->playerId) == _players.end())
        return;

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
    if (!target->alive) return;

    target->hp = max(0.f, target->hp - pkt.amount);

    SC_HIT_PKT hitPkt{};
    hitPkt.attackerId = attacker->playerId;
    hitPkt.targetId   = target->playerId;
    hitPkt.amount     = pkt.amount;
    hitPkt.remainHp   = target->hp;
    Broadcast(MakeSendBuffer(hitPkt, PacketId::SC_HIT));

    cout << "[데미지] attacker=" << attacker->playerId
         << " target=" << target->playerId
         << " remainHp=" << target->hp << endl;

    if (target->hp <= 0.f)
    {
        target->alive = false;
        cout << "[사망] playerId=" << target->playerId << endl;
        CheckGameEnd();
    }
}

void Room::CheckGameEnd()
{
    if (_players.size() < 2) return;

    PlayerRef winner = nullptr;
    int32 aliveCount = 0;
    for (auto& [id, player] : _players)
    {
        if (player->alive)
        {
            aliveCount++;
            winner = player;
        }
    }

    if (aliveCount > 1) return;

    SC_GAME_END_PKT endPkt{};
    endPkt.winnerId = (aliveCount == 1) ? winner->playerId : 0;
    Broadcast(MakeSendBuffer(endPkt, PacketId::SC_GAME_END));

    if (aliveCount == 1)
        cout << "[게임종료] 승자 playerId=" << endPkt.winnerId << endl;
    else
        cout << "[게임종료] 무승부" << endl;

    // 다음 라운드를 위해 모든 플레이어 HP/alive 초기화
    for (auto& [id, player] : _players)
    {
        player->hp    = 100.f;
        player->alive = true;
    }

    _recentDropPositions.clear();
}

void Room::HandleItemState(PlayerRef player, CS_ITEM_STATE_PKT pkt)
{
    SC_ITEM_STATE_PKT out{};
    out.playerId = player->playerId;
    out.itemType = pkt.itemType;
    Broadcast(MakeSendBuffer(out, PacketId::SC_ITEM_STATE), player->playerId);
}

void Room::HandleThrowBomb(PlayerRef player, CS_THROW_BOMB_PKT pkt)
{
    SC_THROW_BOMB_PKT out{};
    out.playerId = player->playerId;
    out.x = pkt.x; out.y = pkt.y; out.z = pkt.z;
    out.yaw   = pkt.yaw;
    out.pitch = pkt.pitch;
    Broadcast(MakeSendBuffer(out, PacketId::SC_THROW_BOMB), player->playerId);
}

void Room::HandleItemDrop(PlayerRef player, CS_ITEM_DROP_PKT pkt)
{
    // 반경 100 이내에 이미 처리된 드롭 위치가 있으면 중복 무시
    for (auto& pos : _recentDropPositions)
    {
        float dx = pos.x - pkt.x;
        float dy = pos.y - pkt.y;
        if (dx*dx + dy*dy < 100.f*100.f) return;
    }
    _recentDropPositions.push_back({pkt.x, pkt.y});

    SC_ITEM_DROP_PKT out{};
    out.itemType = pkt.itemType;
    out.x = pkt.x; out.y = pkt.y; out.z = pkt.z;
    Broadcast(MakeSendBuffer(out, PacketId::SC_ITEM_DROP));
}

void Room::HandleChat(PlayerRef player, CS_CHAT_PKT pkt)
{
    SC_CHAT_PKT chatPkt{};
    chatPkt.playerId = player->playerId;
    ::wcsncpy_s(chatPkt.msg, pkt.msg, 128);
    auto sb = MakeSendBuffer(chatPkt, PacketId::SC_CHAT);
    Broadcast(sb);
}
