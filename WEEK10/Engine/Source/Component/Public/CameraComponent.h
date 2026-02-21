#pragma once

#include "Component/Public/SceneComponent.h"
#include "Global/CameraTypes.h"

/**
 * UCameraComponent
 * Component that provides camera view and projection functionality
 * Similar to Unreal Engine's UCameraComponent
 */
UCLASS()
class UCameraComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraComponent, USceneComponent)

public:
	UCameraComponent();
	virtual ~UCameraComponent() override;

	void BeginPlay() override;
	void TickComponent(float DeltaTime) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// Camera Info Getter
	/**
	 * Get the complete camera view information
	 * @return FMinimalViewInfo containing all camera parameters
	 */
	FMinimalViewInfo GetCameraView() const;

	/**
	 * Update camera for rendering
	 * Updates aspect ratio from viewport and recalculates camera constants
	 * @param InViewport D3D11 viewport information for aspect ratio calculation
	 */
	void Update(const D3D11_VIEWPORT& InViewport);

	/**
	 * Get cached camera constants for rendering pipeline
	 */
	const FCameraConstants& GetCameraConstants() const { return CachedCameraConstants; }

	/**
	 * Update camera constants for rendering
	 * Should be called before rendering
	 */
	void UpdateCameraConstants();

	// Field of View
	float GetFieldOfView() const { return FieldOfView; }
	void SetFieldOfView(float InFOV);

	// Aspect Ratio
	float GetAspectRatio() const { return AspectRatio; }
	void SetAspectRatio(float InAspectRatio);

	// Clipping Planes
	float GetNearClipPlane() const { return NearClipPlane; }
	void SetNearClipPlane(float InNearClip);

	float GetFarClipPlane() const { return FarClipPlane; }
	void SetFarClipPlane(float InFarClip);

	// Projection Mode
	ECameraProjectionMode GetProjectionMode() const { return ProjectionMode; }
	void SetProjectionMode(ECameraProjectionMode InMode);

	// Orthographic Settings
	float GetOrthoWidth() const { return OrthoWidth; }
	void SetOrthoWidth(float InWidth);

	// Constraint Aspect Ratio
	bool ShouldConstrainAspectRatio() const { return bConstrainAspectRatio; }
	void SetConstrainAspectRatio(bool bConstrain) { bConstrainAspectRatio = bConstrain; }

	// Use Pawn Control Rotation (for future PlayerController integration)
	bool ShouldUsePawnControlRotation() const { return bUsePawnControlRotation; }
	void SetUsePawnControlRotation(bool bUse) { bUsePawnControlRotation = bUse; }

	// Active Camera (for ViewTarget selection)
	bool IsActive() const { return bIsActive; }
	void SetActive(bool bActive) { bIsActive = bActive; }

	UObject* Duplicate() override;

	// Widget
	UClass* GetSpecificWidgetClass() const override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

private:
	// Core Camera Properties
	/** Field of view (in degrees) for perspective projection */
	float FieldOfView = 90.0f;

	/** Aspect ratio (width / height) */
	float AspectRatio = 1.777f;

	/** Near clipping plane distance */
	float NearClipPlane = 10.0f;

	/** Far clipping plane distance */
	float FarClipPlane = 10000.0f;

	/** Projection mode (Perspective or Orthographic) */
	ECameraProjectionMode ProjectionMode = ECameraProjectionMode::Perspective;

	/** Orthographic width (only used if ProjectionMode is Orthographic) */
	float OrthoWidth = 512.0f;

	/** Cached camera constants for rendering */
	FCameraConstants CachedCameraConstants;

	// Camera Behavior Flags
	/** Constrain aspect ratio when viewport changes */
	bool bConstrainAspectRatio = false;

	/** Use pawn's control rotation instead of component rotation */
	bool bUsePawnControlRotation = false;

	/** Whether this camera is active and can be used as ViewTarget */
	bool bIsActive = true;

	/** Cached to detect changes and update constants */
	mutable bool bNeedsConstantsUpdate = true;

	// Helper Methods
	/**
	 * Calculate perspective projection matrix
	 */
	FMatrix CalculatePerspectiveProjectionMatrix() const;

	/**
	 * Calculate orthographic projection matrix
	 */
	FMatrix CalculateOrthographicProjectionMatrix() const;

	/**
	 * Calculate view matrix from current world transform
	 */
	FMatrix CalculateViewMatrix() const;

public:
	// Mark transform dirty to trigger constants update
	void MarkAsDirty() override;
};
