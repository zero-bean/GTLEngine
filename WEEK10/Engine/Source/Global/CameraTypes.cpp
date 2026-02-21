#include "pch.h"
#include "Global/CameraTypes.h"
#include "Global/CurveTypes.h"
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include <algorithm>

void FMinimalViewInfo::UpdateCameraConstants()
{
	// Calculate camera basis vectors
	const FVector Forward = Rotation.RotateVector(FVector::ForwardVector());
	const FVector Right = Rotation.RotateVector(FVector::RightVector());
	const FVector Up = Rotation.RotateVector(FVector::UpVector());

	// View Matrix
	CameraConstants.View = FMatrix(
		Right.X, Up.X, Forward.X, 0.0f,
		Right.Y, Up.Y, Forward.Y, 0.0f,
		Right.Z, Up.Z, Forward.Z, 0.0f,
		-Right.Dot(Location), -Up.Dot(Location), -Forward.Dot(Location), 1.0f
	);

	// Projection Matrix
	if (ProjectionMode == ECameraProjectionMode::Perspective)
	{
		// Perspective projection
		const float HalfFOVRadians = (FOV * 0.5f) * (PI / 180.0f);
		const float TanHalfFOV = tanf(HalfFOVRadians);

		CameraConstants.Projection = FMatrix(
			1.0f / (AspectRatio * TanHalfFOV), 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f / TanHalfFOV, 0.0f, 0.0f,
			0.0f, 0.0f, FarClipPlane / (FarClipPlane - NearClipPlane), 1.0f,
			0.0f, 0.0f, -(FarClipPlane * NearClipPlane) / (FarClipPlane - NearClipPlane), 0.0f
		);
	}
	else
	{
		// Orthographic projection
		const float OrthoHeight = OrthoWidth / AspectRatio;

		CameraConstants.Projection = FMatrix(
			2.0f / OrthoWidth, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f / OrthoHeight, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f / (FarClipPlane - NearClipPlane), 0.0f,
			0.0f, 0.0f, -NearClipPlane / (FarClipPlane - NearClipPlane), 1.0f
		);
	}

	CameraConstants.ViewWorldLocation = Location;
	CameraConstants.NearClip = NearClipPlane;
	CameraConstants.FarClip = FarClipPlane;
}

FMinimalViewInfo FMinimalViewInfo::Blend(
	const FMinimalViewInfo& A,
	const FMinimalViewInfo& B,
	float Alpha,
	const FCurve* Curve)
{
	// Clamp input alpha to 0~1
	Alpha = std::clamp(Alpha, 0.0f, 1.0f);

	// Evaluate curve (can return overshoot values outside 0~1)
	float BlendedAlpha = Alpha; // Default: linear
	if (Curve)
	{
		BlendedAlpha = Curve->Evaluate(Alpha);
	}

	// Blend the view info
	FMinimalViewInfo Result;

	// Lerp location (overshoot is SAFE and desirable for spring effects)
	Result.Location = FVector(
		A.Location.X + (B.Location.X - A.Location.X) * BlendedAlpha,
		A.Location.Y + (B.Location.Y - A.Location.Y) * BlendedAlpha,
		A.Location.Z + (B.Location.Z - A.Location.Z) * BlendedAlpha
	);

	// Slerp rotation (MUST clamp - Slerp doesn't support overshoot)
	float RotationAlpha = std::clamp(BlendedAlpha, 0.0f, 1.0f);
	Result.Rotation = FQuaternion::SlerpShortestPath(A.Rotation, B.Rotation, RotationAlpha);

	// Lerp FOV (safe, but clamp to reasonable range)
	Result.FOV = A.FOV + (B.FOV - A.FOV) * BlendedAlpha;
	Result.FOV = std::clamp(Result.FOV, 1.0f, 179.0f); // Safety clamp

	// Lerp aspect ratio (overshoot generally safe)
	Result.AspectRatio = A.AspectRatio + (B.AspectRatio - A.AspectRatio) * BlendedAlpha;

	// Lerp clipping planes (MUST clamp - negative values cause rendering crashes)
	Result.NearClipPlane = A.NearClipPlane + (B.NearClipPlane - A.NearClipPlane) * BlendedAlpha;
	Result.NearClipPlane = std::max(Result.NearClipPlane, 0.001f); // Must be positive

	Result.FarClipPlane = A.FarClipPlane + (B.FarClipPlane - A.FarClipPlane) * BlendedAlpha;
	Result.FarClipPlane = std::max(Result.FarClipPlane, Result.NearClipPlane + 0.1f); // Must be > near

	// Use projection mode from B when blending is > 0.5
	float ProjectionModeThreshold = std::clamp(BlendedAlpha, 0.0f, 1.0f);
	Result.ProjectionMode = (ProjectionModeThreshold < 0.5f) ? A.ProjectionMode : B.ProjectionMode;

	// Lerp ortho width (overshoot generally safe)
	Result.OrthoWidth = A.OrthoWidth + (B.OrthoWidth - A.OrthoWidth) * BlendedAlpha;

	// Lerp fade amount (clamp to 0~1 for safety)
	Result.FadeAmount = A.FadeAmount + (B.FadeAmount - A.FadeAmount) * BlendedAlpha;
	Result.FadeAmount = std::clamp(Result.FadeAmount, 0.0f, 1.0f);

	// Lerp fade color (clamp each component to 0~1)
	Result.FadeColor = FVector4(
		std::clamp(A.FadeColor.X + (B.FadeColor.X - A.FadeColor.X) * BlendedAlpha, 0.0f, 1.0f),
		std::clamp(A.FadeColor.Y + (B.FadeColor.Y - A.FadeColor.Y) * BlendedAlpha, 0.0f, 1.0f),
		std::clamp(A.FadeColor.Z + (B.FadeColor.Z - A.FadeColor.Z) * BlendedAlpha, 0.0f, 1.0f),
		std::clamp(A.FadeColor.W + (B.FadeColor.W - A.FadeColor.W) * BlendedAlpha, 0.0f, 1.0f)
	);

	return Result;
}
