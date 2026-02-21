#pragma once
#include "Core/Public/Object.h"

/**
 * @brief Subsystem base class
 * 모든 서브시스템의 기본 클래스
 */
UCLASS()
class USubsystem : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(USubsystem, UObject)

public:
	// 초기화 & 삭제에 대한 함수 제공
	// 1단계: 다른 서브시스템/매니저에 대한 의존성이 없는 자체 초기화
	virtual void Initialize()
	{
	}

	// 2단계: 모든 서브시스템이 생성된 후, 의존성을 갖는 초기화
	virtual void PostInitialize()
	{
	}

	virtual void Deinitialize()
	{
	}

	// Getter
	bool IsInitialized() const { return bIsInitialized; }

	// Special member function
	USubsystem();
	~USubsystem() override = default;

protected:
	// Initialized by subsystem only
	void SetInitialized(bool bInInitialized) { bIsInitialized = bInInitialized; }

private:
	bool bIsInitialized = false;
};
