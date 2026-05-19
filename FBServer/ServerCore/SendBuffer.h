#pragma once

/*--------------------------------------
    SendBuffer (Phase 1 - 단순 버전)
    WSASend에 넘길 데이터를 담는 버퍼.
    Phase 2에서 청크/TLS 방식으로 교체.
----------------------------------------*/
class SendBuffer : public RefCountable
{
public:
    SendBuffer(int32 allocSize);
    ~SendBuffer() = default;

    BYTE*   Buffer()        { return _buffer.data(); }
    uint32  AllocSize()     { return _allocSize; }
    uint32  WriteSize()     { return _writeSize; }
    void    Close(uint32 writeSize);

private:
    uint32          _allocSize  = 0;
    uint32          _writeSize  = 0;
    vector<BYTE>    _buffer;
};

using SendBufferRef = TSharedPtr<SendBuffer>;

/*--------------------------------------
    SendBufferManager (Phase 1 stub)
    Phase 2에서 청크 풀로 교체 예정.
    CoreGlobal이 인스턴스를 보유.
----------------------------------------*/
class SendBufferManager
{
public:
    SendBufferRef   Open(uint32 size);
};
