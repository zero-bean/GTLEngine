#pragma once
#include "ResourceBase.h"

/*
* @brief 모든 종류의 애니메이션의 공통된 최상위 부모 클래스
* @note 해당 클래스 타입을 활용하여 애니메이션 관련 클래스 제어가 가능합니다.
*/
class UAnimAsset : public UResourceBase
{
public:
    DECLARE_CLASS(UAnimAsset, UResourceBase);

    UAnimAsset() = default;
    virtual ~UAnimAsset() override = default;

    // 모든 애니메이션(시퀀스, 몽타주 등)의 재생 길이 반환
    virtual float GetPlayLength() const { return 0.f; }

    // UResourceBase의 FilePath를 사용하므로 SourceFile 관련 함수는 FilePath로 대체
    // GetFilePath(), SetFilePath()는 UResourceBase에서 상속

    const FString& GetAssetName() const { return AssetName; }
    void SetAssetName(const FString& InAssetName) { AssetName = InAssetName; }

    const FString& GetSkeletonName() const { return SkeletonName; }
    void SetSkeletonName(const FString& InSkeletonName) { SkeletonName = InSkeletonName; }
    bool HasSkeletonBinding() const { return !SkeletonName.empty(); }

    // 공통 메타데이터 직렬화
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
    // FilePath는 UResourceBase에서 상속받음
    FString AssetName = {};      // 에셋 표시 이름
    FString SkeletonName = {};   // 레퍼런스 스켈레톤 이름
};

