#pragma once

class IocpObject : public RefCountable
{
public:
    virtual HANDLE  GetHandle() = 0;
    virtual void    Dispatch(struct IocpEvent* iocpEvent, int32 numOfBytes = 0) = 0;
};

using IocpObjectRef = TSharedPtr<IocpObject>;
