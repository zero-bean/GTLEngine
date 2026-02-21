#pragma once
#include "Render/Camera/Public/CameraModifier.h"

/**
 * Oscillation parameters for camera shake
 */
struct FOscillator
{
	/** Amplitude of the oscillation */
	float Amplitude = 0.0f;

	/** Frequency of the oscillation (cycles per second) */
	float Frequency = 0.0f;

	/** Current phase of the oscillation */
	float Phase = 0.0f;

	FOscillator() = default;
	FOscillator(float InAmplitude, float InFrequency)
		: Amplitude(InAmplitude), Frequency(InFrequency), Phase(0.0f)
	{
	}

	/** Get the current oscillation value and update phase */
	float Update(float DeltaTime)
	{
		Phase += DeltaTime * Frequency * 2.0f * PI;
		return sinf(Phase) * Amplitude;
	}
};

/**
 * UCameraModifier_CameraShake
 * Applies random camera shake/vibration effect
 */
UCLASS()
class UCameraModifier_CameraShake : public UCameraModifier
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraModifier_CameraShake, UCameraModifier)

public:
	UCameraModifier_CameraShake();
	virtual ~UCameraModifier_CameraShake() override;

	/**
	 * Called when added to camera - initializes and starts the shake
	 */
	virtual void AddedToCamera(APlayerCameraManager* Camera) override;

	/**
	 * Set overall intensity multiplier for all shake amplitudes
	 * @param InIntensity Intensity multiplier (1.0 = default, 2.0 = double intensity)
	 */
	void SetIntensity(float InIntensity);

	/**
	 * Set shake duration
	 * @param InDuration Total duration of the shake (0 = infinite)
	 */
	void SetDuration(float InDuration);

	/**
	 * Set blend in time
	 * @param InTime Time to blend in the shake effect
	 */
	void SetBlendInTime(float InTime);

	/**
	 * Set blend out time
	 * @param InTime Time to blend out the shake effect
	 */
	void SetBlendOutTime(float InTime);

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

	/**
	 * Override to stop oscillation when fully disabled
	 */
	virtual void DisableModifier(bool bImmediate) override;

protected:
	virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FQuaternion ViewRotation, float FOV,
	                          FVector& NewViewLocation, FQuaternion& NewViewRotation, float& NewFOV) override;

public:
	/** Location oscillation (X, Y, Z) */
	FOscillator LocOscillation[3];

	/** Rotation oscillation (Pitch, Yaw, Roll) in degrees */
	FOscillator RotOscillation[3];

	/** FOV oscillation */
	FOscillator FOVOscillation;

	/** Total duration of the shake (0 = infinite) */
	float Duration = 1.0f;

	/** Time elapsed since shake started */
	float ElapsedTime = 0.0f;

	/** Whether shake is currently playing */
	bool bIsPlaying = false;

private:
	/** Blend in time for the shake */
	float PendingBlendInTime = 0.2f;

	/** Blend out time for the shake */
	float PendingBlendOutTime = 0.2f;
};
