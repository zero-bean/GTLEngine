#pragma once

#include "UEngineSubsystem.h"

enum class EEngineShowFlags : uint64
{
	SF_None				= 0,
	SF_Primitives		= 1 << 0,
	SF_BillboardText	= 1 << 1,
	SF_All				= ~SF_None
};

class UShowFlagManager : public UEngineSubsystem
{
	DECLARE_UCLASS(UShowFlagManager, UEngineSubsystem)
public:
	UShowFlagManager();

	// 특정 플래그를 킨다
	void Set(EEngineShowFlags Flag);

	// 특정 플래그를 끊다
	void Clear(EEngineShowFlags Flag);

	// 특정 플래그의 상태를 반전
	void Toggle(EEngineShowFlags Flag);

	// 특정 플래그가 켜져 있는지 확인
	bool IsEnabled(EEngineShowFlags Flag) const;

private:
	// 모든 플래그의 ON/OFF 비트셋
	uint64 ShowFlags;
};
