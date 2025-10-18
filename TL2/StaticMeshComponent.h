#pragma once
#include "MeshComponent.h"
#include "Enums.h"
#include "StaticMesh.h"
#include "FViewport.h"

class UStaticMesh;
class UShader;
class UTexture;
struct FPrimitiveData;
struct FComponentData;

/* *
* @param bChangedByUser - Material의 UI를 통한 유저의 write 상태를 기록합니다.
* @param bOverrideNormalTexture - true에서 NormalTextureOverride 값을 사용합니다.
*           false에서 MaterialName에 해당하는 원본 머터리얼 노멀 맵을 사용합니다.
* @param NormalTextureOverride - 유저가 수정한 오버라이드할 텍스처 포인터입니다.
*/
struct FMaterialSlot
{
    FString MaterialName;
    UTexture* NormalTextureOverride = nullptr;
    bool bOverrideNormalTexture = false; 
    bool bChangedByUser = false; 
};

class UStaticMeshComponent : public UMeshComponent
{
public:
    DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)
    UStaticMeshComponent();

protected:
    ~UStaticMeshComponent() override;

public:
    void Render(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj, const EEngineShowFlags ShowFlags) override;

    void SetStaticMesh(const FString& PathFileName);
    UStaticMesh* GetStaticMesh() const { return StaticMesh; }

    /**
     * @brief 특정 슬롯의 노멀 맵 오버라이드를 설정합니다.
     * @param SlotIndex 머티리얼 슬롯 인덱스
     * @param InTexture 오버라이드할 텍스처. (nullptr은 'None'으로 오버라이드)
     */
    void SetNormalTextureOverride(int32 SlotIndex, UTexture* InTexture);

    /**
     * @brief 특정 슬롯의 노멀 맵 오버라이드를 제거하고 원본 머티리얼의 설정을 사용합니다.
     * @param SlotIndex 머티리얼 슬롯 인덱스
     */
    void ClearNormalTextureOverride(int32 SlotIndex);

    /**
     * @brief 특정 슬롯의 노멀 맵 오버라이드 텍스처를 가져옵니다.
     * @return 오버라이드가 활성화되어 있으면 텍스처(혹은 nullptr), 아니면 nullptr.
     * @note 오버라이드 여부는 HasNormalTextureOverride로 확인해야 합니다.
     */
    UTexture* GetNormalTextureOverride(int32 SlotIndex) const;

    /**
     * @brief 특정 슬롯에 노멀 맵 오버라이드가 활성화되어 있는지 확인합니다.
     * @return 오버라이드가 활성화되어 있으면 true
     */
    bool HasNormalTextureOverride(int32 SlotIndex) const;

    // 씬 포맷(FPrimitiveData)을 이용한 컴포넌트 직렬화/역직렬화
    // - bIsLoading == true  : InOut로부터 읽어서 컴포넌트 상태(메시) 설정
    // - bIsLoading == false : 컴포넌트 상태를 InOut에 기록
    void Serialize(bool bIsLoading, FPrimitiveData& InOut);

    // V2 씬 포맷(FComponentData)을 이용한 컴포넌트 직렬화/역직렬화
    void Serialize(bool bIsLoading, FComponentData& InOut);

    void SetMaterialByUser(const uint32 InMaterialSlotIndex, const FString& InMaterialName);

    const TArray<FMaterialSlot>& GetMaterailSlots() const { return MaterailSlots; }
    const FAABB GetWorldAABB() const override;
    const FAABB& GetLocalAABB() const;
    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

    // Editor Details
    void RenderDetails() override;



protected:
    // [PIE] 주소 복사 / NOTE: 만약 복사 후에도 GPU 버퍼 내용을 다르게 갖고 싶은 경우 깊은 복사를 해서 버퍼를 2개 생성하는 방법도 고려
    UStaticMesh* StaticMesh = nullptr;
    // [PIE] 값 복사 (배열 전체 값 복사)
    TArray<FMaterialSlot> MaterailSlots;
};

