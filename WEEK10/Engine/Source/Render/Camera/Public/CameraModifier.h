#pragma once
#include "Core/Public/Object.h"
#include "Global/CameraTypes.h"

class APlayerCameraManager;
struct FCurve;

/**
 * A CameraModifier is a base class for objects that may adjust the final camera properties after
 * being computed by the APlayerCameraManager. A CameraModifier can be stateful, and is associated
 * uniquely with a specific APlayerCameraManager.
 */
UCLASS()
class UCameraModifier : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraModifier, UObject)

public:
	UCameraModifier();
	virtual ~UCameraModifier() override;

	/**
	 * Allows any custom initialization. Called immediately after creation.
	 * @param Camera - The camera this modifier should be associated with.
	 */
	virtual void AddedToCamera(APlayerCameraManager* Camera);

	/**
	 * Directly modifies variables in the owning camera.
	 * This is called by PlayerCameraManager and handles alpha blending.
	 * @param DeltaTime Change in time since last update
	 * @param InOutPOV Current Point of View, to be updated.
	 * @return bool True if should STOP looping the chain, false otherwise
	 */
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV);

	/**
	 * Responsible for updating alpha blend value.
	 * @param DeltaTime Amount of time since last update
	 */
	virtual void UpdateAlpha(float DeltaTime);

	// Getter/Setter
	virtual bool IsDisabled() const { return bDisabled; }
	virtual bool IsPendingDisable() const { return bPendingDisable; }

	/**
	 * Disables this modifier.
	 * @param bImmediate true to disable with no blend out, false (default) to allow blend out
	 */
	virtual void DisableModifier(bool bImmediate = false);

	/** Enables this modifier. */
	virtual void EnableModifier();

	uint8 GetPriority() const { return Priority; }
	void SetPriority(uint8 InPriority) { Priority = InPriority; }

	float GetAlpha() const { return Alpha; }

	class AActor* GetViewTarget() const;

	/**
	 * Set alpha-in curve
	 * @param Curve Curve for alpha blending in (nullptr = linear)
	 */
	void SetAlphaInCurve(const FCurve* Curve);

	/**
	 * Set alpha-out curve
	 * @param Curve Curve for alpha blending out (nullptr = linear)
	 */
	void SetAlphaOutCurve(const FCurve* Curve);

	/** Get/Set alpha in time */
	float GetAlphaInTime() const { return AlphaInTime; }
	void SetAlphaInTime(float InTime) { AlphaInTime = InTime; }

	/** Get/Set alpha out time */
	float GetAlphaOutTime() const { return AlphaOutTime; }
	void SetAlphaOutTime(float InTime) { AlphaOutTime = InTime; }

protected:
	/** @return Returns the ideal blend alpha for this modifier. Interpolation will seek this value. */
	virtual float GetTargetAlpha();

	/**
	 * Allows modifying the camera transform in native code.
	 * Override this in derived classes to implement camera effects (shake, zoom, etc).
	 * Called by public ModifyCamera() with unpacked view info.
	 */
	virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FQuaternion ViewRotation, float FOV,
	                          FVector& NewViewLocation, FQuaternion& NewViewRotation, float& NewFOV);

	/**
	 * Allows modifying the post process in native code.
	 * Override this in derived classes to implement post-process effects (fade, vignette, etc).
	 * Called by public ModifyCamera() to adjust post-process settings.
	 */
	virtual void ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings);

protected:
	/** If true, do not apply this modifier to the camera. */
	bool bDisabled = false;

	/** If true, this modifier will disable itself when finished interpolating out. */
	bool bPendingDisable = false;

	/** Camera this object is associated with. */
	APlayerCameraManager* CameraOwner = nullptr;

	/** Priority value that determines the order in which modifiers are applied. 0 = highest priority, 255 = lowest. */
	uint8 Priority = 0;

	/** When blending in, alpha proceeds from 0 to 1 over this time */
	float AlphaInTime = 0.0f;

	/** When blending out, alpha proceeds from 1 to 0 over this time */
	float AlphaOutTime = 0.0f;

	/** Current blend alpha. */
	float Alpha = 0.0f;

	/** Curve for alpha blending in (nullptr = linear) */
	const FCurve* AlphaInCurve = nullptr;

	/** Curve for alpha blending out (nullptr = linear) */
	const FCurve* AlphaOutCurve = nullptr;

	/** Elapsed time in current blend state (for curve evaluation) */
	float StateElapsedTime = 0.0f;

	/** Alpha blend state */
	enum class EAlphaBlendState : uint8
	{
		Idle,        // Not blending
		BlendingIn,  // Blending from 0 to 1
		BlendingOut  // Blending from 1 to 0
	};
	EAlphaBlendState BlendState = EAlphaBlendState::Idle;

	friend class APlayerCameraManager;
};

