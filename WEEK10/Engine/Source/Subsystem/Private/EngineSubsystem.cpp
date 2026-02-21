#include "pch.h"
#include "Subsystem/Public/EngineSubsystem.h"

IMPLEMENT_CLASS(UEngineSubsystem, USubsystem)

UEngineSubsystem::UEngineSubsystem()
{
}

void UEngineSubsystem::Initialize()
{
	USubsystem::Initialize();
	SetInitialized(true);
}

void UEngineSubsystem::Deinitialize()
{
	USubsystem::Deinitialize();
	SetInitialized(false);
}
