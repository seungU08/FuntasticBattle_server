#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

// ========================
//       Service
// ========================

Service::Service(ServiceType type, NetAddress netAddress, IocpCoreRef iocpCore, SessionFactory factory, int32 maxSessionCount)
    : _type(type), _netAddress(netAddress), _iocpCore(iocpCore), _sessionFactory(factory), _maxSessionCount(maxSessionCount)
{
}

Service::~Service()
{
}

void Service::CloseService()
{
    // TODO: 세션 전체 종료
}

SessionRef Service::CreateSession()
{
    SessionRef session = _sessionFactory();
    session->SetService(this);
    AddSession(session);
    return session;
}

void Service::AddSession(SessionRef session)
{
    WRITE_LOCK;
    _sessionCount++;
    _sessions.insert(session);
}

void Service::ReleaseSession(SessionRef session)
{
    WRITE_LOCK;
    if (_sessions.erase(session))
        _sessionCount--;
}

// ========================
//     ServerService
// ========================

ServerService::ServerService(NetAddress netAddress, IocpCoreRef iocpCore, SessionFactory factory, int32 maxSessionCount)
    : Service(ServiceType::Server, netAddress, iocpCore, factory, maxSessionCount)
{
}

ServerService::~ServerService()
{
    CloseService();
}

bool ServerService::Start()
{
    if (!CanStart())
        return false;

    _listener = MakeShared<Listener>();
    if (!_listener->StartAccept(this))
        return false;

    cout << "[ServerService] 서버 시작 Port : " << _netAddress.GetPort() << endl;
    return true;
}

void ServerService::CloseService()
{
    if (_listener)
        _listener->CloseSocket();

    Service::CloseService();
}

// ========================
//     ClientService
// ========================

ClientService::ClientService(NetAddress netAddress, IocpCoreRef iocpCore, SessionFactory factory, int32 maxSessionCount)
    : Service(ServiceType::Client, netAddress, iocpCore, factory, maxSessionCount)
{
}

bool ClientService::Start()
{
    if (!CanStart())
        return false;

    // maxSessionCount 개수만큼 동시 연결 시도
    // IOCP 등록 및 ConnectEx는 Session::RegisterConnect() 내부에서 처리
    for (int32 i = 0; i < _maxSessionCount; i++)
    {
        SessionRef session = CreateSession();
        if (!session->Connect())
        {
            ReleaseSession(session);
            return false;
        }
    }

    return true;
}
