#include "pch.h"
#include "Render/Renderer/Public/SceneView.h"

#include "Render/UI/Viewport/Public/GameViewportClient.h"
#include "Render/UI/Viewport/Public/Viewport.h"

/**
 * @brief FSceneView 클래스의 생성자로 모든 변환 행렬을 단위 행렬로 초기화
 */
FSceneView::FSceneView()
{
	ViewMatrix = FMatrix::Identity();
	ProjectionMatrix = FMatrix::Identity();
	ViewProjectionMatrix = FMatrix::Identity();
}

FSceneView::~FSceneView() = default;

/**
 * @brief FSceneView 객체를 초기화하고 뷰포트 정보를 설정하는 함수
 * 뷰포트 크기와 종횡비를 계산
 * 행렬은 InitializeWithMatrices()를 통해 별도로 설정해야 함
 * @param InViewport 뷰가 렌더링될 대상 FViewport 포인터
 * @param InWorld 뷰가 속한 UWorld 포인터
 */
void FSceneView::Initialize(FViewport* InViewport, UWorld* InWorld)
{
	World = InWorld;

	if (InViewport)
	{
		const FRect& ViewportRect = InViewport->GetRect();

		ViewportSize = FVector2(
			static_cast<float>(ViewportRect.Width),
			static_cast<float>(ViewportRect.Height)
		);

		ViewRect = ViewportRect;

		// Aspect Ratio 계산
		if (ViewportSize.Y > 0)
		{
			AspectRatio = ViewportSize.X / ViewportSize.Y;
		}
	}
}

/**
 * @brief 행렬을 직접 받아 FSceneView를 초기화하는 함수
 * ViewportClient가 직접 행렬과 카메라 속성을 제공
 * @param InViewMatrix 뷰 행렬 (월드 공간 -> 뷰 공간)
 * @param InProjectionMatrix 투영 행렬 (뷰 공간 -> 클립 공간)
 * @param InViewLocation 뷰의 월드 공간 위치
 * @param InViewRotation 뷰의 오일러 회전각
 * @param InViewport 렌더링될 뷰포트
 * @param InWorld 뷰가 속한 월드
 * @param InViewMode 렌더링 모드 (Lit, Unlit, Wireframe 등)
 * @param InFOV 시야각 (Field of View, 기본값 90.0)
 * @param InNearClip 근접 클리핑 평면 (기본값 0.1)
 * @param InFarClip 원거리 클리핑 평면 (기본값 4000.0)
 */
void FSceneView::InitializeWithMatrices(
	const FMatrix& InViewMatrix,
	const FMatrix& InProjectionMatrix,
	const FVector& InViewLocation,
	const FVector& InViewRotation,
	FViewport* InViewport,
	UWorld* InWorld,
	EViewModeIndex InViewMode,
	float InFOV,
	float InNearClip,
	float InFarClip
)
{
	Viewport = InViewport;
	World = InWorld;
	ViewModeIndex = InViewMode;

	// 행렬 설정
	ViewMatrix = InViewMatrix;
	ProjectionMatrix = InProjectionMatrix;
	ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;

	// 뷰 위치/회전 설정
	ViewLocation = InViewLocation;
	ViewRotation = FQuaternion::FromEuler(InViewRotation);

	// FOV 및 클리핑 평면 설정
	FOV = InFOV;
	NearClip = InNearClip;
	FarClip = InFarClip;

	// 뷰포트 정보 설정
	if (Viewport)
	{
		const FRect ViewportRect = Viewport->GetRect();

		ViewportSize = FVector2(
			static_cast<float>(ViewportRect.Width),
			static_cast<float>(ViewportRect.Height)
		);

		ViewRect = ViewportRect;

		// Aspect Ratio 계산
		if (ViewportSize.Y > 0)
		{
			AspectRatio = ViewportSize.X / ViewportSize.Y;
		}
	}
}
