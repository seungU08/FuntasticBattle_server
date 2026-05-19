#include "pch.h"
#include "Listener.h"
#include "Service.h"

Listener::~Listener()
{
    CloseSocket();

    for (AcceptEvent* acceptEvent : _acceptEvents)
    {
        // 대기 중인 이벤트의 owner ref 해제
        acceptEvent->owner = nullptr;
        delete acceptEvent;
    }
}

HANDLE Listener::GetHandle()
{
    return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
    ASSERT_CRASH(iocpEvent->eventType == EventType::Accept);
    AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
    ProcessAccept(acceptEvent);
}

bool Listener::StartAccept(ServerService* service)
{
    _service = service;

    if (_service == nullptr)
        return false;

    _socket = SocketUtils::CreateSocket();
    if (_socket == INVALID_SOCKET)
        return false;

    // 리스너 소켓을 IOCP에 등록
    IocpObjectRef listenerRef(this);
    if (!_service->GetIocpCore()->Register(listenerRef))
        return false;

    SocketUtils::SetLinger(_socket, 0, 0);
    SocketUtils::SetReuseAddress(_socket, true);

    // 바인드 + 리슨
    if (!SocketUtils::BindAnyAddress(_socket, _service->GetNetAddress().GetPort()))
        return false;

    if (!SocketUtils::Listen(_socket))
        return false;

    // AcceptEx 사전 등록 (4개)
    const int32 acceptCount = 4;
    for (int32 i = 0; i < acceptCount; i++)
    {
        AcceptEvent* acceptEvent = new AcceptEvent();
        _acceptEvents.push_back(acceptEvent);
        RegisterAccept(acceptEvent);
    }

    return true;
}

void Listener::CloseSocket()
{
    if (_socket != INVALID_SOCKET)
    {
        ::closesocket(_socket);
        _socket = INVALID_SOCKET;
    }
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
    acceptEvent->Init();
    acceptEvent->owner        = IocpObjectRef(this);
    acceptEvent->clientSocket = SocketUtils::CreateSocket();

    DWORD bytesReceived = 0;

    if (!SocketUtils::AcceptEx(
        _socket,
        acceptEvent->clientSocket,
        acceptEvent->acceptBuffer,
        0,
        sizeof(SOCKADDR_IN) + 16,
        sizeof(SOCKADDR_IN) + 16,
        OUT &bytesReceived,
        static_cast<LPOVERLAPPED>(acceptEvent)))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            // AcceptEx 실패 시 소켓 정리 후 재시도
            ::closesocket(acceptEvent->clientSocket);
            acceptEvent->clientSocket = INVALID_SOCKET;
            acceptEvent->owner        = nullptr;
            RegisterAccept(acceptEvent);
        }
    }
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
    acceptEvent->owner = nullptr;

    // 클라이언트 소켓이 유효한지 확인
    if (acceptEvent->clientSocket == INVALID_SOCKET)
    {
        RegisterAccept(acceptEvent);
        return;
    }

    // SO_UPDATE_ACCEPT_CONTEXT 설정 (필수)
    if (!SocketUtils::SetUpdateAcceptSocket(acceptEvent->clientSocket, _socket))
    {
        ::closesocket(acceptEvent->clientSocket);
        acceptEvent->clientSocket = INVALID_SOCKET;
        RegisterAccept(acceptEvent);
        return;
    }

    // 원격 주소 확인
    SOCKADDR_IN sockAddr = {};
    int32 addrLen = sizeof(sockAddr);
    if (SOCKET_ERROR == ::getpeername(
        acceptEvent->clientSocket,
        reinterpret_cast<SOCKADDR*>(&sockAddr),
        &addrLen))
    {
        ::closesocket(acceptEvent->clientSocket);
        acceptEvent->clientSocket = INVALID_SOCKET;
        RegisterAccept(acceptEvent);
        return;
    }

    NetAddress netAddress(sockAddr);

    // 세션 생성 및 설정
    SessionRef session = _service->CreateSession();
    session->SetNetAddress(netAddress);
    session->_socket = acceptEvent->clientSocket;
    acceptEvent->clientSocket = INVALID_SOCKET; // 소켓 소유권을 세션에 넘김

    // 세션 소켓을 IOCP에 등록
    if (_service->GetIocpCore()->Register(IocpObjectRef(session)))
    {
        session->_connected.store(true);
        session->OnConnected();
        session->RegisterRecv();
    }
    else
    {
        _service->ReleaseSession(session);
    }

    // 다음 Accept 사전 등록
    RegisterAccept(acceptEvent);
}
