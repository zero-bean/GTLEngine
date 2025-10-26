#pragma once

#include "ShadowConfiguration.h"
#include "ShadowMap.h"

// Forward Declarations
class D3D11RHI;
class USpotLightComponent;
struct FMatrix;

/**
 * @brief 쉐도우 캐스팅이 가능한 라이트들을 타입별로 그룹화한 구조체
 */
struct FShadowCastingLights
{
	const TArray<UDirectionalLightComponent*>& DirectionalLights;
	const TArray<USpotLightComponent*>& SpotLights;
	const TArray<UPointLightComponent*>& PointLights;

	FShadowCastingLights(
		const TArray<UDirectionalLightComponent*>& InDirectionalLights,
		const TArray<USpotLightComponent*>& InSpotLights,
		const TArray<UPointLightComponent*>& InPointLights)
		: DirectionalLights(InDirectionalLights)
		, SpotLights(InSpotLights)
		, PointLights(InPointLights)
	{
	}
};

// Shadow 렌더링 컨텍스트
// BeginShadowRender 호출 시 반환되는 정보
struct FShadowRenderContext
{
	FMatrix LightView;
	FMatrix LightProjection;
	int32 ShadowMapIndex;

	FShadowRenderContext()
		: LightView(FMatrix::Identity())
		, LightProjection(FMatrix::Identity())
		, ShadowMapIndex(-1)
	{}
};

// Shadow 시스템 관리자
// FLightManager로부터 Shadow 관련 책임을 분리
// SRP (Single Responsibility Principle) 준수
class FShadowManager
{
public:
	FShadowManager();
	~FShadowManager();

	// Shadow 시스템 초기화
	// @param RHI - D3D11 RHI 디바이스
	// @param InConfig - Shadow 설정
	void Initialize(D3D11RHI* RHI, const FShadowConfiguration& InConfig = FShadowConfiguration::GetPlatformDefault());

	// Shadow 시스템 해제
	void Release();

    /**
    * @brief 매 프레임에 활성화된 라이트 목록을 기반으로 섀도우 맵 인덱스를 할당합니다.
    * @param RHIDevice - RHI 디바이스
    * @param ShadowLights - 타입별로 그룹화된 섀도우 캐스팅 라이트 구조체
    */
    void AssignShadowMapIndices(D3D11RHI* RHI, const FShadowCastingLights& InLights);

	// Shadow 렌더링 시작 - SpotLight
	// @param RHI - D3D11 RHI 디바이스
	// @param Light - 렌더링할 SpotLight
	// @param OutContext - (출력) Shadow 렌더링 컨텍스트
	// @return 성공 여부
	bool BeginShadowRender(D3D11RHI* RHI, USpotLightComponent* Light, FShadowRenderContext& OutContext);

	// Shadow 렌더링 시작 - DirectionalLight
	// @param RHI - D3D11 RHI 디바이스
	// @param Light - 렌더링할 DirectionalLight
	// @param CameraView - 카메라의 View 행렬
	// @param CameraProjection - 카메라의 Projection 행렬
	// @param OutContext - (출력) Shadow 렌더링 컨텍스트
	// @return 성공 여부
	bool BeginShadowRender(D3D11RHI* RHI, UDirectionalLightComponent* Light,
		const FMatrix& CameraView, const FMatrix& CameraProjection, FShadowRenderContext& OutContext);

	// Shadow 렌더링 종료
	// @param RHI - D3D11 RHI 디바이스
	void EndShadowRender(D3D11RHI* RHI);

	// Shadow Map 텍스처를 셰이더에 바인딩
	// @param RHI - D3D11 RHI 디바이스
	void BindShadowResources(D3D11RHI* RHI);

	// Shadow Map 텍스처 언바인딩
	// @param RHI - D3D11 RHI 디바이스
	void UnbindShadowResources(D3D11RHI* RHI);

	// Query 메서드들
	const FShadowConfiguration& GetShadowConfiguration() const { return Config; }
	int32 GetShadowMapIndex(USpotLightComponent* Light) const;
	FShadowMap& GetSpotLightShadowMap() { return SpotLightShadowMap; }
	const FShadowMap& GetSpotLightShadowMap() const { return SpotLightShadowMap; }

private:
	// RHI 디바이스 참조
	D3D11RHI* RHIDevice;

	// Shadow 설정
	FShadowConfiguration Config;

	// 초기화 플래그
	bool bIsInitialized = false;

	// Shadow Map 리소스
	FShadowMap SpotLightShadowMap;
	FShadowMap DirectionalLightShadowMap;
};
