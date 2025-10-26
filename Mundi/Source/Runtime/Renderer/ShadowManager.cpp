#include "pch.h"
#include "ShadowManager.h"
#include "ShadowViewProjection.h"
#include "SpotLightComponent.h"
#include "DirectionalLightComponent.h"
#include "PointLightComponent.h"
#include "D3D11RHI.h"

FShadowManager::FShadowManager()
	: RHIDevice(nullptr)
{
}

FShadowManager::~FShadowManager()
{
	Release();
}

void FShadowManager::Initialize(D3D11RHI* RHI, const FShadowConfiguration& InConfig)
{
	if (bIsInitialized)
	{
		return;  // 이미 초기화되어 있으면 스킵
	}

	RHIDevice = RHI;
	Config = InConfig;

	// Shadow Map Array 초기화
	SpotLightShadowMap.Initialize(RHI, Config.ShadowMapResolution, Config.ShadowMapResolution, Config.MaxShadowCastingLights);
	DirectionalLightShadowMap.Initialize(RHI, Config.ShadowMapResolution, Config.ShadowMapResolution, Config.MaxShadowCastingLights);

	// PointLight Shadow Map 초기화 (Config.MaxShadowCastingLights 사용)
	// 주의: Cube Map은 메모리를 많이 사용함 (각 라이트당 6개 면)
	// Cube Map: 각 라이트당 6개 면 필요
	PointLightCubeShadowMap.Initialize(RHI, Config.ShadowMapResolution, Config.ShadowMapResolution, Config.MaxShadowCastingLights, true);

	// Paraboloid Map: 각 라이트당 2개 반구 필요
	PointLightParaboloidShadowMap.Initialize(RHI, Config.ShadowMapResolution, Config.ShadowMapResolution, Config.MaxShadowCastingLights * 2, false);

	bIsInitialized = true;
}

void FShadowManager::Release()
{
	if (!bIsInitialized)
	{
		return;  // 초기화되지 않았으면 스킵
	}

	SpotLightShadowMap.Release();
	DirectionalLightShadowMap.Release();
	PointLightCubeShadowMap.Release();
	PointLightParaboloidShadowMap.Release();

	bIsInitialized = false;
}

void FShadowManager::AssignShadowMapIndices(D3D11RHI* RHI, const FShadowCastingLights& InLights)
{
	// Lazy initialization: 최초 호출 시 ShadowMap 초기화
	if (!bIsInitialized)
	{
		Initialize(RHI, FShadowConfiguration::GetPlatformDefault());
	}

	// 매 프레임 초기화 (Dynamic 환경)
	uint32 CurrentShadowIndex = 0;

	// 1. DirectionalLight 처리
	uint32 DirectionalLightCount = 0;
	for (UDirectionalLightComponent* DirLight : InLights.DirectionalLights)
	{
		// 유효성 검사
		if (!DirLight ||
			!DirLight->IsVisible() ||
			!DirLight->GetOwner()->IsActorVisible() ||
			!DirLight->GetIsCastShadows())
		{
			if (DirLight) { DirLight->SetShadowMapIndex(-1); }
			continue;
		}

		// Week08: 방향광은 1개만 지원
		if (DirectionalLightCount >= 1)
		{
			DirLight->SetShadowMapIndex(-1);
			continue;
		}

		DirLight->SetShadowMapIndex(CurrentShadowIndex);
		CurrentShadowIndex++;
		DirectionalLightCount++;
	}

	// 2. PointLight 처리
	uint32 PointLightCount = 0;
	for (UPointLightComponent* PointLight : InLights.PointLights)
	{
		if (!PointLight ||
			!PointLight->IsVisible() ||
			!PointLight->GetOwner()->IsActorVisible() ||
			!PointLight->GetIsCastShadows())
		{
			if (PointLight) { PointLight->SetShadowMapIndex(-1); }
			continue;
		}

		// 섀도우맵 배열 상한선 검사 (Config 설정 사용)
		if (PointLightCount >= Config.MaxShadowCastingLights)
		{
			PointLight->SetShadowMapIndex(-1);
			continue;
		}

		// 섀도우맵 인덱스 할당 (PointLight는 별도 카운터 사용)
		PointLight->SetShadowMapIndex(PointLightCount);
		PointLightCount++;
	}

	// 3. SpotLight 처리
	for (USpotLightComponent* SpotLight : InLights.SpotLights)
	{
		// 유효성 검사
		if (!SpotLight ||
			!SpotLight->IsVisible() ||
			!SpotLight->GetOwner()->IsActorVisible() ||
			!SpotLight->GetIsCastShadows())
		{
			if (SpotLight) { SpotLight->SetShadowMapIndex(-1); }
			continue;
		}

		// 쉐도우 맵 배열 상한선 검사
		if (CurrentShadowIndex >= static_cast<int32>(Config.MaxShadowCastingLights))
		{
			SpotLight->SetShadowMapIndex(-1);
			continue;
		}

		SpotLight->SetShadowMapIndex(CurrentShadowIndex);
		CurrentShadowIndex++;
	}
}

bool FShadowManager::BeginShadowRender(D3D11RHI* RHI, USpotLightComponent* Light, FShadowRenderContext& OutContext)
{
	// 이 라이트가 Shadow Map을 할당받았는지 확인
	int32 Index = Light->GetShadowMapIndex();
	if (Index < 0)
	{
		return false;
	}

	// FShadowViewProjection 헬퍼 사용하여 VP 행렬 계산
	FShadowViewProjection ShadowVP = FShadowViewProjection::CreateForSpotLight(
		Light->GetWorldLocation(),
		Light->GetDirection(),
		Light->GetOuterConeAngle() * 2.0f,  // 전체 FOV
		Light->GetAttenuationRadius());

	// 출력 컨텍스트 설정
	OutContext.LightView = ShadowVP.View;
	OutContext.LightProjection = ShadowVP.Projection;
	OutContext.ShadowMapIndex = Index;

	// Shadow Map 렌더링 시작 (DSV 바인딩, Viewport 설정)
	SpotLightShadowMap.BeginRender(RHI, Index);

	return true;
}

bool FShadowManager::BeginShadowRender(D3D11RHI* RHI, UDirectionalLightComponent* Light,
	const FMatrix& CameraView, const FMatrix& CameraProjection, FShadowRenderContext& OutContext)
{
	// 이 라이트가 Shadow Map을 할당받았는지 확인
	int32 Index = Light->GetShadowMapIndex();
	if (Index < 0)
	{
		return false;
	}

	// FShadowViewProjection 헬퍼 사용하여 VP 행렬 계산 (Frustum 기반)
	FShadowViewProjection ShadowVP = FShadowViewProjection::CreateForDirectionalLight(
		Light->GetLightDirection(),
		CameraView,
		CameraProjection);

	// 출력 컨텍스트 설정
	OutContext.LightView = ShadowVP.View;
	OutContext.LightProjection = ShadowVP.Projection;
	OutContext.ShadowMapIndex = Index;

	// Light Component에 계산된 ViewProjection 저장 (Light Buffer 업데이트 시 사용)
	Light->SetLightViewProjection(ShadowVP.ViewProjection);

	// DirectionalLight Shadow Map 렌더링 시작 (DSV 바인딩, Viewport 설정)
	DirectionalLightShadowMap.BeginRender(RHI, Index);

	return true;
}

bool FShadowManager::BeginShadowRenderCube(D3D11RHI* RHI, UPointLightComponent* Light, uint32 CubeFaceIndex, FShadowRenderContext& OutContext)
{
	// 이 라이트가 Shadow Map을 할당받았는지 확인
	int32 Index = Light->GetShadowMapIndex();
	if (Index < 0)
	{
		return false;
	}

	// Cube Face 인덱스 유효성 검사
	if (CubeFaceIndex >= 6)
	{
		return false;
	}

	// FShadowViewProjection 헬퍼 사용하여 6개 VP 행렬 계산
	TArray<FShadowViewProjection> ShadowVPs = FShadowViewProjection::CreateForPointLightCube(
		Light->GetWorldLocation(),
		Light->GetAttenuationRadius());

	// 해당 Cube Face의 VP 행렬 사용
	FShadowViewProjection& ShadowVP = ShadowVPs[CubeFaceIndex];

	// 출력 컨텍스트 설정
	OutContext.LightView = ShadowVP.View;
	OutContext.LightProjection = ShadowVP.Projection;
	OutContext.ShadowMapIndex = Index;

	// Cube Shadow Map 렌더링 시작
	// ArrayIndex = (LightIndex * 6) + CubeFaceIndex
	uint32 ArrayIndex = (Index * 6) + CubeFaceIndex;
	PointLightCubeShadowMap.BeginRender(RHI, ArrayIndex);

	return true;
}

bool FShadowManager::BeginShadowRenderParaboloid(D3D11RHI* RHI, UPointLightComponent* Light, bool bFrontHemisphere, FShadowRenderContext& OutContext)
{
	// 이 라이트가 Shadow Map을 할당받았는지 확인
	int32 Index = Light->GetShadowMapIndex();
	if (Index < 0)
	{
		return false;
	}

	// FShadowViewProjection 헬퍼 사용하여 2개 VP 행렬 계산
	TArray<FShadowViewProjection> ShadowVPs = FShadowViewProjection::CreateForPointLightParaboloid(
		Light->GetWorldLocation(),
		Light->GetAttenuationRadius());

	// 전면(0) 또는 후면(1) 반구 선택
	uint32 HemisphereIndex = bFrontHemisphere ? 0 : 1;
	FShadowViewProjection& ShadowVP = ShadowVPs[HemisphereIndex];

	// 출력 컨텍스트 설정
	OutContext.LightView = ShadowVP.View;
	OutContext.LightProjection = ShadowVP.Projection;
	OutContext.ShadowMapIndex = Index;

	// Paraboloid Shadow Map 렌더링 시작
	// ArrayIndex = (LightIndex * 2) + HemisphereIndex
	uint32 ArrayIndex = (Index * 2) + HemisphereIndex;
	PointLightParaboloidShadowMap.BeginRender(RHI, ArrayIndex);

	return true;
}

void FShadowManager::EndShadowRender(D3D11RHI* RHI)
{
	SpotLightShadowMap.EndRender(RHI);
	DirectionalLightShadowMap.EndRender(RHI);
	PointLightCubeShadowMap.EndRender(RHI);
	PointLightParaboloidShadowMap.EndRender(RHI);
}

void FShadowManager::BindShadowResources(D3D11RHI* RHI)
{
	// SpotLight Shadow Map Texture Array를 셰이더 슬롯 t5에 바인딩
	ID3D11ShaderResourceView* SpotShadowMapSRV = SpotLightShadowMap.GetSRV();
	RHI->GetDeviceContext()->PSSetShaderResources(5, 1, &SpotShadowMapSRV);

	// DirectionalLight Shadow Map Texture Array를 셰이더 슬롯 t6에 바인딩
	ID3D11ShaderResourceView* DirShadowMapSRV = DirectionalLightShadowMap.GetSRV();
	RHI->GetDeviceContext()->PSSetShaderResources(6, 1, &DirShadowMapSRV);

	// PointLight Cube Shadow Map Texture Cube Array를 셰이더 슬롯 t7에 바인딩
	ID3D11ShaderResourceView* PointCubeShadowMapSRV = PointLightCubeShadowMap.GetSRV();
	RHI->GetDeviceContext()->PSSetShaderResources(7, 1, &PointCubeShadowMapSRV);

	// PointLight Paraboloid Shadow Map Texture Array를 셰이더 슬롯 t8에 바인딩
	ID3D11ShaderResourceView* PointParaboloidShadowMapSRV = PointLightParaboloidShadowMap.GetSRV();
	RHI->GetDeviceContext()->PSSetShaderResources(8, 1, &PointParaboloidShadowMapSRV);

	// Shadow Comparison Sampler를 슬롯 s2에 바인딩
	ID3D11SamplerState* ShadowSampler = RHI->GetShadowComparisonSamplerState();
	RHI->GetDeviceContext()->PSSetSamplers(2, 1, &ShadowSampler);
}

void FShadowManager::UnbindShadowResources(D3D11RHI* RHI)
{
	// Shadow Map 언바인딩 (t5, t6, t7, t8 동시 해제)
	ID3D11ShaderResourceView* NullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
	RHI->GetDeviceContext()->PSSetShaderResources(5, 4, NullSRVs);

	// Shadow Sampler 언바인딩
	ID3D11SamplerState* NullSampler = nullptr;
	RHI->GetDeviceContext()->PSSetSamplers(2, 1, &NullSampler);
}

int32 FShadowManager::GetShadowMapIndex(USpotLightComponent* Light) const
{
	return Light ? Light->GetShadowMapIndex() : -1;
}
