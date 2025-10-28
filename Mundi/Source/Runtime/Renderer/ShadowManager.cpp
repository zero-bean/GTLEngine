#include "pch.h"
#include "ShadowManager.h"
#include "ShadowViewProjection.h"
#include "SpotLightComponent.h"
#include "DirectionalLightComponent.h"
#include "PointLightComponent.h"
#include "D3D11RHI.h"
#include "ResourceManager.h"
#include "Shader.h"

FShadowManager::FShadowManager()
	: RHIDevice(nullptr)
	, ShadowVSM_PS(nullptr)
	, ShadowESM_PS(nullptr)
	, ShadowEVSM_PS(nullptr)
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

	// Shadow Map Array 초기화 (광원 타입별로 각각의 해상도 사용, 필터 타입 전달)
	SpotLightShadowMap.Initialize(RHI, Config.SpotLightResolution, Config.SpotLightResolution, Config.MaxSpotLights, false, Config.FilterType);

	// DirectionalLight는 CSM을 사용할 수 있으므로 최대 6개 캐스케이드를 지원하도록 할당
	// (각 라이트가 CSM을 사용할지는 런타임에 DirectionalLightComponent의 ShadowMapType으로 결정)
	constexpr uint32 MaxCascadesPerLight = 6;
	uint32 DirectionalArraySize = Config.MaxDirectionalLights * MaxCascadesPerLight;
	DirectionalLightShadowMap.Initialize(RHI, Config.DirectionalLightResolution, Config.DirectionalLightResolution, DirectionalArraySize);

	PointLightCubeShadowMap.Initialize(RHI, Config.PointLightResolution, Config.PointLightResolution, Config.MaxPointLights, true, Config.FilterType);

	// VSM/ESM/EVSM용 쉐이더 로드
	if (!ShadowVSM_PS)
	{
		ShadowVSM_PS = UResourceManager::GetInstance().Load<UShader>("Shaders/Common/ShadowVSM_PS.hlsl");
	}
	if (!ShadowESM_PS)
	{
		ShadowESM_PS = UResourceManager::GetInstance().Load<UShader>("Shaders/Common/ShadowESM_PS.hlsl");
	}
	if (!ShadowEVSM_PS)
	{
		ShadowEVSM_PS = UResourceManager::GetInstance().Load<UShader>("Shaders/Common/ShadowEVSM_PS.hlsl");
	}

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

void FShadowManager::SetFilterType(EShadowFilterType NewFilterType)
{
	// 초기화되지 않았으면 설정만 변경
	if (!bIsInitialized)
	{
		Config.FilterType = NewFilterType;
		return;
	}

	// 이미 초기화된 경우 재초기화
	Config.FilterType = NewFilterType;
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
	//Stats.ShadowMapResolution = Config.ShadowMapResolution;
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
	Stats.PointLightAllocatedBytes = PointLightCubeShadowMap.GetAllocatedMemoryBytes();

	// 각 쉐도우 맵의 실제 사용 중인 메모리 계산 (활성 라이트 수 기반)
	Stats.DirectionalLightUsedBytes = DirectionalLightShadowMap.GetUsedMemoryBytes(DirectionalLightCount);
	Stats.SpotLightUsedBytes = SpotLightShadowMap.GetUsedMemoryBytes(SpotLightCount);
	Stats.PointLightUsedBytes = PointLightCubeShadowMap.GetUsedMemoryBytes(PointLightCount * 6); // 큐브맵이므로 6개 면

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

bool FShadowManager::BeginShadowRenderCSM(D3D11RHI* RHI, UDirectionalLightComponent* Light,
	const FMatrix& CameraView, const FMatrix& CameraProjection,
	int CascadeIndex, float SplitNear, float SplitFar, FShadowRenderContext& OutContext)
{
	// DirectionalLight의 NumCascades 가져오기
	const int32 NumCascades = Light->GetNumCascades();

	// 캐스케이드 인덱스 유효성 검사
	if (CascadeIndex < 0 || CascadeIndex >= NumCascades)
	{
		return false;
	}

	// 라이트의 Shadow Map Index 확인
	int32 LightShadowIndex = Light->GetShadowMapIndex();
	if (LightShadowIndex < 0)
	{
		return false;
	}

	// 1. 부분 Frustum 코너 계산
	TArray<FVector> CascadeFrustumCorners;
	GetFrustumCornersWorldSpace_Partial(
		CameraView,
		CameraProjection,
		SplitNear,
		SplitFar,
		CascadeFrustumCorners);

	// 2. Light VP 계산 (새로운 FromCorners 버전 사용)
	FShadowViewProjection ShadowVP = FShadowViewProjection::CreateForDirectionalLight_FromCorners(
		Light->GetLightDirection(),
		CascadeFrustumCorners);

	// 3. Texture Array Index 계산: (LightIndex * MaxCascades) + CascadeIndex
	// 최대 6개 캐스케이드를 지원하도록 할당했으므로, MaxCascades = 6 사용
	constexpr int32 MaxCascades = 6;
	int32 ArrayIndex = LightShadowIndex * MaxCascades + CascadeIndex;

	// 4. 출력 컨텍스트 설정
	OutContext.LightView = ShadowVP.View;
	OutContext.LightProjection = ShadowVP.Projection;
	OutContext.ShadowMapIndex = ArrayIndex;
	OutContext.ShadowBias = Light->GetShadowBias();
	OutContext.ShadowSlopeBias = Light->GetShadowSlopeBias();

	// 5. Light Component에 VP 저장
	Light->SetCascadeViewProjection(CascadeIndex, ShadowVP.ViewProjection);
	Light->SetCascadeSplitDistance(CascadeIndex, SplitFar);

	// 6. Shadow Map 렌더링 시작
	DirectionalLightShadowMap.BeginRender(
		RHI,
		ArrayIndex,
		OutContext.ShadowBias,
		OutContext.ShadowSlopeBias);

	return true;
}

TArray<float> FShadowManager::CalculateCSMSplits(float CameraNear, float CameraFar, int NumCascades, float Lambda) const
{
	TArray<float> Splits;
	Splits.Reserve(NumCascades);

	for (int i = 0; i < NumCascades; ++i)
	{
		float p = (i + 1) / (float)NumCascades;

		// 로그 분할
		float LogSplit = CameraNear * pow(CameraFar / CameraNear, p);

		// 선형 분할
		float LinearSplit = CameraNear + (CameraFar - CameraNear) * p;

		// PSSM 혼합
		float Split = Lambda * LogSplit + (1.0f - Lambda) * LinearSplit;
		Splits.Add(Split);
	}

	return Splits;
}

bool FShadowManager::BeginShadowRenderCube(D3D11RHI* RHI, UPointLightComponent* Light, uint32 CubeFaceIndex, const FShadowViewProjection& ShadowVP, FShadowRenderContext& OutContext)
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

	// 미리 계산된 VP 행렬 사용 (중복 계산 제거)
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
	ID3D11ShaderResourceView* SpotShadowMapSRV = SpotLightShadowMap.GetSRV();
	ID3D11ShaderResourceView* DirShadowMapSRV = DirectionalLightShadowMap.GetSRV();
	ID3D11ShaderResourceView* PointCubeShadowMapSRV = PointLightCubeShadowMap.GetSRV();

	// FilterType에 따라 다른 슬롯에 바인딩
	if (Config.FilterType == EShadowFilterType::NONE || Config.FilterType == EShadowFilterType::PCF)
	{
		// NONE/PCF: Depth 포맷 텍스처를 t5, t6, t7에 바인딩
		RHI->GetDeviceContext()->PSSetShaderResources(5, 1, &SpotShadowMapSRV);
		RHI->GetDeviceContext()->PSSetShaderResources(6, 1, &DirShadowMapSRV);
		RHI->GetDeviceContext()->PSSetShaderResources(7, 1, &PointCubeShadowMapSRV);
	}
	else // VSM, ESM, EVSM
	{
		// VSM/ESM/EVSM: Float 포맷 텍스처를 t8, t9, t10에 바인딩
		RHI->GetDeviceContext()->PSSetShaderResources(8, 1, &SpotShadowMapSRV);
		RHI->GetDeviceContext()->PSSetShaderResources(9, 1, &DirShadowMapSRV);
		RHI->GetDeviceContext()->PSSetShaderResources(10, 1, &PointCubeShadowMapSRV);
	}

	// Shadow Comparison Sampler를 슬롯 s2에 바인딩
	ID3D11SamplerState* ShadowSampler = RHI->GetShadowComparisonSamplerState();
	RHI->GetDeviceContext()->PSSetSamplers(2, 1, &ShadowSampler);

	// Linear Sampler를 슬롯 s3에 바인딩 (VSM/ESM/EVSM용)
	ID3D11SamplerState* LinearSampler = RHI->GetLinearSamplerState();
	RHI->GetDeviceContext()->PSSetSamplers(3, 1, &LinearSampler);
}

void FShadowManager::UnbindShadowResources(D3D11RHI* RHI)
{
	// Shadow Map 언바인딩 (모든 슬롯 해제)
	ID3D11ShaderResourceView* NullSRVs[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	// t5~t10 모두 언바인딩 (NONE/PCF: t5,t6,t7 / VSM/ESM/EVSM: t8,t9,t10)
	RHI->GetDeviceContext()->PSSetShaderResources(5, 6, NullSRVs);

	// Shadow Sampler 언바인딩
	ID3D11SamplerState* NullSampler = nullptr;
	RHI->GetDeviceContext()->PSSetSamplers(2, 1, &NullSampler);
}

int32 FShadowManager::GetShadowMapIndex(USpotLightComponent* Light) const
{
	return Light ? Light->GetShadowMapIndex() : -1;
}

UShader* FShadowManager::GetShadowPixelShaderForFilterType(EShadowFilterType FilterType) const
{
	switch (FilterType)
	{
	case EShadowFilterType::VSM:
		return ShadowVSM_PS;
	case EShadowFilterType::ESM:
		return ShadowESM_PS;
	case EShadowFilterType::EVSM:
		return ShadowEVSM_PS;
	case EShadowFilterType::NONE:
	case EShadowFilterType::PCF:
	default:
		return nullptr;  // Depth-only rendering (픽셀 쉐이더 불필요)
	}
}
