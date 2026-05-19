#include "pch.h"
#include "ServerSession.h"
#include "MakeSendBuffer.h"

// 연결된 세션 목록 (ServerSession::OnConnected/OnDisconnected에서 관리)
Mutex           GSessionMutex;
vector<ServerSessionRef> GDummySessions;

void RegisterSession(ServerSessionRef s)
{
    lock_guard<Mutex> guard(GSessionMutex);
    GDummySessions.push_back(s);
}

void UnregisterSession(ServerSessionRef s)
{
    lock_guard<Mutex> guard(GSessionMutex);
    GDummySessions.erase(
        remove_if(GDummySessions.begin(), GDummySessions.end(),
            [&](const ServerSessionRef& r) { return r == s; }),
        GDummySessions.end());
}

int main(int argc, char* argv[])
{
    int32 sessionCount = 1;
    if (argc >= 2)
        sessionCount = atoi(argv[1]);
    sessionCount = max(1, min(sessionCount, 10));

    cout << "[DummyClient] " << sessionCount << "개 세션 연결 시작" << endl;

    SocketUtils::Init();

    ClientServiceRef service = MakeShared<ClientService>(
        NetAddress(L"127.0.0.1", 7777),
        IocpCoreRef(GIocpCore),
        []() -> SessionRef { return MakeShared<ServerSession>(); },
        sessionCount
    );

    ASSERT_CRASH(service->Start());

    // 워커 스레드
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

    // 메인 스레드: 100ms마다 방에 있는 세션들에게 CS_MOVE 전송
    int32 tick = 0;
    while (true)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        tick++;

        vector<ServerSessionRef> sessions;
        {
            lock_guard<Mutex> guard(GSessionMutex);
            sessions = GDummySessions;
        }

        for (auto& ss : sessions)
        {
            if (ss && ss->inRoom)
            {
                float t = tick * 0.1f;
                float offset = static_cast<float>(ss->myPlayerId);
                CS_MOVE_PKT movePkt{};
                movePkt.x   =  cosf(t + offset) * 200.f;
                movePkt.y   =  0.f;
                movePkt.z   =  sinf(t + offset) * 200.f;
                movePkt.yaw =  t * 57.3f;
                movePkt.vx  = -sinf(t + offset) * 200.f;
                movePkt.vy  =  0.f;
                movePkt.vz  =  cosf(t + offset) * 200.f;
                ss->Send(MakeSendBuffer(movePkt, PacketId::CS_MOVE));
            }
        }
    }

    GThreadManager->Join();
    SocketUtils::Clear();
}
