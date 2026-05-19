#pragma once

class ServerService;

class Listener : public IocpObject
{
public:
    Listener() = default;
    ~Listener();

    bool    StartAccept(ServerService* service);
    void    CloseSocket();

public:
    virtual HANDLE  GetHandle() override;
    virtual void    Dispatch(IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
    void    RegisterAccept(AcceptEvent* acceptEvent);
    void    ProcessAccept(AcceptEvent* acceptEvent);

private:
    SOCKET                  _socket = INVALID_SOCKET;
    ServerService*          _service = nullptr;
    vector<AcceptEvent*>    _acceptEvents;
};

using ListenerRef = TSharedPtr<Listener>;
