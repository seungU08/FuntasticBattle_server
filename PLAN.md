# FuntasticBattle IOCP 서버 — 다중 클라이언트 동기화 구현

## Context

졸업작품 **FuntasticBattle**(실시간 액션 게임, 동시접속 2~4명)의 자체 IOCP 서버를 구축하는 과제. 클라이언트(언리얼)는 이미 캐릭터·무기·UI까지 만들어져 있고, 서버 측은 `FBServer.sln` 솔루션·`ServerCore` 정적 라이브러리·`GameServer`/`DummyClient` 실행 파일까지 구조만 잡혀 있으나 **네트워크 레이어가 전부 비어 있는 상태**다 (Lock·ThreadManager·RefCounting·DeadLockProfiler까지만 구현, IOCP·Session·Service·Buffer 전무).

이번 작업의 목적: ServerCore에 IOCP 네트워크 레이어를 깔고, 위에 패킷 시스템과 Room 기반 게임 로직을 올려, **DummyClient 여러 개로 위치·애니메이션·채팅이 서로 동기화되는 것을 검증**한다. 언리얼 측 연동은 별도 작업으로 분리.

설계 결정:
- **패킷 직렬화**: POD 구조체 + `#pragma pack` (수작업, 외부 라이브러리 없음)
- **동시성 모델**: 루키스 강의 정석의 **JobQueue 기반** (Room이 JobQueue 상속, 패킷 처리 직렬화)
- **권위 모델**: 클라이언트 권위 + 서버 중계 (서버는 검증 없이 브로드캐스트)
- **이번 범위**: Phase 1(IOCP 코어) + Phase 2(패킷 시스템) + Phase 3(Room + 동기화)까지

---

## 사전 정리 — 기존 코드 버그 수정

작업 시작 전 컴파일 막혀있을 가능성이 큰 부분부터 정리한다.

| 파일 | 문제 | 수정 |
| --- | --- | --- |
| `ServerCore/CoreGlobal.h` | `GthreadManager` (소문자 t) | `GThreadManager` (대문자 T)로 통일 |
| `ServerCore/CoreGlobal.cpp` | `CoreGlobal` 클래스만 정의되고 인스턴스 없음 → `GThreadManager`가 nullptr | 파일 끝에 `CoreGlobal GCoreGlobal;` 정적 인스턴스 추가 |
| `GameServer/GameServer.cpp` | `ThreadMain` 미정의 + `GthreadManager` 사용 | main 함수 자체를 Phase 1 작업에서 재작성 |
| `ServerCore/CorePch.h` | WinSock 헤더 없음 | `<WinSock2.h>`, `<MSWSock.h>`, `<WS2tcpip.h>` 추가 + `#pragma comment(lib, "ws2_32")`, `"mswsock"` 추가 |

---

## Phase 1 — IOCP 네트워크 코어

### 추가 파일 (모두 `ServerCore/`, vcxproj filter: `Network`)

| 파일 | 역할 |
| --- | --- |
| `SocketUtils.h/cpp` | WSAStartup/Cleanup, `AcceptEx`·`ConnectEx`·`DisconnectEx` 함수 포인터(WSAIoctl) 로드, SetReuseAddress/SetTcpNoDelay/SetLinger 등 옵션 헬퍼 |
| `NetAddress.h/cpp` | `SOCKADDR_IN` 래퍼 (IP/Port → SOCKADDR_IN 변환) |
| `IocpCore.h/cpp` | `HANDLE _iocpHandle`. `Register(IocpObjectRef)`로 핸들을 IOCP에 묶고, `Dispatch(timeoutMs)`에서 `GetQueuedCompletionStatus`로 이벤트 받아 해당 객체의 `Dispatch(event, bytes)` 호출 |
| `IocpEvent.h/cpp` | `enum class EventType { Connect, Disconnect, Accept, Recv, Send }`, `struct IocpEvent : OVERLAPPED` (base), `ConnectEvent`/`DisconnectEvent`/`AcceptEvent`/`RecvEvent`/`SendEvent` 파생 + `owner = IocpObjectRef` |
| `IocpObject.h` | 추상 인터페이스. `virtual HANDLE GetHandle() = 0`, `virtual void Dispatch(IocpEvent*, int32 numOfBytes) = 0`. `RefCountable` 상속 |
| `Session.h/cpp` | `IocpObject` 상속. `SOCKET _socket`, `NetAddress`, `Atomic<bool> _connected`, `RecvEvent`, `vector<SendBufferRef> _sendQueue` + `USE_LOCK`. 메서드: `Connect/Disconnect/Send/RegisterRecv/RegisterSend/Register*`. Phase 1에서는 임시로 raw byte echo만 구현하고 OnRecv/OnSend는 virtual 콜백으로 비워둠 |
| `Listener.h/cpp` | `IocpObject` 상속. AcceptEx를 `_acceptCount`(예: 4)개 미리 큐잉. `Dispatch`에서 AcceptEvent 받으면 새 세션 등록 후 다시 RegisterAccept |
| `Service.h/cpp` | base `Service` + `ServerService`(Listener 보유, Start로 listen 시작) + `ClientService`(여러 세션을 ConnectEx로 동시 연결). `function<SessionRef()> _sessionFactory`로 세션 생성 |

### vcxproj/filters 갱신
- `ServerCore.vcxproj`에 위 파일들을 `<ClInclude>/<ClCompile>`로 추가
- `ServerCore.vcxproj.filters`에 `Network` 필터로 분류

### Phase 1 검증
- GameServer main에서 `ServerService` 띄우고, DummyClient main에서 `ClientService`로 N개 동시 연결
- 콘솔에 Accept 성공 / Connect 성공 로그 출력
- 강제 종료해도 깨끗하게 disconnect 처리되는지 확인

---

## Phase 2 — 버퍼 + 패킷 시스템 + JobQueue

### 추가 파일 (ServerCore)

| 파일 | 역할 |
| --- | --- |
| `RecvBuffer.h/cpp` | 링 버퍼 형태. `WritePos`/`ReadPos`/`Capacity`, `OnWrite(n)`/`OnRead(n)`/`Clean()` (반쪽 차면 앞으로 메모리 이동) |
| `SendBuffer.h/cpp` | 청크 일부를 점유하는 `SendBuffer`. `Buffer()`, `WriteSize()`, `Close(writtenSize)` |
| `SendBufferChunk.h/cpp` | 6KB 같은 큰 청크. `Open(allocSize)` → 청크 잔여 공간에서 SendBuffer 자름 |
| `SendBufferManager.h/cpp` | 전역 `GSendBufferManager`. TLS 청크 캐시 + `Open(size)`로 SendBuffer 발급 |
| `Job.h/cpp` | `function<void()>` 캐싱. 인자 바인딩 포함 |
| `JobQueue.h/cpp` | `Push(JobRef)`. 첫 일감 진입 시 Execute 진입. 큐 비울 때까지 Pop&실행 |
| `PacketSession.h/cpp` | `Session` 상속. `OnRecv` override: 헤더 단위로 잘라가며 `OnRecvPacket(buffer, len)` 호출 |

### 공유 헤더 신설 — `FBServer/Protocol/Protocol.h`

POD 구조체로 모든 패킷 정의. 솔루션 루트 아래 `Protocol/` 폴더를 만들어 GameServer / DummyClient / 추후 언리얼 모두가 같은 파일을 참조.

```cpp
#pragma pack(push, 1)

enum class PacketId : uint16 {
    // C → S
    CS_LOGIN        = 1,
    CS_ENTER_ROOM   = 2,
    CS_LEAVE_ROOM   = 3,
    CS_MOVE         = 10,
    CS_ANIM_STATE   = 11,
    CS_CHAT         = 20,
    // S → C
    SC_LOGIN_RES    = 1001,
    SC_ENTER_GAME   = 1002,   // 본인 입장 + 기존 플레이어 스냅샷
    SC_PLAYER_ENTER = 1003,   // 다른 플레이어 입장
    SC_PLAYER_LEAVE = 1004,
    SC_MOVE         = 1010,
    SC_ANIM_STATE   = 1011,
    SC_CHAT         = 1020,
};

struct PacketHeader { uint16 size; uint16 id; };

struct CS_LOGIN_PKT        { PacketHeader h; wchar_t name[16]; };
struct SC_LOGIN_RES_PKT    { PacketHeader h; uint64 playerId; uint8 success; };

struct CS_ENTER_ROOM_PKT   { PacketHeader h; uint32 roomId; };
struct SC_ENTER_GAME_PKT   { PacketHeader h; uint64 myPlayerId; uint8 otherCount; /* 뒤로 PlayerSnapshot 가변 */ };
struct PlayerSnapshot      { uint64 playerId; float x,y,z; float yaw; wchar_t name[16]; };

struct CS_MOVE_PKT         { PacketHeader h; float x,y,z; float yaw; float vx,vy,vz; };
struct SC_MOVE_PKT         { PacketHeader h; uint64 playerId; float x,y,z; float yaw; float vx,vy,vz; };

struct CS_ANIM_STATE_PKT   { PacketHeader h; uint8 state; }; // 0=Normal,1=Attack,2=Hit,3=Stun
struct SC_ANIM_STATE_PKT   { PacketHeader h; uint64 playerId; uint8 state; };

struct CS_CHAT_PKT         { PacketHeader h; wchar_t msg[128]; };
struct SC_CHAT_PKT         { PacketHeader h; uint64 playerId; wchar_t msg[128]; };

struct SC_PLAYER_ENTER_PKT { PacketHeader h; PlayerSnapshot p; };
struct SC_PLAYER_LEAVE_PKT { PacketHeader h; uint64 playerId; };

#pragma pack(pop)
```

### 빌드 설정
- `GameServer.vcxproj` IncludePath에 `$(SolutionDir)Protocol\;` 추가
- `DummyClient.vcxproj`도 동일

### Phase 2 검증
- 더미 클라이언트가 `CS_LOGIN` 보내면 서버가 `SC_LOGIN_RES` 응답
- 콘솔에 송수신 패킷 ID/size 로그

---

## Phase 3 — Room 기반 동기화

### GameServer 추가

| 파일 | 역할 |
| --- | --- |
| `GameSession.h/cpp` | `PacketSession` 상속. `OnRecvPacket`에서 `ClientPacketHandler::HandlePacket(this, buffer, len)` 호출. `PlayerRef _player` 보유 |
| `GameSessionManager.h/cpp` | 전역 `GSessionManager`. Add/Remove/Count. `USE_LOCK` |
| `Player.h/cpp` | `RefCountable`. `uint64 playerId`, `wstring name`, `float x/y/z/yaw`, `vx/vy/vz`, `uint8 animState`, `RoomRef room`, `GameSessionRef ownerSession` |
| `Room.h/cpp` | **`JobQueue` 상속**. `map<uint64, PlayerRef> _players`. `Enter/Leave/Broadcast/HandleMove/HandleAnim/HandleChat`. 모든 메서드는 외부에서 `Push(Job)` 형태로 위임 → JobQueue가 직렬 실행 |
| `RoomManager.h/cpp` | 전역 `GRoomManager`. 기본 방 1개, `GetRoom(roomId)` |
| `ClientPacketHandler.h/cpp` | PacketId switch로 라우팅. `Handle_CS_LOGIN`, `Handle_CS_ENTER_ROOM`, `Handle_CS_LEAVE_ROOM`, `Handle_CS_MOVE`, `Handle_CS_ANIM_STATE`, `Handle_CS_CHAT` |
| `MakeSendBuffer.h/cpp` | `SC_*` 패킷 빌더 헬퍼 함수 |

### GameServer.cpp main
```
SocketUtils::Init()
→ ServerService 생성 (listen 7777, sessionFactory = GameSession)
→ service->Start()
→ 워커 스레드 N개 Launch: while(true) { GIocpCore->Dispatch(10); }
→ GThreadManager->Join()
```

### DummyClient 추가

| 파일 | 역할 |
| --- | --- |
| `ServerSession.h/cpp` | `PacketSession` 상속. `OnRecvPacket`에서 `ServerPacketHandler::HandlePacket` 호출 |
| `ServerPacketHandler.h/cpp` | `Handle_SC_*` 핸들러. 콘솔에 수신 내용 출력 |
| `DummyClient.cpp` main | `ClientService`로 N개 세션 동시 연결 (커맨드라인 인자). 워커 스레드 Dispatch + 주기적 CS_MOVE 송신 (sin/cos 이동) |

### Phase 3 검증
1. `GameServer.exe` 실행
2. `DummyClient.exe 3` 으로 3개 세션 동시 연결
3. LOGIN → ENTER_ROOM 자동 처리 후 100ms마다 CS_MOVE 송신
4. 각 더미가 **자신을 제외한 나머지의 SC_MOVE 수신** 콘솔 출력 확인
5. CS_CHAT 브로드캐스트 확인
6. 더미 강제 종료 시 나머지가 SC_PLAYER_LEAVE 수신 확인

---

## 변경 대상 파일 요약

```
FBServer/
├─ ServerCore/
│   ├─ CorePch.h                    ← 수정 (WinSock 포함)
│   ├─ CoreGlobal.h                 ← 수정 (GThreadManager 대소문자)
│   ├─ CoreGlobal.cpp               ← 수정 (정적 인스턴스)
│   ├─ ServerCore.vcxproj           ← 수정 (파일 추가)
│   ├─ ServerCore.vcxproj.filters   ← 수정 (Network 필터)
│   ├─ SocketUtils.h/cpp            ← 신규
│   ├─ NetAddress.h/cpp             ← 신규
│   ├─ IocpCore.h/cpp               ← 신규
│   ├─ IocpEvent.h/cpp              ← 신규
│   ├─ IocpObject.h                 ← 신규
│   ├─ Session.h/cpp                ← 신규
│   ├─ Listener.h/cpp               ← 신규
│   ├─ Service.h/cpp                ← 신규
│   ├─ RecvBuffer.h/cpp             ← 신규
│   ├─ SendBuffer.h/cpp             ← 신규
│   ├─ SendBufferChunk.h/cpp        ← 신규
│   ├─ SendBufferManager.h/cpp      ← 신규
│   ├─ Job.h/cpp                    ← 신규
│   ├─ JobQueue.h/cpp               ← 신규
│   └─ PacketSession.h/cpp          ← 신규
│
├─ Protocol/                        ← 신규 폴더
│   └─ Protocol.h
│
├─ GameServer/
│   ├─ GameServer.cpp               ← 전면 재작성
│   ├─ GameServer.vcxproj           ← IncludePath 추가
│   ├─ GameSession.h/cpp            ← 신규
│   ├─ GameSessionManager.h/cpp     ← 신규
│   ├─ Player.h/cpp                 ← 신규
│   ├─ Room.h/cpp                   ← 신규
│   ├─ RoomManager.h/cpp            ← 신규
│   ├─ ClientPacketHandler.h/cpp    ← 신규
│   └─ MakeSendBuffer.h/cpp         ← 신규
│
└─ DummyClient/
    ├─ DummyClient.cpp              ← 전면 재작성
    ├─ DummyClient.vcxproj          ← IncludePath 추가
    ├─ ServerSession.h/cpp          ← 신규
    └─ ServerPacketHandler.h/cpp    ← 신규
```

## 빌드 / 실행 순서

```
1. ServerCore 빌드  → Libraries/Debug/ServerCore.lib
2. GameServer 빌드  → Binary/Debug/GameServer.exe
3. DummyClient 빌드 → Binary/Debug/DummyClient.exe
4. GameServer.exe 실행
5. DummyClient.exe 3 실행
```

---

## 이후 작업 — 언리얼 연동 (다음 세션)

- `SimpleShooter.Build.cs`에 `"Sockets"`, `"Networking"` 모듈 추가
- `Protocol.h`를 언리얼 쪽 IncludePath에 추가
- `UNetworkSubsystem` (`UGameInstanceSubsystem`) 생성
  - 별도 RecvWorker 스레드 (`FRunnable`)
  - 게임 스레드와 `TQueue<FPacket>` 메일박스 통신
- `ULoginWidget` → CS_LOGIN 송신 / `OnLoginSuccess` 콜백
- `AShoorterCharater::Tick` → 주기적 CS_MOVE 송신
- SC_MOVE 수신 시 원격 캐릭터 위치 보간 (Lerp + 데드 레커닝)
- 애니메이션 몽타주 재생 시 CS_ANIM_STATE 송신
