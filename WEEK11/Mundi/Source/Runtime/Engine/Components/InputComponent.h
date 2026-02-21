#pragma once
#include "ActorComponent.h"
#include "UInputComponent.generated.h"

// TODO: 해당 클래스의 바인딩 로직은 모든 상황에 대하여 커버를 하지 않습니다.
// - 일반 함수: 8바이트
// - virtual 함수: 16바이트 (지원)
// - virtual inheritance: 24바이트 이상 (지원 X)
// - 컴파일러마다 최대 32바이트까지 변형이 존재할 수도 있음.
constexpr size_t MAX_MEMBER_FUNC_PTR_SIZE = 16;

UCLASS(DisplayName = "인풋컴포넌트", Description = "사용자의 제어를 입력받는 오브젝트입니다.")
class UInputComponent : public UActorComponent
{
public:
	GENERATED_REFLECTION_BODY()

	UInputComponent() = default;

	// C++에서 사용 - 멤버 함수 포인터로 바인딩
	template<class UserClass>
	void BindAxis(const FString& AxisName, int KeyCode, float Scale, UserClass* Object, void(UserClass::*Func)(float))
	{
		// 컴파일 타임 크기 검증
		static_assert(sizeof(Func) <= MAX_MEMBER_FUNC_PTR_SIZE,
			"Member function pointer too large! "
			"Do NOT use virtual inheritance. Virtual functions are supported.");

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
		Binding.FunctionName = "";  // C++에서는 함수 이름 불필요

		// 안전하게 버퍼에 저장
		memcpy(Binding.FunctionPtrStorage, &Func, sizeof(Func));

		// 호출 래퍼: 버퍼에서 타입을 복원해서 실제 함수 호출
		Binding.ExecuteAxis = [](UObject* Obj, const char* Storage, float Value) {
			using FuncType = void(UserClass::*)(float);
			FuncType TypedFunc{};
			memcpy(&TypedFunc, Storage, sizeof(TypedFunc));
			(static_cast<UserClass*>(Obj)->*TypedFunc)(Value);
		}; 

		AxisBindings.Add(Binding);
	}

	template<class UserClass>
	void BindAction(const FString& ActionName, int KeyCode, UserClass* Object, void(UserClass::* Func)())
	{
		// 컴파일 타임 크기 검증
		static_assert(sizeof(Func) <= MAX_MEMBER_FUNC_PTR_SIZE,
			"Member function pointer too large! "
			"Do NOT use virtual inheritance. Virtual functions are supported.");

		// 중복 제거: 같은 KeyCode + 같은 Object 조합이면 기존 바인딩 삭제
		ActionBindings.erase(std::remove_if(ActionBindings.begin(), ActionBindings.end(),
			[&](const FActionBinding& B) {return B.KeyCode == KeyCode && B.Object == Object;}),
			ActionBindings.end()
		);

		FActionBinding Binding;
		Binding.ActionName = ActionName;
		Binding.KeyCode = KeyCode;
		Binding.Object = Object;
		Binding.FunctionName = "";  // C++에서는 함수 이름 불필요

		// 안전하게 버퍼에 저장
		memcpy(Binding.FunctionPtrStorage, &Func, sizeof(Func));

		// 호출 래퍼: 버퍼에서 타입을 복원해서 실제 함수 호출
		Binding.ExecuteAction = [](UObject* Obj, const char* Storage) {
			using FuncType = void(UserClass::*)();
			FuncType TypedFunc{};
			memcpy(&TypedFunc, Storage, sizeof(TypedFunc));
			(static_cast<UserClass*>(Obj)->*TypedFunc)();
		};

		ActionBindings.Add(Binding);
	}

	// 루아 스크립트 연결용 래핑 함수
	void BindAxisByName(const FString& AxisName, int KeyCode, float Scale, UObject* Object, const FString& FunctionName)
	{
		BindAxisInternal(AxisName, KeyCode, Scale, Object, FunctionName);
	}

	// 루아 스크립트 연결용 래핑 함수
	void BindActionByName(const FString& ActionName, int KeyCode, UObject* Object, const FString& FunctionName)
	{
		BindActionInternal(ActionName, KeyCode, Object, FunctionName);
	}

	void ProcessInput();

protected:
	virtual ~UInputComponent() override = default;

private:
	// 루아 스크립트의 BindAxisByName 호출을 처리하는 함수
	void BindAxisInternal(const FString& AxisName, int KeyCode, float Scale, UObject* Object, const FString& FuncName);

	// 루아 스크립트의 BindActionByName 호출을 처리하는 함수
	void BindActionInternal(const FString& ActionName, int KeyCode, UObject* Object, const FString& FuncName);

	// 키가 눌려있는 기간, 매 프레임 처리를 위한 구조체
	struct FAxisBinding
	{
		FString AxisName;
		int KeyCode;
		float Scale;
		UObject* Object;  // 콜백 호출할 객체
		FString FunctionName;  // 루아용: 함수 이름 문자열
		alignas(16) char FunctionPtrStorage[MAX_MEMBER_FUNC_PTR_SIZE] = {0}; // C++
		void (*ExecuteAxis)(UObject*, const char*, float) = nullptr;  // 호출 래퍼
	};

	// 키가 눌린 순간, 1회성 처리를 위한 구조체
	struct FActionBinding
	{
		FString ActionName;
		int KeyCode;
		UObject* Object;  // 콜백 호출할 객체
		FString FunctionName;  // 루아용: 함수 이름 문자열
		alignas(16) char FunctionPtrStorage[MAX_MEMBER_FUNC_PTR_SIZE] = {0};  // C++
		void (*ExecuteAction)(UObject*, const char*) = nullptr;  // 호출 래퍼
	};

	TArray<FAxisBinding> AxisBindings;
	TArray<FActionBinding> ActionBindings;
};

