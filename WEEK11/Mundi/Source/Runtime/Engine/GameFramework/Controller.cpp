#include "pch.h"
#include "Controller.h"
#include "Pawn.h"

void AController::Possess(APawn* InPawn)
{
	// 기존 Pawn이 있으면 먼저 해제
	if (Pawn) { UnPossess(); }

	// 새 Pawn 소유
	Pawn = InPawn;
	if (Pawn)
	{
		Pawn->SetController(this);
		Pawn->SetupPlayerInputComponent();
	}
}

void AController::UnPossess()
{
	if (Pawn)
	{
		Pawn->SetController(nullptr);
		Pawn = nullptr;
	}
}
