#include "pch.h"
#include "Pawn.h"
#include "InputComponent.h"
#include "MovementComponent.h"

APawn::APawn()
{
	// InputComponent 생성 - ActorComponent이므로 CreateDefaultSubobject 사용
	InputComponent = CreateDefaultSubobject<UInputComponent>("InputComponent");
}

void APawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APawn::SetupPlayerInputComponent()
{
	if (!InputComponent) { return; }
}

void APawn::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// OwnedComponents에서 컴포넌트 복원
	for (UActorComponent* Component : OwnedComponents)
	{
		if (UInputComponent* InputComp = Cast<UInputComponent>(Component))
		{
			InputComponent = InputComp;
		}
		else if (UMovementComponent* MoveComp = Cast<UMovementComponent>(Component))
		{
			MovementComponent = MoveComp;
		}
	}
}

void APawn::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// OwnedComponents에서 컴포넌트 복원
		for (UActorComponent* Component : OwnedComponents)
		{
			if (UInputComponent* InputComp = Cast<UInputComponent>(Component))
			{
				InputComponent = InputComp;
			}
			else if (UMovementComponent* MoveComp = Cast<UMovementComponent>(Component))
			{
				MovementComponent = MoveComp;
			}
		}

		// Controller는 런타임에 Possess()로 연결됨
	}
}
