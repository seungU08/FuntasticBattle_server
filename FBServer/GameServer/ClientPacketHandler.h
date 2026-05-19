#pragma once
#include "Protocol.h"

class GameSession;
using GameSessionRef = TSharedPtr<GameSession>;

class ClientPacketHandler
{
public:
    static void HandlePacket(GameSessionRef session, BYTE* buffer, int32 len);

private:
    static void Handle_CS_LOGIN(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_ENTER_ROOM(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_LEAVE_ROOM(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_MOVE(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_ANIM_STATE(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_DAMAGE(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_ITEM_STATE(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_THROW_BOMB(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_CHAT(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_ITEM_DROP(GameSessionRef session, BYTE* buffer, int32 len);
    static void Handle_CS_ITEM_PICKUP(GameSessionRef session, BYTE* buffer, int32 len);
};
