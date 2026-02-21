#include "pch.h"
#include "AnimSequenceBase.h"
#include "JsonSerializer.h"

IMPLEMENT_CLASS(UAnimSequenceBase)

void UAnimSequenceBase::AddNotify(const FAnimNotifyEvent& Notify, int32 TrackIndex)
{
    FString TestSrt = Notify.NotifyName.ToString();
    // 이진 탐색을 통해 추가할 지점을 찾아 삽입합니다.
    auto InsertPos = std::lower_bound(Notifies.begin(), Notifies.end(), Notify,
        [](const FAnimNotifyEvent& A, const FAnimNotifyEvent& B)
        {
            return A.TriggerTime < B.TriggerTime;
        });

    size_t InsertIndex = std::distance(Notifies.begin(), InsertPos);
    Notifies.insert(InsertPos, Notify);
    NotifyDisplayTrackIndices.insert(NotifyDisplayTrackIndices.begin() + InsertIndex, TrackIndex);
}

void UAnimSequenceBase::SetNotifies(const TArray<FAnimNotifyEvent>& InNotifies)
{
    Notifies = InNotifies;
    SortNotifies();
}

void UAnimSequenceBase::RemoveNotifiesByName(const FName& InName)
{
    if (Notifies.empty()) { return; }

    // 인덱스를 역순으로 순회하며 삭제 (인덱스 불일치 방지)
    for (int32 i = static_cast<int32>(Notifies.size()) - 1; i >= 0; --i)
    {
        if (Notifies[i].NotifyName == InName)
        {
            Notifies.erase(Notifies.begin() + i);
            if (i < NotifyDisplayTrackIndices.size())
            {
                NotifyDisplayTrackIndices.erase(NotifyDisplayTrackIndices.begin() + i);
            }
        }
    }
}

// 애니메이션의 특정 구간에서 노티파이 이벤트 목록들을 수집해서 반환합니다
// ============================================================================
// UAnimSequenceBase::GetAnimNotifiesInRange
// 
// 역할:
//   애니메이션이 StartTime → EndTime 구간 동안 "지나간" AnimNotify들을 찾아
//   OutNotifies 배열에 채워 넣는다.
//
// 왜 필요한가?
//   애니메이션은 매 프레임 시간(dt)만큼 재생되므로,
//   이전 프레임 위치(StartTime)에서 이번 프레임 위치(EndTime)으로
//   이동하는 동안 Notify(예: 발소리, 몽타주 이벤트)가
//   이 구간을 통과했는지를 체크해야 한다.
//
// 주요 특징:
//   • Notify.TriggerTime > StartTime && <= EndTime 조건으로
//     "이번 프레임에서 막 지나간" Notify만 가져온다
//   • 루프 애니메이션의 경우 Start > End(예: 0.95 → 0.02) 상황을 처리
//   • 오염 방지를 위해 OutNotifies.clear()
// ============================================================================

void UAnimSequenceBase::GetAnimNotifiesInRange(float StartTime, float EndTime, TArray<FAnimNotifyEvent>& OutNotifies) const
{
    OutNotifies.clear(); // 이전 프레임 데이터 오염 방지

    if (Notifies.empty()) { return; }

    // --------------------------
    // 1) 정방향 재생 (일반 상황)
    //    예: Start = 0.3 → End = 0.45
    // --------------------------
    if (StartTime <= EndTime)
    {
        for (const FAnimNotifyEvent& Notify : Notifies)
        {
            // Notify.TriggerTime 이 해당 프레임에서 "막 지남"을 의미
            // > 를 사용하는 이유: StartTime == NotifyTime 인 프레임에서
            //                      중복 발생을 방지
            if (Notify.TriggerTime > StartTime && Notify.TriggerTime <= EndTime)
            {
                OutNotifies.Add(Notify);
            }
        }
    }
    // --------------------------
    // 2) 루프 상황 (애니메이션이 끝에서 다시 0으로 돌아감)
    //    예: Start=0.95, End=0.04
    //
    //    처리 방식:
    //      A) [StartTime ~ SequenceEnd] 에서 지나간 Notify
    //      B) [0 ~ EndTime] 에서 지나간 Notify
    // --------------------------
    else
    {
        // A) 마지막 구간: Start → TotalPlayLength
        for (const FAnimNotifyEvent& Notify : Notifies)
        {
            if (Notify.TriggerTime > StartTime && Notify.TriggerTime <= TotalPlayLength)
            {
                OutNotifies.Add(Notify);
            }
        }

        // B) 루프해서 다시 0 → EndTime
        for (const FAnimNotifyEvent& Notify : Notifies)
        {
            if (Notify.TriggerTime >= 0.0f && Notify.TriggerTime <= EndTime)
            {
                OutNotifies.Add(Notify);
            }
        }
    }
}

void UAnimSequenceBase::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadFloat(InOutHandle, "TotalPlayLength", TotalPlayLength, 0.f, false);
        FJsonSerializer::ReadFloat(InOutHandle, "PlayRate", PlayRate, 1.f, false);
        FJsonSerializer::ReadBool(InOutHandle, "bLoop", bLoop, false, false);

        // NextNotifyTrackNumber 로드
        if (InOutHandle.hasKey("NextNotifyTrackNumber"))
        {
            NextNotifyTrackNumber = InOutHandle["NextNotifyTrackNumber"].ToInt();
        }
        else
        {
            NextNotifyTrackNumber = 1;
        }

        // 노티파이 트랙 인덱스 로드
        JSON TrackArray;
        if (FJsonSerializer::ReadArray(InOutHandle, "NotifyTrackIndices", TrackArray, nullptr, false))
        {
            NotifyTrackIndices.clear();
            for (size_t Idx = 0; Idx < TrackArray.size(); ++Idx)
            {
                const JSON& TrackJson = TrackArray.at(static_cast<unsigned>(Idx));
                if (TrackJson.JSONType() == JSON::Class::Integral)
                {
                    NotifyTrackIndices.Add(TrackJson.ToInt());
                }
            }
        }
        else
        {
            NotifyTrackIndices.clear();
        }

        JSON NotifyArray;
        if (FJsonSerializer::ReadArray(InOutHandle, "Notifies", NotifyArray, nullptr, false))
        {
            Notifies.clear();
            NotifyDisplayTrackIndices.clear();
            for (size_t Idx = 0; Idx < NotifyArray.size(); ++Idx)
            {
                JSON NotifyJson = NotifyArray.at(static_cast<unsigned>(Idx));
                if (NotifyJson.JSONType() != JSON::Class::Object) { continue; }

                FAnimNotifyEvent Notify;
                FJsonSerializer::ReadFloat(NotifyJson, "TriggerTime", Notify.TriggerTime, 0.f, false);
                FJsonSerializer::ReadFloat(NotifyJson, "Duration", Notify.Duration, 0.f, false);

                FString NotifyNameStr;
                if (FJsonSerializer::ReadString(NotifyJson, "NotifyName", NotifyNameStr, "", false))
                {
                    Notify.NotifyName = FName(NotifyNameStr);
                }

                Notifies.Add(Notify);

                // UI 트랙 인덱스 로드
                int32 DisplayTrack = 0;
                if (NotifyJson.hasKey("DisplayTrack"))
                {
                    DisplayTrack = NotifyJson["DisplayTrack"].ToInt();
                }
                NotifyDisplayTrackIndices.Add(DisplayTrack);
            }
            SortNotifies();
        }
        else
        {
            Notifies.clear();
            NotifyDisplayTrackIndices.clear();
        }
    }
    else
    {
        InOutHandle["TotalPlayLength"] = TotalPlayLength;
        InOutHandle["PlayRate"] = PlayRate;
        InOutHandle["bLoop"] = bLoop;
        InOutHandle["NextNotifyTrackNumber"] = NextNotifyTrackNumber;

        // 노티파이 트랙 인덱스 저장
        JSON TrackArray = JSON::Make(JSON::Class::Array);
        for (int32 TrackIndex : NotifyTrackIndices)
        {
            TrackArray.append(TrackIndex);
        }
        InOutHandle["NotifyTrackIndices"] = TrackArray;

        JSON NotifyArray = JSON::Make(JSON::Class::Array);
        for (size_t i = 0; i < Notifies.size(); ++i)
        {
            const FAnimNotifyEvent& Notify = Notifies[i];
            JSON NotifyJson = JSON::Make(JSON::Class::Object);
            NotifyJson["TriggerTime"] = Notify.TriggerTime;
            NotifyJson["Duration"] = Notify.Duration;
            NotifyJson["NotifyName"] = Notify.NotifyName.ToString().c_str();

            // UI 트랙 인덱스 저장
            if (i < NotifyDisplayTrackIndices.size())
            {
                NotifyJson["DisplayTrack"] = NotifyDisplayTrackIndices[i];
            }
            else
            {
                NotifyJson["DisplayTrack"] = 0;
            }

            NotifyArray.append(NotifyJson);
        }
        InOutHandle["Notifies"] = NotifyArray;
    }
}
