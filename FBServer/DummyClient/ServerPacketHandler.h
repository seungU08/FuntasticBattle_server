#pragma once
#include "Protocol.h"

class ServerSession;
using ServerSessionRef = TSharedPtr<ServerSession>;

class ServerPacketHandler
{
public:
    static void HandlePacket(ServerSessionRef session, BYTE* buffer, int32 len);

private:
    static void Handle_SC_LOGIN_RES(ServerSessionRef session, BYTE* buffer, int32 len);
    static void Handle_SC_ENTER_GAME(ServerSessionRef session, BYTE* buffer, int32 len);
    static void Handle_SC_PLAYER_ENTER(ServerSessionRef session, BYTE* buffer, int32 len);
    static void Handle_SC_PLAYER_LEAVE(ServerSessionRef session, BYTE* buffer, int32 len);
    static void Handle_SC_PLAYER_MOVE(ServerSessionRef session, BYTE* buffer, int32 len);
    static void Handle_SC_ANIM_STATE(ServerSessionRef session, BYTE* buffer, int32 len);
    static void Handle_SC_CHAT(ServerSessionRef session, BYTE* buffer, int32 len);
};
