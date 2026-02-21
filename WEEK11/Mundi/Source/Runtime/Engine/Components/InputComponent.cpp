#include "pch.h"
#include "InputComponent.h"
#include "InputManager.h"
#include "LuaBindHelpers.h"
#include "LuaManager.h"
#include "Actor.h"

// 루아 바인딩 등록
LUA_BIND_BEGIN(UInputComponent)
{
	AddMethod(T, "BindAxis", &UInputComponent::BindAxisByName);
	AddMethod(T, "BindAction", &UInputComponent::BindActionByName);
}
LUA_BIND_END()

void UInputComponent::ProcessInput()
{
	UInputManager& Input = UInputManager::GetInstance();

	// 축 입력 처리 (연속 입력 - IsKeyDown)
	for (const FAxisBinding& Binding : AxisBindings)
	{
		if (Input.IsKeyDown(Binding.KeyCode))
		{
			if (Binding.Object)
			{
				// C++ 바인딩: 함수 포인터로 직접 호출
				if (Binding.ExecuteAxis)
				{
					Binding.ExecuteAxis(Binding.Object, Binding.FunctionPtrStorage, Binding.Scale);
				}
				// 루아 바인딩: 함수 이름으로 호출
				else if (!Binding.FunctionName.empty())
				{
					// 루아 함수 호출 (AActor만 지원)
					AActor* Actor = Cast<AActor>(Binding.Object);
					if (Actor && Actor->GetWorld())
					{
						FLuaManager* LuaManamer = Actor->GetWorld()->GetLuaManager();
						if (!LuaManamer) { continue; }

						sol::state_view Lua = LuaManamer->GetState();

						// 객체의 클래스 함수 테이블 가져오기
						sol::table FuncTable = FLuaBindRegistry::Get().EnsureTable(Lua, Binding.Object->GetClass());

						// 함수 이름으로 함수 찾기
						sol::object FuncObj = FuncTable[Binding.FunctionName.c_str()];
						if (FuncObj.valid() && FuncObj.is<sol::function>())
						{
							sol::function LuaFunc = FuncObj.as<sol::function>();

							// LuaComponentProxy 생성해서 this 전달
							LuaComponentProxy Proxy;
							Proxy.Instance = Binding.Object;
							Proxy.Class = Binding.Object->GetClass();

							// 루아 함수 호출: function(self, value)
							auto Result = LuaFunc(Proxy, Binding.Scale);
							if (!Result.valid())
							{
								sol::error Err = Result;
								UE_LOG("Lua function call failed: %s", Err.what());
							}
						}
					}
				}
			}
		}
	}

	// 액션 입력 처리 (1회 입력 - IsKeyPressed)
	for (const FActionBinding& Binding : ActionBindings)
	{
		if (Input.IsKeyPressed(Binding.KeyCode))
		{
			if (Binding.Object)
			{
				// C++ 바인딩: 함수 포인터로 직접 호출
				if (Binding.ExecuteAction)
				{
					Binding.ExecuteAction(Binding.Object, Binding.FunctionPtrStorage);
				}
				// 루아 바인딩: 함수 이름으로 호출
				else if (!Binding.FunctionName.empty())
				{
					// 루아 함수 호출 (AActor만 지원)
					AActor* Actor = Cast<AActor>(Binding.Object);
					if (Actor && Actor->GetWorld())
					{
						FLuaManager* LuaManamer = Actor->GetWorld()->GetLuaManager();
						if (!LuaManamer) { continue; }

						sol::state_view Lua = LuaManamer->GetState();

						// 객체의 클래스 함수 테이블 가져오기
						sol::table FuncTable = FLuaBindRegistry::Get().EnsureTable(Lua, Binding.Object->GetClass());

						// 함수 이름으로 함수 찾기
						sol::object FuncObj = FuncTable[Binding.FunctionName.c_str()];
						if (FuncObj.valid() && FuncObj.is<sol::function>())
						{
							sol::function LuaFunc = FuncObj.as<sol::function>();

							// LuaComponentProxy 생성해서 this 전달
							LuaComponentProxy Proxy;
							Proxy.Instance = Binding.Object;
							Proxy.Class = Binding.Object->GetClass();

							// 루아 함수 호출: function(self)
							auto Result = LuaFunc(Proxy);
							if (!Result.valid())
							{
								sol::error Err = Result;
								UE_LOG("Lua function call failed: %s", Err.what());
							}
						}
					}
				}
			}
		}
	}
}

void UInputComponent::BindAxisInternal(const FString& AxisName, int KeyCode, float Scale, UObject* Object, const FString& FuncName)
{
	// 중복 제거: 같은 KeyCode + 같은 Object 조합이면 기존 바인딩 삭제
	AxisBindings.erase(std::remove_if(AxisBindings.begin(), AxisBindings.end(),
		[&](const FAxisBinding& B) {return B.KeyCode == KeyCode && B.Object == Object;}),
		AxisBindings.end()
	);

	FAxisBinding Binding;
	Binding.AxisName = AxisName;
	Binding.KeyCode = KeyCode;
	Binding.Scale = Scale;
	Binding.Object = Object;
	Binding.FunctionName = FuncName;
	AxisBindings.Add(Binding);
}

void UInputComponent::BindActionInternal(const FString& ActionName, int KeyCode, UObject* Object, const FString& FuncName)
{
	// 중복 제거: 같은 KeyCode + 같은 Object 조합이면 기존 바인딩 삭제
	ActionBindings.erase(std::remove_if(ActionBindings.begin(), ActionBindings.end(),
		[&](const FActionBinding& B) {return B.KeyCode == KeyCode && B.Object == Object;}),
		ActionBindings.end()
	);

	FActionBinding Binding;
	Binding.ActionName = ActionName;
	Binding.KeyCode = KeyCode;
	Binding.Object = Object;
	Binding.FunctionName = FuncName;
	ActionBindings.Add(Binding);
}
