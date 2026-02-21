// DecalComponent.h
#pragma once
#include "PrimitiveComponent.h"
#include "StaticMesh.h"
#include "BoundingVolume.h"

class UDecalComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

    UDecalComponent();
    virtual ~UDecalComponent() override;

    void TickComponent(float DeltaSeconds) override;
    void Render(URenderer* Renderer, UPrimitiveComponent* Component, const FMatrix& View, const FMatrix& Proj, FViewport* Viewport);

    // UV 타일링 설정
    void SetUVTiling(const FVector2D& InTiling) { UVTiling = InTiling; UpdateDecalProjectionMatrix(); }
    FVector2D GetUVTiling() const { return UVTiling; }

    // 데칼 텍스처 설정
    void SetDecalTexture(FString NewTexturePath);
    void SetDecalSize(FVector InDecalSize);
    FVector GetDecalSize() const { return DecalSize; }
    const FString& GetTexturePath() const { return TexturePath; }
    

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

    // Serialize (V2용)
    void Serialize(bool bIsLoading, FComponentData& InOut) override;

    UStaticMesh* GetDecalBoxMesh() const { return DecalBoxMesh; }
    const FOBB GetWorldOBB() const;
    const TArray<FVector> GetBoundingBoxLines() const override;
    const FVector4 GetBoundingBoxColor() const override
    {
        return FVector4(1, 0, 1, 1);
    }

    // Fade Effect Getter/Setter Func
    int32 GetSortOrder() const { return SortOrder; }
    float GetFadeScreenSize() const { return FadeScreenSize; }
    float GetFadeStartDelay() const { return FadeStartDelay; }
    float GetFadeDuration() const { return FadeDuration; }
    float GetFadeInStartDelay() const { return FadeInStartDelay; }
    float GetFadeInDuration() const { return FadeInDuration; }

    void SetSortOrder(int32 InSortOrder) { SortOrder = InSortOrder; }
    void SetFadeScreenSize(float InFadeScreenSize) { FadeScreenSize = InFadeScreenSize; }
    void SetFadeStartDelay(float InFadeStartDelay) { FadeStartDelay = InFadeStartDelay; }
    void SetFadeDuration(float InFadeDuration) { FadeDuration = InFadeDuration; }
    void SetFadeInStartDelay(float InFadeInStartDelay) { FadeInStartDelay = InFadeInStartDelay; }
    void SetFadeInDuration(float InFadeInDuration) { FadeInDuration = InFadeInDuration; }

    // Editor Details
    void RenderDetails() override;

protected:
    virtual void UpdateDecalProjectionMatrix();

    // 데칼 박스 메쉬 (큐브)
    UStaticMesh* DecalBoxMesh = nullptr;
    FMatrix DecalProjectionMatrix;

    // 데칼 크기
    FString TexturePath;

    // UV 타일링
    FVector2D UVTiling = FVector2D(1.0f, 1.0f);

    // 데칼 블렌드 모드
    enum class EDecalBlendMode
    {
        Translucent,    // 반투명 블렌딩
        Stain,          // 곱셈 블렌딩
        Normal,         // 노멀맵
        Emissive        // 발광
    };

    EDecalBlendMode BlendMode = EDecalBlendMode::Translucent;

private:
    FAABB LocalAABB;
    // Decal fade state
    enum class EFadeState
    {
        None,
        FadingIn,
        FadingOut
    };

    EFadeState CurrentFadeState = EFadeState::None;

    float CurrentAlpha = 1.0f;
    float LifetimeTimer = 0.0f;

    int32 SortOrder = 0;
    float FadeScreenSize = 0.01f;
    float FadeStartDelay = 2.0f;
    float FadeDuration = 5.0f;
    float FadeInStartDelay = 2.0f;
    float FadeInDuration = 5.0f;
    FVector DecalSize{ 1.0f,1.0f,1.0f };

    bool bIsDirty = true;
};