#pragma once
#include "Component/Public/SceneComponent.h"
#include "Core/Public/Delegate.h"

// Delegate 선언 - Lua 자동 바인딩 지원
// float CurrentLuminance, float PreviousLuminance
DECLARE_DELEGATE(FOnLightIntensityChanged,
	float, /* CurrentLuminance */
	float  /* PreviousLuminance */
);

/**
 * @brief 특정 월드 좌표의 조명 세기를 측정하는 Component
 *
 * GPU에서 Shadow Map을 포함한 정확한 조명 계산을 수행하여 Luminance 값을 반환합니다.
 * FLightSensorPass를 통해 배치 처리되며, FramesPerRequest 설정에 따라 주기적으로 측정합니다.
 *
 * 사용 예시:
 *   - 스텔스 게임: 플레이어가 어두운 곳에 숨었는지 판단
 *   - AI 시스템: NPC가 밝은 곳을 피해 이동
 *   - 환경 반응: 특정 오브젝트가 빛에 반응
 *
 * Lua에서 사용:
 *   function OnLightIntensityChanged(current, previous)
 *       if current > 0.8 then
 *           print("Bright - visible!")
 *       end
 *   end
 */
UCLASS()
class ULightSensorComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(ULightSensorComponent, USceneComponent)

public:
	ULightSensorComponent();
	virtual ~ULightSensorComponent() = default;

	/*-----------------------------------------------------------------------------
	    UObject Features
	 -----------------------------------------------------------------------------*/
public:
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	virtual UObject* Duplicate() override;

	/*-----------------------------------------------------------------------------
	    UActorComponent Features
	 -----------------------------------------------------------------------------*/
public:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;
	virtual void EndPlay() override;

	/*-----------------------------------------------------------------------------
	    Widget Features
	 -----------------------------------------------------------------------------*/
public:
	virtual UClass* GetSpecificWidgetClass() const override;

	/*-----------------------------------------------------------------------------
	    IDelegateProvider Implementation
	 -----------------------------------------------------------------------------*/
public:
	TArray<FDelegateInfoBase*> GetDelegates() const override;

	/*-----------------------------------------------------------------------------
	    ULightSensorComponent Features
	 -----------------------------------------------------------------------------*/
public:
	// --- Getters & Setters ---

	/**
	 * @brief 몇 프레임마다 조명 측정을 요청할지 설정
	 * @param InFrames 프레임 간격 (최소: 2)
	 */
	void SetFramesPerRequest(int32 InFrames) { FramesPerRequest = std::max(2, InFrames); }

	/**
	 * @brief 조명 측정 프레임 간격 반환
	 */
	int32 GetFramesPerRequest() const { return FramesPerRequest; }

	/**
	 * @brief 현재 측정된 Luminance 값 반환
	 */
	float GetCurrentLuminance() const { return CurrentLuminance; }

	/**
	 * @brief 이전 측정된 Luminance 값 반환
	 */
	float GetPreviousLuminance() const { return PreviousLuminance; }

	/**
	 * @brief 센서 활성화 여부 설정
	 */
	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

	/**
	 * @brief 센서 활성화 여부 반환
	 */
	bool GetEnabled() const { return bEnabled; }

	// --- Delegate ---

	/**
	 * @brief 조명 세기 변경 시 호출되는 Delegate
	 *
	 * @todo Component Delegate 패턴 구현 후 Lua 자동 바인딩 지원 예정
	 * 현재는 C++에서 직접 바인딩 필요:
	 * @code
	 * ULightSensorComponent* Sensor = ...;
	 * Sensor->OnLightIntensityChanged.AddLambda([](float Current, float Previous) {
	 *     UE_LOG("Luminance: %.2f", Current);
	 * });
	 * @endcode
	 *
	 * @param CurrentLuminance 현재 측정된 Luminance
	 * @param PreviousLuminance 이전 측정된 Luminance
	 */
	FOnLightIntensityChanged OnLightIntensityChanged;

private:
	/**
	 * @brief FLightSensorPass에 조명 측정 요청
	 */
	void RequestCalculation();

	/**
	 * @brief GPU 계산 완료 시 호출되는 Callback
	 * @param Luminance 측정된 조명 세기
	 */
	void OnCalculationComplete(float Luminance);

private:
	/** 몇 프레임마다 조명 측정을 요청할지 (최소: 2, 기본: 5) */
	int32 FramesPerRequest = 5;

	/** 현재 프레임 카운터 */
	int32 FrameCounter = 0;

	/** 현재 측정된 Luminance 값 */
	float CurrentLuminance = 0.0f;

	/** 이전 측정된 Luminance 값 (Delegate용) */
	float PreviousLuminance = 0.0f;

	/** 센서 활성화 여부 */
	bool bEnabled = true;

	/** 계산 요청 중인지 여부 (중복 요청 방지) */
	bool bPendingRequest = false;
};
