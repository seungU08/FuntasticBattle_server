#include "pch.h"
#include "Session.h"
#include "Service.h"

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
}

Session::~Session()
{
    if (_socket != INVALID_SOCKET)
        ::closesocket(_socket);
}

HANDLE Session::GetHandle()
{
    return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
    switch (iocpEvent->eventType)
    {
    case EventType::Connect:
        ProcessConnect();
        break;
    case EventType::Disconnect:
        ProcessDisconnect();
        break;
    case EventType::Recv:
        ProcessRecv(numOfBytes);
        break;
    case EventType::Send:
        ProcessSend(numOfBytes);
        break;
    default:
        break;
    }
}

// =====================
//  외부 인터페이스
// =====================

void Session::Send(SendBufferRef sendBuffer)
{
    if (!_connected)
        return;

    bool registerSend = false;
    {
        WRITE_LOCK;
        _sendQueue.push(sendBuffer);
        if (!_sendRegistered)
        {
            _sendRegistered = true;
            registerSend    = true;
        }
    }

    if (registerSend)
        RegisterSend();
}

bool Session::Connect()
{
    return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
    // 이미 끊어진 경우 무시
    if (_connected.exchange(false) == false)
        return;

    cout << "Disconnect : " << string(cause, cause + wcslen(cause)) << endl;
    RegisterDisconnect();
}

// =====================
//  비동기 I/O 등록
// =====================

bool Session::RegisterConnect()
{
    if (IsConnected())
        return false;

    if (_service == nullptr || _service->GetType() != ServiceType::Client)
        return false;

    _socket = SocketUtils::CreateSocket();
    if (_socket == INVALID_SOCKET)
        return false;

    SocketUtils::SetReuseAddress(_socket, true);

    if (!SocketUtils::BindAnyAddress(_socket, 0))
        return false;

    // ConnectEx 전에 소켓을 IOCP에 등록
    _service->GetIocpCore()->Register(IocpObjectRef(this));

    _connectEvent.Init();
    _connectEvent.owner = IocpObjectRef(this);

    DWORD numOfBytes = 0;
    SOCKADDR_IN sockAddr = _service->GetNetAddress().GetSockAddr();

    if (!SocketUtils::ConnectEx(
        _socket,
        reinterpret_cast<SOCKADDR*>(&sockAddr),
        sizeof(sockAddr),
        nullptr,
        0,
        OUT &numOfBytes,
        &_connectEvent))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            _connectEvent.owner = nullptr;
            return false;
        }
    }

    return true;
}

bool Session::RegisterDisconnect()
{
    _disconnectEvent.Init();
    _disconnectEvent.owner = IocpObjectRef(this);

    if (!SocketUtils::DisconnectEx(
        _socket,
        &_disconnectEvent,
        TF_REUSE_SOCKET,
        0))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            _disconnectEvent.owner = nullptr;
            return false;
        }
    }

    return true;
}

void Session::RegisterRecv()
{
    if (!_connected)
        return;

    _recvEvent.Init();
    _recvEvent.owner = IocpObjectRef(this);

    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
    wsaBuf.len = _recvBuffer.FreeSize();

    DWORD numOfBytes = 0;
    DWORD flags      = 0;

    if (SOCKET_ERROR == ::WSARecv(
        _socket,
        &wsaBuf, 1,
        OUT &numOfBytes,
        OUT &flags,
        &_recvEvent,
        nullptr))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            HandleError(errorCode);
            _recvEvent.owner = nullptr;
        }
    }
}

void Session::RegisterSend()
{
    if (!_connected)
    {
        WRITE_LOCK;
        _sendRegistered = false;
        return;
    }

    _sendEvent.Init();
    _sendEvent.owner = IocpObjectRef(this);

    // 큐에 쌓인 버퍼를 모두 꺼냄
    {
        WRITE_LOCK;
        while (!_sendQueue.empty())
        {
            _sendEvent.sendBuffers.push_back(_sendQueue.front());
            _sendQueue.pop();
        }
    }

    // WSABUF 배열 구성
    vector<WSABUF> wsaBufs;
    wsaBufs.reserve(_sendEvent.sendBuffers.size());
    for (auto& sendBuffer : _sendEvent.sendBuffers)
        wsaBufs.push_back({ sendBuffer->WriteSize(), reinterpret_cast<char*>(sendBuffer->Buffer()) });

    DWORD numOfBytes = 0;
    if (SOCKET_ERROR == ::WSASend(
        _socket,
        wsaBufs.data(),
        static_cast<DWORD>(wsaBufs.size()),
        OUT &numOfBytes,
        0,
        &_sendEvent,
        nullptr))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            HandleError(errorCode);
            _sendEvent.owner = nullptr;
            _sendEvent.sendBuffers.clear();
            WRITE_LOCK;
            _sendRegistered = false;
        }
    }
}

// =====================
//  완료 처리
// =====================

void Session::ProcessConnect()
{
    _connectEvent.owner = nullptr;

    // ConnectEx 완료 후 필수 설정
    ::setsockopt(_socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);

    _connected.store(true);

    // IOCP 등록은 RegisterConnect()에서 이미 완료됨 — 여기서 하면 중복
    OnConnected();
    RegisterRecv();
}

void Session::ProcessDisconnect()
{
    _disconnectEvent.owner = nullptr;

    OnDisconnected();

    if (_service)
        _service->ReleaseSession(SessionRef(this));
}

void Session::ProcessRecv(int32 numOfBytes)
{
    _recvEvent.owner = nullptr;

    if (numOfBytes == 0)
    {
        Disconnect(L"Recv 0");
        return;
    }

    if (!_recvBuffer.OnWrite(numOfBytes))
    {
        Disconnect(L"OnWrite overflow");
        return;
    }

    int32 dataSize   = _recvBuffer.DataSize();
    int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);

    if (processLen < 0 || dataSize < processLen || !_recvBuffer.OnRead(processLen))
    {
        Disconnect(L"OnRead overflow");
        return;
    }

    _recvBuffer.Clean();
    RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
    _sendEvent.owner = nullptr;
    _sendEvent.sendBuffers.clear();

    if (numOfBytes == 0)
    {
        Disconnect(L"Send 0");
        return;
    }

    OnSend(numOfBytes);

    bool registerSend = false;
    {
        WRITE_LOCK;
        if (_sendQueue.empty())
            _sendRegistered = false;
        else
            registerSend = true;
    }

    if (registerSend)
        RegisterSend();
}

void Session::HandleError(int32 errorCode)
{
    switch (errorCode)
    {
    case WSAECONNRESET:
    case WSAECONNABORTED:
        Disconnect(L"HandleError");
        break;
    default:
        cout << "HandleError : " << errorCode << endl;
        break;
    }
}
