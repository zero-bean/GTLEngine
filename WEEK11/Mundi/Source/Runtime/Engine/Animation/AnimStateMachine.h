#pragma once
#include "Source/Runtime/Engine/Animation/AnimState.h"
#include "Source/Runtime/Engine/Animation/AnimTransition.h"
#include "Source/Runtime/Engine/Animation/AnimationTypes.h"
#include "UAnimStateMachine.generated.h"

class UAnimInstance;

class UAnimStateMachine : public UObject
{
	GENERATED_REFLECTION_BODY()

public:
	UAnimStateMachine() = default;
	virtual ~UAnimStateMachine();
	void SetOwner(UAnimInstance* InOwner)
	{
		Owner = InOwner;
	}
	void StartStateMachine(const FString& StateName, const float InBlendTime = 0.0f);
	void StartStateMachine(UAnimState* StartState, const float InBlendTime = 0.0f);
	UAnimState* AddSequenceInState(const FString& InName, UAnimSequence* Sequence, float InBlendValue = 0.0f);
	void RemoveState(const FString& InName);

	UAnimTransition* AddTransition(const FString& StartStateName, const FString& EndStateName);
	UAnimTransition* AddTransition(UAnimState* StartState, UAnimState* EndState);
	void RemoveTransition(UAnimTransition* InTransition);

	void Tick(float DeltaSeconds);
	void SetCurrentState(UAnimState* InAnimState, UAnimTransition* InTransition);
	UAnimState* GetState(const FString& StateName);

	void EndState();


private:
	bool HasState(const FString& InName);

	//제거될 스테이트와 연결된 트랜지션 제거
	void RemoveStateTransition(UAnimState* RemoveState);
	UAnimInstance* Owner;
	UAnimState* CurrentState = nullptr;
	TArray<UAnimState*> States;
	TArray<UAnimTransition*> Transitions;
};