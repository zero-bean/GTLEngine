#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimStateMachine.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"


IMPLEMENT_CLASS(UAnimStateMachine)

UAnimStateMachine::~UAnimStateMachine()
{
	for (UAnimState* State : States)
	{
		DeleteObject(State);
	}
	States.clear();
	for (UAnimTransition* Transition : Transitions)
	{
		DeleteObject(Transition);
	}
	Transitions.clear();
}

void UAnimStateMachine::Tick(float DeltaSeconds)
{
	if (CurrentState == nullptr)
	{
		return;
	}

	for (UAnimTransition* Transition : Transitions)
	{
		if (Transition->From == CurrentState && Transition->CanEnter())
		{
			SetCurrentState(Transition->To, Transition);
			break;
		}
	}
}

void UAnimStateMachine::EndState()
{
	for (UAnimTransition* Transition : Transitions)
	{
		if (Transition->From == CurrentState && Transition->Condition == nullptr)
		{
			SetCurrentState(Transition->To, Transition);
			break;
		}
	}
}

void UAnimStateMachine::SetCurrentState(UAnimState* InAnimState, UAnimTransition* InTransition)
{
	CurrentState = InAnimState;
	Owner->ChangeStatePending(CurrentState, InTransition);
}
UAnimState* UAnimStateMachine::GetState(const FString& StateName)
{
	for (UAnimState* State : States)
	{
		if (State->Name == StateName)
		{
			return State;
		}
	}
	return nullptr;
}

void UAnimStateMachine::StartStateMachine(const FString& StateName, const float InBlendTime)
{
	StartStateMachine(GetState(StateName), InBlendTime);
}
void UAnimStateMachine::StartStateMachine(UAnimState* StartState, const float InBlendTime)
{
	if (States.Contains(StartState) == false)
	{
		//없으면 트랜지션이 연결 안되어 있기 때문에 안되도록 처리했음
		return;
	}
	CurrentState = StartState;
	Owner->ChangeStatePending(CurrentState, InBlendTime);
}

UAnimState* UAnimStateMachine::AddSequenceInState(const FString& InName, UAnimSequence* Sequence, float InBlendValue)
{
	UAnimState* State = GetState(InName);
	if (State == nullptr)
	{
		State = NewObject<UAnimState>();
		State->SetName(InName);
		States.Push(State);
	}
	State->AddSequence(Sequence, InBlendValue);
	return State;
}

void UAnimStateMachine::RemoveState(const FString& InName)
{
	int StateNum = States.Num();
	for (int i = StateNum - 1; i >= 0; --i)
	{
		if (States[i]->Name == InName)
		{
			RemoveStateTransition(States[i]);
			States.RemoveAt(i);
			DeleteObject(States[i]);
		}
	}
}

UAnimTransition* UAnimStateMachine::AddTransition(const FString& StartStateName, const FString& EndStateName)
{
	UAnimState* StartState = GetState(StartStateName);
	UAnimState* EndState = GetState(EndStateName);
	if (StartState && EndState)
	{
		return AddTransition(StartState, EndState);
	}
	return nullptr;
}
UAnimTransition* UAnimStateMachine::AddTransition(UAnimState* StartState, UAnimState* EndState)
{
	if (States.Contains(StartState) && States.Contains(EndState))
	{
		UAnimTransition* Transition = NewObject<UAnimTransition>();
		Transition->SetRoot(StartState, EndState);
		Transitions.Push(Transition);
		return Transition;
	}
	return nullptr;
}

void UAnimStateMachine::RemoveTransition(UAnimTransition* InTransition)
{
	Transitions.Remove(InTransition);
	DeleteObject(InTransition);
}


bool UAnimStateMachine::HasState(const FString& InName)
{
	for (UAnimState* State : States)
	{
		if (State->Name == InName)
		{
			return true;
		}
	}
	return false;
}

void UAnimStateMachine::RemoveStateTransition(UAnimState* RemoveState)
{
	int TransitionCount = Transitions.Num();
	for (int i = TransitionCount - 1; i >= 0; i--)
	{
		UAnimTransition* Transition = Transitions[i];
		if (Transition->To == RemoveState || Transition->From == RemoveState)
		{
			Transitions.RemoveAt(i);
			DeleteObject(Transitions[i]);
		}
	}
}
