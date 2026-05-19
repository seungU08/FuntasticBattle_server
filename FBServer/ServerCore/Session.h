#pragma once

class Service;

class Session : public IocpObject
{
    friend class Listener;
    friend class IocpCore;
    friend class Service;

    enum { BUFFER_SIZE = 0x10000 }; // 64KB

public:
    Session();
    virtual ~Session();

    // 외부 호출
    void        Send(SendBufferRef sendBuffer);
    bool        Connect();
    void        Disconnect(const WCHAR* cause);

    // 서비스 연결
    void        SetService(Service* service)    { _service = service; }
    Service*    GetService()                    { return _service; }

    void        SetNetAddress(NetAddress addr)  { _netAddress = addr; }
    NetAddress  GetAddress()                    { return _netAddress; }
    SOCKET      GetSocket()                     { return _socket; }
    bool        IsConnected()                   { return _connected; }

public:
    // 컨텐츠 코드에서 오버라이드
    virtual void    OnConnected()                       {}
    virtual int32   OnRecv(BYTE* buffer, int32 len)     { return len; }
    virtual void    OnSend(int32 len)                   {}
    virtual void    OnDisconnected()                    {}

private:
    virtual HANDLE  GetHandle() override;
    virtual void    Dispatch(IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

    // 비동기 I/O 등록
    bool    RegisterConnect();
    bool    RegisterDisconnect();
    void    RegisterRecv();
    void    RegisterSend();

    // 완료 처리
    void    ProcessConnect();
    void    ProcessDisconnect();
    void    ProcessRecv(int32 numOfBytes);
    void    ProcessSend(int32 numOfBytes);

    void    HandleError(int32 errorCode);

private:
    Service*        _service    = nullptr;
    SOCKET          _socket     = INVALID_SOCKET;
    NetAddress      _netAddress = {};
    Atomic<bool>    _connected  = false;

private:
    USE_LOCK;

    // Recv
    RecvBuffer      _recvBuffer;

    // Send
    bool                    _sendRegistered = false;
    queue<SendBufferRef>    _sendQueue;

private:
    ConnectEvent    _connectEvent;
    DisconnectEvent _disconnectEvent;
    RecvEvent       _recvEvent;
    SendEvent       _sendEvent;
};

using SessionRef = TSharedPtr<Session>;
using SessionFactory = function<SessionRef(void)>;
