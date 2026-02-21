#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include "Global/Quaternion.h"

// Forward declarations
struct FCurve;

/**
 * Camera Constants for Rendering
 * View and Projection matrices with clip planes
 */
struct FCameraConstants
{
	FCameraConstants() : NearClip(0), FarClip(0)
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
	FVector ViewWorldLocation;
	float NearClip;
	float FarClip;
};

/**
 * Camera Projection Mode
 */
enum class ECameraProjectionMode : uint8
{
	Perspective,
	Orthographic
};

// Removed EViewTargetBlendFunction - use curve names instead (e.g., "Linear", "EaseIn", etc.)

/**
 * Camera Shake Play Space
 */
enum class ECameraShakePlaySpace : uint8
{
	CameraLocal,
	World,
	UserDefined
};

/**
 * Minimal View Info for Camera
 * Contains essential camera parameters for rendering
 */
struct FMinimalViewInfo
{
	/** Camera location (world space) */
	FVector Location = FVector::ZeroVector();

	/** Camera rotation (world space) */
	FQuaternion Rotation = FQuaternion::Identity();

	/** Field of view (degrees) */
	float FOV = 90.0f;

	/** Desired aspect ratio */
	float AspectRatio = 1.777f;

	/** Near clipping plane */
	float NearClipPlane = 0.1f;

	/** Far clipping plane */
	float FarClipPlane = 1000.0f;

	/** Projection mode */
	ECameraProjectionMode ProjectionMode = ECameraProjectionMode::Perspective;

	/** Ortho width */
	float OrthoWidth = 512.0f;

	/** Cached camera constants (generated from above) */
	FCameraConstants CameraConstants;

	/** Fade amount (0=transparent, 1=fully faded) */
	float FadeAmount = 0.0f;

	/** Fade color (RGBA) */
	FVector4 FadeColor = FVector4(0.0f, 0.0f, 0.0f, 1.0f);

	/**
	 * Generate camera constants from this POV
	 */
	void UpdateCameraConstants();

	/**
	 * Blend between two POVs using curve
	 * @param A Starting view
	 * @param B Target view
	 * @param Alpha Blend alpha (0.0 ~ 1.0, input is clamped)
	 * @param Curve Resolved curve pointer (if null, uses linear blend)
	 * @return Blended view
	 * @note Alpha input is clamped to 0~1, but curve evaluation can return overshoot values.
	 *       Overshoot is safe for Location, but Rotation/ClipPlanes will be clamped.
	 */
	static FMinimalViewInfo Blend(
		const FMinimalViewInfo& A,
		const FMinimalViewInfo& B,
		float Alpha,
		const FCurve* Curve = nullptr
	);
};

struct FPostProcessSettings
{
	// --- LetterBox ---
	float LetterBoxAmount = 0.0f;
	float TargetAspectRatio = 1.777f; // (기본 16:9)

	// --- Vignette ---
	bool  bEnableVignette = true;
	float VignetteIntensity = 0.8f;

	// --- Gamma ---
	float Gamma = 2.2f;

	// 기본값으로 리셋하는 함수
	void Reset()
	{
		*this = FPostProcessSettings(); // C++ 기본 생성자로 덮어쓰기
	}
};
