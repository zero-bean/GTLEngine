#pragma once
#include "Object.h"
#include "VertexData.h"

/**
 * @brief FBX에서 변환된 본 트랙 정보를 유지하며 재생 길이/프레임 정보 등을 제공하는 데이터 컨테이너
 */
// 
class UAnimDataModel : public UObject
{
public:
    DECLARE_CLASS(UAnimDataModel, UObject);

    UAnimDataModel() = default;
    virtual ~UAnimDataModel() override = default;

    // FBoneAnimationTrack 목록을 직접 제어해 FBX에서 넘어온 원시 애니메이션 데이터를 보관한다.
    const TArray<FBoneAnimationTrack>& GetBoneTracks() const { return BoneAnimationTracks; }
    void SetBoneTracks(const TArray<FBoneAnimationTrack>& InTracks);
    void SetBoneTracks(TArray<FBoneAnimationTrack>&& InTracks);
    void AddBoneTrack(const FBoneAnimationTrack& Track);
    void Reset();

    // 본 이름/인덱스 기반으로 빠르게 트랙을 찾을 수 있게 간단한 질의 함수 제공.
    int32 GetNumTracks() const { return static_cast<int32>(BoneAnimationTracks.size()); }
    const FBoneAnimationTrack* FindTrackByBoneName(const FName& BoneName) const;
    const FBoneAnimationTrack* FindTrackByBoneIndex(int32 BoneIndex) const;

    const FFrameRate& GetFrameRate() const { return FrameRate; }
    void SetFrameRate(const FFrameRate& InFrameRate);

    // 파생 메타데이터: 트랙 수, 키 수, 전체 재생 시간
    int32 GetNumberOfFrames() const { return NumberOfFrames; }
    int32 GetNumberOfKeys() const { return NumberOfKeys; }
    // SequenceBase의 TotalPlayLength와 구분되는 DataModel 전용 길이 캐시
    float GetPlayLengthSeconds() const { return DataDurationSeconds; }

private:
    // BoneAnimationTracks 혹은 FrameRate가 바뀔 때 파생값들을 갱신한다.
    void RefreshDerivedData();

    // 본 별 , 프레임 배열이다. 
    TArray<FBoneAnimationTrack> BoneAnimationTracks{}; // FBX에서 추출한 본별 애니메이션 트랙
    TArray<FCurveTrack> CurveTracks;  

public:
    // 커브 트랙 접근자/설정자
    const TArray<FCurveTrack>& GetCurveTracks() const { return CurveTracks; }
    void SetCurveTracks(const TArray<FCurveTrack>& InCurves) { CurveTracks = InCurves; }
    void SetCurveTracks(TArray<FCurveTrack>&& InCurves) { CurveTracks = std::move(InCurves); }
    void AddCurveTrack(const FCurveTrack& Curve) { CurveTracks.Add(Curve); }

    FFrameRate FrameRate{};                           // 원본 샘플링 레이트(FPS)
    int32 NumberOfFrames = 0;                         // 한 트랙에서의 최대 키 개수
    int32 NumberOfKeys = 0;                           // 전체 키 개수 합
    float DataDurationSeconds = 0.f;                  // FrameRate를 적용한 재생 시간(초)
};
