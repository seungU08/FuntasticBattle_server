#pragma once
#include "PacketSession.h"
#include "Protocol.h"

class ServerSession : public PacketSession
{
public:
    virtual void OnConnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override;
    virtual void OnDisconnected() override;

public:
    uint64      myPlayerId  = 0;
    wstring     myName;
    bool        loggedIn    = false;
    bool        inRoom      = false;
};

using ServerSessionRef = TSharedPtr<ServerSession>;
