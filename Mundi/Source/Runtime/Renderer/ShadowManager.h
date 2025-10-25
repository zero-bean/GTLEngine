#pragma once

#include "ShadowConfiguration.h"

// Forward Declarations
class D3D11RHI;
class FShadowMap;
class USpotLightComponent;
struct FMatrix;

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

	// Shadow Map 인덱스 할당
	// @param RHI - D3D11 RHI 디바이스
	// @param VisibleSpotLights - 현재 뷰에 보이는 SpotLight 목록
	void AssignShadowIndices(D3D11RHI* RHI, const TArray<USpotLightComponent*>& VisibleSpotLights);

	// Shadow 렌더링 시작
	// @param RHI - D3D11 RHI 디바이스
	// @param Light - 렌더링할 SpotLight
	// @param OutContext - (출력) Shadow 렌더링 컨텍스트
	// @return 성공 여부
	bool BeginShadowRender(D3D11RHI* RHI, USpotLightComponent* Light, FShadowRenderContext& OutContext);

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
	FShadowMap* GetShadowMapArray() const { return ShadowMapArray; }

private:
	// RHI 디바이스 참조
	D3D11RHI* RHIDevice;

	// Shadow 설정
	FShadowConfiguration Config;

	// Shadow Map 리소스
	FShadowMap* ShadowMapArray;

	// 라이트 → Shadow Map Index 매핑
	TMap<USpotLightComponent*, int32> LightToShadowIndex;
};
