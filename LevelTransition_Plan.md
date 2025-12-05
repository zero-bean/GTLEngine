# 레벨 전환 시스템 구현 계획

## 개요
게임 플레이 중 특정 레벨로 전환하는 기능 구현. 코드 직접 호출 + 트리거 볼륨 방식 모두 지원.

## 요구사항
- **트리거 방식**: 코드에서 직접 호출 + 트리거 박스
- **시각적 효과**: 로딩 화면
- **데이터 유지**: DontDestroyOnLoad 스타일 (일부 액터 유지)

---

## 구현 단계

### 1단계: AActor에 Persistent 플래그 추가
**파일**: `Mundi/Source/Runtime/Core/Object/Actor.h`

```cpp
// protected 섹션에 추가
bool bPersistAcrossLevelTransition = false;

// public 섹션에 추가
void SetPersistAcrossLevelTransition(bool bPersist) { bPersistAcrossLevelTransition = bPersist; }
bool IsPersistentActor() const { return bPersistAcrossLevelTransition; }
```

---

### 2단계: ALevelTransitionManager 생성
**파일**: `Mundi/Source/Runtime/Engine/GameFramework/LevelTransitionManager.h/cpp`

```cpp
enum class ELevelTransitionState : uint8 { Idle, FadingOut, Loading, FadingIn };

class ALevelTransitionManager : public AInfo
{
public:
    // === 주요 API ===
    void TransitionToLevel(const FWideString& LevelPath);  // 페이드 효과와 함께 전환
    void LoadLevelImmediate(const FWideString& LevelPath); // 즉시 전환

    // === Persistent Actor 관리 ===
    void MarkActorPersistent(AActor* Actor);
    bool IsTransitioning() const;

    // === 설정 ===
    float FadeDuration = 0.5f;

private:
    ELevelTransitionState TransitionState = ELevelTransitionState::Idle;
    FWideString PendingLevelPath;
    float TransitionProgress = 0.0f;
    TArray<AActor*> PersistentActors;  // 전환 중 보관

    void ExtractPersistentActors();    // 레벨에서 추출
    void RestorePersistentActors();    // 새 레벨에 복원
};
```

**전환 흐름**:
1. `TransitionToLevel()` 호출 → FadingOut 상태 시작
2. Tick에서 페이드 애니메이션 (alpha 0→1)
3. 페이드 완료 시 Persistent 액터 추출 + `UWorld::LoadLevelFromFile()` 호출
4. 로드 완료 후 Persistent 액터 복원
5. FadingIn 상태 (alpha 1→0)
6. 완료 → Idle

---

### 3단계: UWorld에 LevelTransitionManager 통합
**파일**: `Mundi/Source/Runtime/Engine/GameFramework/World.h/cpp`

```cpp
// World.h private 섹션
ALevelTransitionManager* LevelTransitionManager = nullptr;

// World.h public 섹션
ALevelTransitionManager* GetLevelTransitionManager();
void TransitionToLevel(const FWideString& LevelPath);  // 편의 메서드
```

```cpp
// World.cpp
ALevelTransitionManager* UWorld::GetLevelTransitionManager()
{
    if (!LevelTransitionManager && bPie)
    {
        LevelTransitionManager = SpawnActor<ALevelTransitionManager>();
        LevelTransitionManager->SetPersistAcrossLevelTransition(true);
    }
    return LevelTransitionManager;
}
```

---

### 4단계: SetLevel 수정 (Persistent Actor 지원)
**파일**: `Mundi/Source/Runtime/Engine/GameFramework/World.cpp`

`SetLevel()` 내에서 `bPersistAcrossLevelTransition == true`인 액터는 삭제하지 않도록 수정:

```cpp
void UWorld::SetLevel(std::unique_ptr<ULevel> InLevel)
{
    // 기존 액터 정리 시 persistent 액터 제외
    for (AActor* Actor : Level->GetActors())
    {
        if (Actor && !Actor->IsPersistentActor())
        {
            ObjectFactory::DeleteObject(Actor);
        }
    }
    // ... 나머지 기존 로직
}
```

---

### 5단계: ALevelTriggerVolume 생성
**파일**: `Mundi/Source/Runtime/Engine/GameFramework/LevelTriggerVolume.h/cpp`

```cpp
class ALevelTriggerVolume : public AActor
{
public:
    UPROPERTY(EditAnywhere, Category="LevelTrigger")
    FWideString TargetLevelPath;      // 전환할 레벨 경로

    UPROPERTY(EditAnywhere, Category="LevelTrigger")
    FString RequiredActorTag;         // 빈 문자열이면 모든 액터

    UPROPERTY(EditAnywhere, Category="LevelTrigger")
    bool bTriggerOnce = true;

private:
    UBoxComponent* TriggerComponent;
    bool bHasTriggered = false;

    void OnTriggerOverlap(UPrimitiveComponent* MyComp, UPrimitiveComponent* OtherComp);
};
```

기존 `OnComponentBeginOverlap` 델리게이트 활용 (Actor.h:24에 이미 존재).

---

### 6단계: 로딩 화면 UI
**파일**: `Mundi/Source/Slate/GameUI/SLoadingScreen.h/cpp`

간단한 전체화면 페이드 오버레이:

```cpp
class SLoadingScreen : public SWidget
{
public:
    void SetAlpha(float InAlpha);
    float GetAlpha() const;
    void SetBackgroundColor(const FVector4& Color);

    virtual void Paint(FD2DRenderer& Renderer, const FGeometry& Geometry) override;

private:
    float Alpha = 0.0f;
    FVector4 BackgroundColor = FVector4(0, 0, 0, 1);  // 검정
};
```

---

## 수정할 파일 목록

| 파일 | 변경 내용 |
|------|----------|
| `Mundi/Source/Runtime/Core/Object/Actor.h` | `bPersistAcrossLevelTransition` 플래그 추가 |
| `Mundi/Source/Runtime/Engine/GameFramework/World.h` | LevelTransitionManager 포인터 및 접근자 추가 |
| `Mundi/Source/Runtime/Engine/GameFramework/World.cpp` | SetLevel() 수정, GetLevelTransitionManager() 구현 |
| **신규** `Mundi/Source/Runtime/Engine/GameFramework/LevelTransitionManager.h/cpp` | 전환 로직 구현 |
| **신규** `Mundi/Source/Runtime/Engine/GameFramework/LevelTriggerVolume.h/cpp` | 트리거 볼륨 액터 |
| **신규** `Mundi/Source/Slate/GameUI/SLoadingScreen.h/cpp` | 로딩 화면 위젯 |

---

## 사용 예시

### 코드에서 직접 호출
```cpp
// Blueprint/Lua에서
World->TransitionToLevel(L"Data/Scenes/Level2.scene");

// 또는 GameMode에서
World->GetLevelTransitionManager()->TransitionToLevel(L"Data/Scenes/Boss.scene");
```

### Persistent Actor 설정
```cpp
// 플레이어를 레벨 전환 시에도 유지
PlayerActor->SetPersistAcrossLevelTransition(true);
```

### 트리거 볼륨 (에디터에서 배치)
- `ALevelTriggerVolume` 액터를 레벨에 배치
- `TargetLevelPath` 속성에 전환할 레벨 경로 설정
- `RequiredActorTag`에 "Player" 설정 시 플레이어만 트리거

---

## 구현 순서 (권장)

1. **Actor.h** - Persistent 플래그 추가 (가장 간단)
2. **LevelTransitionManager** - 핵심 전환 로직
3. **World 통합** - Manager 생성 및 SetLevel 수정
4. **SLoadingScreen** - 로딩 화면 UI
5. **LevelTriggerVolume** - 트리거 박스 액터
