#pragma once
#include <d3d11.h>

/**
 * @brief FLightPass가 생성한 Light 관련 리소스들을 다른 Pass와 공유하기 위한 구조체
 *
 * FLightPass가 이 구조체를 소유하며, Execute() 시점에 FRenderingContext를 통해
 * 다른 Pass들에게 전달됩니다. 리소스의 생명주기는 FLightPass가 관리합니다.
 */
struct FSharedLightResources
{
	/** Ambient + Directional Light 정보 (cbuffer GlobalLightConstant) */
	ID3D11Buffer* GlobalLightConstantBuffer = nullptr;

	/** Cluster 설정 정보 (cbuffer ClusterSliceInfo) */
	ID3D11Buffer* ClusterSliceInfoConstantBuffer = nullptr;

	/** Light 개수 정보 (cbuffer LightCountInfo) */
	ID3D11Buffer* LightCountInfoConstantBuffer = nullptr;

	/** Point Light Indices (Clustered, StructuredBuffer<int>) */
	ID3D11ShaderResourceView* PointLightIndicesSRV = nullptr;

	/** Spot Light Indices (Clustered, StructuredBuffer<int>) */
	ID3D11ShaderResourceView* SpotLightIndicesSRV = nullptr;

	/** Point Light 정보 배열 (StructuredBuffer<FPointLightInfo>) */
	ID3D11ShaderResourceView* PointLightInfosSRV = nullptr;

	/** Spot Light 정보 배열 (StructuredBuffer<FSpotLightInfo>) */
	ID3D11ShaderResourceView* SpotLightInfosSRV = nullptr;
};
