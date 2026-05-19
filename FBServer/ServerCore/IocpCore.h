#pragma once

class IocpCore : public RefCountable
{
public:
    IocpCore();
    ~IocpCore();

    HANDLE  GetHandle() { return _iocpHandle; }

    bool    Register(IocpObjectRef iocpObject);
    bool    Dispatch(uint32 timeoutMs = INFINITE);

private:
    HANDLE  _iocpHandle;
};

using IocpCoreRef = TSharedPtr<IocpCore>;
