#pragma once
#include "Editor/Public/Editor.h"

class UGameViewportClient;
class FViewport;
class UWorld;

/**
 * @brief Viewport 단위 Rendering State를 관리하는 View Class
 * ViewportClient로부터 뷰/투영 행렬과 카메라 속성을 받아 렌더링 상태를 관리
 * @note 내부적으로 현재 구색만 맞춰놓고 Renderer의 RenderPass를 활용,
 *
 * @param Viewport 뷰가 렌더링될 대상 뷰포트 (화면 영역)
 * @param World 뷰가 속한 게임 월드 또는 레벨
 * @param ViewMatrix 월드 공간을 뷰 공간으로 변환하는 행렬
 * @param ProjectionMatrix 뷰 공간을 클립 공간으로 변환하는 행렬
 * @param ViewProjectionMatrix ViewMatrix와 ProjectionMatrix를 결합한 행렬 (최종 월드-클립 공간 변환)
 * @param ViewLocation 뷰의 월드 공간 위치
 * @param ViewRotation 뷰의 월드 공간 회전 (쿼터니언)
 * @param FOV 시야각 (Field of View)
 * @param NearClip 근접 클리핑 평면 (Near Clipping Plane)
 * @param FarClip 원거리 클리핑 평면 (Far Clipping Plane)
 * @param AspectRatio 뷰포트의 종횡비 (가로/세로 비율)
 * @param ViewModeIndex 렌더링할 뷰 모드 (예: Lit, Unlit, Wireframe 등)
 * @param ViewType 뷰 타입 (Perspective, OrthoTop, OrthoFront 등)
 * @param ViewportSize 뷰포트의 픽셀 크기 (FVector2D)
 * @param ViewRect 뷰가 렌더링될 실제 사각형 영역
 */
class FSceneView
{
public:
	FSceneView();
	~FSceneView();

	void Initialize(FViewport* InViewport, UWorld* InWorld);
	void InitializeWithMatrices(
		const FMatrix& InViewMatrix,
		const FMatrix& InProjectionMatrix,
		const FVector& InViewLocation,
		const FVector& InViewRotation,
		FViewport* InViewport,
		UWorld* InWorld,
		EViewModeIndex InViewMode,
		float InFOV = 90.0f,
		float InNearClip = 0.1f,
		float InFarClip = 4000.0f
	);

	// Getter & Setter
	const FMatrix& GetViewMatrix() const { return ViewMatrix; }
	const FMatrix& GetProjectionMatrix() const { return ProjectionMatrix; }
	const FMatrix& GetViewProjectionMatrix() const { return ViewProjectionMatrix; }

	FVector GetViewLocation() const { return ViewLocation; }
	FQuaternion GetViewRotation() const { return ViewRotation; }

	TObjectPtr<UWorld> GetWorld() const { return World; }

	// GameViewportClient
	void SetViewport(FViewport* InViewport) { Viewport = InViewport; }
	FViewport* GetViewport() const { return Viewport; }

	EViewModeIndex GetViewModeIndex() const { return ViewModeIndex; }
	void SetViewModeIndex(EViewModeIndex InViewMode) { ViewModeIndex = InViewMode; }

	EViewType GetViewType() const { return ViewType; }
	void SetViewType(EViewType InViewType) { ViewType = InViewType; }

	// Viewport Info
	FVector2 GetViewportSize() const { return ViewportSize; }
	FRect GetViewRect() const { return ViewRect; }

	// Frustum
	float GetFOV() const { return FOV; }
	float GetNearClippingPlane() const { return NearClip; }
	float GetFarClippingPlane() const { return FarClip; }

private:
	TObjectPtr<UWorld> World = nullptr;
	FViewport* Viewport = nullptr; // StandAlone 전용

	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;
	FMatrix ViewProjectionMatrix;

	FVector ViewLocation;
	FQuaternion ViewRotation;

	float FOV = 90.0f;
	float NearClip = 0.1f;  // 카메라 컴포넌트와 일치
	float FarClip = 4000.0f; // 카메라 컴포넌트와 일치
	float AspectRatio = 1.777f;

	EViewModeIndex ViewModeIndex = EViewModeIndex::VMI_Gouraud;
	EViewType ViewType = EViewType::Perspective;
	FVector2 ViewportSize;
	FRect ViewRect;
};
