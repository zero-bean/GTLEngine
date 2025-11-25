#pragma once
#include "ParticleModuleTypeDataBase.h"
#include "UParticleModuleTypeDataBeam.generated.h"

struct FRawDistributionFloat;

// 빔 종류
enum class EBeamMethod : uint8
{
    // 시작점에서 x축 방향으로 길이만큼
    EBM_Distance,
    // 시작점에서 끝점까지
    EBM_Target,
    EMB_MAX

    // 언리얼에 보면 Branch가 있는데 이건 cascade에서 미구현 상태임
    // 그래서 구현하지 않을 예정이고 branch 효과를 표현하고 싶으면 여러개의 beam을 조합하는 방식으로 구현
};

// 빔의 굵기 조절
enum class EBeamTaperMethod : uint8
{
    EBTM_None,     
    EBTM_Full,    
    EBTM_Partial,
    EBTM_MAX
};

UCLASS(DisplayName = "빔 파티클 모듈 타입 데이터", Description = "")
class
UParticleModuleTypeDataBeam : public UParticleModuleTypeDataBase
{
public:
    GENERATED_REFLECTION_BODY()
    UParticleModuleTypeDataBeam() = default;

    EBeamMethod BeamMethod;
    
    EBeamTaperMethod BeamTaperMethod;

    // 텍스처 반복 횟수 
    int32 TextureTile;
    // 텍스처 반복 길이
    // TextureTile과 동시에 사용할 수 없음. 이게 0보다 크면 우선 사용
    float TextureTileDistance;

    // 빔의 겹침 횟수, 몇 개 겹칠 지 결정
    int32 Sheets;

    int32 MaxBeamCount;

    // 빔이 목표 지점까지 날아가는 속도, 0 이면 instance
    float Speed;

    float BaseWidth = 1.0f;

    // 시작점과 끝점 사이에 찍을 점의 갯수
    // segments 숫자
    int32 InterpolationPoints;

    // 수명이 끝나도 켜질 지 
    int8 bAlwaysOn;

    // UpVector 계산 빈도
    // 0 : 매 점마다 계산
    // 1 : 시작점에서 한 번만 계산
    // N : 매 N번 째 점마다 계산한다는데 언리얼 주석에 unsupported라고 되어 있네
    int32 UpVectorStepSize;

    FRawDistributionFloat Distance;

    // 진행 정도에 따른 굵기
    // 0 : 시작점, 1 : 끝점
    FRawDistributionFloat TaperFactor;
    
    FRawDistributionFloat TaperScale;

    // 언리얼은 모듈로 되어 있는데 간단한 구현을 위해 FVector 사용
    FVector Source;
    FVector Target;

    FVector4 Color;
};
