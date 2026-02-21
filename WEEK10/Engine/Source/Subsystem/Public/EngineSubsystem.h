#pragma once
#include "Subsystem.h"

/**
 * @brief 엔진 서브시스템 기본 클래스
 * StandAlone 모드에서 엔진 생명주기 동안 지속되는 서브시스템
 */
UCLASS()
class UEngineSubsystem : public USubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UEngineSubsystem, USubsystem)

public:
	// USubsystem interface
	void Initialize() override;
	void Deinitialize() override;

	// Special member function
	UEngineSubsystem();
	~UEngineSubsystem() override = default;
};
