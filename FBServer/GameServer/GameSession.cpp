#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "Room.h"

GameSession::~GameSession()
{
    cout << "~GameSession" << endl;
}

void GameSession::OnConnected()
{
    cout << "[서버] 클라이언트 접속" << endl;
    GSessionManager->Add(GameSessionRef(this));
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    ClientPacketHandler::HandlePacket(GameSessionRef(this), buffer, len);
}

void GameSession::OnSend(int32 len)
{
}

void GameSession::OnDisconnected()
{
    cout << "[서버] 클라이언트 종료" << endl;

    // 방에 있었다면 퇴장 처리
    if (_player && _player->room)
    {
        PlayerRef player = _player;
        RoomRef room = _player->room;
        room->Push(MakeShared<Job>([room, player]() mutable {
            room->Leave(player);
        }));
    }

    GSessionManager->Remove(GameSessionRef(this));
}
