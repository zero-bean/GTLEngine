#pragma once

class UAmbientLightComponent;
class UDirectionalLightComponent;
class UPointLightComponent;
class USpotLightComponent;
class ULightComponent;
class D3D11RHI;
class FShadowMap;
class USceneComponent;

enum class ELightType
{
    AmbientLight,
    DirectionalLight,
    PointLight,
    SpotLight,
};
struct FAmbientLightInfo
{
    FLinearColor Color;     // 16 bytes - Color already includes Intensity and Temperature
    // Total: 16 bytes
};

struct FDirectionalLightInfo
{
    FLinearColor Color;     // 16 bytes - Color already includes Intensity and Temperature
    FVector Direction;      // 12 bytes
    float Padding;          // 4 bytes padding to reach 32 bytes (16-byte aligned)
    // Total: 32 bytes
};

struct FPointLightInfo
{
    FLinearColor Color;         // 16 bytes - Color already includes Intensity and Temperature
    FVector Position;           // 12 bytes
    float AttenuationRadius;    // 4 bytes (moved up to fill slot)
    float FalloffExponent;      // 4 bytes - Falloff exponent for artistic control
    uint32 bUseInverseSquareFalloff; // 4 bytes - true = physically accurate, false = exponent-based
	FVector2D Padding;            // 8 bytes padding to reach 48 bytes (16-byte aligned)
	// Total: 48 bytes
};

struct FSpotLightInfo
{
    FLinearColor Color;         // 16 bytes - Color already includes Intensity and Temperature
    FVector Position;           // 12 bytes
    float InnerConeAngle;       // 4 bytes
    FVector Direction;          // 12 bytes
    float OuterConeAngle;       // 4 bytes
    float AttenuationRadius;    // 4 bytes
    float FalloffExponent;      // 4 bytes - Falloff exponent for artistic control
    uint32 bUseInverseSquareFalloff; // 4 bytes - true = physically accurate, false = exponent-based
    uint32 bCastShadow;         // 4 bytes - true = this light casts shadow
    uint32 ShadowMapIndex;      // 4 bytes - index to shadow map array (-1 if no shadow)
    FVector Padding;            // 4 bytes - padding
    FMatrix LightViewProjection; // 64 bytes - Light space transformation matrix
	// Total: 144 bytes
};

class FLightManager
{

public:
    FLightManager() = default;
    ~FLightManager();

    void Initialize(D3D11RHI* RHIDevice);
    void Release();

    void UpdateLightBuffer(D3D11RHI* RHIDevice);
    void SetDirtyFlag();

    /**
    * @brief 매 프레임에 활성화된 스포트라이트 목록을 기반으로 섀도우 맵 인덱스를 할당합니다.
    * @param VisibleSpotLights - SceneRenderer가 수집한, 현재 뷰에 보이는 스포트라이트 목록
    */
    void AssignShadowMapIndices(D3D11RHI* RHIDevice, const TArray<USpotLightComponent*>& VisibleSpotLights);

    /**
     * @brief 특정 라이트의 섀도우 맵 렌더링을 시작하고, 렌더링에 필요한 행렬을 반환합니다.
     * @param RHI - RHI 디바이스
     * @param Light - 렌더링할 스포트라이트
     * @param OutLightView - (출력) 라이트의 뷰 행렬
     * @param OutLightProj - (출력) 라이트의 투영 행렬
     * @return 렌더링할 섀도우 맵 배열 인덱스. 할당되지 않았다면 -1.
     */
    int32 BeginShadowMapRender(D3D11RHI* RHI, USpotLightComponent* Light, FMatrix& OutLightView, FMatrix& OutLightProj);

    /**
     * @brief 섀도우 맵 렌더링을 종료합니다. (현재는 특별한 작업 없음, 추후 확장용)
     */
    void EndShadowMapRender(D3D11RHI* RHI);

    TArray<FPointLightInfo>& GetPointLightInfoList() { return PointLightInfoList; }
    TArray<FSpotLightInfo>& GetSpotLightInfoList() { return SpotLightInfoList; }

    /**
     * Bind shadow map textures to shader
     * @param RHIDevice - D3D11 device interface
     */
    void BindShadowMaps(D3D11RHI* RHIDevice);

    template<typename T>
    void RegisterLight(T* LightComponent);
    template<typename T>
    void DeRegisterLight(T* LightComponent);
    template<typename T>
    void UpdateLight(T* LightComponent);

    void ClearAllLightList();
private:
    /**
     * Create shadow map for a specific spotlight
     * @param RHIDevice - D3D11 device interface
     * @param SpotLight - The spotlight component
     * @return Shadow map index, or -1 if failed
     */
    int32 CreateShadowMapForLight(D3D11RHI* RHIDevice, USpotLightComponent* SpotLight);

    bool bHaveToUpdate = true;
    bool bPointLightDirty = true;
    bool bSpotLightDirty = true;

    //structured buffer
    ID3D11Buffer* PointLightBuffer = nullptr;
    ID3D11Buffer* SpotLightBuffer = nullptr;
    ID3D11ShaderResourceView* PointLightBufferSRV = nullptr;
    ID3D11ShaderResourceView* SpotLightBufferSRV = nullptr;

    // Shadow mapping resources
    FShadowMap* ShadowMapArray; // Single shadow map array for all shadow-casting spotlights
    TMap<USpotLightComponent*, int32> LightToShadowMapIndex; // Map light to shadow map index

    TArray<UAmbientLightComponent*> AmbientLightList;
    TArray<UDirectionalLightComponent*> DIrectionalLightList;
    TArray<UPointLightComponent*> PointLightList;
    TArray<USpotLightComponent*> SpotLightList;

    //업데이트 시에만 clear하고 다시 수집할 실제 데이터 리스트
    TArray<FPointLightInfo> PointLightInfoList;
    TArray<FSpotLightInfo> SpotLightInfoList;

    //이미 레지스터된 라이트인지 확인하는 용도
    TSet<ULightComponent*> LightComponentList;
    uint32 PointLightNum = 0;
    uint32 SpotLightNum = 0;
};

template<> void FLightManager::RegisterLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent);
template<> void FLightManager::RegisterLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent);
template<> void FLightManager::RegisterLight<UPointLightComponent>(UPointLightComponent* LightComponent);
template<> void FLightManager::RegisterLight<USpotLightComponent>(USpotLightComponent* LightComponent);

template<> void FLightManager::DeRegisterLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent);
template<> void FLightManager::DeRegisterLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent);
template<> void FLightManager::DeRegisterLight<UPointLightComponent>(UPointLightComponent* LightComponent);
template<> void FLightManager::DeRegisterLight<USpotLightComponent>(USpotLightComponent* LightComponent);

template<> void FLightManager::UpdateLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent);
template<> void FLightManager::UpdateLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent);
template<> void FLightManager::UpdateLight<UPointLightComponent>(UPointLightComponent* LightComponent);
template<> void FLightManager::UpdateLight<USpotLightComponent>(USpotLightComponent* LightComponent);