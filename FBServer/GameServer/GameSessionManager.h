#pragma once
#include "GameSession.h"

class GameSessionManager
{
public:
    void Add(GameSessionRef session);
    void Remove(GameSessionRef session);
    int32 GetCount();
    void BroadcastToAll(SendBufferRef sb);

private:
    USE_LOCK;
    set<GameSessionRef> _sessions;
};

extern GameSessionManager* GSessionManager;
