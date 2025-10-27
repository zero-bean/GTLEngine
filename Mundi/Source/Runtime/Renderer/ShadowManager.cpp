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
	RHIDevice = RHI;
	Config = InConfig;

	// Shadow Map Array 초기화 (광원 타입별로 각각의 해상도 사용)
	SpotLightShadowMap.Initialize(RHI, Config.SpotLightResolution, Config.SpotLightResolution, Config.MaxSpotLights);
	DirectionalLightShadowMap.Initialize(RHI, Config.DirectionalLightResolution, Config.DirectionalLightResolution, Config.MaxDirectionalLights);
	PointLightCubeShadowMap.Initialize(RHI, Config.PointLightResolution, Config.PointLightResolution, Config.MaxPointLights, true);

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

	bIsInitialized = false;
}

void FShadowManager::SetDirectionalLightResolution(uint32 NewResolution)
{
	// 초기화되지 않았으면 설정만 변경
	if (!bIsInitialized)
	{
		Config.DirectionalLightResolution = NewResolution;
		return;
	}

	// 이미 초기화된 경우 재초기화
	Config.DirectionalLightResolution = NewResolution;
	Release();
	Initialize(RHIDevice, Config);
}

void FShadowManager::SetSpotLightResolution(uint32 NewResolution)
{
	// 초기화되지 않았으면 설정만 변경
	if (!bIsInitialized)
	{
		Config.SpotLightResolution = NewResolution;
		return;
	}

	// 이미 초기화된 경우 재초기화
	Config.SpotLightResolution = NewResolution;
	Release();
	Initialize(RHIDevice, Config);
}

void FShadowManager::SetPointLightResolution(uint32 NewResolution)
{
	// 초기화되지 않았으면 설정만 변경
	if (!bIsInitialized)
	{
		Config.PointLightResolution = NewResolution;
		return;
	}

	// 이미 초기화된 경우 재초기화
	Config.PointLightResolution = NewResolution;
	Release();
	Initialize(RHIDevice, Config);
}

void FShadowManager::AssignShadowMapIndices(D3D11RHI* RHI, const FShadowCastingLights& InLights)
{
	// Lazy initialization: 최초 호출 시 ShadowMap 초기화
	if (!bIsInitialized)
	{
		Initialize(RHI, FShadowConfiguration::GetPlatformDefault());
	}

	// 매 프레임 초기화 (Dynamic 환경) - 광원별로 독립적인 인덱스 사용
	uint32 DirectionalLightIndex = 0;
	uint32 SpotLightIndex = 0;
	uint32 PointLightIndex = 0;

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

		// 방향광 최대 개수 검사
		if (DirectionalLightIndex >= Config.MaxDirectionalLights)
		{
			DirLight->SetShadowMapIndex(-1);
			continue;
		}

		DirLight->SetShadowMapIndex(DirectionalLightIndex);
		DirectionalLightIndex++;
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

		// 포인트 라이트 최대 개수 검사
		if (PointLightIndex >= Config.MaxPointLights)
		{
			PointLight->SetShadowMapIndex(-1);
			continue;
		}

		// 섀도우맵 인덱스 할당 (PointLight는 별도 카운터 사용)
		PointLight->SetShadowMapIndex(PointLightCount);
		PointLightCount++;
	}

	// 3. SpotLight 처리
	uint32 SpotLightCount = 0;
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

		// 스팟 라이트 최대 개수 검사
		if (SpotLightIndex >= Config.MaxSpotLights)
		{
			SpotLight->SetShadowMapIndex(-1);
			continue;
		}

		SpotLight->SetShadowMapIndex(SpotLightIndex);
		SpotLightIndex++;
		SpotLightCount++;
	}

	// 쉐도우 맵 통계 수집 및 업데이트
	FShadowStats Stats;
	Stats.MaxShadowCastingLights = Config.MaxShadowCastingLights;

	// 광원별 해상도
	Stats.DirectionalLightResolution = DirectionalLightShadowMap.GetWidth();
	Stats.SpotLightResolution = SpotLightShadowMap.GetWidth();
	Stats.PointLightResolution = PointLightCubeShadowMap.GetWidth();

	// 실제 쉐도우 캐스팅 라이트 수
	Stats.DirectionalLightCount = DirectionalLightCount;
	Stats.SpotLightCount = SpotLightCount;
	Stats.PointLightCount = PointLightCount;

	// 각 쉐도우 맵의 할당된 메모리 계산 (전체 텍스처 배열 크기)
	Stats.DirectionalLightAllocatedBytes = DirectionalLightShadowMap.GetAllocatedMemoryBytes();
	Stats.SpotLightAllocatedBytes = SpotLightShadowMap.GetAllocatedMemoryBytes();
	Stats.PointLightAllocatedBytes = 0; // TODO: PointLight 쉐도우 맵 구현 시 추가

	// 각 쉐도우 맵의 실제 사용 중인 메모리 계산 (활성 라이트 수 기반)
	Stats.DirectionalLightUsedBytes = DirectionalLightShadowMap.GetUsedMemoryBytes(DirectionalLightCount);
	Stats.SpotLightUsedBytes = SpotLightShadowMap.GetUsedMemoryBytes(SpotLightCount);
	Stats.PointLightUsedBytes = 0; // TODO: PointLight 쉐도우 맵 구현 시 추가

	// 총 할당된 메모리
	Stats.TotalAllocatedBytes = Stats.DirectionalLightAllocatedBytes + Stats.SpotLightAllocatedBytes + Stats.PointLightAllocatedBytes;

	// 총 사용 중인 메모리
	Stats.TotalUsedBytes = Stats.DirectionalLightUsedBytes + Stats.SpotLightUsedBytes + Stats.PointLightUsedBytes;

	// 통계 매니저에 업데이트
	FShadowStatManager::GetInstance().UpdateStats(Stats);
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
	OutContext.ShadowBias = Light->GetShadowBias();
	OutContext.ShadowSlopeBias = Light->GetShadowSlopeBias();

	// Shadow Map 렌더링 시작 (DSV 바인딩, Viewport 설정)
	SpotLightShadowMap.BeginRender(RHI, Index, OutContext.ShadowBias, OutContext.ShadowSlopeBias);

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
	OutContext.ShadowBias = Light->GetShadowBias();
	OutContext.ShadowSlopeBias = Light->GetShadowSlopeBias();

	// Light Component에 계산된 ViewProjection 저장 (Light Buffer 업데이트 시 사용)
	Light->SetLightViewProjection(ShadowVP.ViewProjection);

	// DirectionalLight Shadow Map 렌더링 시작 (DSV 바인딩, Viewport 설정)
	DirectionalLightShadowMap.BeginRender(RHI, Index, OutContext.ShadowBias, OutContext.ShadowSlopeBias);

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
	OutContext.ShadowBias = Light->GetShadowBias();
	OutContext.ShadowSlopeBias = Light->GetShadowSlopeBias();

	// Cube Shadow Map 렌더링 시작
	// ArrayIndex = (LightIndex * 6) + CubeFaceIndex
	uint32 ArrayIndex = (Index * 6) + CubeFaceIndex;
	PointLightCubeShadowMap.BeginRender(RHI, ArrayIndex, OutContext.ShadowBias, OutContext.ShadowSlopeBias);

	return true;
}

void FShadowManager::EndShadowRender(D3D11RHI* RHI)
{
	SpotLightShadowMap.EndRender(RHI);
	DirectionalLightShadowMap.EndRender(RHI);
	PointLightCubeShadowMap.EndRender(RHI);
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

	// Shadow Comparison Sampler를 슬롯 s2, s3에 바인딩
	ID3D11SamplerState* ShadowSampler = RHI->GetShadowComparisonSamplerState();
	ID3D11SamplerState* DirectionalShadowSampler = RHI->GetDirectionalShadowComparisonSamplerState();
	ID3D11SamplerState* Samplers[2] = { ShadowSampler, DirectionalShadowSampler };
	RHI->GetDeviceContext()->PSSetSamplers(2, 2, Samplers);  // s2: Reverse-Z, s3: Forward-Z
}

void FShadowManager::UnbindShadowResources(D3D11RHI* RHI)
{
	// Shadow Map 언바인딩 (t5, t6, t7 동시 해제)
	ID3D11ShaderResourceView* NullSRVs[3] = { nullptr, nullptr, nullptr };
	RHI->GetDeviceContext()->PSSetShaderResources(5, 3, NullSRVs);

	// Shadow Sampler 언바인딩 (s2, s3)
	ID3D11SamplerState* NullSamplers[2] = { nullptr, nullptr };
	RHI->GetDeviceContext()->PSSetSamplers(2, 2, NullSamplers);
}

int32 FShadowManager::GetShadowMapIndex(USpotLightComponent* Light) const
{
	return Light ? Light->GetShadowMapIndex() : -1;
}
