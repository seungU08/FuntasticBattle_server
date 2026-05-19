#pragma once

/*--------------------------------------
    RecvBuffer
    슬라이딩 윈도우 방식의 수신 버퍼.
    버퍼 용량의 절반 이상을 읽었으면
    데이터를 앞으로 당겨 여유 공간 확보.
----------------------------------------*/
class RecvBuffer
{
    enum { BUFFER_COUNT = 10 };

public:
    RecvBuffer(int32 bufferSize);
    ~RecvBuffer() = default;

    void    Clean();
    bool    OnWrite(int32 numOfBytes);
    bool    OnRead(int32 numOfBytes);

    BYTE*   ReadPos()   { return &_buffer[_readPos]; }
    BYTE*   WritePos()  { return &_buffer[_writePos]; }
    int32   DataSize()  { return _writePos - _readPos; }
    int32   FreeSize()  { return _capacity - _writePos; }

private:
    int32           _capacity   = 0;
    int32           _bufferSize = 0;
    int32           _readPos    = 0;
    int32           _writePos   = 0;
    vector<BYTE>    _buffer;
};
