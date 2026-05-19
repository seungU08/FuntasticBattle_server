#pragma once

class NetAddress
{
public:
    NetAddress() = default;
    NetAddress(SOCKADDR_IN sockAddr);
    NetAddress(wstring ip, uint16 port);

    SOCKADDR_IN     GetSockAddr()   { return _sockAddr; }
    wstring         GetIp();
    uint16          GetPort();
    wstring         GetIpPort();

    static IN_ADDR  Ip2Address(const WCHAR* ip);

private:
    SOCKADDR_IN _sockAddr = {};
};
