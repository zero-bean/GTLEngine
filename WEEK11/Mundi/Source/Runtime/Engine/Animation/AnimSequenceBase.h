#pragma once
#include "AnimAsset.h"

/* 애니메이션에 트리거를 넣어 Notify 기능을 가능하도록 만드는 구조체 */
struct FAnimNotifyEvent
{
    float TriggerTime = 0.f;    // 이벤트가 발생의 시작 지점
    float Duration = 0.f;       // 이벤트 트리거가 유효할 시간
    FName NotifyName;

    float GetEndTime() const { return TriggerTime + Duration; }
    bool IsWithin(float StartTime, float EndTime) const
    {
        // Unreal-style: notify triggers only once when crossing its time point
        // Use > instead of >= to prevent duplicate triggers across frames
        return TriggerTime > StartTime && TriggerTime <= EndTime;
    }
};

/*
* @brief 모든 유형의 애니메이션 클래스는 해당 클래스를 거쳐 파생 클래스로 정의해야 합니다.
*/
class UAnimSequenceBase : public UAnimAsset
{
public:
    DECLARE_CLASS(UAnimSequenceBase, UAnimAsset);

    UAnimSequenceBase() = default;
    virtual ~UAnimSequenceBase() override = default;

    float GetPlayLength() const override { return TotalPlayLength; }

    const TArray<FAnimNotifyEvent>& GetNotifies() const { return Notifies; }
    void SetNotifies(const TArray<FAnimNotifyEvent>& InNotifies);
    void AddNotify(const FAnimNotifyEvent& Notify, int32 TrackIndex = 0);
    void RemoveNotifiesByName(const FName& InName);
    void ClearNotifies() { Notifies.clear(); NotifyDisplayTrackIndices.clear(); }
    void GetAnimNotifiesInRange(float StartTime, float EndTime, TArray<FAnimNotifyEvent>& OutNotifies) const;

    // Notify 트랙 관리 (AnimSequenceViewer UI용)
    const TArray<int32>& GetNotifyTrackIndices() const { return NotifyTrackIndices; }
    void SetNotifyTrackIndices(const TArray<int32>& InTracks) { NotifyTrackIndices = InTracks; }
    const TArray<int32>& GetNotifyDisplayTrackIndices() const { return NotifyDisplayTrackIndices; }
    void SetNotifyDisplayTrackIndices(const TArray<int32>& InDisplayTracks) { NotifyDisplayTrackIndices = InDisplayTracks; }
    int32 GetNextNotifyTrackNumber() const { return NextNotifyTrackNumber; }
    void SetNextNotifyTrackNumber(int32 InNumber) { NextNotifyTrackNumber = InNumber; }

    float GetSequenceLength() const { return TotalPlayLength; }
    void SetSequenceLength(float InLength) { TotalPlayLength = std::max(0.f, InLength); }

    float GetPlayScale() const { return PlayRate; }
    void SetPlayScale(float InPlayRate) { PlayRate = std::max(0.0001f, InPlayRate); }

    bool IsLooping() const { return bLoop; }
    void SetLooping(bool bInLoop) { bLoop = bInLoop; }

    // 공통 메타데이터 직렬화
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    // Binary 저장 (파생 클래스에서 재정의)
    virtual void SaveBinary(const FString& InFilePath) const {}

protected:
    // 내부 헬퍼 함수를 통해서 Notify 배열의 시간대별 정렬을 보장
    void SortNotifies()
    {
        std::sort(Notifies.begin(), Notifies.end(),
            [](const FAnimNotifyEvent& A, const FAnimNotifyEvent& B)
            {
                return A.TriggerTime < B.TriggerTime;
            });
    }

protected:
    TArray<FAnimNotifyEvent> Notifies{};       // 시퀀스에 배치된 노티파이 목록
    TArray<int32> NotifyTrackIndices{};        // 노티파이 트랙 번호 목록 (UI용)
    TArray<int32> NotifyDisplayTrackIndices{}; // 각 노티파이의 UI 트랙 인덱스 (Notifies와 1:1 대응)
    int32 NextNotifyTrackNumber = 1;           // 다음 트랙 번호
    float TotalPlayLength = 0.f;               // 전체 재생 시간(초)
    float PlayRate = 1.f;                      // 속도 배율 (1.0 = 원래 속도)
    bool bLoop = false;                        // 애니메이션 반복 여부
};
