#pragma once
#include "Enums.h"
#include "Object.h"

class AActor;

class UActorComponent : public UObject
{
public:
    DECLARE_CLASS(UActorComponent, UObject)
    UActorComponent();

protected:
    ~UActorComponent() override;

public:
    // ───────────────
    // Lifecycle
    // ───────────────
    virtual void InitializeComponent();   // 액터에 붙을 때
    virtual void BeginPlay();             // 월드 시작 시
    virtual void TickComponent(float DeltaSeconds); // 매 프레임
    virtual void EndPlay(EEndPlayReason::Type EndPlayReason); // 파괴/종료 시
    
    // Component registration
    virtual void RegisterComponent();
    virtual void OnRegister();          // 컴포넌트 등록
    virtual void OnUnregister();        // 컴포넌트 제거
    bool bIsRegistered = false;

    // ───────────────
    // 활성화/비활성
    // ───────────────
    void SetActive(bool bNewActive) { bIsActive = bNewActive; }
    void SetEditable(bool bIsEditable) { bEdiableWhenInherited = bIsEditable; }
    void SetShowflag(bool bIsShow) { bIsRender = bIsShow; }
    bool IsActive() const { return bIsActive; }
    bool IsRender() const { return bIsRender; }
    bool IsEditable() const { return bEdiableWhenInherited; }

    void SetTickEnabled(bool bNewTick) { bCanEverTick = bNewTick; }
    bool CanEverTick() const { return bCanEverTick; }

    // ───────────────
    // Owner Actor
    // ───────────────
    void SetOwner(AActor* InOwner) {
        Owner = InOwner;
    }
    AActor* GetOwner() const { return Owner; }

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;
    EComponentWorldTickMode WorldTickMode; //

    // ───────────────
    // Editor Details
    // ───────────────
    // 각 컴포넌트가 자신의 디테일 패널을 렌더링하는 가상 함수
    virtual void RenderDetails() {}
 
protected:
    // [PIE] 외부에서 초기화 필요
    AActor* Owner = nullptr;  // 자신을 보유한 액터

    // [PIE] 값 복사
    bool bIsActive = true;    // 활성 상태
    bool bCanEverTick = false; // 매 프레임 Tick 가능 여부
    bool bEdiableWhenInherited = true;  //디테일에서 Editing 가능 여부
    bool bIsRender = true;

   
};