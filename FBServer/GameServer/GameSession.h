#pragma once
#include "PacketSession.h"

class Player;
using PlayerRef = TSharedPtr<Player>;

class GameSession : public PacketSession
{
public:
    ~GameSession();

    virtual void OnConnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override;
    virtual void OnDisconnected() override;

public:
    PlayerRef _player;
};

using GameSessionRef = TSharedPtr<GameSession>;
