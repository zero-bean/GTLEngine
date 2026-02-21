#include "pch.h"
#include "NotifyDispatcher.h"

//
// FNotifyDispatcher::Get()
//
//  - NotifyDispatcher의 전역 싱글톤 인스턴스를 반환한다.
//  - AnimInstance, LuaManager 등 엔진 전역에서 동일한 Dispatcher를 공유하기 위함.
//  - 한 번 생성된 FNotifyDispatcher DispatcherInstance를 재사용하여
//    Notify 등록/실행을 모두 이 객체 한 곳에서 처리한다.
//
FNotifyDispatcher& FNotifyDispatcher::Get()
{
    static FNotifyDispatcher DispatcherInstance;
    return DispatcherInstance;
}

//
// Clear()
//
//  - 모든 Notify 핸들러를 제거한다.
//  - 보통 월드 리셋 또는 새 Lua Config 로드 시 전체 매핑을 초기화할 때 호출.
//  - HandlersBySeq = TMap<SequenceKey, TMap<NotifyName, Handler>>
//
void FNotifyDispatcher::Clear()
{
    HandlersBySeq.clear();
}

//
// Register(SeqKey, NotifyName, Handler)
//
//  - 특정 애니메이션 시퀀스(SeqKey)에 대해,
//    특정 NotifyName이 발생했을 때 실행될 함수를 등록한다.
//  
//  사용 예:
//      Register("Data/Animations/Walk.fbx", "Footstep", HandlerFunc);
//
//  동작 설명:
//  1) 시퀀스 키(파일 경로)로 맵을 찾는다.
//  2) 없다면 새 NotifyMap을 만들어 추가.
//  3) 해당 NotifyName에 대응하는 Handler(std::function)를 저장.
//
//  결과:
//      HandlersBySeq[SeqKey][NotifyName] = Handler;
//
void FNotifyDispatcher::Register(const FString& SeqKey, const FName& NotifyName, const FNotifyHandler& Handler)
{
    auto* MapPtr = HandlersBySeq.Find(SeqKey);
    if (!MapPtr)
    {
        // 해당 시퀀스에 대한 첫 Notify 등록
        TMap<FName, FNotifyHandler> NewMap;
        NewMap.Add(NotifyName, Handler);
        HandlersBySeq.Add(SeqKey, NewMap);
        return;
    }

    // 기존 시퀀스에 새로운 NotifyName 핸들러 추가
    MapPtr->Add(NotifyName, Handler);
}

// Dispatch(SeqKey, Event)
//
//  - AnimInstance가 Notify 발생 시 호출하는 함수.
//  - 등록된 Notify 핸들러를 찾아 실행한다.
//
// 동작 과정:
//
//  1) SeqKey(파일 경로)로 해당 시퀀스의 NotifyMap을 찾는다.
//      → 없으면 이 시퀀스는 처리할 Notify가 없음 → return
//
//  2) NotifyName(Event.NotifyName)으로 등록된 Handler를 찾는다.
//      → 없으면 처리할 함수 없음 → return
//
//  3) Debug Log 출력
//      “[Notify][Lua] Seq='...' Name='...' t=0.3”
//
//  4) Handler(Event) 호출 → C++ 또는 Lua 로직 실행
//
void FNotifyDispatcher::Dispatch(const FString& SeqKey, const FAnimNotifyEvent& Event)
{
    if (!bEnabled)
    {
        UE_LOG("[Notify] Dispatcher is disabled, skipping notify '%s'",
            Event.NotifyName.ToString().c_str());
        return;
    }

    // 시퀀스(애니메이션 파일 경로)에 대한 NotifyMap 찾기
    auto* MapPtr = HandlersBySeq.Find(SeqKey);
    if (!MapPtr)
    {
        UE_LOG("[Notify] WARNING: No handlers registered for sequence '%s' (notify '%s')",
            SeqKey.c_str(), Event.NotifyName.ToString().c_str());
        UE_LOG("[Notify] Available sequences:");
        for (const auto& kv : HandlersBySeq)
        {
            UE_LOG("[Notify]   - '%s'", kv.first.c_str());
        }
        return;
    }

    // NotifyName(FName)으로 등록된 핸들러 찾기
    auto* Fn = MapPtr->Find(Event.NotifyName);
    if (!Fn)
    {
        UE_LOG("[Notify] WARNING: No handler for notify '%s' in sequence '%s'",
            Event.NotifyName.ToString().c_str(), SeqKey.c_str());
        UE_LOG("[Notify] Available notifies for this sequence:");
        for (const auto& kv : *MapPtr)
        {
            UE_LOG("[Notify]   - '%s'", kv.first.ToString().c_str());
        }
        return;
    }

    // Debug 로그 출력 (VS Output에서 확인 가능)
    UE_LOG("[Notify][Lua] SUCCESS! Seq='%s' Name='%s' t=%.3f",
        SeqKey.c_str(), Event.NotifyName.ToString().c_str(), Event.TriggerTime);

    // 등록된 Notify Handler 실행 (C++ or Lua 함수)
    (*Fn)(Event);
}
