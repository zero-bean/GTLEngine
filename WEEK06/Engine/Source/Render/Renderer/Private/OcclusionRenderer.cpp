#include "pch.h"

#include "cpp-thread-pool/thread_pool.h"

#include "Global/CoreTypes.h"
#include "Level/Public/Level.h"
#include "Editor/Public/EditorEngine.h"
#include "Render/Renderer/Public/OcclusionRenderer.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/Camera.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

#define MULTI_THREADING

IMPLEMENT_SINGLETON_CLASS_BASE(UOcclusionRenderer)

UOcclusionRenderer::UOcclusionRenderer() = default;
UOcclusionRenderer::~UOcclusionRenderer()
{
	Release();
}

void UOcclusionRenderer::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, uint32 InWidth, uint32 InHeight)
{
	if (!InDevice)
	{
		return;
	}
	Device = InDevice;

	if (!InDeviceContext)
	{
		return;
	}
	DeviceContext = InDeviceContext;

	Width = InWidth;
	Height = InHeight;

	CreateShader(Device);
	CreateHiZResource(Device);
}

void UOcclusionRenderer::Release()
{
	ReleaseShader();
	ReleaseHiZResource();

	Device = nullptr;
	DeviceContext = nullptr;
}

void UOcclusionRenderer::BuildScreenSpaceBoundingVolumes(
	ID3D11DeviceContext* InDeviceContext,
	UCamera* InCamera,
	const TArray<TObjectPtr<UPrimitiveComponent>>& PrimitiveComponents)
{
	// ========================================================= //
	// 1. 준비: 기존 데이터 클리어 & 리사이즈
	// ========================================================= //
	if (PrimitiveComponents.empty())
		return;

	BoundingVolumes.clear();
	BoundingVolumes.resize(PrimitiveComponents.size());

	PrimitiveComponentUUIDs.clear();
	PrimitiveComponentUUIDs.resize(PrimitiveComponents.size());

	FViewProjConstants ViewProj = InCamera->GetFViewProjConstants();
	FMatrix ViewProjMatrix = ViewProj.View * ViewProj.Projection;

#ifdef MULTI_THREADING
	static ThreadPool Pool(NUM_WORKER_THREADS); // Initialize thread pool once

	const size_t NumPrimitives = PrimitiveComponents.size();
	const size_t ChunkSize = (NumPrimitives + NUM_WORKER_THREADS - 1) / NUM_WORKER_THREADS;

	std::vector<std::future<void>> Futures;
	for (size_t i = 0; i < NUM_WORKER_THREADS; ++i)
	{
		const size_t StartIndex = i * ChunkSize;
		const size_t EndIndex = std::min(StartIndex + ChunkSize, NumPrimitives);

		if (StartIndex >= EndIndex) continue;

		Futures.emplace_back(Pool.Enqueue([this, StartIndex, EndIndex, &PrimitiveComponents, ViewProjMatrix]()
		{
			ProcessBoundingVolume(StartIndex, EndIndex, PrimitiveComponents, ViewProjMatrix);
		}));
	}

	for (auto& Future : Futures)
	{
		Future.get(); // Wait for all tasks to complete
	}

#else // Single-threaded version
	ProcessBoundingVolume(0, PrimitiveComponents.size(), PrimitiveComponents, ViewProjMatrix);
#endif
}

void UOcclusionRenderer::ProcessBoundingVolume(size_t InStartIndex, size_t InEndIndex, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents, const FMatrix& InViewProjMatrix)
{
	for (size_t i = InStartIndex; i < InEndIndex; ++i)
	{
		const auto& Primitive = InPrimitiveComponents[i];
		if (!Primitive)
		{
			BoundingVolumes[i] = { FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0) };
			continue;
		}
		PrimitiveComponentUUIDs[i] = InPrimitiveComponents[i]->GetUUID();

		// --- (1) 월드 공간 AABB 가져오기 ---
		FVector WorldMin, WorldMax;
		Primitive->GetWorldAABB(WorldMin, WorldMax);

		// --- (2) 8개 코너 생성 ---
		FVector corners[8] = {
			{ WorldMin.X, WorldMin.Y, WorldMin.Z },
			{ WorldMax.X, WorldMin.Y, WorldMin.Z },
			{ WorldMin.X, WorldMax.Y, WorldMin.Z },
			{ WorldMin.X, WorldMin.Y, WorldMax.Z },
			{ WorldMax.X, WorldMax.Y, WorldMin.Z },
			{ WorldMax.X, WorldMin.Y, WorldMax.Z },
			{ WorldMin.X, WorldMax.Y, WorldMax.Z },
			{ WorldMax.X, WorldMax.Y, WorldMax.Z }
		};

		// --- (3) Clip 공간 AABB 초기화 ---
		FVector4 ClipMin(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
		FVector4 ClipMax(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

		// --- (4) 각 코너 변환 후 Min/Max 업데이트 ---
		for (int j = 0; j < 8; ++j)
		{
			FVector4 clipPos = FMatrix::VectorMultiply(
				FVector4(corners[j].X, corners[j].Y, corners[j].Z, 1.0f),
				InViewProjMatrix
			);

			if (clipPos.W <= 0.0f) continue; // Discard points behind or on the projection plane

			FVector4 ndcPos(
				clipPos.X / clipPos.W,
				clipPos.Y / clipPos.W,
				clipPos.Z / clipPos.W,
				1.0f // Set W to 1.0f for NDC
			);

			// Clamp NDC coordinates to [-1, 1] range for X and Y, and [0, 1] for Z
			ndcPos.X = std::clamp(ndcPos.X, -1.0f, 1.0f);
			ndcPos.Y = std::clamp(ndcPos.Y, -1.0f, 1.0f);
			ndcPos.Z = std::clamp(ndcPos.Z, 0.0f, 1.0f); // Assuming Z is [0, 1] based on projection matrix analysis

			ClipMin.X = std::min(ClipMin.X, ndcPos.X);
			ClipMin.Y = std::min(ClipMin.Y, ndcPos.Y);
			ClipMin.Z = std::min(ClipMin.Z, ndcPos.Z);
			ClipMin.W = std::min(ClipMin.W, ndcPos.W);

			ClipMax.X = std::max(ClipMax.X, ndcPos.X);
			ClipMax.Y = std::max(ClipMax.Y, ndcPos.Y);
			ClipMax.Z = std::max(ClipMax.Z, ndcPos.Z);
			ClipMax.W = std::max(ClipMax.W, ndcPos.W);
		}
		// --- (5) 결과 저장 ---
		BoundingVolumes[i] = { ClipMin, ClipMax };
	}
}


void UOcclusionRenderer::GenerateHiZ(
	ID3D11Device* InDevice,
	ID3D11DeviceContext* InDeviceContext,
	ID3D11ShaderResourceView* InDepthShaderResourceView)
{
	// ========================================================= //
	// 1. Mip 0 초기 복사 (Depth → HiZ)
	// ========================================================= //
	InDeviceContext->CSSetShader(HiZCopyDepthShader, nullptr, 0);
	InDeviceContext->CSSetShaderResources(0, 1, &InDepthShaderResourceView);
	InDeviceContext->CSSetUnorderedAccessViews(0, 1, &HiZUnorderedAccessViews[0], nullptr);

	UINT Mip0Width = Width;
	UINT Mip0Height = Height;
	InDeviceContext->Dispatch((Mip0Width + 15) / 16, (Mip0Height + 15) / 16, 1);

	// --- 리소스 언바인딩 ---
	{
		ID3D11ShaderResourceView* NullSRV = nullptr;
		ID3D11UnorderedAccessView* NullUAV = nullptr;
		InDeviceContext->CSSetShaderResources(0, 1, &NullSRV);
		InDeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUAV, nullptr);
	}

	// ========================================================= //
	// 2. 다운샘플링 루프 (Mip 1 ~ N-1)
	// ========================================================= //
	InDeviceContext->CSSetShader(HiZDownSampleShader, nullptr, 0);
	InDeviceContext->CSSetSamplers(0, 1, &HiZSamplerState);

	for (UINT Mip = 1; Mip < MipLevels; ++Mip)
	{
		UINT MipWidth = std::max(1u, Width >> Mip);
		UINT MipHeight = std::max(1u, Height >> Mip);

		// --- (1) 상수 버퍼 업데이트 ---
		FHiZDownsampleConstants constants;
		constants.TextureWidth	= MipWidth;
		constants.TextureHeight = MipHeight;
		constants.MipLevel		= Mip;
		constants.Padding		= 0;

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		if (SUCCEEDED(InDeviceContext->Map(HiZDownsampleConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
		{
			memcpy(MappedResource.pData, &constants, sizeof(FHiZDownsampleConstants));
			InDeviceContext->Unmap(HiZDownsampleConstantBuffer, 0);
		}
		InDeviceContext->CSSetConstantBuffers(0, 1, &HiZDownsampleConstantBuffer);

		// --- (2) 이전 Mip을 SRV로, 현재 Mip을 UAV로 바인딩 ---
		InDeviceContext->CSSetShaderResources(0, 1, &HiZShaderResourceViews[Mip - 1]);
		InDeviceContext->CSSetUnorderedAccessViews(0, 1, &HiZUnorderedAccessViews[Mip], nullptr);

		// --- (3) 다운샘플 Dispatch ---
		InDeviceContext->Dispatch((MipWidth + 15) / 16, (MipHeight + 15) / 16, 1);

		// --- (4) 리소스 언바인딩 ---
		{
			ID3D11ShaderResourceView* NullSRV  = nullptr;
			ID3D11UnorderedAccessView* NullUAV = nullptr;
			ID3D11Buffer* NullCB			   = nullptr;
			ID3D11SamplerState* NullSampler = nullptr;

			InDeviceContext->CSSetShaderResources(0, 1, &NullSRV);
			InDeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUAV, nullptr);
			InDeviceContext->CSSetConstantBuffers(0, 1, &NullCB);
			InDeviceContext->CSSetSamplers(0, 1, &NullSampler);
		}
	}

	// ========================================================= //
	// 3. 최종 정리
	// ========================================================= //
	InDeviceContext->CSSetShader(nullptr, nullptr, 0);
}


void UOcclusionRenderer::OcclusionTest(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	// ========================================================= //
	// 초기화: 입력이 비어있으면 바로 리턴
	// ========================================================= //
	if (BoundingVolumes.empty())
	{
		return;
	}

	// ========================================================= //
	// 1. 상수 버퍼 업데이트
	// ========================================================= //
	InDeviceContext->CSSetShader(HiZOcclusionShader, nullptr, 0);

	FHiZOcclusionConstants Constants;
	Constants.NumBoundingVolumes = BoundingVolumes.size();
	Constants.ScreenSize		 = FVector2((float)Width, (float)Height);
	Constants.MipLevels			 = MipLevels;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HRESULT hResult = InDeviceContext->Map(HiZOcclusionConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	if (FAILED(hResult)) return;

	memcpy(MappedResource.pData, &Constants, sizeof(FHiZOcclusionConstants));
	InDeviceContext->Unmap(HiZOcclusionConstantBuffer, 0);

	InDeviceContext->CSSetConstantBuffers(0, 1, &HiZOcclusionConstantBuffer);

	// ========================================================= //
	// 2. 출력 UAV (가시성 플래그 버퍼)
	// ========================================================= //
	D3D11_BUFFER_DESC BufferDesc   = {};
	BufferDesc.ByteWidth		   = sizeof(uint32) * BoundingVolumes.size();
	BufferDesc.Usage			   = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags		   = D3D11_BIND_UNORDERED_ACCESS;
	BufferDesc.MiscFlags		   = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BufferDesc.StructureByteStride = sizeof(uint32);

	ID3D11Buffer* VisibilityUAVBuffer = nullptr;
	if (FAILED(InDevice->CreateBuffer(&BufferDesc, nullptr, &VisibilityUAVBuffer))) return;

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.Format							 = DXGI_FORMAT_UNKNOWN;
	UAVDesc.ViewDimension					 = D3D11_UAV_DIMENSION_BUFFER;
	UAVDesc.Buffer.NumElements				 = BoundingVolumes.size();

	ID3D11UnorderedAccessView* VisibilityUAV = nullptr;
	if (FAILED(InDevice->CreateUnorderedAccessView(VisibilityUAVBuffer, &UAVDesc, &VisibilityUAV))) return;

	// ========================================================= //
	// 3. 입력 StructuredBuffer (BoundingVolumes)
	// ========================================================= //
	D3D11_BUFFER_DESC BVDesc   = {};
	BVDesc.ByteWidth		   = sizeof(FBoundingVolume) * BoundingVolumes.size();
	BVDesc.Usage			   = D3D11_USAGE_DEFAULT;
	BVDesc.BindFlags		   = D3D11_BIND_SHADER_RESOURCE;
	BVDesc.MiscFlags		   = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BVDesc.StructureByteStride = sizeof(FBoundingVolume);

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem				= BoundingVolumes.data();

	ID3D11Buffer* BVBuffer = nullptr;
	if (FAILED(InDevice->CreateBuffer(&BVDesc, &InitData, &BVBuffer))) return;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format							= DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension					= D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.NumElements				= (UINT)BoundingVolumes.size();

	ID3D11ShaderResourceView* BVSRV = nullptr;
	if (FAILED(InDevice->CreateShaderResourceView(BVBuffer, &SRVDesc, &BVSRV)))
	{
		BVBuffer->Release();
		return;
	}

	// ========================================================= //
	// 4. 리소스 바인딩
	// ========================================================= //
	InDeviceContext->CSSetShaderResources(0, 1, &BVSRV);
	InDeviceContext->CSSetShaderResources(1, 1, &HiZFullMipShaderResourceView);
	InDeviceContext->CSSetSamplers(0, 1, &HiZSamplerState);
	InDeviceContext->CSSetUnorderedAccessViews(0, 1, &VisibilityUAV, nullptr);

	// ========================================================= //
	// 5. Compute Shader 실행
	// ========================================================= //
	UINT numGroups = (BoundingVolumes.size() + 63) / 64; // 64 threads per group
	InDeviceContext->Dispatch(numGroups, 1, 1);

	// 리소스 언바인딩
	ID3D11ShaderResourceView* NullSRV[2] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* NullUAV	 = nullptr;
	ID3D11Buffer* NullCB				 = nullptr;
	ID3D11SamplerState* NullSampler		 = nullptr;
	InDeviceContext->CSSetShaderResources(0, 2, NullSRV);
	InDeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUAV, nullptr);
	InDeviceContext->CSSetConstantBuffers(0, 1, &NullCB);
	InDeviceContext->CSSetSamplers(0, 1, &NullSampler);

	// ========================================================= //
	// 6. 결과 Readback
	// ========================================================= //
	D3D11_BUFFER_DESC ReadbackDesc	 = {};
	ReadbackDesc.ByteWidth			 = sizeof(uint32) * BoundingVolumes.size();
	ReadbackDesc.Usage				 = D3D11_USAGE_STAGING;
	ReadbackDesc.CPUAccessFlags		 = D3D11_CPU_ACCESS_READ;
	ReadbackDesc.MiscFlags			 = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	ReadbackDesc.StructureByteStride = sizeof(uint32);

	ID3D11Buffer* ReadbackBuffer = nullptr;
	if (FAILED(InDevice->CreateBuffer(&ReadbackDesc, nullptr, &ReadbackBuffer))) return;

	InDeviceContext->CopyResource(ReadbackBuffer, VisibilityUAVBuffer);

	hResult = InDeviceContext->Map(ReadbackBuffer, 0, D3D11_MAP_READ, 0, &MappedResource);
	if (SUCCEEDED(hResult))
	{
		uint32* flags = reinterpret_cast<uint32*>(MappedResource.pData);
		uint32 CulledObjectCount = 0;
		for (size_t i = 0; i < BoundingVolumes.size(); ++i)
		{
			auto& history = VisibilityHistory[PrimitiveComponentUUIDs[i]];
			history <<= 1;
			history |= (flags[i] == 1 ? 1u : 0u);
			if (flags[i] == 0)
			{
				CulledObjectCount++;
			}
		}
		InDeviceContext->Unmap(ReadbackBuffer, 0);

		//UE_LOG("Occlusion Culling: %d objects culled this frame.", CulledObjectCount);
	}

	// ========================================================= //
	// 7. 리소스 해제
	// ========================================================= //
	if (VisibilityUAVBuffer) VisibilityUAVBuffer->Release();
	if (VisibilityUAV) VisibilityUAV->Release();
	if (ReadbackBuffer) ReadbackBuffer->Release();
	if (BVSRV) BVSRV->Release();
	if (BVBuffer) BVBuffer->Release();
}

bool UOcclusionRenderer::IsPrimitiveVisible(const UPrimitiveComponent* InPrimitiveComponent) const
{
	if (!InPrimitiveComponent)
	{
		return true;
	}

	uint32 PrimitiveComponentUUID = InPrimitiveComponent->GetUUID();
	auto It = VisibilityHistory.find(PrimitiveComponentUUID);
	if (It != VisibilityHistory.end())
	{
		return It->second.any();
	}
	return true;
}

void UOcclusionRenderer::CreateShader(ID3D11Device* InDevice)
{
	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	HRESULT hResult;

	// Compile HiZDownSampleCS.hlsl
	hResult = D3DCompileFromFile(
		L"Asset/Shader/HiZDownSampleCS.hlsl",
		nullptr,
		nullptr,
		"main",
		"cs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pShaderBlob,
		&pErrorBlob
	);

	if (FAILED(hResult))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}

	hResult = InDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &HiZDownSampleShader);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}
	if (pShaderBlob) pShaderBlob->Release();

	// Compile HiZOcclusionCS.hlsl
	pShaderBlob = nullptr;
	pErrorBlob = nullptr;
	hResult = D3DCompileFromFile(
		L"Asset/Shader/HiZOcclusionCS.hlsl",
		nullptr,
		nullptr,
		"main",
		"cs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pShaderBlob,
		&pErrorBlob
	);

	if (FAILED(hResult))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}

	hResult = InDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &HiZOcclusionShader);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}
	if (pShaderBlob) pShaderBlob->Release();

	// Compile HiZCopyDepthCS.hlsl
	pShaderBlob = nullptr;
	pErrorBlob = nullptr;
	hResult = D3DCompileFromFile(
		L"Asset/Shader/HiZCopyDepthCS.hlsl",
		nullptr,
		nullptr,
		"main",
		"cs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pShaderBlob,
		&pErrorBlob
	);

	if (FAILED(hResult))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}

	hResult = InDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &HiZCopyDepthShader);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}
	if (pShaderBlob) pShaderBlob->Release();

	// Create HiZSamplerState
	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hResult = InDevice->CreateSamplerState(&SamplerDesc, &HiZSamplerState);
	if (FAILED(hResult))
	{
		OutputDebugStringA("Failed to create HiZSamplerState.\n");
		return;
	}

	// Create HiZOcclusionConstantBuffer
	D3D11_BUFFER_DESC HiZOcclusionCbDesc = {};
	HiZOcclusionCbDesc.ByteWidth = sizeof(FHiZOcclusionConstants);
	HiZOcclusionCbDesc.Usage = D3D11_USAGE_DYNAMIC;
	HiZOcclusionCbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HiZOcclusionCbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hResult = InDevice->CreateBuffer(&HiZOcclusionCbDesc, nullptr, &HiZOcclusionConstantBuffer);
	if (FAILED(hResult))
	{
		return;
	}
}

void UOcclusionRenderer::CreateHiZResource(ID3D11Device* InDevice)
{
	MipLevels = static_cast<UINT>(floor(log2(max(Width, Height)))) + 1;

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = MipLevels;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	HRESULT hResult = InDevice->CreateTexture2D(&TextureDesc, nullptr, &HiZTexture);
	if (FAILED(hResult))
	{
		return;
	}

	// Create a single SRV for the entire mip chain (for SampleLevel in OcclusionTest)
	D3D11_SHADER_RESOURCE_VIEW_DESC FullMipSRVDesc = {};
	FullMipSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	FullMipSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	FullMipSRVDesc.Texture2D.MostDetailedMip = 0;
	FullMipSRVDesc.Texture2D.MipLevels = MipLevels;
	hResult = InDevice->CreateShaderResourceView(HiZTexture, &FullMipSRVDesc, &HiZFullMipShaderResourceView);
	if (FAILED(hResult))
	{
		return;
	}

	HiZShaderResourceViews.resize(MipLevels);
	HiZUnorderedAccessViews.resize(MipLevels);

	for (UINT i = 0; i < MipLevels; ++i)
	{
		// Create individual SRVs for each mip level (for CSSetShaderResources in GenerateHiZ)
		D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
		ShaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		ShaderResourceViewDesc.Texture2D.MostDetailedMip = i;
		ShaderResourceViewDesc.Texture2D.MipLevels = 1;
		hResult = InDevice->CreateShaderResourceView(HiZTexture, &ShaderResourceViewDesc, &HiZShaderResourceViews[i]);
		if (FAILED(hResult))
		{
			return;
		}

		// Create individual UAVs for each mip level
		D3D11_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessViewDesc = {};
		UnorderedAccessViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		UnorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		UnorderedAccessViewDesc.Texture2D.MipSlice = i;

		ID3D11UnorderedAccessView* UAV = nullptr;
		hResult = InDevice->CreateUnorderedAccessView(HiZTexture, &UnorderedAccessViewDesc, &HiZUnorderedAccessViews[i]);
		if (FAILED(hResult))
		{
			return;
		}
	}

	D3D11_BUFFER_DESC CbDesc = {};
	CbDesc.ByteWidth = sizeof(FHiZDownsampleConstants);
	CbDesc.Usage = D3D11_USAGE_DYNAMIC;
	CbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	CbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hResult = InDevice->CreateBuffer(&CbDesc, nullptr, &HiZDownsampleConstantBuffer);
	if (FAILED(hResult))
	{
		return;
	}
}

void UOcclusionRenderer::ReleaseShader()
{
	if (HiZDownSampleShader)
	{
		HiZDownSampleShader->Release();
		HiZDownSampleShader = nullptr;
	}
	if (HiZOcclusionShader)
	{
		HiZOcclusionShader->Release();
		HiZOcclusionShader = nullptr;
	}
	if (HiZCopyDepthShader)
	{
		HiZCopyDepthShader->Release();
		HiZCopyDepthShader = nullptr;
	}

	if (HiZSamplerState)
	{
		HiZSamplerState->Release();
		HiZSamplerState = nullptr;
	}
	if (HiZOcclusionConstantBuffer)
	{
		HiZOcclusionConstantBuffer->Release();
		HiZOcclusionConstantBuffer = nullptr;
	}
}



void UOcclusionRenderer::ReleaseHiZResource()
{
	if (HiZTexture)
	{
		HiZTexture->Release();
		HiZTexture = nullptr;
	}
	if (HiZFullMipShaderResourceView)
	{
		HiZFullMipShaderResourceView->Release();
		HiZFullMipShaderResourceView = nullptr;
	}
	for (ID3D11ShaderResourceView* SRV : HiZShaderResourceViews)
	{
		if (SRV) SRV->Release();
	}
	HiZShaderResourceViews.clear();
	for (ID3D11UnorderedAccessView* UAV : HiZUnorderedAccessViews)
	{
		if (UAV) UAV->Release();
	}
	HiZUnorderedAccessViews.clear();

	if (HiZDownsampleConstantBuffer)
	{
		HiZDownsampleConstantBuffer->Release();
		HiZDownsampleConstantBuffer = nullptr;
	}
}
