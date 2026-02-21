#include "pch.h"
#include "Render/RenderPass/Public/LightSensorPass.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/RenderPass/Public/SharedLightResources.h"
#include "Render/RenderPass/Public/ShadowMapPass.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Texture/Public/ShadowMapResources.h"

// SensorParams cbuffer 구조체
// IMPORTANT: HLSL의 float3 배열은 각 요소가 16-byte로 패딩됨!
struct FSensorParamsData
{
	FVector4 SensorWorldPositions[32];  // 32 × 16 = 512 bytes (w는 사용 안 함)
	uint32 SensorCount;                 // 4 bytes
	float Padding[3];                   // 12 bytes
};  // Total: 528 bytes

FLightSensorPass::FLightSensorPass(UPipeline* InPipeline, ID3D11Buffer* InCameraConstantBuffer)
	: FRenderPass(InPipeline, InCameraConstantBuffer, nullptr)
{
	// Compute Shader 생성 - BLINN_PHONG 매크로로 컴파일하여 섀도우 계산 활성화
	D3D_SHADER_MACRO macros[] = {
		{ "LIGHTING_MODEL_BLINNPHONG", "1" },
		{ nullptr, nullptr }
	};
	FRenderResourceFactory::CreateComputeShader(L"Asset/Shader/LightIntensitySensor.hlsl", &LightSensorCS, "main", macros);
	if (!LightSensorCS)
	{
		UE_LOG("[LightSensorPass] ERROR: Failed to create Compute Shader!");
	}

	// Constant Buffer 생성
	SensorParamsConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FSensorParamsData>();
	if (!SensorParamsConstantBuffer)
	{
		UE_LOG("[LightSensorPass] ERROR: Failed to create SensorParams CB!");
	}

	// Output Buffer 생성 (최대 32개 float)
	OutputBuffer = FRenderResourceFactory::CreateRWStructuredBuffer(sizeof(float), MaxBatchSize);
	if (!OutputBuffer)
	{
		UE_LOG("[LightSensorPass] ERROR: Failed to create OutputBuffer!");
	}

	FRenderResourceFactory::CreateUnorderedAccessView(OutputBuffer, &OutputBufferUAV);
	if (!OutputBufferUAV)
	{
		UE_LOG("[LightSensorPass] ERROR: Failed to create OutputBufferUAV!");
	}

	// Readback Buffer 생성 (Staging)
	D3D11_BUFFER_DESC stagingDesc = {};
	stagingDesc.ByteWidth = sizeof(float) * MaxBatchSize;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.BindFlags = 0;
	stagingDesc.MiscFlags = 0;

	ID3D11Device* Device = URenderer::GetInstance().GetDevice();
	HRESULT hr = Device->CreateBuffer(&stagingDesc, nullptr, &ReadbackBuffer);
	if (FAILED(hr))
	{
		UE_LOG("[LightSensorPass] ERROR: ReadbackBuffer creation failed! HRESULT=0x%08X", hr);
		ReadbackBuffer = nullptr;
	}
}

void FLightSensorPass::SetRenderTargets(class UDeviceResources* DeviceResources)
{
	// CS 전용 Pass
}

void FLightSensorPass::Execute(FRenderingContext& Context)
{
	// 1. 이전 프레임 결과 처리
	ProcessPendingReadbacks();

	// 2. 새 요청 처리
	if (!RequestQueue.empty())
	{
		ProcessNewRequests(Context);
	}
}

void FLightSensorPass::Release()
{
	SafeRelease(LightSensorCS);
	SafeRelease(SensorParamsConstantBuffer);
	SafeRelease(OutputBuffer);
	SafeRelease(OutputBufferUAV);
	SafeRelease(ReadbackBuffer);
}

void FLightSensorPass::EnqueueRequest(const FLightSensorRequest& Request)
{
	RequestQueue.push(Request);
}

void FLightSensorPass::ProcessPendingReadbacks()
{
	if (PendingReadbacks.empty())
		return;

	ID3D11DeviceContext* DeviceContext = URenderer::GetInstance().GetDeviceContext();

	// 가장 오래된 Readback 처리
	FPendingReadback& Pending = PendingReadbacks.front();

	// Map the staging buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = DeviceContext->Map(ReadbackBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);

	if (SUCCEEDED(hr))
	{
		float* results = reinterpret_cast<float*>(mappedResource.pData);

		// Callback 호출
		for (uint32 i = 0; i < Pending.Count; ++i)
		{
			if (i < static_cast<uint32>(Pending.Requests.Num()))
			{
				Pending.Requests[i].Callback(results[i]);
			}
		}

		DeviceContext->Unmap(ReadbackBuffer, 0);
	}
	else
	{
		UE_LOG("[LightSensorPass] ERROR: Failed to map readback buffer! HRESULT = 0x%08X", hr);
	}

	PendingReadbacks.pop();
}

void FLightSensorPass::ProcessNewRequests(FRenderingContext& Context)
{
	// SharedLightResources 확인
	if (!Context.SharedLightResources)
	{
		UE_LOG("[LightSensorPass] SharedLightResources is nullptr!");
		return;
	}

	const FSharedLightResources& LightRes = *Context.SharedLightResources;

	// RequestQueue에서 최대 MaxBatchSize개 가져오기
	TArray<FLightSensorRequest> BatchRequests;
	BatchRequests.Reserve(MaxBatchSize);

	while (!RequestQueue.empty() && BatchRequests.Num() < static_cast<int32>(MaxBatchSize))
	{
		BatchRequests.Add(RequestQueue.front());
		RequestQueue.pop();
	}

	if (BatchRequests.IsEmpty())
		return;

	uint32 batchCount = static_cast<uint32>(BatchRequests.Num());

	// SensorParams Constant Buffer 업데이트
	FSensorParamsData sensorData = {};
	sensorData.SensorCount = batchCount;
	for (uint32 i = 0; i < batchCount; ++i)
	{
		const FVector& pos = BatchRequests[i].WorldPosition;
		sensorData.SensorWorldPositions[i] = FVector4(pos.X, pos.Y, pos.Z, 1.0f);
	}

	// Constant Buffer 업데이트
	ID3D11DeviceContext* DeviceContext = URenderer::GetInstance().GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT hr = DeviceContext->Map(SensorParamsConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	if (SUCCEEDED(hr))
	{
		memcpy(MappedResource.pData, &sensorData, sizeof(FSensorParamsData));
		DeviceContext->Unmap(SensorParamsConstantBuffer, 0);
	}
	else
	{
		UE_LOG("[LightSensorPass] ERROR: ConstantBuffer Map failed! HRESULT=0x%08X", hr);
		return;
	}

	// --- Constant Buffers 바인딩 ---
	Pipeline->SetConstantBuffer(1, EShaderType::CS, ConstantBufferCamera);
	Pipeline->SetConstantBuffer(3, EShaderType::CS, LightRes.GlobalLightConstantBuffer);
	Pipeline->SetConstantBuffer(4, EShaderType::CS, LightRes.ClusterSliceInfoConstantBuffer);
	Pipeline->SetConstantBuffer(5, EShaderType::CS, LightRes.LightCountInfoConstantBuffer);
	Pipeline->SetConstantBuffer(7, EShaderType::CS, SensorParamsConstantBuffer);

	// --- Shadow Resources 바인딩 (ShadowMapPass에서 가져오기) ---
	URenderer& Renderer = URenderer::GetInstance();
	FShadowMapPass* ShadowPass = Renderer.GetShadowMapPass();
	if (ShadowPass)
	{
		// b6: CascadeShadowMapData
		ID3D11Buffer* CascadeBuffer = ShadowPass->GetCascadeShadowMapDataBuffer();
		if (CascadeBuffer)
		{
			Pipeline->SetConstantBuffer(6, EShaderType::CS, CascadeBuffer);
		}

		// t10, t11: Shadow Atlas textures
		FShadowMapResource* ShadowAtlas = ShadowPass->GetShadowAtlas();
		if (ShadowAtlas && ShadowAtlas->IsValid())
		{
			Pipeline->SetShaderResourceView(10, EShaderType::CS, ShadowAtlas->ShadowSRV.Get());
			Pipeline->SetShaderResourceView(11, EShaderType::CS, ShadowAtlas->VarianceShadowSRV.Get());
		}

		// t12, t13, t14: Tile Position buffers
		Pipeline->SetShaderResourceView(12, EShaderType::CS, ShadowPass->GetDirectionalLightTilePosSRV());
		Pipeline->SetShaderResourceView(13, EShaderType::CS, ShadowPass->GetSpotLightTilePosSRV());
		Pipeline->SetShaderResourceView(14, EShaderType::CS, ShadowPass->GetPointLightTilePosSRV());
	}

	// --- Samplers 바인딩 ---
	Pipeline->SetSamplerState(1, EShaderType::CS, Renderer.GetShadowComparisonSampler());  // ShadowSampler (s1)
	Pipeline->SetSamplerState(2, EShaderType::CS, Renderer.GetVarianceShadowSampler());    // VarianceShadowSampler (s2)
	Pipeline->SetSamplerState(3, EShaderType::CS, Renderer.GetPointShadowSampler());       // PointShadowSampler (s3)

	// --- Structured Buffers 바인딩 (Light data) ---
	if (!LightRes.PointLightInfosSRV)
	{
		UE_LOG("[LightSensorPass] ERROR: PointLightInfosSRV is nullptr!");
	}
	if (!LightRes.SpotLightInfosSRV)
	{
		UE_LOG("[LightSensorPass] ERROR: SpotLightInfosSRV is nullptr!");
	}

	Pipeline->SetShaderResourceView(6, EShaderType::CS, LightRes.PointLightIndicesSRV);  // t6
	Pipeline->SetShaderResourceView(7, EShaderType::CS, LightRes.SpotLightIndicesSRV);   // t7
	Pipeline->SetShaderResourceView(8, EShaderType::CS, LightRes.PointLightInfosSRV);    // t8
	Pipeline->SetShaderResourceView(9, EShaderType::CS, LightRes.SpotLightInfosSRV);     // t9

	// Output UAV 바인딩 및 Compute Shader Dispatch
	Pipeline->SetUnorderedAccessView(0, OutputBufferUAV);
	Pipeline->DispatchCS(LightSensorCS, 1, 1, 1);
	Pipeline->SetUnorderedAccessView(0, nullptr);

	// --- Unbind Constant Buffers (CS) ---
	Pipeline->SetConstantBuffer(1, EShaderType::CS, nullptr);
	Pipeline->SetConstantBuffer(3, EShaderType::CS, nullptr);
	Pipeline->SetConstantBuffer(4, EShaderType::CS, nullptr);
	Pipeline->SetConstantBuffer(5, EShaderType::CS, nullptr);
	Pipeline->SetConstantBuffer(6, EShaderType::CS, nullptr);
	Pipeline->SetConstantBuffer(7, EShaderType::CS, nullptr);

	// --- Unbind Structured Buffers (CS) ---
	Pipeline->SetShaderResourceView(6, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(7, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(8, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(9, EShaderType::CS, nullptr);

	// --- Unbind Shadow Resources (CS) ---
	Pipeline->SetShaderResourceView(10, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(11, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(12, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(13, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(14, EShaderType::CS, nullptr);

	// --- Unbind Samplers (CS) ---
	Pipeline->SetSamplerState(1, EShaderType::CS, nullptr);
	Pipeline->SetSamplerState(2, EShaderType::CS, nullptr);
	Pipeline->SetSamplerState(3, EShaderType::CS, nullptr);

	// GPU → Staging Buffer로 복사 (DeviceContext 재사용)
	DeviceContext->CopyResource(ReadbackBuffer, OutputBuffer);

	// PendingReadbacks에 추가
	FPendingReadback pending;
	pending.Requests = MoveTemp(BatchRequests);
	pending.Count = batchCount;
	PendingReadbacks.push(MoveTemp(pending));
}
