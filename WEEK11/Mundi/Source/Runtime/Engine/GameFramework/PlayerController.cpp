#include "pch.h"
#include "PlayerController.h"
#include "Pawn.h"
#include "InputComponent.h"

void APlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Pawn의 InputComponent 처리
	if (Pawn && Pawn->InputComponent)
	{
		Pawn->InputComponent->ProcessInput();
	}
}
