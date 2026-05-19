#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "DeadLockProfiler.h"
#include "IocpCore.h"
#include "SendBuffer.h"

ThreadManager*      GThreadManager      = nullptr;
DeadLockProfiler*   GDeadLockProfiler   = nullptr;
IocpCore*           GIocpCore           = nullptr;
SendBufferManager*  GSendBufferManager  = nullptr;

class CoreGlobal
{
public:
    CoreGlobal()
    {
        GThreadManager      = new ThreadManager();
        GDeadLockProfiler   = new DeadLockProfiler();
        GIocpCore           = new IocpCore();
        GSendBufferManager  = new SendBufferManager();
    }

    ~CoreGlobal()
    {
        delete GThreadManager;
        delete GDeadLockProfiler;
        delete GIocpCore;
        delete GSendBufferManager;
    }
};

CoreGlobal GCoreGlobal;