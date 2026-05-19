#include "pch.h"
#include "PacketSession.h"

int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
    int32 processLen = 0;

    while (true)
    {
        int32 dataSize = len - processLen;

        // 헤더 크기도 안 들어왔으면 대기
        if (dataSize < sizeof(PacketHeader))
            break;

        PacketHeader header = *reinterpret_cast<PacketHeader*>(&buffer[processLen]);

        // 헤더에 명시된 크기만큼 안 들어왔으면 대기
        if (dataSize < header.size)
            break;

        // 패킷 최소/최대 크기 검증
        if (header.size < sizeof(PacketHeader))
        {
            // 비정상 패킷 — 세션 끊기
            Disconnect(L"Invalid packet size");
            break;
        }

        OnRecvPacket(&buffer[processLen], header.size);
        processLen += header.size;
    }

    return processLen;
}
