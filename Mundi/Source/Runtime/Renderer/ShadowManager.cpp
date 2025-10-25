#include "pch.h"
#include "ShadowManager.h"
#include "ShadowMap.h"
#include "ShadowViewProjection.h"
#include "SpotLightComponent.h"
#include "D3D11RHI.h"

FShadowManager::FShadowManager()
	: RHIDevice(nullptr)
	, ShadowMapArray(nullptr)
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

	// Shadow Map Array 초기화
	if (!ShadowMapArray)
	{
		ShadowMapArray = new FShadowMap();
		ShadowMapArray->Initialize(RHI, Config.ShadowMapResolution, Config.ShadowMapResolution, Config.MaxShadowCastingLights);
	}
}

void FShadowManager::Release()
{
	if (ShadowMapArray)
	{
		ShadowMapArray->Release();
		delete ShadowMapArray;
		ShadowMapArray = nullptr;
	}

	LightToShadowIndex.clear();
}

void FShadowManager::AssignShadowIndices(D3D11RHI* RHI, const TArray<USpotLightComponent*>& VisibleSpotLights)
{
	// 매 프레임 초기화 (Dynamic 환경)
	LightToShadowIndex.clear();

	int32 CurrentIndex = 0;
	for (USpotLightComponent* Light : VisibleSpotLights)
	{
		// 비활성화 혹은 존재하지 않는다면 쉐도우 렌더링에 제외
		if (!Light || !Light->GetIsCastShadows())
		{
			Light->SetShadowMapIndex(-1);
			continue;
		}

		// 쉐도우 렌더 수 상한선에 도달하면 쉐도우 렌더링에 제외
		if (CurrentIndex >= static_cast<int32>(Config.MaxShadowCastingLights))
		{
			Light->SetShadowMapIndex(-1);
			continue;
		}

		// 쉐도우 렌더링 목록에 포함
		LightToShadowIndex.Add(Light, CurrentIndex);
		Light->SetShadowMapIndex(CurrentIndex);
		CurrentIndex++;
	}
}

bool FShadowManager::BeginShadowRender(D3D11RHI* RHI, USpotLightComponent* Light, FShadowRenderContext& OutContext)
{
	// 이 라이트가 Shadow Map을 할당받았는지 확인
	if (!LightToShadowIndex.Contains(Light))
	{
		return false;
	}

	int32 Index = LightToShadowIndex[Light];

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
	ShadowMapArray->BeginRender(RHI, Index);

	return true;
}

void FShadowManager::EndShadowRender(D3D11RHI* RHI)
{
	if (ShadowMapArray)
	{
		ShadowMapArray->EndRender(RHI);
	}
}

void FShadowManager::BindShadowResources(D3D11RHI* RHI)
{
	// Shadow Map Texture Array를 셰이더 슬롯 t5에 바인딩
	if (ShadowMapArray)
	{
		ID3D11ShaderResourceView* ShadowMapSRV = ShadowMapArray->GetSRV();
		RHI->GetDeviceContext()->PSSetShaderResources(5, 1, &ShadowMapSRV);
	}
}

void FShadowManager::UnbindShadowResources(D3D11RHI* RHI)
{
	// Shadow Map 언바인딩
	ID3D11ShaderResourceView* NullSRV = nullptr;
	RHI->GetDeviceContext()->PSSetShaderResources(5, 1, &NullSRV);
}

int32 FShadowManager::GetShadowMapIndex(USpotLightComponent* Light) const
{
	if (LightToShadowIndex.Contains(Light))
	{
		return LightToShadowIndex.FindRef(Light);
	}
	return -1;
}
