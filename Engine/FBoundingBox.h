#pragma once
#include "stdafx.h"

struct FBoundingBox
{
    // 각 축의 최소값
    FVector Min;
    // 각 축의 최대값
    FVector Max;
    // 이 두 점을 이용해 AABB를 표현할 수 있습니다.

    static FBoundingBox Empty() {
        return { FVector(FLT_MAX,  FLT_MAX,  FLT_MAX),
                 FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX) };
    }
    /*
        각 축(X, Y, Z)에 대해
        Min은 현재 값과 p의 값을 비교해 더 작은 값으로,
        Max는 현재 값과 p의 값을 비교해 더 큰 값으로 바꿉니다.
        이렇게 여러 점에 대해 Expand(p)를 반복 호출하면,
        결국 그 모든 점을 감싸는 축정렬 경계박스(AABB)가 됩니다.
    */
    // 메시의 정점 위치 
    void Expand(const FVector& pos) {
        Min.X = min(Min.X, pos.X); Min.Y = min(Min.Y, pos.Y); Min.Z = min(Min.Z, pos.Z);
        Max.X = max(Max.X, pos.X); Max.Y = max(Max.Y, pos.Y); Max.Z = max(Max.Z, pos.Z);
    }
    // 렌더용 단위 큐브(−1...1)를 스케일/이동할 때 쓰는 유틸용
    static inline void CenterExtents(const FBoundingBox& box, FVector& center, FVector& half) {
        center = (box.Min + box.Max) * 0.5f;
        half = (box.Max - box.Min) * 0.5f;
    }
};
