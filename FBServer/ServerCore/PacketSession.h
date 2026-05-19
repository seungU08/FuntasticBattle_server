#pragma once
#include "Session.h"

struct PacketHeader
{
    uint16 size;
    uint16 id;
};

class PacketSession : public Session
{
public:
    virtual int32 OnRecv(BYTE* buffer, int32 len) override final;
    virtual void  OnRecvPacket(BYTE* buffer, int32 len) abstract;
};

using PacketSessionRef = TSharedPtr<PacketSession>;
