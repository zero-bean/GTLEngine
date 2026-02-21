#pragma once
#include "Source/Runtime/Core/Object/Object.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/Engine/Animation/AnimationTypes.h"
#include "Source/Runtime/Engine/Animation/AnimStateMachine.h"
#include "UAnimInstance.generated.h"

class USkeletalMeshComponent;

class UAnimInstance : public UObject
{
	GENERATED_REFLECTION_BODY()
public:
	UAnimInstance();
	virtual ~UAnimInstance();
	void TriggerAnimNotifies(float DeltaSeconds);
	virtual void Tick(float DeltaSeconds);
	void ChangeStatePending(UAnimState* AnimState, UAnimTransition* AnimTransition);
	void ChangeStatePending(UAnimState* AnimState, float InTransitionTime);

	UAnimState* AddSequenceInState(const FString& InName, UAnimSequence* Sequence, const float InBlendValue = 0.0f);

	void SetBlendValueInState(const FString& InName, const float InBlendValue);

	//UFUNCTION(LuaBind, DisplayName = "AddSequenceInState")
	UAnimState* AddSequenceInState(const FString& InName, const FString& AnimPath, const float InBlendValue = 0.0f);

	//UFUNCTION(LuaBind, DisplayName = "AddTransition")
	UAnimTransition* AddTransition(const FString& StartStateName, const FString& EndStateName);

	//UFUNCTION(LuaBind, DisplayName = "SetStartState")
	void SetStartState(const FString& StartStateName);

	void SetStateLoop(const FString& InName, const bool InLoop);
	void SetStateExitTime(const FString& InName, const float InExitTime);

	void SetOwner(USkeletalMeshComponent* InOwner)
	{
		OwnerComponent = InOwner;
	}

	//UFUNCTION(LuaBind, DisplayName = "SetSpeed")
	void SetStateSpeed(const FString& InName, const float InSpeed);
	void SetTime(const float InTime)
	{
		CurrentTime = InTime;
	}
	//UFUNCTION(LuaBind, DisplayName = "Play")
	void Play()
	{
		bPlay = true;
	}
	//UFUNCTION(LuaBind, DisplayName = "Pause")
	void Pause()
	{
		bPlay = false;
	}
	//UFUNCTION(LuaBind, DisplayName = "Replay")
	void Replay(const float StartTime)
	{
		bPlay = true;
		CurrentTime = StartTime;
	}
	FString GetCurrentStateName()
	{
		if (CurrentState == nullptr)
		{
			return "None";
		}
		return RegistChangeState->Name;
	}
	UAnimStateMachine& GetStateMachine()
	{
		return AnimStateMachine;
	}
	USkeletalMeshComponent* GetOwner() const { return OwnerComponent; }

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds);
	float CurrentTime = 0;
	float PrevTime = 0;
	bool bPlay = false;

	USkeletalMeshComponent* OwnerComponent = nullptr;
	UAnimStateMachine AnimStateMachine;

private:
	void ChangeState();
	FPoseContext CachedPose;
	UAnimState* CurrentState = nullptr;
	float TransitionTime = 0;
	float CurTransitionTime = 0;

	UAnimState* RegistChangeState = nullptr;
	float RegistChangeTransitionTime = 0.0f;

};