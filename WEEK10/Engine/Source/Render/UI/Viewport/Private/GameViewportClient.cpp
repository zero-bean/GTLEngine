#include "pch.h"
#include "Render/UI/Viewport/Public/GameViewportClient.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Level/Public/World.h"
#include "Level/Public/GameInstance.h"
#include "Render/Renderer/Public/SceneView.h"
#include "Render/Renderer/Public/SceneViewFamily.h"
#include "Render/Renderer/Public/SceneRenderer.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Component/Public/CameraComponent.h"
#include "Editor/Public/Camera.h"

IMPLEMENT_CLASS(UGameViewportClient, UObject)

UGameViewportClient::UGameViewportClient()
{
	// 카메라를 소유하지 않음 - Transform만 초기화
	// 실제 카메라는 PlayerController나 GameMode에서 관리
}

void UGameViewportClient::Tick()
{
	// PlayerCameraManager의 POV를 ViewportClient Transform으로 동기화
	if (!GWorld)
	{
		return;
	}

	AGameMode* GameMode = GWorld->GetGameMode();
	if (!GameMode)
	{
		return;
	}

	APlayerCameraManager* CameraManager = GameMode->GetPlayerCameraManager();
	if (!CameraManager)
	{
		return;
	}

	// CameraManager의 최신 POV 가져오기
	const FMinimalViewInfo& POV = CameraManager->GetCameraCachePOV();

	// ViewportClient의 Transform 업데이트
	ViewLocation = POV.Location;

	// ToEuler() returns (Roll, Pitch, Yaw) = (X, Y, Z)
	// But we need (Pitch, Yaw, Roll) for compatibility with legacy code
	FVector Euler = POV.Rotation.ToEuler();
	ViewRotation = FVector(Euler.Y, Euler.Z, Euler.X);  // Convert to (Pitch, Yaw, Roll)

	FOV = POV.FOV;
	NearZ = POV.CameraConstants.NearClip;
	FarZ = POV.CameraConstants.FarClip;
}

/**
 * @brief ViewportClient 렌더링
 * SceneViewFamily와 SceneView를 생성하여 월드를 렌더링
 * @param InViewport 렌더링할 Viewport
 */
void UGameViewportClient::Draw(FViewport* InViewport) const
{
	if (!InViewport)
	{
		UE_LOG_WARNING("GameViewportClient::Draw - Invalid viewport");
		return;
	}

	// World 가져오기
	if (!GWorld)
	{
		UE_LOG_WARNING("GameViewportClient::Draw - No World");
		return;
	}

	// Legacy Camera 업데이트 (레거시 RenderPass 지원용)
	const_cast<UGameViewportClient*>(this)->UpdateLegacyCamera();

	// SceneViewFamily 생성
	FSceneViewFamily ViewFamily;
	ViewFamily.SetRenderTarget(InViewport);
	ViewFamily.SetCurrentTime(0.0f); // TODO: World Time 연결
	ViewFamily.SetDeltaWorldTime(0.016f); // TODO: DeltaTime 연결

	// SceneView 생성
	FSceneView* View = new FSceneView();
	View->InitializeWithMatrices(
		GetViewMatrix(),
		GetProjectionMatrix(),
		ViewLocation,
		ViewRotation,
		InViewport,
		GWorld,
		EViewModeIndex::VMI_BlinnPhong,
		FOV,
		NearZ,
		FarZ
	);

	// GameViewportClient 설정 (Legacy Camera 접근용)
	View->SetViewport(InViewport);

	ViewFamily.AddView(View);

	// SceneRenderer로 렌더링
	FSceneRenderer* SceneRenderer = FSceneRenderer::CreateSceneRenderer(ViewFamily);
	if (SceneRenderer)
	{
		SceneRenderer->Render();
		delete SceneRenderer;
	}

	// View 정리
	delete View;
}

/**
 * @brief View Matrix 계산
 * ViewLocation과 ViewRotation으로부터 View 행렬 생성
 */
FMatrix UGameViewportClient::GetViewMatrix() const
{
	FMatrix ViewMatrix;

	// ViewRotation을 사용하여 방향 벡터 계산
	FVector Radians = FVector::GetDegreeToRadian(ViewRotation);
	FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);

	FVector Forward = FMatrix::VectorMultiply(FVector::ForwardVector(), RotationMatrix);
	Forward.Normalize();

	const FVector WorldUp = FVector(0, 0, 1);
	FVector Right = WorldUp.Cross(Forward);

	// Forward가 거의 Z축과 평행한 경우 (Pitch ±90° 근처)
	if (Right.LengthSquared() < MATH_EPSILON)
	{
		// Y축을 대체 Right로 사용
		Right = FVector(0, 1, 0);
	}

	Right.Normalize();
	FVector Up = Forward.Cross(Right);
	Up.Normalize();

	// DirectX View Matrix 구성
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

	ViewMatrix.Data[3][0] = -Right.Dot(ViewLocation);
	ViewMatrix.Data[3][1] = -Up.Dot(ViewLocation);
	ViewMatrix.Data[3][2] = -Forward.Dot(ViewLocation);
	ViewMatrix.Data[3][3] = 1.0f;

	return ViewMatrix;
}

/**
 * @brief Projection Matrix 계산
 * FOV, NearZ, FarZ, Viewport 크기로부터 Projection 행렬 생성
 */
FMatrix UGameViewportClient::GetProjectionMatrix() const
{
	FMatrix ProjectionMatrix;

	if (!OwningViewport)
	{
		return FMatrix::Identity();
	}

	const FRect& ViewportRect = OwningViewport->GetRect();
	float AspectRatio = static_cast<float>(ViewportRect.Width) / static_cast<float>(ViewportRect.Height);

	// 안전한 aspect ratio (0으로 나누기 방지)
	if (ViewportRect.Height == 0)
	{
		AspectRatio = 1.777f; // 16:9 기본값
	}

	// Perspective Projection Matrix
	const float FovRadians = FVector::GetDegreeToRadian(FOV);
	const float TanHalfFov = tan(FovRadians / 2.0f);

	ProjectionMatrix.Data[0][0] = 1.0f / (AspectRatio * TanHalfFov);
	ProjectionMatrix.Data[0][1] = 0.0f;
	ProjectionMatrix.Data[0][2] = 0.0f;
	ProjectionMatrix.Data[0][3] = 0.0f;

	ProjectionMatrix.Data[1][0] = 0.0f;
	ProjectionMatrix.Data[1][1] = 1.0f / TanHalfFov;
	ProjectionMatrix.Data[1][2] = 0.0f;
	ProjectionMatrix.Data[1][3] = 0.0f;

	ProjectionMatrix.Data[2][0] = 0.0f;
	ProjectionMatrix.Data[2][1] = 0.0f;
	ProjectionMatrix.Data[2][2] = FarZ / (FarZ - NearZ);
	ProjectionMatrix.Data[2][3] = 1.0f;

	ProjectionMatrix.Data[3][0] = 0.0f;
	ProjectionMatrix.Data[3][1] = 0.0f;
	ProjectionMatrix.Data[3][2] = -(FarZ * NearZ) / (FarZ - NearZ);
	ProjectionMatrix.Data[3][3] = 0.0f;

	return ProjectionMatrix;
}

/**
 * @brief Projection Matrix의 역행렬 계산
 * 수치 안정성을 위한 분석적 역행렬
 */
FMatrix UGameViewportClient::GetProjectionMatrixInverse() const
{
	FMatrix InvProjection;

	if (!OwningViewport)
	{
		return FMatrix::Identity();
	}

	const FRect& ViewportRect = OwningViewport->GetRect();
	float AspectRatio = static_cast<float>(ViewportRect.Width) / static_cast<float>(ViewportRect.Height);

	if (ViewportRect.Height == 0)
	{
		AspectRatio = 1.777f;
	}

	const float FovRadians = FVector::GetDegreeToRadian(FOV);
	const float TanHalfFov = tan(FovRadians / 2.0f);

	// Analytical inverse of perspective projection matrix
	InvProjection.Data[0][0] = AspectRatio * TanHalfFov;
	InvProjection.Data[0][1] = 0.0f;
	InvProjection.Data[0][2] = 0.0f;
	InvProjection.Data[0][3] = 0.0f;

	InvProjection.Data[1][0] = 0.0f;
	InvProjection.Data[1][1] = TanHalfFov;
	InvProjection.Data[1][2] = 0.0f;
	InvProjection.Data[1][3] = 0.0f;

	InvProjection.Data[2][0] = 0.0f;
	InvProjection.Data[2][1] = 0.0f;
	InvProjection.Data[2][2] = 0.0f;
	InvProjection.Data[2][3] = -(FarZ - NearZ) / (FarZ * NearZ);

	InvProjection.Data[3][0] = 0.0f;
	InvProjection.Data[3][1] = 0.0f;
	InvProjection.Data[3][2] = 1.0f;
	InvProjection.Data[3][3] = 1.0f / FarZ;

	return InvProjection;
}

/**
 * @brief PlayerCameraManager의 CameraComponent로부터 Legacy UCamera 객체 생성
 * 레거시 RenderPass 호환성을 위해 CameraComponent 정보를 UCamera로 재조립
 */
void UGameViewportClient::UpdateLegacyCamera()
{
	if (!GWorld)
	{
		return;
	}

	AGameMode* GameMode = GWorld->GetGameMode();
	if (!GameMode)
	{
		return;
	}

	APlayerCameraManager* PlayerCameraManager = GameMode->GetPlayerCameraManager();
	if (!PlayerCameraManager)
	{
		return;
	}

	// Legacy Camera 생성 (최초 1회)
	if (!LegacyCamera)
	{
		LegacyCamera = NewObject<UCamera>();
	}

	// PlayerCameraManager로부터 캐시된 POV 가져오기
	const FMinimalViewInfo& POV = PlayerCameraManager->GetCameraCachePOV();

	// CameraComponent로부터 정보 복사
	LegacyCamera->SetLocation(POV.Location);
	LegacyCamera->SetRotationQuat(POV.Rotation);
	LegacyCamera->SetFovY(POV.FOV);
	LegacyCamera->SetAspect(POV.AspectRatio);
	LegacyCamera->SetNearZ(POV.CameraConstants.NearClip);
	LegacyCamera->SetFarZ(POV.CameraConstants.FarClip);
}
