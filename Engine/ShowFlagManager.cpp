#include "stdafx.h"
#include "ShowFlagManager.h"

IMPLEMENT_UCLASS(UShowFlagManager, UEngineSubsystem)

UShowFlagManager::UShowFlagManager()
	: ShowFlags(static_cast<uint64>(EEngineShowFlags::SF_All))
{}

void UShowFlagManager::Set(EEngineShowFlags Flag)
{
	// OR 연산: 해당 비트만 1로 만듭니다.
	ShowFlags |= static_cast<uint64>(Flag);
}

void UShowFlagManager::Clear(EEngineShowFlags Flag)
{
	// AND 연산과 NOT 연산: 해당 비트만 0으로 만듭니다. 
	ShowFlags &= ~static_cast<uint64>(Flag);
}

void UShowFlagManager::Toggle(EEngineShowFlags Flag)
{
	// XOR 연산: 해당 비트만 반전시킵니다.
	ShowFlags ^= static_cast<uint64>(Flag);
}

bool UShowFlagManager::IsEnabled(EEngineShowFlags Flag) const
{
	// AND 연산: 해당 비트가 1인지 확인합니다. 결과가 0이 아니면 켜진 상태입니다.
	return (ShowFlags & static_cast<uint64>(Flag)) != 0;
}
