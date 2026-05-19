#include "pch.h"
#include "SocketUtils.h"

LPFN_CONNECTEX      SocketUtils::ConnectEx    = nullptr;
LPFN_DISCONNECTEX   SocketUtils::DisconnectEx = nullptr;
LPFN_ACCEPTEX       SocketUtils::AcceptEx     = nullptr;

void SocketUtils::Init()
{
    WSADATA wsaData;
    ASSERT_CRASH(::WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);

    SOCKET dummySocket = CreateSocket();
    ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_CONNECTEX,   ConnectEx));
    ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_DISCONNECTEX, DisconnectEx));
    ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_ACCEPTEX,    AcceptEx));
    ::closesocket(dummySocket);
}

void SocketUtils::Clear()
{
    ::WSACleanup();
}

bool SocketUtils::SetLinger(SOCKET socket, uint16 onoff, uint16 linger)
{
    LINGER opt = { onoff, linger };
    return setsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&opt), sizeof(opt)) == 0;
}

bool SocketUtils::SetReuseAddress(SOCKET socket, bool flag)
{
    return SetSocketOpt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketUtils::SetRecvBufferSize(SOCKET socket, int32 size)
{
    return setsockopt(socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&size), sizeof(size)) == 0;
}

bool SocketUtils::SetSendBufferSize(SOCKET socket, int32 size)
{
    return setsockopt(socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&size), sizeof(size)) == 0;
}

bool SocketUtils::SetTcpNoDelay(SOCKET socket, bool flag)
{
    return SetSocketOpt(socket, IPPROTO_TCP, TCP_NODELAY, flag);
}

bool SocketUtils::SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket)
{
    return setsockopt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        reinterpret_cast<char*>(&listenSocket), sizeof(listenSocket)) == 0;
}

SOCKET SocketUtils::CreateSocket()
{
    return ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
}

bool SocketUtils::BindAnyAddress(SOCKET socket, uint16 port)
{
    SOCKADDR_IN myAddress = {};
    myAddress.sin_family      = AF_INET;
    myAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddress.sin_port        = htons(port);
    return ::bind(socket, reinterpret_cast<SOCKADDR*>(&myAddress), sizeof(myAddress)) != SOCKET_ERROR;
}

bool SocketUtils::Listen(SOCKET socket, int32 backlog)
{
    return ::listen(socket, backlog) != SOCKET_ERROR;
}

bool SocketUtils::Connect(SOCKET socket, NetAddress netAddress)
{
    SOCKADDR_IN sockAddr = netAddress.GetSockAddr();
    return ::connect(socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr)) != SOCKET_ERROR;
}

template<typename T>
bool SocketUtils::BindWindowsFunction(SOCKET socket, GUID guid, T& func)
{
    DWORD bytes = 0;
    return ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guid, sizeof(guid), &func, sizeof(func),
        &bytes, nullptr, nullptr) != SOCKET_ERROR;
}

bool SocketUtils::SetSocketOpt(SOCKET socket, int32 level, int32 optName, bool flag)
{
    int32 opt = flag ? 1 : 0;
    return ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&opt), sizeof(opt)) != SOCKET_ERROR;
}
