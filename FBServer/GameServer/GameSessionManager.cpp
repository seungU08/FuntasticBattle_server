#include "pch.h"
#include "GameSessionManager.h"

GameSessionManager* GSessionManager = nullptr;

void GameSessionManager::Add(GameSessionRef session)
{
    WRITE_LOCK;
    _sessions.insert(session);
}

void GameSessionManager::Remove(GameSessionRef session)
{
    WRITE_LOCK;
    _sessions.erase(session);
}

int32 GameSessionManager::GetCount()
{
    READ_LOCK;
    return static_cast<int32>(_sessions.size());
}

void GameSessionManager::BroadcastToAll(SendBufferRef sb)
{
    READ_LOCK;
    for (GameSessionRef session : _sessions)
        session->Send(sb);
}
