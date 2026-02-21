#include "pch.h"
#include "Component/Public/ScriptComponent.h"
#include "Manager/Lua/Public/LuaManager.h"
#include "Render/UI/Widget/Public/ScriptComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(UScriptComponent, UActorComponent)

UScriptComponent::UScriptComponent()
{
    bCanEverTick = true;
}


void UScriptComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!ScriptName.IsNone())
    {
        AssignScript(ScriptName);
    }
}

void UScriptComponent::EndPlay()
{
    // Delegate 해제
    UnbindOwnerDelegates();

    ClearScript();
}

void UScriptComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    if (LuaEnv.valid())
    {
        CallLuaCallback("Tick", DeltaTime);
    	UpdateCoroutines(DeltaTime);
    }
}

void UScriptComponent::AssignScript(const FName& NewScriptName)
{
    UE_LOG("Assign Script: %s", NewScriptName.ToString().c_str());

    // 기존 바인딩 해제 (중복 방지)
    UnbindOwnerDelegates();

    ScriptName = NewScriptName;
    LuaEnv = ULuaManager::GetInstance().LoadLuaEnvironment(this, NewScriptName);

    if (LuaEnv.valid())
    {
        BeginLuaEnv();
    }
    else
    {
        UE_LOG_ERROR("[UScriptComponent] '%s' 스크립트 로드 실패", ScriptName.ToString().c_str());
        ScriptName = FName();
    }
}

void UScriptComponent::CreateAndAssignScript(const FName& NewScriptName)
{
    if (ScriptName == NewScriptName)
    {
        UE_LOG_ERROR("[UScriptComponent] 현재 할당된 스크립트와 이름(%s)이 동일함", NewScriptName.ToString().c_str());
        return;
    }
    ClearScript();

    LuaEnv = ULuaManager::GetInstance().CreateLuaEnvironment(this, NewScriptName);
    if (LuaEnv.valid())
    {
        ScriptName = NewScriptName;
        BeginLuaEnv();
    }
    else
    {
        UE_LOG_ERROR("[UScriptComponent] '%s' 스크립트 생성 실패", NewScriptName.ToString().c_str());
    }
}

void UScriptComponent::OpenCurrentScriptInEditor()
{
    if (ScriptName.IsNone())
    {
        UE_LOG("[UScriptComponent] 편집할 스크립트가 할당되지 않았습니다.");
        return;
    }
    ULuaManager::GetInstance().OpenScriptInEditor(ScriptName);
}

void UScriptComponent::ClearScript()
{
    // Delegate 해제
    UnbindOwnerDelegates();

    if (LuaEnv.valid())
    {
        CallLuaCallback("EndPlay");
    }

    if (!ScriptName.IsNone())
    {
        ULuaManager::GetInstance().UnregisterComponent(this, ScriptName);
    }

    LuaEnv = sol::environment{};
    ScriptName = FName();
}

void UScriptComponent::HotReload(sol::environment NewEnv)
{
    // 기존 Delegate 해제
    UnbindOwnerDelegates();

    if (LuaEnv.valid())
    {
        CallLuaCallback("EndPlay");
    }

    if (NewEnv.valid())
    {
        LuaEnv = NewEnv;
        BeginLuaEnv();
    }
    else
    {
        LuaEnv = sol::environment{};
    }
}

void UScriptComponent::BeginLuaEnv()
{
    LuaEnv["Owner"] = GetOwner();
    LuaEnv["Self"] = this;
    BindOwnerDelegates();

    CallLuaCallback("BeginPlay");
}

UClass* UScriptComponent::GetSpecificWidgetClass() const
{
    return UScriptComponentWidget::StaticClass();
}

UObject* UScriptComponent::Duplicate()
{
    UScriptComponent* Duplicated = Cast<UScriptComponent>(Super::Duplicate());
    Duplicated->ScriptName = ScriptName;
    return Duplicated;
}

void UScriptComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    if (bInIsLoading)
    {
        FString ScriptNameStr;
        FJsonSerializer::ReadString(InOutHandle, "ScriptName", ScriptNameStr, "");
        ScriptName = ScriptNameStr;

    }
    else
    {
        if (!ScriptName.IsNone())
        {
            InOutHandle["ScriptName"] = ScriptName.ToString();
        }
    }
}

void UScriptComponent::BindOwnerDelegates()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // 1. Actor의 Delegate 바인딩
    if (IDelegateProvider* ActorProvider = dynamic_cast<IDelegateProvider*>(Owner))
    {
        for (FDelegateInfoBase* DelegateInfo : ActorProvider->GetDelegates())
        {
            if (DelegateInfo)
            {
                uint32 BindingID = DelegateInfo->AddLuaHandler(this);
                if (BindingID != 0)
                {
                    BoundDelegates.Add({DelegateInfo, BindingID});
                }
            }
        }
    }

    // 2. 모든 Component의 Delegate 바인딩
    TArray<UActorComponent*>& Components = Owner->GetOwnedComponents();
    for (UActorComponent* Comp : Components)
    {
        if (Comp == this) continue;  // 자기 자신 제외

        if (IDelegateProvider* CompProvider = dynamic_cast<IDelegateProvider*>(Comp))
        {
            for (FDelegateInfoBase* DelegateInfo : CompProvider->GetDelegates())
            {
                if (DelegateInfo)
                {
                    uint32 BindingID = DelegateInfo->AddLuaHandler(this);
                    if (BindingID != 0)
                    {
                        BoundDelegates.Add({DelegateInfo, BindingID});
                    }
                }
            }
        }
    }
}

void UScriptComponent::UnbindOwnerDelegates()
{
    // 저장된 바인딩 ID로 명시적으로 제거
    for (const auto& Pair : BoundDelegates)
    {
        FDelegateInfoBase* DelegateInfo = Pair.first;
        uint32 BindingID = Pair.second;

        if (DelegateInfo)
        {
            DelegateInfo->Remove(BindingID);
        }
    }

    BoundDelegates.Empty();
}

void UScriptComponent::StartCoroutine(const std::string& FuncName)
{
	if (ActiveCoroutines.Num() > 100) { return; } // 현재 계속해서 들어오면 터지는 문제가 있어서 제한
	sol::function CoroutineFunc = LuaEnv[FuncName];
	if (!LuaEnv.valid() || !CoroutineFunc.valid())
	{
		UE_LOG_ERROR("[StartCoroutine] 유효하지 않은 Lua 함수");
		return;
	}

	sol::state_view Lua(LuaEnv.lua_state());
	sol::thread NewThread = sol::thread::create(Lua);
	sol::coroutine Coro = sol::coroutine(NewThread.state(), CoroutineFunc);

	FCoroutineHandle Handle;
	Handle.Thread = NewThread;
	Handle.Coroutine = Coro;
	Handle.GCRef = sol::make_reference(Lua, NewThread);
	Handle.FuncName = FString(FuncName);
	Handle.bIsRunning = true;

	ActiveCoroutines.Add(Handle);

	UE_LOG("[StartCoroutine] 코루틴 %s 시작", FuncName.c_str());
}

void UScriptComponent::StopCoroutine(const std::string& FuncName)
{
	for (auto& Handle : ActiveCoroutines)
	{
		if (Handle.FuncName == FName(FuncName))
		{
			Handle.bIsRunning = false;
			UE_LOG("[StopCoroutine] 코루틴 %s 중지", FuncName.c_str());
			return;
		}
	}
}

void UScriptComponent::StopAllCoroutines()
{
	ActiveCoroutines.Empty();
	UE_LOG("[StopAllCoroutines] 모든 코루틴 중지");
}

void UScriptComponent::UpdateCoroutines(float DeltaTime)
{
	TArray<int32> ToRemove;

	for (int32 Idx = 0; Idx < ActiveCoroutines.Num(); ++Idx)
	{
		auto& ActiveCoroutine = ActiveCoroutines[Idx];

		if (!ActiveCoroutine.bIsRunning)
		{
			ToRemove.Add(Idx);
			continue;
		}

		if (ActiveCoroutine.Time > 0)
		{
			ActiveCoroutine.Time -= DeltaTime;
			if (ActiveCoroutine.Time > 0)
				continue;
		}

		auto& Co = ActiveCoroutine.Coroutine; // ✅ 복사 금지!
		if (!Co.valid()) { continue; }
		auto Result = Co();

		if (!Result.valid())
		{
			sol::error Err = Result;
			UE_LOG_ERROR("[Coroutine %s] 에러: %s", ActiveCoroutine.FuncName.ToString().c_str(), Err.what());
			ActiveCoroutine.bIsRunning = false;
			ToRemove.Add(Idx);
			continue;
		}

		sol::call_status Status = Result.status();
		if (Status == sol::call_status::ok)
		{
			UE_LOG("[Coroutine %s] 정상 종료", ActiveCoroutine.FuncName.ToString().c_str());
			ActiveCoroutine.bIsRunning = false;
			ToRemove.Add(Idx);
		}
		else if (Status == sol::call_status::yielded)
		{
			if (Result.return_count() >= 1)
				ActiveCoroutine.Time = static_cast<float>(Result[0]);
		}
		else
		{
			UE_LOG_ERROR("[Coroutine %s] 비정상 상태 %d", ActiveCoroutine.FuncName.ToString().c_str(), (int32)Status);
			ActiveCoroutine.bIsRunning = false;
			ToRemove.Add(Idx);
		}
	}

	// 역순으로 제거 (RemoveAtSwap로 해도 OK)
	for (int32 i = ToRemove.Num() - 1; i >= 0; --i)
	{
		ActiveCoroutines.RemoveAtSwap(ToRemove[i]);
	}
}
