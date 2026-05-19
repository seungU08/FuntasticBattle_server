#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(SOCKADDR_IN sockAddr) : _sockAddr(sockAddr)
{
}

NetAddress::NetAddress(wstring ip, uint16 port)
{
    ::memset(&_sockAddr, 0, sizeof(_sockAddr));
    _sockAddr.sin_family = AF_INET;
    _sockAddr.sin_addr   = Ip2Address(ip.c_str());
    _sockAddr.sin_port   = ::htons(port);
}

wstring NetAddress::GetIp()
{
    WCHAR buf[100];
    ::InetNtopW(AF_INET, &_sockAddr.sin_addr, buf, sizeof(buf) / sizeof(WCHAR));
    return wstring(buf);
}

uint16 NetAddress::GetPort()
{
    return ::ntohs(_sockAddr.sin_port);
}

wstring NetAddress::GetIpPort()
{
    return GetIp() + L":" + to_wstring(GetPort());
}

IN_ADDR NetAddress::Ip2Address(const WCHAR* ip)
{
    IN_ADDR addr;
    ::InetPtonW(AF_INET, ip, &addr);
    return addr;
}
