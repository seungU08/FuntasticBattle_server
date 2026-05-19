#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "RoomManager.h"

int main()
{
    GSessionManager = new GameSessionManager();
    GRoomManager    = new RoomManager();

    SocketUtils::Init();

    ServerServiceRef service = MakeShared<ServerService>(
        NetAddress(L"0.0.0.0", 7777),
        IocpCoreRef(GIocpCore),
        []() -> SessionRef { return MakeShared<GameSession>(); },
        100
    );

    ASSERT_CRASH(service->Start());

    int32 workerCount = max(1, static_cast<int32>(thread::hardware_concurrency()));
    for (int32 i = 0; i < workerCount; i++)
    {
        GThreadManager->Launch([=]()
        {
            while (true)
            {
                GIocpCore->Dispatch();
            }
        });
    }

    cout << "[GameServer] 포트 7777 대기 중..." << endl;

    GThreadManager->Join();
    SocketUtils::Clear();

    delete GRoomManager;
    delete GSessionManager;
}
