#pragma once

class FViewport;

/**
 * @brief Game/Runtime용 Viewport Client
 * StandAlone 모드에서 단일 fullscreen viewport 관리
 * 카메라를 소유하지 않고 자체 Transform만 보유 (UE 방식)
 * 실제 카메라는 PlayerController나 GameMode에서 관리
 */
UCLASS()
class UGameViewportClient : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UGameViewportClient, UObject)

public:
	UGameViewportClient();
	~UGameViewportClient() override = default;

	// Viewport 소유권 관리
	void SetOwningViewport(FViewport* InViewport) { OwningViewport = InViewport; }
	FViewport* GetOwningViewport() const { return OwningViewport; }

	// Transform 접근 (카메라 대신 자체 Transform 사용)
	void SetViewLocation(const FVector& InLocation) { ViewLocation = InLocation; }
	const FVector& GetViewLocation() const { return ViewLocation; }

	void SetViewRotation(const FVector& InRotation) { ViewRotation = InRotation; }
	const FVector& GetViewRotation() const { return ViewRotation; }

	// Camera Parameters
	void SetFOV(float InFOV) { FOV = InFOV; }
	float GetFOV() const { return FOV; }

	void SetNearZ(float InNearZ) { NearZ = InNearZ; }
	float GetNearZ() const { return NearZ; }

	void SetFarZ(float InFarZ) { FarZ = InFarZ; }
	float GetFarZ() const { return FarZ; }

	// View/Projection 행렬 계산
	FMatrix GetViewMatrix() const;
	FMatrix GetProjectionMatrix() const;
	FMatrix GetProjectionMatrixInverse() const;

	// Viewport 크기 관리
	void SetViewportSize(const FPoint& InSize) { ViewSize = InSize; }
	const FPoint& GetViewportSize() const { return ViewSize; }

	// Viewport Rect 반환 (SceneView 호환성)
	FRect GetRect() const { return FRect(0, 0, ViewSize.X, ViewSize.Y); }

	// Legacy Camera (RenderPass 호환성을 위한 임시 객체)
	void UpdateLegacyCamera();
	class UCamera* GetLegacyCamera() const { return LegacyCamera; }

	// 업데이트
	void Tick();
	void Draw(FViewport* InViewport) const;

	// 입력 처리 (Game용 - 간단한 처리)
	static void MouseMove(FViewport* Viewport, int32 X, int32 Y) {}
	void CapturedMouseMove(FViewport* Viewport, int32 X, int32 Y)
	{
		LastDrag = { X, Y };
	}

	// 리사이즈/포커스
	void OnResize(const FPoint& InNewSize) { ViewSize = InNewSize; }
	static void OnGainFocus() {}
	static void OnLoseFocus() {}
	static void OnClose() {}

private:
	// Viewport 참조
	FViewport* OwningViewport = nullptr;

	// View Transform (카메라를 소유하지 않고 Transform만 관리)
	FVector ViewLocation = FVector(-15.0f, 0.f, 10.0f);
	FVector ViewRotation = FVector(0, 0, 0);
	float FOV = 60.0f;
	float NearZ = 0.1f;
	float FarZ = 4000.0f;

	// Legacy Camera (레거시 RenderPass 지원용 - CameraComponent로부터 재조립)
	class UCamera* LegacyCamera = nullptr;

	// 뷰/입력 상태
	FPoint ViewSize{ 0, 0 };
	FPoint LastDrag{ 0, 0 };
};
