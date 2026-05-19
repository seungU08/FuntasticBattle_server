#pragma once

#include "Types.h"
#include "CoreGlobal.h"
#include "CoreTLS.h"
#include "CoreMacro.h"

#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <algorithm>
#include <atomic>
#include <string>
#include <thread>
#include <mutex>

// WinSock2는 반드시 Windows.h 앞에 포함해야 한다
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#include <Windows.h>
#include <iostream>
using namespace std;

#include "Lock.h"
#include "RefCounting.h"
#include "ThreadManager.h"

// 네트워크 레이어 (의존 순서 유지)
#include "NetAddress.h"
#include "SocketUtils.h"
#include "IocpObject.h"
#include "IocpEvent.h"
#include "IocpCore.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "Session.h"
#include "Listener.h"
#include "Service.h"
#include "Job.h"
#include "JobQueue.h"
#include "PacketSession.h"



