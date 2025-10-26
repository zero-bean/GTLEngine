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

		// 쉐도우 맵 배열 상한선 검사
		if (CurrentShadowIndex >= static_cast<int32>(Config.MaxShadowCastingLights))
		{
			PointLight->SetShadowMapIndex(-1);
			continue;
		}

		// TODO
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

void FShadowManager::EndShadowRender(D3D11RHI* RHI)
{
	SpotLightShadowMap.EndRender(RHI);
	DirectionalLightShadowMap.EndRender(RHI);
}

void FShadowManager::BindShadowResources(D3D11RHI* RHI)
{
	// SpotLight Shadow Map Texture Array를 셰이더 슬롯 t5에 바인딩
	ID3D11ShaderResourceView* SpotShadowMapSRV = SpotLightShadowMap.GetSRV();
	RHI->GetDeviceContext()->PSSetShaderResources(5, 1, &SpotShadowMapSRV);

	// DirectionalLight Shadow Map Texture Array를 셰이더 슬롯 t6에 바인딩
	ID3D11ShaderResourceView* DirShadowMapSRV = DirectionalLightShadowMap.GetSRV();
	RHI->GetDeviceContext()->PSSetShaderResources(6, 1, &DirShadowMapSRV);

	// Shadow Comparison Sampler를 슬롯 s2에 바인딩
	ID3D11SamplerState* ShadowSampler = RHI->GetShadowComparisonSamplerState();
	RHI->GetDeviceContext()->PSSetSamplers(2, 1, &ShadowSampler);
}

void FShadowManager::UnbindShadowResources(D3D11RHI* RHI)
{
	// Shadow Map 언바인딩 (t5, t6 동시 해제)
	ID3D11ShaderResourceView* NullSRVs[2] = { nullptr, nullptr };
	RHI->GetDeviceContext()->PSSetShaderResources(5, 2, NullSRVs);

	// Shadow Sampler 언바인딩
	ID3D11SamplerState* NullSampler = nullptr;
	RHI->GetDeviceContext()->PSSetSamplers(2, 1, &NullSampler);
}

int32 FShadowManager::GetShadowMapIndex(USpotLightComponent* Light) const
{
	return Light ? Light->GetShadowMapIndex() : -1;
}
