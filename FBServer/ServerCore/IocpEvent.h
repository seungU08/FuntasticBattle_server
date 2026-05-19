#pragma once

enum class EventType : uint8
{
    Connect,
    Disconnect,
    Accept,
    Recv,
    Send,
};

/*----------------------------
        IocpEvent (base)
  WSAOVERLAPPED을 상속해서
  GetQueuedCompletionStatus의
  OVERLAPPED* 를 직접 캐스팅
-----------------------------*/
struct IocpEvent : public WSAOVERLAPPED
{
    IocpEvent(EventType type) : eventType(type)
    {
        Init();
    }

    void Init()
    {
        Internal      = 0;
        InternalHigh  = 0;
        Offset        = 0;
        OffsetHigh    = 0;
        hEvent        = 0;
    }

    EventType       eventType;
    IocpObjectRef   owner;  // 이벤트가 완료될 때까지 객체를 살려두는 레퍼런스
};

struct ConnectEvent : IocpEvent
{
    ConnectEvent() : IocpEvent(EventType::Connect) {}
};

struct DisconnectEvent : IocpEvent
{
    DisconnectEvent() : IocpEvent(EventType::Disconnect) {}
};

struct AcceptEvent : IocpEvent
{
    AcceptEvent() : IocpEvent(EventType::Accept) {}

    SOCKET  clientSocket = INVALID_SOCKET;
    // AcceptEx용 주소 버퍼 (로컬 + 원격 각각 SOCKADDR_IN + 16바이트)
    BYTE    acceptBuffer[(sizeof(SOCKADDR_IN) + 16) * 2] = {};
};

struct RecvEvent : IocpEvent
{
    RecvEvent() : IocpEvent(EventType::Recv) {}
};

struct SendEvent : IocpEvent
{
    SendEvent() : IocpEvent(EventType::Send) {}

    // WSASend 중에 버퍼가 해제되지 않도록 ref 유지
    vector<TSharedPtr<class SendBuffer>> sendBuffers;
};
