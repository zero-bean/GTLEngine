#pragma once
#include "MeshComponent.h"

class UBillboardComponent : public UMeshComponent
{
public:
    DECLARE_CLASS(UBillboardComponent, UMeshComponent)
    UBillboardComponent();

protected:
    ~UBillboardComponent() override;

public:
    void Render(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj, const EEngineShowFlags ShowFlags) override;
    
    // Size settings
    void SetBillboardSize(float InSize) { BillboardSize = InSize; }
    float GetBillboardWidth() const { return BillboardSize; }
    float GetBillboardHeight() const { return BillboardSize; }
    
    // Texture settings
    void SetTexture(const FString& TexturePath);
    const FString& GetTexturePath() const { return TexturePath; }
    
    // UV coordinate settings
    void SetUVCoords(float U, float V, float UL, float VL);
    float GetU() const { return UCoord; }
    float GetV() const { return VCoord; }
    float GetUL() const { return ULength; }
    float GetVL() const { return VLength; }
    
    // Screen size scaling
    void SetScreenSizeScaled(bool bScaled) { bIsScreenSizeScaled = bScaled; }
    bool IsScreenSizeScaled() const { return bIsScreenSizeScaled; }
    void SetScreenSize(float Size) { ScreenSize = Size; }
    float GetScreenSize() const { return ScreenSize; }

    UTextQuad* GetStaticMesh() const { return BillboardQuad; }

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

    // Serialize (V2용)
    void Serialize(bool bIsLoading, FComponentData& InOut) override;

    // Editor Details
    void RenderDetails() override;

private:
    void CreateBillboardVertices();

private:
    // [PIE] 주소 복사
    UTextQuad* BillboardQuad = nullptr;
    
    // Size properties
    // [PIE] 값 복사
    float BillboardSize = 0.5f;
    
    // Texture properties
    // [PIE] 값 복사
    FString TexturePath = "Editor/Icon/Pawn_64x.dds";
    
    // UV coordinate properties
    // [PIE] 값 복사
    float UCoord = 0.0f;    // U start coordinate
    float VCoord = 0.0f;    // V start coordinate
    float ULength = 1.0f;   // U length (UL)
    float VLength = 1.0f;   // V length (VL)
    
    // Screen scaling properties
    // [PIE] 값 복사
    bool bIsScreenSizeScaled = false;
    float ScreenSize = 0.0025f;
};