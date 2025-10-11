#pragma once
#include "PrimitiveComponent.h"
#include "Physics/Public/OBB.h"

namespace json { class JSON; }
using JSON = json::JSON;

class UMaterial;

struct FDecalFadeProperty
{
public:
	float FadeInDuration = 0.f;			// 페이드 인 완료까지 걸리는 시간
	float FadeInStartDelay = 0.f;		// 페이드 인 시작 전 대기 시간
	float FadeDuration = 0.f;			// 페이드 아웃 완료까지 걸리는 시간
	float FadeStartDelay = 0.f;			// 페이드 아웃 시작 전 대기 시간
	float FadeAlpha = 0.f;				// 현재 알파값 (0 ~ 1)
	float ElapsedFadeTime = 0.f;		// 현재 페이드 구간의 경과 시간
	bool bIsFadingIn = false;			// 현재 페이드 인 중인가?
	bool bIsFadingOut = false;			// 현재 페이드 아웃 중인가?
	bool bFadeCompleted = false;		// 전체 페이드 사이클 완료 여부
	bool bDestroyedAfterFade = false;	// 페이드 아웃 완료 후 제거 여부

	void StartFadeIn()
	{
		bIsFadingIn = true;
		bIsFadingOut = false;
		bFadeCompleted = false;
		FadeAlpha = 0.f;
		ElapsedFadeTime = -FadeInStartDelay;
	}

	void StartFadeOut()
	{
		bIsFadingOut = true;
		bIsFadingIn = false;
		FadeAlpha = 1.f;
		ElapsedFadeTime = -FadeStartDelay;
	}

	// 매 프레임 갱신 (DeltaTime 단위로)
	bool Update(float DeltaTime)
	{
		if (!bIsFadingIn && !bIsFadingOut) { return false; }

		ElapsedFadeTime += DeltaTime;

		// 아직 시작 딜레이 중이면 무시
		if (ElapsedFadeTime < 0.f) { return false; }

		if (bIsFadingIn)
		{
			FadeAlpha = std::clamp(ElapsedFadeTime / FadeInDuration, 0.f, 1.f);
			if (FadeAlpha >= 1.f)
			{
				bIsFadingIn = false;
				bFadeCompleted = true;
			}
		}
		else if (bIsFadingOut)
		{
			FadeAlpha = 1.f - std::clamp(ElapsedFadeTime / FadeDuration, 0.f, 1.f);
			if (FadeAlpha <= 0.f)
			{
				bIsFadingOut = false;
				bFadeCompleted = true;
			}
		}

		return true;
	}
};

UCLASS()
class UDecalComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

public:
	UDecalComponent();
	virtual ~UDecalComponent();
	void TickComponent(float DeltaSeconds) override;

	/* *
	 * @brief 페이드 제어 및 상태 확인을 위한 public 함수들
	 */
	void StartFadeIn(float Duration, float Delay = 0.f);
	void StartFadeOut(float Duration, float Delay = 0.f, bool bDestroyOwner = false);
	float GetFadeAlpha() const { return FadeProperty.FadeAlpha; }
	bool IsFadeCompleted() const { return FadeProperty.bFadeCompleted; }
	FDecalFadeProperty& GetFadeProperty() { return FadeProperty; }

	void SetDecalMaterial(UMaterial* InMaterial);
	UMaterial* GetDecalMaterial() const;

	FOBB* GetProjectionBox() const { return ProjectionBox; }

	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
	UMaterial* DecalMaterial;
	FOBB* ProjectionBox;
	FDecalFadeProperty FadeProperty;
};
