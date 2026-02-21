# AnimNotify System - 핵심 구조

## 시스템 구조 (간단 버전)

```mermaid
flowchart LR
    Lua["Lua 파일<br/>핸들러 정의"]
    LM["LuaManager<br/>파싱"]
    Disp["Dispatcher<br/>싱글톤"]
    Anim["AnimInstance<br/>트리거"]

    Lua -->|PIE 시작| LM
    LM -->|Register| Disp
    Anim -->|Dispatch| Disp
    Disp -->|실행| Lua

    style Disp fill:#bbf,stroke:#333,stroke-width:3px
```

---

## 실행 흐름

```mermaid
sequenceDiagram
    participant Lua as Lua 파일
    participant LM as LuaManager
    participant D as Dispatcher
    participant A as AnimInstance

    Note over Lua,LM: PIE 시작
    LM->>Lua: 로드
    LM->>D: Register(핸들러)

    Note over A,Lua: 게임 플레이
    A->>D: Dispatch(이벤트)
    D->>Lua: 핸들러 실행
```

---

## 데이터 구조

```mermaid
graph TB
    Disp["FNotifyDispatcher"]
    Map["Map&lt;시퀀스, Map&lt;이름, 핸들러&gt;&gt;"]

    Disp --> Map
    Map --> Walk["Walk.fbx"]
    Walk --> Foot["Footstep → function()"]
```

---

## 핵심 3단계

1. **등록**: PIE 시작 → Lua 파싱 → Register
2. **저장**: Dispatcher가 2단계 맵에 저장
3. **실행**: 노티파이 발생 → Dispatch → Lua 실행
