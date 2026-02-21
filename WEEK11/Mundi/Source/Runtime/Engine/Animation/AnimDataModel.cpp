#include "pch.h"
#include "AnimDataModel.h"

IMPLEMENT_CLASS(UAnimDataModel)

// 외부에서 구성한 트랙 배열을 받아 복사한다.
void UAnimDataModel::SetBoneTracks(const TArray<FBoneAnimationTrack>& InTracks)
{
    BoneAnimationTracks = InTracks;
    RefreshDerivedData();
}

// 이미 생성된 배열의 소유권을 이전받는 버전.
void UAnimDataModel::SetBoneTracks(TArray<FBoneAnimationTrack>&& InTracks)
{
    BoneAnimationTracks = std::move(InTracks);
    RefreshDerivedData();
}

// 트랙 배열에서 한 트랙씩 추가가 가능하도록 지원합니다.
void UAnimDataModel::AddBoneTrack(const FBoneAnimationTrack& Track)
{
    BoneAnimationTracks.Add(Track);
    RefreshDerivedData();
}

// 모든 데이터를 초기화.
void UAnimDataModel::Reset()
{
    BoneAnimationTracks.clear();
    CurveTracks.clear();
    NumberOfFrames = 0;
    NumberOfKeys = 0;
    DataDurationSeconds = 0.f;
}

// 본 이름으로 트랙을 찾는다. (UI, 에디터에서 사용할지도..)
const FBoneAnimationTrack* UAnimDataModel::FindTrackByBoneName(const FName& BoneName) const
{
    for (const FBoneAnimationTrack& Track : BoneAnimationTracks)
    {
        if (Track.BoneName == BoneName) { return &Track; }
    }

    return nullptr;
}

// 본 인덱스로 트랙을 찾는다. (UI, 에디터에서 사용할지도..)
const FBoneAnimationTrack* UAnimDataModel::FindTrackByBoneIndex(int32 BoneIndex) const
{
    for (const FBoneAnimationTrack& Track : BoneAnimationTracks)
    {
        if (Track.BoneIndex == BoneIndex) { return &Track; }
    }

    return nullptr;
}

// 프레임레이트를 갱신하며 재생 길이 캐시도 다시 계산.
void UAnimDataModel::SetFrameRate(const FFrameRate& InFrameRate)
{
    FrameRate = InFrameRate;
    RefreshDerivedData();
}

// 트랙 목록과 프레임레이트를 바탕으로 파생 메타데이터를 계산한다.
void UAnimDataModel::RefreshDerivedData()
{
    int32 MaxKeysPerTrack = 0;
    int32 TotalKeys = 0;
    float MaxDurationSeconds = 0.f;

    for (const FBoneAnimationTrack& Track : BoneAnimationTracks)
    {
        const int32 TrackKeys = Track.InternalTrack.GetNumKeys();
        MaxKeysPerTrack = std::max(MaxKeysPerTrack, TrackKeys);
        TotalKeys += TrackKeys;

        if (Track.InternalTrack.KeyTimes.empty() == false)
        {
            MaxDurationSeconds = std::max(MaxDurationSeconds, Track.InternalTrack.KeyTimes.back());
        }
    }

    NumberOfFrames = MaxKeysPerTrack;
    NumberOfKeys = TotalKeys;

    if (MaxDurationSeconds > 0.f)
    {
        DataDurationSeconds = MaxDurationSeconds;
    }
    else if (FrameRate.IsValid() && NumberOfFrames > 1)
    {
        DataDurationSeconds = FrameRate.AsSeconds(NumberOfFrames - 1);
    }
    else
    {
        DataDurationSeconds = 0.f;
    }
}
