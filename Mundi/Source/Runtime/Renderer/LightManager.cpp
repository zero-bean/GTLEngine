#include "pch.h"
#include "LightManager.h"
#include "AmbientLightComponent.h"
#include "DirectionalLightComponent.h"
#include "SpotLightComponent.h"
#include "PointLightComponent.h"
#include "D3D11RHI.h"
#include "ShadowMap.h"
#include "SceneComponent.h"

#define NUM_POINT_LIGHT_MAX 256
#define NUM_SPOT_LIGHT_MAX 256
#define MAX_SHADOW_CASTING_LIGHTS 10

FLightManager::~FLightManager()
{
	Release();
}
void FLightManager::Initialize(D3D11RHI* RHIDevice)
{
	if (!PointLightBuffer)
	{
		RHIDevice->CreateStructuredBuffer(sizeof(FPointLightInfo), NUM_POINT_LIGHT_MAX, nullptr, &PointLightBuffer);
		RHIDevice->CreateStructuredBufferSRV(PointLightBuffer, &PointLightBufferSRV);
	}
	if (!SpotLightBuffer)
	{
		RHIDevice->CreateStructuredBuffer(sizeof(FSpotLightInfo), NUM_SPOT_LIGHT_MAX, nullptr, &SpotLightBuffer);
		RHIDevice->CreateStructuredBufferSRV(SpotLightBuffer, &SpotLightBufferSRV);
	}

	// Initialize shadow map array
	if (!ShadowMapArray)
	{
		ShadowMapArray = new FShadowMap();
		ShadowMapArray->Initialize(RHIDevice, 1024, 1024, MAX_SHADOW_CASTING_LIGHTS);
	}
}
void FLightManager::Release()
{
	if (PointLightBuffer)
	{
		PointLightBuffer->Release();
		PointLightBuffer = nullptr;
	}
	if (PointLightBufferSRV)
	{
		PointLightBufferSRV->Release();
		PointLightBufferSRV = nullptr;
	}

	if (SpotLightBuffer)
	{
		SpotLightBuffer->Release();
		SpotLightBuffer = nullptr;
	}
	if (SpotLightBufferSRV)
	{
		SpotLightBufferSRV->Release();
		SpotLightBufferSRV = nullptr;
	}

	// Release shadow map array
	if (ShadowMapArray)
	{
		ShadowMapArray->Release();
		delete ShadowMapArray;
		ShadowMapArray = nullptr;
	}
	LightToShadowMapIndex.clear();
}

void FLightManager::UpdateLightBuffer(D3D11RHI* RHIDevice)
{
	if (bHaveToUpdate)
	{
		if (!PointLightBuffer)
		{
			Initialize(RHIDevice);
		}

		FLightBufferType LightBuffer{};

		if (AmbientLightList.Num() > 0)
		{
			if (AmbientLightList[0]->IsVisible()&&
				AmbientLightList[0]->GetOwner()->IsActorVisible())
			{
				LightBuffer.AmbientLight = AmbientLightList[0]->GetLightInfo();
			}
		}

		if (DIrectionalLightList.Num() > 0)
		{
			if (DIrectionalLightList[0]->IsVisible() &&
				DIrectionalLightList[0]->GetOwner()->IsActorVisible())
			{
				LightBuffer.DirectionalLight = DIrectionalLightList[0]->GetLightInfo();
			}
		}


		if (bPointLightDirty)
		{
			PointLightInfoList.clear();
			for (int Index = 0; Index < PointLightList.Num(); Index++)
			{
				if (PointLightList[Index]->IsVisible()&&
					PointLightList[Index]->GetOwner()->IsActorVisible())
				{
					PointLightInfoList.Add(PointLightList[Index]->GetLightInfo());
				}
			}
			RHIDevice->UpdateStructuredBuffer(PointLightBuffer, PointLightInfoList.data(), PointLightNum * sizeof(FPointLightInfo));
			bPointLightDirty = false;
		}

		if (bSpotLightDirty)
		{
			SpotLightInfoList.clear();
			for (int Index = 0; Index < SpotLightList.Num(); Index++)
			{
				if (SpotLightList[Index]->IsVisible()&&
					SpotLightList[Index]->GetOwner()->IsActorVisible())
				{
					SpotLightInfoList.Add(SpotLightList[Index]->GetLightInfo());
				}
			}

			RHIDevice->UpdateStructuredBuffer(SpotLightBuffer, SpotLightInfoList.data(), 
				SpotLightNum * sizeof(FSpotLightInfo));

			bSpotLightDirty = false;
		}

		LightBuffer.PointLightCount = PointLightNum;
		LightBuffer.SpotLightCount = SpotLightNum;

		//슬롯 재활용 하고싶을 시 if문 밖으로 빼서 매 프레임 세팅 해줘야함.
		RHIDevice->SetAndUpdateConstantBuffer(LightBuffer);
	
		ID3D11ShaderResourceView* SRVList[2]{ PointLightBufferSRV, SpotLightBufferSRV };

		//Gouraud shader 사용 여부로 아래 세팅 분기도 가능, 일단 둘다 바인딩함
		RHIDevice->GetDeviceContext()->PSSetShaderResources(3, 2, SRVList);
		RHIDevice->GetDeviceContext()->VSSetShaderResources(3, 2, SRVList);

		bHaveToUpdate = false;
	}
	
}

void FLightManager::SetDirtyFlag()
{
	bHaveToUpdate = true;
	bPointLightDirty = true;
	bSpotLightDirty = true;
}

void FLightManager::AssignShadowMapIndices(D3D11RHI* RHIDevice, const TArray<USpotLightComponent*>& VisibleSpotLights)
{
	LightToShadowMapIndex.clear(); // 매 프레임 초기화 (Dynamic 환경)

	int32 CurrentIndex = 0;
	for (USpotLightComponent* Light : VisibleSpotLights)
	{
		// Case A. 비활성화 혹은 존재하지 않는다면 쉐도우 렌더링에 제외합니다.
		if (!Light || !Light->GetIsCastShadows())
		{
			Light->SetShadowMapIndex(-1);
			continue;
		}

		// Case B. 쉐도우 렌더 수 상한선에 도달하면 쉐도우 렌더링에 제외합니다.
		if (CurrentIndex >= MAX_SHADOW_CASTING_LIGHTS)
		{
			Light->SetShadowMapIndex(-1);
			continue;
		}

		// 쉐도우 렌더링 목록에 포함합니다.
		LightToShadowMapIndex.Add(Light, CurrentIndex);
		Light->SetShadowMapIndex(CurrentIndex); // 라이트 컴포넌트 자체에도 인덱스 저장
		CurrentIndex++;
	}
}

int32 FLightManager::BeginShadowMapRender(D3D11RHI* RHI, USpotLightComponent* Light, FMatrix& OutLightView, FMatrix& OutLightProj)
{
	if (!LightToShadowMapIndex.Contains(Light))
	{
		return -1; // 이 라이트는 섀도우 맵이 할당되지 않음
	}

	int32 Index = LightToShadowMapIndex[Light];

	// 라이트 공간 View 행렬 계산 (월드 좌표 사용!)
	FVector LightPos = Light->GetWorldLocation();  // ✓ 월드 좌표로 수정
	FVector LightDir = Light->GetDirection();
	FVector LightUp = FVector(0, 1, 0);
	if (FMath::Abs(FVector::Dot(LightDir, LightUp)) > 0.99f)
	{
		LightUp = FVector(1, 0, 0);
	}
	OutLightView = FMatrix::LookAtLH(LightPos, LightPos + LightDir, LightUp);

	// 라이트 공간 Projection 행렬 계산
	float FOV = Light->GetOuterConeAngle() * 2.0f;
	float AspectRatio = 1.0f; // 섀도우 맵은 정사각형
	float NearPlane = 0.1f;
	float FarPlane = Light->GetAttenuationRadius();
	OutLightProj = FMatrix::PerspectiveFovLH(DegreesToRadians(FOV), AspectRatio, NearPlane, FarPlane);

	// 실제 섀도우 맵 리소스(DSV) 바인딩
	ShadowMapArray->BeginRender(RHI, Index);

	return Index;
}

void FLightManager::EndShadowMapRender(D3D11RHI* RHI)
{
	// 현재는 BeginRender에서 DSV를 바인딩하므로 특별히 할 일은 없지만,
	// 명시적인 상태 관리를 위해 남겨둡니다.
}

void FLightManager::ClearAllLightList()
{
	AmbientLightList.clear();
	DIrectionalLightList.clear();
	PointLightList.clear();
	SpotLightList.clear();

	//업데이트 시에만 clear하고 다시 수집할 실제 데이터 리스트
	PointLightInfoList.clear();
	SpotLightInfoList.clear();

	//이미 레지스터된 라이트인지 확인하는 용도
	LightComponentList.clear();
	PointLightNum = 0;
	SpotLightNum = 0;
}

template<>
void FLightManager::RegisterLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent)
{
	if (LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Add(LightComponent);
	AmbientLightList.Add(LightComponent);
	bHaveToUpdate = true;
}
template<>
void FLightManager::RegisterLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent)
{
	if (LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Add(LightComponent);
	DIrectionalLightList.Add(LightComponent);
	bHaveToUpdate = true;
}
template<>
void FLightManager::RegisterLight<UPointLightComponent>(UPointLightComponent* LightComponent)
{
	if (LightComponentList.Contains(LightComponent)||
		Cast<USpotLightComponent>(LightComponent))
	{
		return;
	}
	LightComponentList.Add(LightComponent);
	PointLightList.Add(LightComponent);
	PointLightInfoList.Add(LightComponent->GetLightInfo());
	PointLightNum++;
	bPointLightDirty = true;
	bHaveToUpdate = true;
}
template<>
void FLightManager::RegisterLight<USpotLightComponent>(USpotLightComponent* LightComponent)
{
	if (LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Add(LightComponent);
	SpotLightList.Add(LightComponent);
	SpotLightInfoList.Add(LightComponent->GetLightInfo());
	SpotLightNum++;
	bSpotLightDirty = true;
	bHaveToUpdate = true;
}


template<>
void FLightManager::DeRegisterLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Remove(LightComponent);
	AmbientLightList.Remove(LightComponent);
	bHaveToUpdate = true;
}
template<>
void FLightManager::DeRegisterLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Remove(LightComponent);
	DIrectionalLightList.Remove(LightComponent);
	bHaveToUpdate = true;
}
template<>
void FLightManager::DeRegisterLight<UPointLightComponent>(UPointLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent) ||
		Cast<USpotLightComponent>(LightComponent))
	{
		return;
	}
	LightComponentList.Remove(LightComponent);
	PointLightList.Remove(LightComponent);
	PointLightNum--;
	bPointLightDirty = true;
	bHaveToUpdate = true;
}
template<>
void FLightManager::DeRegisterLight<USpotLightComponent>(USpotLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Remove(LightComponent);
	SpotLightList.Remove(LightComponent);
	SpotLightNum--;
	bSpotLightDirty = true;
	bHaveToUpdate = true;
}


template<> void FLightManager::UpdateLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	bHaveToUpdate = true;
}
template<> void FLightManager::UpdateLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	bHaveToUpdate = true;
}
template<> void FLightManager::UpdateLight<UPointLightComponent>(UPointLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent) ||
		Cast<USpotLightComponent>(LightComponent))
	{
		return;
	}
	bPointLightDirty = true;
	bHaveToUpdate = true;
}
template<> void FLightManager::UpdateLight<USpotLightComponent>(USpotLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	bSpotLightDirty = true;
	bHaveToUpdate = true;
}

int32 FLightManager::CreateShadowMapForLight(D3D11RHI* RHIDevice, USpotLightComponent* SpotLight)
{
	// Check if this light already has a shadow map
	if (LightToShadowMapIndex.Contains(SpotLight))
	{
		return LightToShadowMapIndex[SpotLight];
	}

	// Check if we have reached the maximum number of shadow-casting lights
	int32 Index = LightToShadowMapIndex.Num();
	if (Index >= MAX_SHADOW_CASTING_LIGHTS)
	{
		// Cannot create more shadow maps
		return -1;
	}

	// Assign next available array slot
	LightToShadowMapIndex.Add(SpotLight, Index);

	return Index;
}

void FLightManager::BindShadowMaps(D3D11RHI* RHIDevice)
{
	// Bind shadow map texture array to shader slot t5
	if (ShadowMapArray)
	{
		ID3D11ShaderResourceView* SRV = ShadowMapArray->GetSRV();
		RHIDevice->GetDeviceContext()->PSSetShaderResources(5, 1, &SRV);
	}

	// Bind shadow comparison sampler to shader slot s2
	ID3D11SamplerState* ShadowSampler = RHIDevice->GetShadowComparisonSamplerState();
	RHIDevice->GetDeviceContext()->PSSetSamplers(2, 1, &ShadowSampler);
}