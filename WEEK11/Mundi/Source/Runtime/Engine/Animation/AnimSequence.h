#pragma once
#include "AnimSequenceBase.h"
#include "VertexData.h"

class UAnimDataModel;

/**
 * @brief 단일 FBX 시퀀스를 표현하는 애니메이션 에셋.
 * @details 실제 본 키·프레임 데이터는 UAnimDataModel에 저장하고,
 *          상위 UAnimSequenceBase와 재생 길이 정보를 동기화한다.
 */
class UAnimSequence : public UAnimSequenceBase
{
public:
    DECLARE_CLASS(UAnimSequence, UAnimSequenceBase);

    UAnimSequence() = default;
    virtual ~UAnimSequence() override = default;

    void Load(const FString& InFilePath, ID3D11Device* InDevice);

    // 바이너리 캐시 직렬화 (Mesh처럼 .bin 파일 사용)
    void LoadBinary(const FString& InFilePath);
    void SaveBinary(const FString& InFilePath) const override;

    // DataModel은 실제 본 키 데이터를 가진 컨테이너이며 UAnimSequence가 소유한다.
    UAnimDataModel* GetDataModel() { return DataModel.get(); }
    const UAnimDataModel* GetDataModel() const { return DataModel.get(); }
    void SetDataModel(std::unique_ptr<UAnimDataModel> InDataModel);
    void ResetDataModel();

    // SequenceBase 외부에서도 본 트랙 정보를 직접 조회할 수 있도록 얇은 래퍼 제공.
    const TArray<FBoneAnimationTrack>& GetBoneTracks() const;
    const FBoneAnimationTrack* FindTrackByBoneName(const FName& BoneName) const;
    const FBoneAnimationTrack* FindTrackByBoneIndex(int32 BoneIndex) const;

    // 커브 트랙 접근
    const TArray<FCurveTrack>& GetCurveTracks() const;

    // DataModel에서 산출한 파생 메타데이터(길이, 키 수, FPS)를 노출한다.
    int32 GetNumberOfFrames() const;
    int32 GetNumberOfKeys() const;
    const FFrameRate& GetFrameRateStruct() const;
    float GetFrameRate() const { return GetFrameRateStruct().AsDecimal(); }

    // 직렬화
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
    // DataModel 변화를 TotalPlayLength 등 SequenceBase 상태로 반영
    void SyncDerivedMetadata();

    std::unique_ptr<UAnimDataModel> DataModel; // 애니메이션 본 키 데이터 소유 포인터
};
