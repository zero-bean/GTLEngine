#pragma once
#include "SceneComponent.h"
#include "BoundingVolume.h"
#include "Material.h"
#include "FViewport.h"

// 전방 선언
struct FPrimitiveData;
struct FComponentData;

class URenderer;

class UPrimitiveComponent :public USceneComponent
{
public:
    DECLARE_ABSTRACT_CLASS(UPrimitiveComponent, USceneComponent)

    UPrimitiveComponent() = default;
    virtual ~UPrimitiveComponent() = default;

    virtual void SetMaterial(const FString& FilePath);
    virtual UMaterial* GetMaterial() { return Material; }

    // 트랜스폼 직렬화/역직렬화 (월드 트랜스폼 기준)
    virtual void Serialize(bool bIsLoading, FPrimitiveData& InOut);

    // 컴포넌트 직렬화/역직렬화 (상대 트랜스폼 기준, V2용)
    virtual void Serialize(bool bIsLoading, FComponentData& InOut);
    virtual void Render(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj, const EEngineShowFlags ShowFlags) {}

    virtual const FAABB GetWorldAABB() const;
    virtual const TArray<FVector> GetBoundingBoxLines() const;
    virtual const FVector4 GetBoundingBoxColor() const;

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;


protected:
    // [PIE] 주소 복사 / NOTE: 만약 복사 후에도 GPU 버퍼 내용을 다르게 갖고 싶은 경우 깊은 복사를 해서 버퍼를 2개 생성하는 방법도 고려
    UMaterial* Material = nullptr;
};