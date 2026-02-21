# MiniDump 시스템 흐름

## 1. 랜덤 크래시 발생 흐름

```mermaid
sequenceDiagram
    participant Console as 콘솔
    participant CH as CrashHandler
    participant World as World::Tick
    participant Obj as GUObjectArray
    participant Game as 게임 코드

    Console->>CH: RANDOMCRASH 3 입력
    CH->>CH: EnableRandomCrashBombard(true, 3)
    Note over CH: g_Bombard = true<br/>g_BombardPerFrame = 3

    loop 매 프레임
        World->>CH: RandomCrashTick()
        CH->>CH: if (g_Bombard) 체크

        loop 3번 반복
            CH->>Obj: 랜덤 인덱스 선택
            Obj-->>CH: UObject* victim
            CH->>Obj: DeleteObject(victim)
            Note over CH,Obj: 메모리 해제!<br/>(여기선 크래시 안 남)
        end
    end

    Note over Game: 게임 계속 실행...
    Game->>Obj: victim->Tick() 접근
    Note over Game: 💥 UAF 크래시 발생!
```

---

## 2. 덤프 파일 생성 흐름

```mermaid
flowchart TB
    Crash["💥 크래시 발생<br/>(Access Violation)"]
    OS["Windows OS<br/>예외 감지"]
    Handler["OnUnhandledException()<br/>(SEH 핸들러)"]
    Ctx["EXCEPTION_POINTERS<br/>• ExceptionRecord<br/>• ContextRecord"]
    Write["WriteMiniDump()"]
    API["MiniDumpWriteDump()<br/>Windows API"]
    File["Crash_PID_날짜.dmp<br/>파일 생성"]
    Path1["Saved/Crashes/"]
    Path2["실행 파일 옆"]
    Path3["%TEMP%"]
    Done["프로세스 종료"]

    Crash --> OS
    OS --> Handler
    Handler --> Ctx
    Ctx --> Write
    Write --> API
    API --> Path1
    Path1 -->|실패 시| Path2
    Path2 -->|실패 시| Path3
    Path3 --> File
    File --> Done

    style Crash fill:#f99,stroke:#333,stroke-width:3px
    style API fill:#9cf,stroke:#333,stroke-width:2px
    style File fill:#9f9,stroke:#333,stroke-width:2px
```

---

## 3. 전체 시스템 통합 흐름

```mermaid
flowchart LR
    subgraph Init["초기화"]
        Start["프로그램 시작"]
        InitHandler["FCrashHandler::Initialize()"]
        SetFilter["SetUnhandledExceptionFilter()"]
    end

    subgraph Runtime["런타임"]
        Cmd["콘솔: RANDOMCRASH"]
        Enable["폭격 모드 ON"]
        Tick["매 프레임<br/>RandomCrashTick()"]
        Delete["랜덤 객체 삭제"]
    end

    subgraph Crash["크래시"]
        UAF["UAF 발생"]
        Exception["예외 핸들러 호출"]
        Dump["덤프 생성"]
    end

    Start --> InitHandler
    InitHandler --> SetFilter
    SetFilter --> Cmd
    Cmd --> Enable
    Enable --> Tick
    Tick --> Delete
    Delete -.->|반복| Tick
    Delete -.->|언젠가| UAF
    UAF --> Exception
    Exception --> Dump

    style UAF fill:#f99,stroke:#333,stroke-width:3px
    style Dump fill:#9f9,stroke:#333,stroke-width:3px
```

---

## 4. 랜덤 크래시 상세 로직

```mermaid
flowchart TB
    Start["RandomCrash() 호출"]
    Check["GUObjectArray 검사"]
    Valid["유효 객체 수집<br/>TArray&lt;int32&gt; valid"]
    Random["랜덤 선택<br/>std::mt19937"]
    Pick["victim = GUObjectArray[pick]"]
    Log["로그 출력<br/>'Deleted: Actor_123'"]
    Delete["ObjectFactory::DeleteObject(victim)"]
    Profile["SetNextDumpProfileDataSegsOnly()<br/>(경량 덤프 설정)"]
    Return["리턴<br/>(크래시 안 남!)"]

    Start --> Check
    Check --> Valid
    Valid --> Random
    Random --> Pick
    Pick --> Log
    Log --> Delete
    Delete --> Profile
    Profile --> Return

    style Delete fill:#f99,stroke:#333,stroke-width:2px
    style Return fill:#9f9,stroke:#333,stroke-width:2px
```

---

## 5. 덤프 타입 결정

```mermaid
flowchart LR
    Check["g_NextDumpProfile 확인"]
    Full["Full Memory<br/>(기본)"]
    Data["DataSegs Only<br/>(경량)"]
    API["MiniDumpWriteDump()"]

    Check -->|profile == 0| Full
    Check -->|profile == 1| Data
    Full --> API
    Data --> API

    Note1["• 모든 메모리<br/>• 힙, 스택, 전역변수<br/>• 크기: 수백 MB~GB"]
    Note2["• 스택 + 예외<br/>• 데이터 세그먼트<br/>• 크기: 수십 MB"]

    Full -.-> Note1
    Data -.-> Note2

    style Full fill:#faa,stroke:#333,stroke-width:2px
    style Data fill:#afa,stroke:#333,stroke-width:2px
```

---

## 핵심 포인트

### 랜덤 크래시
1. **명령어 1번 입력** → 폭격 모드 활성화
2. **매 프레임 N개씩 삭제** → World::Tick에서 자동 호출
3. **삭제 시점에 크래시 안 남** → 자연스러운 UAF 유도
4. **실제 크래시 사이트 포착** → 디버깅 가치 높음

### 덤프 생성
1. **SEH 핸들러 등록** → Windows 전역 예외 처리
2. **EXCEPTION_POINTERS** → 크래시 순간 스냅샷
3. **3단계 폴백 경로** → 덤프 생성 보장
4. **두 가지 덤프 타입** → Full(기본) / DataSegs(경량)
