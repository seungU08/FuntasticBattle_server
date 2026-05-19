#pragma once

class SocketUtils
{
public:
    static LPFN_CONNECTEX       ConnectEx;
    static LPFN_DISCONNECTEX    DisconnectEx;
    static LPFN_ACCEPTEX        AcceptEx;

    static void     Init();
    static void     Clear();

    static bool     SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
    static bool     SetReuseAddress(SOCKET socket, bool flag);
    static bool     SetRecvBufferSize(SOCKET socket, int32 size);
    static bool     SetSendBufferSize(SOCKET socket, int32 size);
    static bool     SetTcpNoDelay(SOCKET socket, bool flag);
    static bool     SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket);

    static SOCKET   CreateSocket();
    static bool     BindAnyAddress(SOCKET socket, uint16 port);
    static bool     Listen(SOCKET socket, int32 backlog = SOMAXCONN);
    static bool     Connect(SOCKET socket, NetAddress netAddress);

private:
    template<typename T>
    static bool     BindWindowsFunction(SOCKET socket, GUID guid, T& func);

    static bool     SetSocketOpt(SOCKET socket, int32 level, int32 optName, bool flag);
};
