#include "pch.h"
#include "Render/Camera/Public/CameraModifier.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Actor/Public/Actor.h"
#include "Global/CurveTypes.h"
#include <algorithm>

IMPLEMENT_CLASS(UCameraModifier, UObject)

UCameraModifier::UCameraModifier()
{
}

UCameraModifier::~UCameraModifier() = default;

void UCameraModifier::AddedToCamera(APlayerCameraManager* Camera)
{
	CameraOwner = Camera;
}

bool UCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// Unpack view info for native modify function
	FVector ViewLocation = InOutPOV.Location;
	FQuaternion ViewRotation = InOutPOV.Rotation;
	float FOV = InOutPOV.FOV;

	FVector NewViewLocation = ViewLocation;
	FQuaternion NewViewRotation = ViewRotation;
	float NewFOV = FOV;

	// Call protected modify camera (override point for derived classes)
	ModifyCamera(DeltaTime, ViewLocation, ViewRotation, FOV, NewViewLocation, NewViewRotation, NewFOV);

	// Apply alpha blending
	if (Alpha < 1.0f)
	{
		// Blend location
		InOutPOV.Location = FVector::Lerp(ViewLocation, NewViewLocation, Alpha);

		// Blend rotation (quaternion slerp)
		InOutPOV.Rotation = FQuaternion::Slerp(ViewRotation, NewViewRotation, Alpha);

		// Blend FOV
		InOutPOV.FOV = FOV + (NewFOV - FOV) * Alpha;
	}
	else
	{
		InOutPOV.Location = NewViewLocation;
		InOutPOV.Rotation = NewViewRotation;
		InOutPOV.FOV = NewFOV;
	}

	// Modify post-process settings (if camera owner exists)
	if (CameraOwner)
	{
		float PostProcessBlendWeight = 1.0f;
		FPostProcessSettings& PostProcessSettings = CameraOwner->GetPostProcessSettings();
		ModifyPostProcess(DeltaTime, PostProcessBlendWeight, PostProcessSettings);

		// Apply alpha to post-process blend weight
		PostProcessBlendWeight *= Alpha;
	}

	return false; // Continue modifier chain by default
}

void UCameraModifier::ModifyCamera(float DeltaTime, FVector ViewLocation, FQuaternion ViewRotation, float FOV,
                                    FVector& NewViewLocation, FQuaternion& NewViewRotation, float& NewFOV)
{
	// Base implementation does nothing
	// Override in derived classes
	NewViewLocation = ViewLocation;
	NewViewRotation = ViewRotation;
	NewFOV = FOV;
}

void UCameraModifier::ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings)
{
	// Base implementation does nothing
	// Override in derived classes
}

void UCameraModifier::UpdateAlpha(float DeltaTime)
{
	if (bDisabled)
	{
		return;
	}

	// State transitions
	bool bShouldBeActive = !bPendingDisable;

	if (BlendState == EAlphaBlendState::Idle)
	{
		if (bShouldBeActive && Alpha < 1.0f)
		{
			BlendState = EAlphaBlendState::BlendingIn;
			StateElapsedTime = 0.0f;
		}
		else if (!bShouldBeActive && Alpha > 0.0f)
		{
			BlendState = EAlphaBlendState::BlendingOut;
			StateElapsedTime = 0.0f;
		}
	}
	else if (BlendState == EAlphaBlendState::BlendingIn)
	{
		if (!bShouldBeActive)
		{
			BlendState = EAlphaBlendState::BlendingOut;
			StateElapsedTime = 0.0f;
		}
	}
	else if (BlendState == EAlphaBlendState::BlendingOut)
	{
		if (bShouldBeActive)
		{
			BlendState = EAlphaBlendState::BlendingIn;
			StateElapsedTime = 0.0f;
		}
	}

	// Update state elapsed time (for curve evaluation)
	if (BlendState != EAlphaBlendState::Idle)
	{
		StateElapsedTime += DeltaTime;
	}

	// Update alpha based on current state
	if (BlendState == EAlphaBlendState::BlendingIn)
	{
		if (AlphaInTime > 0.0f)
		{
			if (StateElapsedTime >= AlphaInTime)
			{
				Alpha = 1.0f;
				BlendState = EAlphaBlendState::Idle;
			}
			else
			{
				float t = StateElapsedTime / AlphaInTime;
				Alpha = AlphaInCurve ? AlphaInCurve->Evaluate(t) : t;
			}
		}
		else
		{
			Alpha = 1.0f;
			BlendState = EAlphaBlendState::Idle;
		}
	}
	else if (BlendState == EAlphaBlendState::BlendingOut)
	{
		if (AlphaOutTime > 0.0f)
		{
			if (StateElapsedTime >= AlphaOutTime)
			{
				Alpha = 0.0f;
				BlendState = EAlphaBlendState::Idle;

				if (bPendingDisable)
				{
					DisableModifier(true);
				}
			}
			else
			{
				float t = StateElapsedTime / AlphaOutTime;
				float curveValue = AlphaOutCurve ? AlphaOutCurve->Evaluate(t) : t;
				Alpha = 1.0f - curveValue;
			}
		}
		else
		{
			Alpha = 0.0f;
			BlendState = EAlphaBlendState::Idle;

			if (bPendingDisable)
			{
				DisableModifier(true);
			}
		}
	}
	else // Idle
	{
		// Keep alpha at 1.0 during full effect
		if (!bPendingDisable)
		{
			Alpha = 1.0f;
		}
	}
}

float UCameraModifier::GetTargetAlpha()
{
	// If disabled or pending disable, target alpha is 0, otherwise 1
	return (bDisabled || bPendingDisable) ? 0.0f : 1.0f;
}

void UCameraModifier::DisableModifier(bool bImmediate)
{
	if (bImmediate)
	{
		bDisabled = true;
		Alpha = 0.0f;
		bPendingDisable = false;
	}
	else
	{
		// Blend out before disabling
		bPendingDisable = true;
	}
}

void UCameraModifier::EnableModifier()
{
	bDisabled = false;
	bPendingDisable = false;

	// Reset state for fresh start
	Alpha = 0.0f;
	StateElapsedTime = 0.0f;
	BlendState = EAlphaBlendState::Idle;
}

AActor* UCameraModifier::GetViewTarget() const
{
	if (CameraOwner)
	{
		return CameraOwner->GetViewTarget();
	}
	return nullptr;
}

void UCameraModifier::SetAlphaInCurve(const FCurve* Curve)
{
	AlphaInCurve = Curve;
}

void UCameraModifier::SetAlphaOutCurve(const FCurve* Curve)
{
	AlphaOutCurve = Curve;
}
