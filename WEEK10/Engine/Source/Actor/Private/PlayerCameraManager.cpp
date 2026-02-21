#include "pch.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Component/Public/CameraComponent.h"
#include "Actor/Public/Actor.h"
#include "Render/Camera/Public/CameraModifier.h"
#include "Global/CurveTypes.h"

IMPLEMENT_CLASS(APlayerCameraManager, AActor)

APlayerCameraManager::APlayerCameraManager()
{
	// Enable tick for camera updates
	bCanEverTick = true;
}

APlayerCameraManager::~APlayerCameraManager() = default;

void APlayerCameraManager::BeginPlay()
{
	if (bBegunPlay) return;

	Super::BeginPlay();
	//StartCameraFade(0, 1, 5.0, FVector(0.0, 0.0, 0.0), true); FadeIn Test
}

void APlayerCameraManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCamera(DeltaTime);
	UpdatePostProcessAnimations(DeltaTime);
}

void APlayerCameraManager::SetViewTarget(AActor* NewViewTarget)
{
	ViewTarget.Target = NewViewTarget;
	bCameraDirty = true;
	CachedCameraComponent = nullptr; // 캐시 무효화

	bIsBlending = false;
	CurrentBlendTime = 0.f;
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
	// Get POV from view target
	GetViewTargetPOV(ViewTarget);

	const FMinimalViewInfo& GoalPOV = ViewTarget.POV;
	UpdateCameraBlending(DeltaTime, GoalPOV);

	// Apply camera modifiers (empty for now)
	ApplyCameraModifiers(DeltaTime, CameraCachePOV);

	// Update camera constants for rendering
	CameraCachePOV.UpdateCameraConstants();

	// Camera is now up to date
	bCameraDirty = false;
}

const FMinimalViewInfo& APlayerCameraManager::GetCameraCachePOV() const
{
	// Update camera if dirty
	if (bCameraDirty)
	{
		const_cast<APlayerCameraManager*>(this)->UpdateCamera(0.0f);
	}
	return CameraCachePOV;
}

void APlayerCameraManager::GetViewTargetPOV(FViewTarget& OutVT)
{
	if (!OutVT.Target)
	{
		OutVT.POV.FOV = DefaultFOV;
		OutVT.POV.AspectRatio = DefaultAspectRatio;
		CachedCameraComponent = nullptr;
		return;
	}

	if (CachedCameraComponent && CachedCameraComponent->IsActive())
	{
		OutVT.POV = CachedCameraComponent->GetCameraView();
		OutVT.POV.AspectRatio = DefaultAspectRatio;
	}
	else
	{
		// --- 캐시 미스 ---
		UE_LOG_INFO("APlayerCameraManager: Cache miss. Re-searching for active camera component.");

		TArray<UCameraComponent*> CameraComponents = OutVT.Target->GetComponentsByClass<UCameraComponent>();
		UCameraComponent* ActiveCameraComp = nullptr;

		for (UCameraComponent* CameraComp : CameraComponents)
		{
			if (CameraComp && CameraComp->IsActive())
			{
				ActiveCameraComp = CameraComp;
				break;
			}
		}

		if (ActiveCameraComp)
		{
			OutVT.POV = ActiveCameraComp->GetCameraView();
			OutVT.POV.AspectRatio = DefaultAspectRatio;
			CachedCameraComponent = ActiveCameraComp;
		}
		else
		{
			OutVT.POV.Location = OutVT.Target->GetActorLocation();
			OutVT.POV.Rotation = OutVT.Target->GetActorRotation();
			OutVT.POV.FOV = DefaultFOV;
			OutVT.POV.AspectRatio = DefaultAspectRatio;
			CachedCameraComponent = nullptr;
		}
	}
}

void APlayerCameraManager::UpdateCameraBlending(float DeltaTime, const FMinimalViewInfo& GoalPOV)
{
	if (bIsBlending)
	{
		CurrentBlendTime -= DeltaTime;

		if (CurrentBlendTime <= 0.0f)
		{
			bIsBlending = false;
			CameraCachePOV = GoalPOV;
		}
		else
		{
			float BlendAlpha = (TotalBlendTime - CurrentBlendTime) / TotalBlendTime;
			CameraCachePOV = FMinimalViewInfo::Blend(BlendStartPOV, GoalPOV, BlendAlpha, CurrentBlendCurve);
		}
	}
	else
	{
		CameraCachePOV = GoalPOV;
	}
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// Apply all active camera modifiers
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier && !Modifier->IsDisabled())
		{
			// Update modifier's alpha blend
			Modifier->UpdateAlpha(DeltaTime);

			// Let modifier modify the camera
			if (Modifier->ModifyCamera(DeltaTime, InOutPOV))
			{
				// Modifier returned true, stop processing chain
				break;
			}
		}
	}

	// Apply fade amount if set
	InOutPOV.FadeAmount = FadeAmount;
	InOutPOV.FadeColor = FadeColor;
}

void APlayerCameraManager::EnableLetterBox(float InTargetAspectRatio, float InTransitionTime)
{
	CurrentPostProcessSettings.TargetAspectRatio = InTargetAspectRatio;
	TargetLetterBoxAmount = 1.0f; // 목표 강도를 1.0으로 설정

	// 속도 계산: 1.0의 거리를 InTransitionTime초 만에 가야 함
	LetterBoxTransitionSpeed = (InTransitionTime > 0.001f) ? (1.0f / InTransitionTime) : 1000.0f;
}

void APlayerCameraManager::DisableLetterBox(float InTransitionTime)
{
	TargetLetterBoxAmount = 0.0f;
	LetterBoxTransitionSpeed = (InTransitionTime > 0.001f) ? (1.0f / InTransitionTime) : 1000.0f;
}


void APlayerCameraManager::SetVignetteIntensity(float InIntensity)
{
	CurrentPostProcessSettings.bEnableVignette = true;
	CurrentPostProcessSettings.VignetteIntensity = InIntensity;
}

void APlayerCameraManager::StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FVector Color, bool bHoldWhenFinished, const FCurve* FadeCurve)
{
	FadeStartAlpha = FromAlpha;
	FadeEndAlpha = ToAlpha;
	FadeDuration = Duration;
	FadeTimeRemaining = Duration;
	bHoldFadeWhenFinished = bHoldWhenFinished;
	CurrentFadeCurve = FadeCurve;

	FadeColor.X = Color.X;
	FadeColor.Y = Color.Y;
	FadeColor.Z = Color.Z;

	if (Duration <= 0.0f)
	{
		FadeAmount = ToAlpha;
		FadeTimeRemaining = 0.0f;
	}
	else
	{
		// 즉시 시작 값으로 설정
		FadeAmount = FromAlpha;
	}
}

void APlayerCameraManager::StopCameraFade(bool bStopAtCurrent)
{
	FadeTimeRemaining = 0.0f;

	if (!bStopAtCurrent)
	{
		FadeAmount = 0.0f;
	}

	FadeStartAlpha = FadeAmount;
	FadeEndAlpha = FadeAmount;
}

void APlayerCameraManager::SetManualCameraFade(float InFadeAmount, FVector InFadeColor)
{
	FadeTimeRemaining = 0.0f;

	FadeAmount = InFadeAmount;
	FadeColor.X = InFadeColor.X;
	FadeColor.Y = InFadeColor.Y;
	FadeColor.Z = InFadeColor.Z;

	// 현재 상태를 동기화합니다.
	FadeStartAlpha = InFadeAmount;
	FadeEndAlpha = InFadeAmount;
}

void APlayerCameraManager::UpdatePostProcessAnimations(float DeltaTime)
{
	// LetterBox
	CurrentPostProcessSettings.LetterBoxAmount = InterpTo(CurrentPostProcessSettings.LetterBoxAmount,
			TargetLetterBoxAmount, DeltaTime, LetterBoxTransitionSpeed * 2.0f);
	// Fade
	UpdateCameraFade(DeltaTime);
}

void APlayerCameraManager::UpdateCameraFade(float DeltaTime)
{
	if (FadeTimeRemaining > 0.0f)
	{
		FadeTimeRemaining -= DeltaTime;

		if (FadeTimeRemaining <= 0.0f)
		{
			// --- 페이드 완료 ---
			FadeTimeRemaining = 0.0f;
			FadeAmount = FadeEndAlpha; // 목표 값으로 정확히 스냅

			// "완료 시 유지"가 false라면,
			// 0.0으로 다시 돌아가는 페이드를 *새로* 시작합니다.
			if (!bHoldFadeWhenFinished)
			{
				// 현재 값(FadeEndAlpha)에서 0.0으로, 동일한 시간 동안 페이드 아웃
				StartCameraFade(FadeEndAlpha, 0.0f, FadeDuration, FVector(FadeColor.X, FadeColor.Y, FadeColor.Z), true, CurrentFadeCurve);
			}
		}
	else
	{
		// --- 페이드 진행 중 ---
		// (총 시간 - 남은 시간) / 총 시간 = 진행률 (0.0 -> 1.0)
		float NormalizedTime = 0.0f;
		if (FadeDuration > 0.0f)
		{
			NormalizedTime = 1.0f - (FadeTimeRemaining / FadeDuration);
		}

		// Evaluate curve (linear if curve is null)
		float BlendedAlpha = NormalizedTime;
		if (CurrentFadeCurve)
		{
			BlendedAlpha = CurrentFadeCurve->Evaluate(NormalizedTime);
		}

		// Clamp to 0~1 (fade alpha should not overshoot)
		BlendedAlpha = std::clamp(BlendedAlpha, 0.0f, 1.0f);

		// 시작 알파와 끝 알파 사이를 보간합니다.
		FadeAmount = Lerp(FadeStartAlpha, FadeEndAlpha, BlendedAlpha);
	}
}
}

// Camera Modifier Management

UCameraModifier* APlayerCameraManager::AddCameraModifier(UCameraModifier* NewModifier)
{
	if (!NewModifier)
	{
		return nullptr;
	}

	// Check if this modifier is already in the list
	if (ModifierList.Contains(NewModifier))
	{
		return NewModifier;
	}

	// Add to list
	ModifierList.Add(NewModifier);

	// Initialize the modifier
	NewModifier->AddedToCamera(this);

	// Reset alpha to 0 for fresh blend-in
	// This ensures modifiers always start from 0 alpha regardless of AlphaInTime
	NewModifier->EnableModifier();  // Ensure it's enabled and reset state

	// Sort by priority (lower number = higher priority)
	ModifierList.Sort([](const UCameraModifier* A, const UCameraModifier* B)
	{
		return A->GetPriority() < B->GetPriority();
	});

	return NewModifier;
}

bool APlayerCameraManager::RemoveCameraModifier(UCameraModifier* ModifierToRemove)
{
	if (!ModifierToRemove)
	{
		return false;
	}

	// Find and remove the modifier
	int32 RemovedCount = ModifierList.Remove(ModifierToRemove);
	return RemovedCount > 0;
}

void APlayerCameraManager::ClearCameraModifiers()
{
	ModifierList.Empty();
}

void APlayerCameraManager::SetViewTargetWithBlend(AActor* NewViewTarget, float BlendTime, const FCurve* BlendCurve)
{
	if (BlendTime <= 0.0f || !NewViewTarget)
	{
		SetViewTarget(NewViewTarget);
		return;
	}

	BlendStartPOV = GetCameraCachePOV();
	SetViewTarget(NewViewTarget);

	bIsBlending = true;
	TotalBlendTime = BlendTime;
	CurrentBlendTime = BlendTime;
	CurrentBlendCurve = BlendCurve;
}
