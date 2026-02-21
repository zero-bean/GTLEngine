#include "pch.h"
#include "Component/Public/CameraComponent.h"
#include "Utility/Public/JsonSerializer.h"
#include "Render/UI/Widget/Public/CameraComponentWidget.h"

IMPLEMENT_CLASS(UCameraComponent, USceneComponent)

UCameraComponent::UCameraComponent()
{
	// Default camera settings
	FieldOfView = 90.0f;
	AspectRatio = 1.777f; // 16:9.
	NearClipPlane = 0.1f;
	FarClipPlane = 10000.0f;
	ProjectionMode = ECameraProjectionMode::Perspective;
	OrthoWidth = 512.0f;
	bConstrainAspectRatio = false;
	bUsePawnControlRotation = false;
	bNeedsConstantsUpdate = true;

	// Enable tick for camera updates
	bCanEverTick = true;
}

UCameraComponent::~UCameraComponent() = default;

void UCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateCameraConstants();
}

void UCameraComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	// Update camera constants if transform or properties changed
	if (bNeedsConstantsUpdate)
	{
		UpdateCameraConstants();
	}
}

void UCameraComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "FieldOfView", FieldOfView, 90.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "AspectRatio", AspectRatio, 1.777f);
		FJsonSerializer::ReadFloat(InOutHandle, "NearClipPlane", NearClipPlane, 10.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "FarClipPlane", FarClipPlane, 10000.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "OrthoWidth", OrthoWidth, 512.0f);
		FJsonSerializer::ReadBool(InOutHandle, "bConstrainAspectRatio", bConstrainAspectRatio, false);
		FJsonSerializer::ReadBool(InOutHandle, "bUsePawnControlRotation", bUsePawnControlRotation, false);
		FJsonSerializer::ReadBool(InOutHandle, "bIsActive", bIsActive, true);

		uint32 ProjectionModeValue = static_cast<uint32>(ProjectionMode);
		FJsonSerializer::ReadUint32(InOutHandle, "ProjectionMode", ProjectionModeValue, 0);
		ProjectionMode = static_cast<ECameraProjectionMode>(ProjectionModeValue);

		bNeedsConstantsUpdate = true;
	}
	else
	{
		InOutHandle["FieldOfView"] = FieldOfView;
		InOutHandle["AspectRatio"] = AspectRatio;
		InOutHandle["NearClipPlane"] = NearClipPlane;
		InOutHandle["FarClipPlane"] = FarClipPlane;
		InOutHandle["OrthoWidth"] = OrthoWidth;
		InOutHandle["bConstrainAspectRatio"] = bConstrainAspectRatio;
		InOutHandle["bUsePawnControlRotation"] = bUsePawnControlRotation;
		InOutHandle["bIsActive"] = bIsActive;
		InOutHandle["ProjectionMode"] = static_cast<uint32>(ProjectionMode);
	}
}

FMinimalViewInfo UCameraComponent::GetCameraView() const
{
	FMinimalViewInfo ViewInfo;

	ViewInfo.Location = GetWorldLocation();
	ViewInfo.Rotation = GetWorldRotationAsQuaternion();
	ViewInfo.FOV = FieldOfView;
	ViewInfo.AspectRatio = AspectRatio;
	ViewInfo.NearClipPlane = NearClipPlane;
	ViewInfo.FarClipPlane = FarClipPlane;
	ViewInfo.ProjectionMode = ProjectionMode;
	ViewInfo.OrthoWidth = OrthoWidth;
	ViewInfo.CameraConstants = CachedCameraConstants;

	return ViewInfo;
}

void UCameraComponent::Update(const D3D11_VIEWPORT& InViewport)
{
	if (InViewport.Width > 0.f && InViewport.Height > 0.f)
	{
		const float Aspect = InViewport.Width / InViewport.Height;
		SetAspectRatio(Aspect);
	}

	UpdateCameraConstants();
}

void UCameraComponent::UpdateCameraConstants()
{
	CachedCameraConstants.View = CalculateViewMatrix();

	if (ProjectionMode == ECameraProjectionMode::Perspective)
	{
		CachedCameraConstants.Projection = CalculatePerspectiveProjectionMatrix();
	}
	else
	{
		CachedCameraConstants.Projection = CalculateOrthographicProjectionMatrix();
	}

	CachedCameraConstants.ViewWorldLocation = GetWorldLocation();
	CachedCameraConstants.NearClip = NearClipPlane;
	CachedCameraConstants.FarClip = FarClipPlane;

	bNeedsConstantsUpdate = false;
}

void UCameraComponent::SetFieldOfView(float InFOV)
{
	FieldOfView = InFOV;
	bNeedsConstantsUpdate = true;
}

void UCameraComponent::SetAspectRatio(float InAspectRatio)
{
	AspectRatio = InAspectRatio;
	bNeedsConstantsUpdate = true;
}

void UCameraComponent::SetNearClipPlane(float InNearClip)
{
	NearClipPlane = InNearClip;
	bNeedsConstantsUpdate = true;
}

void UCameraComponent::SetFarClipPlane(float InFarClip)
{
	FarClipPlane = InFarClip;
	bNeedsConstantsUpdate = true;
}

void UCameraComponent::SetProjectionMode(ECameraProjectionMode InMode)
{
	ProjectionMode = InMode;
	bNeedsConstantsUpdate = true;
}

void UCameraComponent::SetOrthoWidth(float InWidth)
{
	OrthoWidth = InWidth;
	bNeedsConstantsUpdate = true;
}

void UCameraComponent::MarkAsDirty()
{
	Super::MarkAsDirty();
	bNeedsConstantsUpdate = true;
}

FMatrix UCameraComponent::CalculatePerspectiveProjectionMatrix() const
{
	// Standard perspective projection matrix
	// FOV is in degrees, convert to radians
	const float HalfFOVRadians = (FieldOfView * 0.5f) * (PI / 180.0f);
	const float TanHalfFOV = tanf(HalfFOVRadians);

	FMatrix ProjectionMatrix;

	// Perspective projection (DirectX-style, left-handed coordinate system)
	ProjectionMatrix.Data[0][0] = 1.0f / (AspectRatio * TanHalfFOV);
	ProjectionMatrix.Data[0][1] = 0.0f;
	ProjectionMatrix.Data[0][2] = 0.0f;
	ProjectionMatrix.Data[0][3] = 0.0f;

	ProjectionMatrix.Data[1][0] = 0.0f;
	ProjectionMatrix.Data[1][1] = 1.0f / TanHalfFOV;
	ProjectionMatrix.Data[1][2] = 0.0f;
	ProjectionMatrix.Data[1][3] = 0.0f;

	ProjectionMatrix.Data[2][0] = 0.0f;
	ProjectionMatrix.Data[2][1] = 0.0f;
	ProjectionMatrix.Data[2][2] = FarClipPlane / (FarClipPlane - NearClipPlane);
	ProjectionMatrix.Data[2][3] = 1.0f;

	ProjectionMatrix.Data[3][0] = 0.0f;
	ProjectionMatrix.Data[3][1] = 0.0f;
	ProjectionMatrix.Data[3][2] = -(FarClipPlane * NearClipPlane) / (FarClipPlane - NearClipPlane);
	ProjectionMatrix.Data[3][3] = 0.0f;

	return ProjectionMatrix;
}

FMatrix UCameraComponent::CalculateOrthographicProjectionMatrix() const
{
	// Orthographic projection matrix
	const float OrthoHeight = OrthoWidth / AspectRatio;

	FMatrix ProjectionMatrix;

	// Orthographic projection (DirectX-style)
	ProjectionMatrix.Data[0][0] = 2.0f / OrthoWidth;
	ProjectionMatrix.Data[0][1] = 0.0f;
	ProjectionMatrix.Data[0][2] = 0.0f;
	ProjectionMatrix.Data[0][3] = 0.0f;

	ProjectionMatrix.Data[1][0] = 0.0f;
	ProjectionMatrix.Data[1][1] = 2.0f / OrthoHeight;
	ProjectionMatrix.Data[1][2] = 0.0f;
	ProjectionMatrix.Data[1][3] = 0.0f;

	ProjectionMatrix.Data[2][0] = 0.0f;
	ProjectionMatrix.Data[2][1] = 0.0f;
	ProjectionMatrix.Data[2][2] = 1.0f / (FarClipPlane - NearClipPlane);
	ProjectionMatrix.Data[2][3] = 0.0f;

	ProjectionMatrix.Data[3][0] = 0.0f;
	ProjectionMatrix.Data[3][1] = 0.0f;
	ProjectionMatrix.Data[3][2] = -NearClipPlane / (FarClipPlane - NearClipPlane);
	ProjectionMatrix.Data[3][3] = 1.0f;

	return ProjectionMatrix;
}

FMatrix UCameraComponent::CalculateViewMatrix() const
{
	// Get camera world transform
	const FVector CameraLocation = GetWorldLocation();
	const FQuaternion CameraRotation = GetWorldRotationAsQuaternion();

	// Calculate camera basis vectors
	const FVector Forward = CameraRotation.RotateVector(FVector::ForwardVector());
	const FVector Right = CameraRotation.RotateVector(FVector::RightVector());
	const FVector Up = CameraRotation.RotateVector(FVector::UpVector());

	// View matrix (inverse of camera transform)
	FMatrix ViewMatrix;

	ViewMatrix.Data[0][0] = Right.X;
	ViewMatrix.Data[0][1] = Up.X;
	ViewMatrix.Data[0][2] = Forward.X;
	ViewMatrix.Data[0][3] = 0.0f;

	ViewMatrix.Data[1][0] = Right.Y;
	ViewMatrix.Data[1][1] = Up.Y;
	ViewMatrix.Data[1][2] = Forward.Y;
	ViewMatrix.Data[1][3] = 0.0f;

	ViewMatrix.Data[2][0] = Right.Z;
	ViewMatrix.Data[2][1] = Up.Z;
	ViewMatrix.Data[2][2] = Forward.Z;
	ViewMatrix.Data[2][3] = 0.0f;

	ViewMatrix.Data[3][0] = -Right.Dot(CameraLocation);
	ViewMatrix.Data[3][1] = -Up.Dot(CameraLocation);
	ViewMatrix.Data[3][2] = -Forward.Dot(CameraLocation);
	ViewMatrix.Data[3][3] = 1.0f;

	return ViewMatrix;
}

UObject* UCameraComponent::Duplicate()
{
	UCameraComponent* DuplicatedComponent = static_cast<UCameraComponent*>(Super::Duplicate());

	if (DuplicatedComponent)
	{
		DuplicatedComponent->FieldOfView = FieldOfView;
		DuplicatedComponent->AspectRatio = AspectRatio;
		DuplicatedComponent->NearClipPlane = NearClipPlane;
		DuplicatedComponent->FarClipPlane = FarClipPlane;
		DuplicatedComponent->ProjectionMode = ProjectionMode;
		DuplicatedComponent->OrthoWidth = OrthoWidth;
		DuplicatedComponent->bConstrainAspectRatio = bConstrainAspectRatio;
		DuplicatedComponent->bUsePawnControlRotation = bUsePawnControlRotation;
		DuplicatedComponent->bIsActive = bIsActive;
		DuplicatedComponent->bNeedsConstantsUpdate = true;
	}

	return DuplicatedComponent;
}

void UCameraComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}

UClass* UCameraComponent::GetSpecificWidgetClass() const
{
	return UCameraComponentWidget::StaticClass();
}
