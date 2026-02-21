#pragma once

#include <bitset>

#include <d3d11.h>

#include "Global/Types.h"
#include "Global/Vector.h"
#include "Core/Public/Object.h"
#include "Core/Public/ObjectPtr.h"

class UPrimitiveComponent;

struct FBoundingVolume
{
	FVector4 Min;
	FVector4 Max;
};

struct FHiZDownsampleConstants
{
	UINT TextureWidth;
	UINT TextureHeight;
	UINT MipLevel;
	UINT Padding;
};

UCLASS()
class UOcclusionRenderer :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UOcclusionRenderer, UObject)
public:
	void Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, uint32 InWidth, uint32 InHeight);

	void Release();

	void Resize(uint32 InWidth, uint32 InHeight)
	{
		Width = InWidth;
		Height = InHeight;

		ReleaseHiZResource();
		CreateHiZResource(Device);
	}

	void BuildScreenSpaceBoundingVolumes(ID3D11DeviceContext* InDeviceContext, UCamera* InCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents);

	void GenerateHiZ(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, ID3D11ShaderResourceView* InDepthShaderResourceView);

	void OcclusionTest(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);

	bool IsPrimitiveVisible(const UPrimitiveComponent* InPrimitiveComponent) const;

private:
	static constexpr size_t NUM_WORKER_THREADS = 8;
	static constexpr size_t OCCLUSION_HISTORY_SIZE = 4;

	void CreateShader(ID3D11Device* InDevice);
	void CreateHiZResource(ID3D11Device* InDevice);

	void ReleaseShader();
	void ReleaseHiZResource();

	void ProcessBoundingVolume(size_t InStartIndex, size_t InEndIndex, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents, const FMatrix& InViewProjMatrix);


	/** @note: UOcclusionRenderer에서는 Device와 DeviceContext의 수명을 관리하지 않음*/
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	/** @brief: HiZ resources */
	ID3D11Texture2D* HiZTexture = nullptr;
	TArray<ID3D11ShaderResourceView*> HiZShaderResourceViews;
	ID3D11ShaderResourceView* HiZFullMipShaderResourceView = nullptr;
	TArray<ID3D11UnorderedAccessView*> HiZUnorderedAccessViews;

	ID3D11Buffer* HiZDownsampleConstantBuffer = nullptr;

	ID3D11ComputeShader* HiZDownSampleShader = nullptr;
	ID3D11ComputeShader* HiZOcclusionShader = nullptr;
	ID3D11ComputeShader* HiZCopyDepthShader = nullptr;

	UINT MipLevels;

	struct FHiZOcclusionConstants
	{
		UINT NumBoundingVolumes;
		FVector2 ScreenSize;
		UINT MipLevels;
	};

	/** @brief: Boudning volume resources */
	[[deprecated]] ID3D11Buffer* BoundingVolumeBuffer = nullptr;
	[[deprecated]] ID3D11ShaderResourceView* BoundingVolumeShaderResourceView = nullptr;
	TArray<FBoundingVolume> BoundingVolumes;
	TArray<uint32> PrimitiveComponentUUIDs;

	ID3D11Buffer* HiZOcclusionConstantBuffer = nullptr;
	ID3D11SamplerState* HiZSamplerState = nullptr;

	uint32 Width;
	uint32 Height;

	TMap<uint32, std::bitset<OCCLUSION_HISTORY_SIZE>> VisibilityHistory;

	//ThreadPool Pool;
};
