#pragma once
#include "Protocol.h"

template<typename PKT>
static SendBufferRef MakeSendBuffer(PKT& pkt, PacketId packetId)
{
    pkt.h.size = sizeof(PKT);
    pkt.h.id   = static_cast<uint16>(packetId);

    SendBufferRef sendBuffer = MakeShared<SendBuffer>(sizeof(PKT));
    ::memcpy(sendBuffer->Buffer(), &pkt, sizeof(PKT));
    sendBuffer->Close(sizeof(PKT));
    return sendBuffer;
}
