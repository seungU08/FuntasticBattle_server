#pragma once

enum class ServiceType : uint8
{
    Server,
    Client,
};

class Service : public RefCountable
{
public:
    Service(ServiceType type, NetAddress netAddress, IocpCoreRef iocpCore, SessionFactory factory, int32 maxSessionCount = 1);
    virtual ~Service();

    virtual bool    Start()         = 0;
    bool            CanStart()      { return _sessionFactory != nullptr; }
    virtual void    CloseService();

    ServiceType     GetType()               { return _type; }
    NetAddress      GetNetAddress()         { return _netAddress; }
    IocpCoreRef     GetIocpCore()           { return _iocpCore; }
    int32           GetCurrentSessionCount(){ return _sessionCount; }
    int32           GetMaxSessionCount()    { return _maxSessionCount; }

    SessionRef      CreateSession();
    void            AddSession(SessionRef session);
    void            ReleaseSession(SessionRef session);

protected:
    USE_LOCK;

    ServiceType     _type;
    NetAddress      _netAddress;
    IocpCoreRef     _iocpCore;

    set<SessionRef>     _sessions;
    int32               _sessionCount    = 0;
    int32               _maxSessionCount = 0;
    SessionFactory      _sessionFactory;
};

using ServiceRef = TSharedPtr<Service>;

// ========================
//     ServerService
// ========================

class ServerService : public Service
{
public:
    ServerService(NetAddress netAddress, IocpCoreRef iocpCore, SessionFactory factory, int32 maxSessionCount);
    virtual ~ServerService();

    virtual bool    Start() override;
    virtual void    CloseService() override;

private:
    ListenerRef     _listener = nullptr;
};

using ServerServiceRef = TSharedPtr<ServerService>;

// ========================
//     ClientService
// ========================

class ClientService : public Service
{
public:
    ClientService(NetAddress netAddress, IocpCoreRef iocpCore, SessionFactory factory, int32 maxSessionCount);
    virtual ~ClientService() = default;

    virtual bool    Start() override;
};

using ClientServiceRef = TSharedPtr<ClientService>;
