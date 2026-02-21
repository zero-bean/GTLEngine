#include "pch.h"
#include "Render/Camera/Public/CameraModifier_CameraShake.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Global/CurveTypes.h"

IMPLEMENT_CLASS(UCameraModifier_CameraShake, UCameraModifier)

UCameraModifier_CameraShake::UCameraModifier_CameraShake()
{
	// Default shake parameters (can be customized per shake instance)
	LocOscillation[0] = FOscillator(10.0f, 10.0f);  // X: 10 units, 10 Hz
	LocOscillation[1] = FOscillator(10.0f, 10.0f);  // Y: 10 units, 10 Hz
	LocOscillation[2] = FOscillator(5.0f, 10.0f);   // Z: 5 units, 10 Hz (less vertical shake)

	RotOscillation[0] = FOscillator(2.0f, 10.0f);   // Pitch: 2 degrees, 10 Hz
	RotOscillation[1] = FOscillator(2.0f, 10.0f);   // Yaw: 2 degrees, 10 Hz
	RotOscillation[2] = FOscillator(1.0f, 10.0f);   // Roll: 1 degree, 10 Hz

	FOVOscillation = FOscillator(2.0f, 8.0f);       // FOV: 2 degrees, 8 Hz

	Duration = 1.0f;
	ElapsedTime = 0.0f;
	bIsPlaying = false;
}

UCameraModifier_CameraShake::~UCameraModifier_CameraShake() = default;

void UCameraModifier_CameraShake::AddedToCamera(APlayerCameraManager* Camera)
{
	// Call base class
	Super::AddedToCamera(Camera);

	// Initialize shake when added to camera
	ElapsedTime = 0.0f;
	bIsPlaying = true;

	// Set alpha blend times
	AlphaInTime = PendingBlendInTime;
	AlphaOutTime = PendingBlendOutTime;

	// Reset oscillator phases for fresh start
	for (int i = 0; i < 3; ++i)
	{
		LocOscillation[i].Phase = 0.0f;
		RotOscillation[i].Phase = 0.0f;
	}
	FOVOscillation.Phase = 0.0f;

	// Enable the modifier - parent's UpdateAlpha() will handle blend in
	EnableModifier();
}

void UCameraModifier_CameraShake::SetIntensity(float InIntensity)
{
	// Apply intensity to all oscillation amplitudes
	LocOscillation[0].Amplitude = 10.0f * InIntensity;
	LocOscillation[1].Amplitude = 10.0f * InIntensity;
	LocOscillation[2].Amplitude = 5.0f * InIntensity;
	RotOscillation[0].Amplitude = 2.0f * InIntensity;
	RotOscillation[1].Amplitude = 2.0f * InIntensity;
	RotOscillation[2].Amplitude = 1.0f * InIntensity;
	FOVOscillation.Amplitude = 2.0f * InIntensity;
}

void UCameraModifier_CameraShake::SetDuration(float InDuration)
{
	Duration = InDuration;
}

void UCameraModifier_CameraShake::SetBlendInTime(float InTime)
{
	PendingBlendInTime = InTime;
}

void UCameraModifier_CameraShake::SetBlendOutTime(float InTime)
{
	PendingBlendOutTime = InTime;
}

void UCameraModifier_CameraShake::DisableModifier(bool bImmediate)
{
	// Stop shake when disabled
	bIsPlaying = false;

	// Call parent
	Super::DisableModifier(bImmediate);
}

bool UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	if (!bIsPlaying)
	{
		return false;
	}

	// Update elapsed time
	ElapsedTime += DeltaTime;

	// Check if it's time to start blend out
	// BlendOut should start at (Duration - AlphaOutTime) so it completes exactly at Duration
	float BlendOutStartTime = (Duration > AlphaOutTime) ? (Duration - AlphaOutTime) : 0.0f;
	if (Duration > 0.0f && ElapsedTime >= BlendOutStartTime && !bPendingDisable)
	{
		// Time to start blend out - let parent's UpdateAlpha() handle it
		Super::DisableModifier(false);
	}

	// Call base class to apply shake with alpha blending
	return Super::ModifyCamera(DeltaTime, InOutPOV);
}

void UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FVector ViewLocation, FQuaternion ViewRotation, float FOV,
                                                FVector& NewViewLocation, FQuaternion& NewViewRotation, float& NewFOV)
{
	// Apply location shake
	NewViewLocation.X = ViewLocation.X + LocOscillation[0].Update(DeltaTime);
	NewViewLocation.Y = ViewLocation.Y + LocOscillation[1].Update(DeltaTime);
	NewViewLocation.Z = ViewLocation.Z + LocOscillation[2].Update(DeltaTime);

	// Apply rotation shake (convert degrees to quaternion)
	float PitchDelta = RotOscillation[0].Update(DeltaTime);
	float YawDelta = RotOscillation[1].Update(DeltaTime);
	float RollDelta = RotOscillation[2].Update(DeltaTime);

	// Create delta rotation quaternion from Euler angles
	FQuaternion DeltaRotation = FQuaternion::FromEuler(FVector(PitchDelta, YawDelta, RollDelta));

	// Apply delta rotation to current rotation
	NewViewRotation = ViewRotation * DeltaRotation;

	// Apply FOV shake
	NewFOV = FOV + FOVOscillation.Update(DeltaTime);
}
