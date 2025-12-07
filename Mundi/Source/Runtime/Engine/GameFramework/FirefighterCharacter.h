#pragma once
#include "Character.h"
#include "AFirefighterCharacter.generated.h"

class UAudioComponent;
class USphereComponent;
class ULuaScriptComponent;
class UParticleSystemComponent;

UCLASS(DisplayName = "파이어 파이터 캐릭터", Description = "렛츠고 파이어 파이터")
class AFirefighterCharacter : public ACharacter
{
    GENERATED_REFLECTION_BODY()

public:
    AFirefighterCharacter();

    // ────────────────────────────────────────────────
    // 스케일 설정
    // ────────────────────────────────────────────────

    /**
     * 캐릭터 스케일을 설정합니다.
     * 캡슐, 메쉬 오프셋, 스프링암, 아이템 감지 반경 등 모든 관련 수치를 조절합니다.
     */
    void SetCharacterScale(float Scale);

    /** 현재 캐릭터 스케일 반환 */
    float GetCharacterScale() const { return CharacterScale; }

    // ────────────────────────────────────────────────
    // 파티클 이펙트
    // ────────────────────────────────────────────────

    /** 소방복 장착 파티클 재생 (Lua에서 호출) */
    UFUNCTION(LuaBind, DisplayName="PlayFireSuitEquipEffect")
    void PlayFireSuitEquipEffect();

protected:
    ~AFirefighterCharacter() override;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void HandleAnimNotify(const FAnimNotifyEvent& NotifyEvent) override;

    // ────────────────────────────────────────────────
    // 아이템 감지
    // ────────────────────────────────────────────────

    /** 아이템 감지용 스피어 컴포넌트 */
    USphereComponent* ItemDetectionSphere;

    /** 아이템 줍기 스크립트 */
    ULuaScriptComponent* ItemPickupScript;

    // ────────────────────────────────────────────────
    // 입력 바인딩
    // ────────────────────────────────────────────────

    virtual void SetupPlayerInputComponent(UInputComponent* InInputComponent) override;

    // ────────────────────────────────────────────────
    // 이동 입력 (카메라 방향 기준)
    // ────────────────────────────────────────────────

    /** 카메라 방향 기준 앞/뒤 이동 */
    void MoveForwardCamera(float Value);

    /** 카메라 방향 기준 좌/우 이동 */
    void MoveRightCamera(float Value);

private:
    // ────────────────────────────────────────────────
    // 스케일 관련
    // ────────────────────────────────────────────────

    /** 현재 캐릭터 스케일 */
    float CharacterScale = 1.0f;

    // ────────────────────────────────────────────────
    // 회전 설정
    // ────────────────────────────────────────────────

    /** 이동 방향으로 캐릭터 회전 여부 */
    bool bOrientRotationToMovement;

    /** 회전 보간 속도 (도/초) */
    float RotationRate;

    /** 현재 프레임의 이동 방향 (회전 계산용) */
    FVector CurrentMovementDirection;

    /** Lua 스크립트 컴포넌트 (애니메이션 제어용) */
    ULuaScriptComponent* LuaScript;

    /** 소방복 장착 파티클 컴포넌트 */
    UParticleSystemComponent* FireSuitEquipParticle;

    //USound* SorrySound;
    //USound* HitSound;
};
