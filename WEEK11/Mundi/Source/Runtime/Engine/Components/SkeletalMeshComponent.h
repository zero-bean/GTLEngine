#pragma once
#include "SkinnedMeshComponent.h"
#include "Source/Runtime/Engine/Animation/AnimationTypes.h"
#include "Source/Runtime/Core/Misc/Delegates.h"
#include "USkeletalMeshComponent.generated.h"

// 전방 선언
class UAnimInstance;
class UAnimSequence;
struct FAnimNotifyEvent;
enum class EAnimationMode : uint8;

UCLASS(DisplayName="스켈레탈 메시 컴포넌트", Description="스켈레탈 메시를 렌더링하는 컴포넌트입니다")
class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
    GENERATED_REFLECTION_BODY()
    
    USkeletalMeshComponent();
    ~USkeletalMeshComponent() override;

    void BeginPlay() override;
    void TickComponent(float DeltaTime) override;
    void SetSkeletalMesh(const FString& PathFileName) override;

// Editor Section
public:
    /**
     * @brief 특정 뼈의 부모 기준 로컬 트랜스폼을 설정
     * @param BoneIndex 수정할 뼈의 인덱스
     * @param NewLocalTransform 새로운 부모 기준 로컬 FTransform
     */
    void SetBoneLocalTransform(int32 BoneIndex, const FTransform& NewLocalTransform);

    void SetBoneWorldTransform(int32 BoneIndex, const FTransform& NewWorldTransform);

    void SetPose(const FPoseContext& Pose);
    const TArray<FTransform>& GetPose() const;

    /**
     * @brief 전체 본 포즈를 직접 설정 (뷰어 전용)
     */
    void SetLocalSpacePose(const TArray<FTransform>& InPose);
    /**
     * @brief 특정 뼈의 현재 로컬 트랜스폼을 반환
     */
    FTransform GetBoneLocalTransform(int32 BoneIndex) const;
    
    /**
     * @brief 기즈모를 렌더링하기 위해 특정 뼈의 월드 트랜스폼을 계산
     */
    FTransform GetBoneWorldTransform(int32 BoneIndex);

    /**
     * @brief 특정 본을 수동 편집 본으로 표시 (애니메이션이 덮어쓰지 않음)
     */
    void MarkBoneAsManuallyEdited(int32 BoneIndex);

    /**
     * @brief 특정 본이 수동 편집되었는지 확인
     */
    bool IsBoneManuallyEdited(int32 BoneIndex) const;

    /**
     * @brief 모든 수동 편집 본 표시 제거
     */
    void ClearManuallyEditedBones();

    UFUNCTION(LuaBind, DisplayName = "PlayAnimation")
    void PlayAnimation(const FString& AnimPath, bool bLoop);


    //----------------
    //UAnimInstance 루아 바인딩 안되서 임시로 스켈레탈메쉬에서 호출
    UFUNCTION(LuaBind, DisplayName = "AddSequenceInState")
    void AddSequenceInState(const FString& InName, const FString& AnimPath, const float InBlendValue);

    UFUNCTION(LuaBind, DisplayName = "SetBlendValueInState")
    void SetBlendValueInState(const FString& InStateName, const float InBlendValue);

    UFUNCTION(LuaBind, DisplayName = "AddTransition")
    void AddTransition(const FString& StartStateName, const FString& EndStateName, const float InBlendTime, std::function<bool()> func);

    UFUNCTION(LuaBind, DisplayName = "SetStartState")
    void SetStartState(const FString& StartStateName);

    UFUNCTION(LuaBind, DisplayName = "SetStateSpeed")
    void SetStateSpeed(const FString& StateName, const float InSpeed);

    UFUNCTION(LuaBind, DisplayName = "Play")
    void Play();

    UFUNCTION(LuaBind, DisplayName = "Pause")
    void Pause();

    UFUNCTION(LuaBind, DisplayName = "Replay")
    void Replay(const float StartTime);

    UFUNCTION(LuaBind, DisplayName = "SetStateLoop")
    void SetStateLoop(const FString& InName, const bool InLoop);

    UFUNCTION(LuaBind, DisplayName = "SetStateExitTime")
    void SetStateExitTime(const FString& InName, const float InExitTime);

    UFUNCTION(LuaBind, DisplayName = "GetCurStateName")
    FString GetCurStateName();
    //------------------------

    void DuplicateSubObjects() override;

    // AnimNotify event delegate: broadcasts Event and SequenceKey (asset name or file path)
    DECLARE_DELEGATE_TYPE_TwoParam(FOnAnimNotify, const FAnimNotifyEvent&, const FString&);
    FOnAnimNotify OnAnimNotify;

    // AnimNotify routing
    void HandleAnimNotify(const FAnimNotifyEvent& Notify);

protected:
    /**
     * @brief CurrentLocalSpacePose의 변경사항을 ComponentSpace -> FinalMatrices 계산까지 모두 수행
     */
    void ForceRecomputePose();

    /**
     * @brief CurrentLocalSpacePose를 기반으로 CurrentComponentSpacePose 채우기
     */
    void UpdateComponentSpaceTransforms();

    /**
     * @brief CurrentComponentSpacePose를 기반으로 TempFinalSkinningMatrices 채우기
     */
    void UpdateFinalSkinningMatrices();

protected:
    /**
     * @brief 각 뼈의 부모 기준 로컬 트랜스폼
     */
    TArray<FTransform> CurrentLocalSpacePose;

    /**
     * @brief LocalSpacePose로부터 계산된 컴포넌트 기준 트랜스폼
     */
    TArray<FTransform> CurrentComponentSpacePose;

    /**
     * @brief 부모에게 보낼 최종 스키닝 행렬 (임시 계산용)
     */
    TArray<FMatrix> TempFinalSkinningMatrices;
    /**
     * @brief CPU 스키닝에 전달할 최종 노말 스키닝 행렬
     */
    TArray<FMatrix> TempFinalSkinningNormalMatrices;

    /**
     * @brief 사용자가 수동으로 편집한 본 인덱스 집합 (애니메이션 재생 시 오버라이드하지 않음)
     */
    TArray<int32> ManuallyEditedBones;

private:
    UPROPERTY(EditAnywhere, Category = "SkeletalComponent")
    UAnimInstance* AnimInstance = nullptr;

// FOR TEST!!!
private:
    float TestTime = 0;
    bool bIsInitialized = false;
    FTransform TestBoneBasePose;
};
