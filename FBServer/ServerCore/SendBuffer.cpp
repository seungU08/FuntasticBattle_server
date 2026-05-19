#include "pch.h"
#include "SendBuffer.h"

SendBuffer::SendBuffer(int32 allocSize) : _allocSize(allocSize)
{
    _buffer.resize(allocSize);
}

void SendBuffer::Close(uint32 writeSize)
{
    ASSERT_CRASH(writeSize <= _allocSize);
    _writeSize = writeSize;
}

SendBufferRef SendBufferManager::Open(uint32 size)
{
    return MakeShared<SendBuffer>(size);
}
