#pragma once
#include "Render/RenderPass/Public/RenderPass.h"
#include "Texture/Public/ShadowMapResources.h"
#include "Global/Types.h"
#include "Render/RenderPass/Public/ShadowData.h"
#include "Manager/Render/Public/CascadeManager.h"

class UMeshComponent;
class ULightComponent;
class UDirectionalLightComponent;
class USpotLightComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * @brief Shadow map 렌더링 전용 pass
 *
 * 각 라이트의 shadow map을 렌더링합니다:
 * - Directional Light: 단일 shadow map (orthographic projection)
 * - Spot Light: 단일 shadow map (perspective projection)
 * - Point Light: Cube shadow map (6면, omnidirectional)
 *
 * StaticMeshPass 이전에 실행되어 depth map을 준비합니다.
 */
class FShadowMapPass : public FRenderPass
{
public:
	FShadowMapPass(UPipeline* InPipeline,
		ID3D11Buffer* InConstantBufferCamera,
		ID3D11Buffer* InConstantBufferModel,
		ID3D11VertexShader* InDepthOnlyVS,
		ID3D11PixelShader* InDepthOnlyPS,
		ID3D11InputLayout* InDepthOnlyInputLayout,
		ID3D11VertexShader* InPointLightShadowVS,
		ID3D11PixelShader* InPointLightShadowPS,
		ID3D11InputLayout* InPointLightShadowInputLayout);

	~FShadowMapPass() override;

	void SetRenderTargets(class UDeviceResources* DeviceResources) override;
	void Execute(FRenderingContext& Context) override;
	void Release() override;

	// --- Public Getters ---
	/**
	 * @brief Directional light의 shadow map 리소스를 가져옵니다.
	 * @param Light Directional light component
	 * @return Shadow map 리소스 포인터 (없으면 nullptr)
	 */
	FShadowMapResource* GetDirectionalShadowMap(UDirectionalLightComponent* Light) { return DirectionalShadowMaps.FindRef(Light); }

	/**
	 * @brief Spot light의 shadow map 리소스를 가져옵니다.
	 * @param Light Spot light component
	 * @return Shadow map 리소스 포인터 (없으면 nullptr)
	 */
	FShadowMapResource* GetSpotShadowMap(USpotLightComponent* Light) { return SpotShadowMaps.FindRef(Light); }

	/**
	 * @brief Point light의 cube shadow map 리소스를 가져옵니다.
	 * @param Light Point light component
	 * @return Cube shadow map 리소스 포인터 (없으면 nullptr)
	 */
	FCubeShadowMapResource* GetPointShadowMap(UPointLightComponent* Light) { return PointShadowMaps.FindRef(Light); }

	FShadowMapResource* GetShadowAtlas() { return &ShadowAtlas; }

	// --- Shadow Resources Access ---
	// TODO: 나중에 FSharedShadowResources 구조체를 만들어 FSharedLightResources와 같은 패턴으로 리팩토링할 것

	/**
	 * @brief Cascade Shadow Map 데이터 Constant Buffer 반환
	 * @return CascadeShadowMapData constant buffer (b6)
	 */
	ID3D11Buffer* GetCascadeShadowMapDataBuffer() const { return ConstantCascadeData; }

	/**
	 * @brief Directional Light Tile Position SRV 반환
	 * @return Directional light shadow atlas tile position SRV (t12)
	 */
	ID3D11ShaderResourceView* GetDirectionalLightTilePosSRV() const { return ShadowAtlasDirectionalLightTilePosStructuredSRV; }

	/**
	 * @brief Spot Light Tile Position SRV 반환
	 * @return Spot light shadow atlas tile position SRV (t13)
	 */
	ID3D11ShaderResourceView* GetSpotLightTilePosSRV() const { return ShadowAtlasSpotLightTilePosStructuredSRV; }

	/**
	 * @brief Point Light Tile Position SRV 반환
	 * @return Point light shadow atlas tile position SRV (t14)
	 */
	ID3D11ShaderResourceView* GetPointLightTilePosSRV() const { return ShadowAtlasPointLightTilePosStructuredSRV; }

	/**
	 * @brief Directional Light의 Atlas Tile 위치를 가져옵니다.
	 * @param Index Directional Light의 인덱스
	 * @return Directional Light의 Atlas Tile 위치
	 * @todo ShadowMapPass에 저장되어있는 인덱스와 외부에서 활용되는 인덱스 일치여부 확인
	 */
	FShadowAtlasTilePos GetDirectionalAtlasTilePos(uint32 Index) const { return ShadowAtlasDirectionalLightTilePosArray[Index]; }

	/**
	 * @brief Spotlight의 Atlas Tile 위치를 가져옵니다.
	 * @param Index Spotlight의 인덱스
	 * @return Spotlight의 Atlas Tile 위치
	 * @todo ShadowMapPass에 저장되어있는 인덱스와 외부에서 활용되는 인덱스 일치여부 확인
	 */
	FShadowAtlasTilePos GetSpotAtlasTilePos(uint32 Index) const { return ShadowAtlasSpotLightTilePosArray[Index]; }

	// Shadow stat information
	uint64 GetTotalShadowMapMemory() const;
	uint32 GetUsedAtlasTileCount() const;
	static uint32 GetMaxAtlasTileCount();

	/**
	 * @brief Point Light의 Atlas Tile 위치를 가져옵니다.
	 * @param Index Point Light의 인덱스
	 * @return Point Light의 Atlas Tile 위치
	 * @todo ShadowMapPass에 저장되어있는 인덱스와 외부에서 활용되는 인덱스 일치여부 확인
	 */
	FShadowAtlasPointLightTilePos GetPointAtlasTilePos(uint32 Index) const { return ShadowAtlasPointLightTilePosArray[Index]; }

private:
	// --- Directional Light Shadow Rendering ---
	/**
	 * @brief Directional light의 shadow map을 렌더링합니다.
	 * @param Light Directional light component
	 * @param StaticMeshes 렌더링할 static mesh 목록
	 * @param SkeletalMeshes 렌더링할 skeletal mesh 목록
	 */
	void RenderDirectionalShadowMap(
		UDirectionalLightComponent* Light,
		const TArray<UStaticMeshComponent*>& StaticMeshes,
		const TArray<USkeletalMeshComponent*>& SkeletalMeshes,
		const FMinimalViewInfo& InViewInfo
		);

	// --- Spot Light Shadow Rendering ---
	/**
	 * @brief Spot light의 shadow map을 렌더링합니다.
	 * @param Light Spot light component
	 * @param StaticMeshes 렌더링할 static mesh 목록
	 * @param SkeletalMeshes 렌더링할 skeletal mesh 목록
	 */
	void RenderSpotShadowMap(
		USpotLightComponent* Light,
		uint32 AtlasIndex,
		const TArray<UStaticMeshComponent*>& StaticMeshes,
		const TArray<USkeletalMeshComponent*>& SkeletalMeshes
	);

	// --- Point Light Shadow Rendering (6 faces) ---
	/**
	 * @brief Point light의 cube shadow map을 렌더링합니다 (6면).
	 * @param Light Point light component
	 * @param StaticMeshes 렌더링할 static mesh 목록
	 * @param SkeletalMeshes 렌더링할 skeletal mesh 목록
	 */
	void RenderPointShadowMap(
		UPointLightComponent* Light,
		uint32 AtlasIndex,
		const TArray<UStaticMeshComponent*>& StaticMeshes,
		const TArray<USkeletalMeshComponent*>& SkeletalMeshes
	);

	void SetShadowAtlasTilePositionStructuredBuffer();

	// --- Helper Functions ---
	/**
	 * @brief Directional light의 view-projection 행렬을 계산합니다.
	 * @param Light Directional light component
	 * @param StaticMeshes 렌더링할 메시 목록 (AABB 계산용)
	 * @param SkeletalMeshes 렌더링할 메시 목록 (AABB 계산용)
	 * @param InCamera Scene camera for PSM calculation
	 * @param OutView 출력 view matrix
	 * @param OutProj 출력 projection matrix
	 */
	void CalculateDirectionalLightViewProj(
		UDirectionalLightComponent* Light,
		const TArray<UStaticMeshComponent*>& StaticMeshes,
		const TArray<USkeletalMeshComponent*>& SkeletalMeshes,
		const FMinimalViewInfo& InViewInfo,
		FMatrix& OutView,
		FMatrix& OutProj);

	/**
	 * @brief Uniform Shadow Map의 view-projection 행렬을 계산합니다 (Sample 버전).
	 * @param Light Directional light component
	 * @param StaticMeshes 렌더링할 스태틱 메시 목록
	 * @param SkeletalMeshes 렌더링할 스켈레탈 메시 목록
	 * @param OutView 출력 view matrix
	 * @param OutProj 출력 projection matrix
	 */
	void CalculateUniformShadowMapViewProj(
		UDirectionalLightComponent* Light,
		const TArray<UStaticMeshComponent*>& StaticMeshes,
		const TArray<USkeletalMeshComponent*>& SkeletalMeshes,
		FMatrix& OutView,
		FMatrix& OutProj);

	/**
	 * @brief Spot light의 view-projection 행렬을 계산합니다.
	 * @param Light Spot light component
	 * @param OutView 출력 view matrix
	 * @param OutProj 출력 projection matrix
	 */
	void CalculateSpotLightViewProj(USpotLightComponent* Light, FMatrix& OutView, FMatrix& OutProj);

	/**
	 * @brief Point light의 6면에 대한 view-projection 행렬을 계산합니다.
	 * @param Light Point light component
	 * @param OutViewProj 출력 배열 (6개)
	 */
	void CalculatePointLightViewProj(UPointLightComponent* Light, FMatrix OutViewProj[6]);

	/**
	 * @brief Shadow map 리소스를 가져오거나 생성합니다.
	 * @param Light Light component
	 * @return Shadow map 리소스 포인터
	 */
	FShadowMapResource* GetOrCreateShadowMap(ULightComponent* Light);

	/**
	 * @brief Cube shadow map 리소스를 가져오거나 생성합니다.
	 * @param Light Point light component
	 * @return Cube shadow map 리소스 포인터
	 */
	FCubeShadowMapResource* GetOrCreateCubeShadowMap(UPointLightComponent* Light);

	void RenderMeshDepth(const UMeshComponent* InMesh, const FMatrix& InView, const FMatrix& InProj) const;

	// /**
	//  * @brief Directional light의 rasterizer state를 가져오거나 생성합니다.
	//  *
	//  * Light별로 DepthBias/SlopeScaledDepthBias가 다르므로, 각 light마다
	//  * 전용 rasterizer state를 캐싱합니다. 매 프레임 생성/해제를 방지하여 성능 향상.
	//  *
	//  * @param Light Directional light component
	//  * @return Light 전용 rasterizer state
	//  */
	// ID3D11RasterizerState* GetOrCreateRasterizerState(UDirectionalLightComponent* Light);
	//
	// /**
	//  * @brief Spot light의 rasterizer state를 가져오거나 생성합니다.
	//  *
	//  * Light별로 DepthBias/SlopeScaledDepthBias가 다르므로, 각 light마다
	//  * 전용 rasterizer state를 캐싱합니다. 매 프레임 생성/해제를 방지하여 성능 향상.
	//  *
	//  * @param Light Spot light component
	//  * @return Light 전용 rasterizer state
	//  */
	// ID3D11RasterizerState* GetOrCreateRasterizerState(USpotLightComponent* Light);
	//
	// /**
	//  * @brief Point light의 rasterizer state를 가져오거나 생성합니다.
	//  *
	//  * Light별로 DepthBias/SlopeScaledDepthBias가 다르므로, 각 light마다
	//  * 전용 rasterizer state를 캐싱합니다. 매 프레임 생성/해제를 방지하여 성능 향상.
	//  *
	//  * @param Light Point light component
	//  * @return Light 전용 rasterizer state
	//  */
	// ID3D11RasterizerState* GetOrCreateRasterizerState(UPointLightComponent* Light);

	ID3D11RasterizerState* GetOrCreateRasterizerState(
		float InShadowBias,
		float InShadowSlopBias
		);

private:
	// Shaders
	ID3D11VertexShader* DepthOnlyVS = nullptr;
	ID3D11PixelShader* DepthOnlyPS = nullptr;
	ID3D11InputLayout* DepthOnlyInputLayout = nullptr;

	// Point Light Shadow Shaders (with linear distance)
	ID3D11VertexShader* LinearDepthOnlyVS = nullptr;
	ID3D11PixelShader* LinearDepthOnlyPS = nullptr;
	ID3D11InputLayout* PointLightShadowInputLayout = nullptr;

	// States
	ID3D11DepthStencilState* ShadowDepthStencilState = nullptr;
	ID3D11RasterizerState* ShadowRasterizerState = nullptr;

	// Shadow map 리소스 관리 (동적 할당)
	TMap<UDirectionalLightComponent*, FShadowMapResource*> DirectionalShadowMaps;
	TMap<USpotLightComponent*, FShadowMapResource*> SpotShadowMaps;
	TMap<UPointLightComponent*, FCubeShadowMapResource*> PointShadowMaps;

	// Rasterizer state 캐싱 (매 프레임 생성/해제 방지)
	// TMap<UDirectionalLightComponent*, ID3D11RasterizerState*> DirectionalRasterizerStates;
	// TMap<USpotLightComponent*, ID3D11RasterizerState*> SpotRasterizerStates;
	// TMap<UPointLightComponent*, ID3D11RasterizerState*> PointRasterizerStates;

	TMap<FString, ID3D11RasterizerState*> LightRasterizerStates;

	// Rasterizer State 캐싱 (Bias, Splop Bias를 키값으로 접근)

	// Constant buffers (DepthOnlyVS.hlsl의 ViewProj와 동일)
	ID3D11Buffer* ShadowViewProjConstantBuffer = nullptr;
	ID3D11Buffer* PointLightShadowParamsBuffer = nullptr;

	TArray<FShadowAtlasTilePos> ShadowAtlasDirectionalLightTilePosArray;
	TArray<FShadowAtlasTilePos> ShadowAtlasSpotLightTilePosArray;
	TArray<FShadowAtlasPointLightTilePos> ShadowAtlasPointLightTilePosArray;

	// 실제 렌더링된 라이트 개수 및 타일 개수
	uint32 ActiveDirectionalLightCount = 0; // Directional Light 개수 (0 또는 1)
	uint32 ActiveDirectionalCascadeCount = 0; // CSM Cascade Count (1 ~ 8)
	uint32 ActiveSpotLightCount = 0;
	uint32 ActivePointLightCount = 0;

	ID3D11Buffer* ShadowAtlasDirectionalLightTilePosStructuredBuffer = nullptr;
	ID3D11ShaderResourceView* ShadowAtlasDirectionalLightTilePosStructuredSRV = nullptr;

	ID3D11Buffer* ShadowAtlasSpotLightTilePosStructuredBuffer = nullptr;
	ID3D11ShaderResourceView* ShadowAtlasSpotLightTilePosStructuredSRV = nullptr;

	ID3D11Buffer* ShadowAtlasPointLightTilePosStructuredBuffer = nullptr;
	ID3D11ShaderResourceView* ShadowAtlasPointLightTilePosStructuredSRV = nullptr;

	FShadowMapResource ShadowAtlas{};

	// Handle Cascade Data
	ID3D11Buffer* ConstantCascadeData = nullptr;
};
