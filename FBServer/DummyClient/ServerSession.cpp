#include "pch.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "MakeSendBuffer.h"

static Atomic<int32> SSessionIndex = 0;

extern void RegisterSession(ServerSessionRef s);
extern void UnregisterSession(ServerSessionRef s);

void ServerSession::OnConnected()
{
    int32 idx = SSessionIndex.fetch_add(1) + 1;
    myName = L"Dummy" + to_wstring(idx);
    cout << "[클라이언트] 서버 연결 성공 (Dummy" << idx << ")" << endl;

    RegisterSession(ServerSessionRef(this));

    CS_LOGIN_PKT loginPkt{};
    ::wcsncpy_s(loginPkt.name, myName.c_str(), 16);
    Send(MakeSendBuffer(loginPkt, PacketId::CS_LOGIN));
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    ServerPacketHandler::HandlePacket(ServerSessionRef(this), buffer, len);
}

void ServerSession::OnSend(int32 len)
{
}

void ServerSession::OnDisconnected()
{
    cout << "[클라이언트] 서버와 연결 끊김 (playerId=" << myPlayerId << ")" << endl;
    UnregisterSession(ServerSessionRef(this));
}
