#include "pch.h"
#include "IocpCore.h"

IocpCore::IocpCore()
{
    _iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
    ::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(IocpObjectRef iocpObject)
{
    return ::CreateIoCompletionPort(
        iocpObject->GetHandle(),
        _iocpHandle,
        reinterpret_cast<ULONG_PTR>(iocpObject.operator->()),
        0
    ) != nullptr;
}

bool IocpCore::Dispatch(uint32 timeoutMs)
{
    DWORD       numOfBytes  = 0;
    ULONG_PTR   key         = 0;
    IocpEvent*  iocpEvent   = nullptr;

    // GetQueuedCompletionStatus는 완료 이벤트가 올 때까지 블로킹
    if (::GetQueuedCompletionStatus(
        _iocpHandle,
        OUT &numOfBytes,
        OUT &key,
        OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent),
        timeoutMs))
    {
        IocpObjectRef iocpObject = iocpEvent->owner;
        iocpObject->Dispatch(iocpEvent, numOfBytes);
    }
    else
    {
        int32 errorCode = ::WSAGetLastError();

        switch (errorCode)
        {
        case WAIT_TIMEOUT:
            return false;

        default:
            // 소켓 오류 (연결 끊김 등) — iocpEvent가 있으면 Dispatch로 처리
            if (iocpEvent != nullptr)
            {
                IocpObjectRef iocpObject = iocpEvent->owner;
                iocpObject->Dispatch(iocpEvent, numOfBytes);
            }
            break;
        }
    }

    return true;
}
