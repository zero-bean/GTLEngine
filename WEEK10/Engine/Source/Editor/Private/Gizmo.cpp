#include "pch.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/GizmoHelper.h"

#include "Editor/Public/Editor.h"
#include "Editor/Public/GizmoMath.h"
#include "Editor/Public/ObjectPicker.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/UI/Overlay/Public/D2DOverlayManager.h"

constexpr float MIN_SCALE_VALUE = 0.001f;

IMPLEMENT_CLASS(UGizmo, UObject)

void UGizmo::SetGizmoMode(EGizmoMode Mode)
{
	GizmoMode = Mode;
	// ConfigManager에 동기화 (소멸 시점 문제 방지)
	UConfigManager::GetInstance().SetCachedGizmoMode(Mode);
}

UGizmo::UGizmo()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Primitives.SetNum(3);
	GizmoColor.SetNum(3);

	/* *
	* @brief 0: Forward(x), 1: Right(y), 2: Up(z)
	*/
	GizmoColor[0] = FVector4(1, 0, 0, 1);
	GizmoColor[1] = FVector4(0, 1, 0, 1);
	GizmoColor[2] = FVector4(0, 0, 1, 1);

	/**
	 * @brief Translation Setting
	 */
	const float ScaleT = TranslateCollisionConfig.Scale;
	Primitives[0].VertexBuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Arrow);
	Primitives[0].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Arrow);
	Primitives[0].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[0].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[0].bShouldAlwaysVisible = true;

	/**
	 * @brief Rotation Setting
	 */
	Primitives[1].VertexBuffer = nullptr;  // 렌더링 시 동적으로 설정
	Primitives[1].NumVertices = 0;
	Primitives[1].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[1].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[1].bShouldAlwaysVisible = true;

	/**
	 * @brief Scale Setting
	 */
	Primitives[2].VertexBuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::CubeArrow);
	Primitives[2].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::CubeArrow);
	Primitives[2].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[2].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[2].bShouldAlwaysVisible = true;

	/* *
	* @brief Render State
	*/
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;
}

UGizmo::~UGizmo() = default;

void UGizmo::UpdateScale(const UCamera* InCamera, const D3D11_VIEWPORT& InViewport)
{
	if (!InCamera)
	{
		return;
	}

	// 기즈모 위치 가져오기 (고정 위치 또는 TargetComponent 위치)
	FVector GizmoLocation;
	if (bUseFixedLocation)
	{
		// 본 선택 시: 고정 위치 사용
		GizmoLocation = FixedLocation;
	}
	else
	{
		// TargetComponent가 이미 설정되어 있지 않으면 GEditor에서 가져오기
		// (뷰어 같은 곳에서는 SetSelectedComponent로 직접 설정됨)
		if (!TargetComponent)
		{
			TargetComponent = Cast<USceneComponent>(GEditor->GetEditorModule()->GetSelectedComponent());
		}

		if (TargetComponent)
		{
			// 컴포넌트 선택 시: TargetComponent 위치 사용
			GizmoLocation = TargetComponent->GetWorldLocation();
		}
		else
		{
			// 아무것도 선택되지 않음
			return;
		}
	}

	// 스크린에서 균일한 사이즈를 가지도록 하기 위한 스케일 조정
	const float Scale = FGizmoMath::CalculateScreenSpaceScale(InCamera, InViewport, GizmoLocation, 120.0f);

	TranslateCollisionConfig.Scale = Scale;
	RotateCollisionConfig.Scale = Scale;
}

void UGizmo::ChangeGizmoMode()
{
	switch (GizmoMode)
	{
	case EGizmoMode::Translate:
		GizmoMode = EGizmoMode::Rotate; break;
	case EGizmoMode::Rotate:
		GizmoMode = EGizmoMode::Scale; break;
	case EGizmoMode::Scale:
		GizmoMode = EGizmoMode::Translate;
	}
}

void UGizmo::SetLocation(const FVector& Location) const
{
	TargetComponent->SetWorldLocation(Location);
}

bool UGizmo::IsInRadius(float Radius) const
{
	if (Radius >= RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale && Radius <= RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale)
		return true;
	return false;
}

void UGizmo::OnMouseDragStart(const FVector& CollisionPoint)
{
	bIsDragging = true;
	DragStartMouseLocation = CollisionPoint;
	PreviousMouseLocation = CollisionPoint;  // 누적 각도 계산을 위한 초기화

	// 스크린 공간 드래그 초기화 (뷰포트 로컬 좌표)
	POINT MousePos;
	GetCursorPos(&MousePos);
	ScreenToClient(GetActiveWindow(), &MousePos);

	const FRect& ViewportRect = UViewportManager::GetInstance().GetActiveViewportRect();
	const FVector2 CurrentScreenPos(
		static_cast<float>(MousePos.x) - static_cast<float>(ViewportRect.Left),
		static_cast<float>(MousePos.y) - static_cast<float>(ViewportRect.Top)
	);
	PreviousScreenPos = CurrentScreenPos;
	DragStartScreenPos = CurrentScreenPos;

	// 드래그 시작 방향 계산 (Arc 렌더링의 시작점으로 사용)
	FVector GizmoCenter = Primitives[static_cast<int>(GizmoMode)].Location;
	DragStartDirection = (CollisionPoint - GizmoCenter).GetNormalized();

	if (TargetComponent)
	{
		DragStartActorLocation = TargetComponent->GetWorldLocation();
		DragStartActorRotation = TargetComponent->GetWorldRotation();
		DragStartActorRotationQuat = TargetComponent->GetWorldRotationAsQuaternion();
		DragStartActorScale = TargetComponent->GetWorldScale3D();
	}
}

// 하이라이트 색상은 렌더 시점에만 계산 (상태 오염 방지)
FVector4 UGizmo::ColorFor(EGizmoDirection InAxis) const
{
	// Scale 모드에서만 Center 선택 시 모든 축 및 평면 하이라이팅
	if (GizmoMode == EGizmoMode::Scale && GizmoDirection == EGizmoDirection::Center)
	{
		if (InAxis == EGizmoDirection::Forward || InAxis == EGizmoDirection::Right || InAxis == EGizmoDirection::Up ||
		    InAxis == EGizmoDirection::XY_Plane || InAxis == EGizmoDirection::XZ_Plane || InAxis == EGizmoDirection::YZ_Plane)
		{
			if (bIsDragging)
			{
				return {0.8f, 0.8f, 0.0f, 1.0f};  // 드래그 중: 짙은 노란색
			}
			else
			{
				return {1.0f, 1.0f, 0.0f, 1.0f};  // 호버 중: 밝은 노란색
			}
		}
	}

	return FGizmoMath::CalculateColor(InAxis, GizmoDirection, bIsDragging, GizmoColor);
}

void UGizmo::CollectRotationAngleOverlay(FD2DOverlayManager& OverlayManager, UCamera* InCamera, const D3D11_VIEWPORT& InViewport, bool bUseViewportManager, bool bCustomSnapEnabled, float CustomSnapAngleDegrees)
{
	if (!bIsDragging || GizmoMode != EGizmoMode::Rotate || !TargetComponent)
	{
		return;
	}

	const FVector GizmoLocation = GetGizmoLocation();
	const FVector LocalGizmoAxis = GetGizmoAxis();
	const FQuaternion StartRotQuat = GetDragStartActorRotationQuat();

	// 월드 공간 회전축
	FVector WorldRotationAxis = LocalGizmoAxis;
	if (!IsWorldMode())
	{
		WorldRotationAxis = StartRotQuat.RotateVector(LocalGizmoAxis);
	}

	// 현재 회전 각도 계산 (스냅 적용)
	float DisplayAngleRadians = GetCurrentRotationAngle();

	// 스냅 설정 결정: ViewportManager 사용 또는 커스텀 설정 사용
	bool bSnapEnabled = false;
	float SnapAngleDegrees = 0.0f;

	if (bUseViewportManager)
	{
		// 메인: ViewportManager 설정 사용
		bSnapEnabled = UViewportManager::GetInstance().IsRotationSnapEnabled();
		SnapAngleDegrees = UViewportManager::GetInstance().GetRotationSnapAngle();
	}
	else
	{
		// 뷰어: 커스텀 스냅 설정 사용
		bSnapEnabled = bCustomSnapEnabled;
		SnapAngleDegrees = CustomSnapAngleDegrees;
	}

	if (bSnapEnabled && SnapAngleDegrees > 0.0f)
	{
		const float SnapAngleRadians = FVector::GetDegreeToRadian(SnapAngleDegrees);
		DisplayAngleRadians = std::round(GetCurrentRotationAngle() / SnapAngleRadians) * SnapAngleRadians;
	}

	// RotateVector 부호 보정
	DisplayAngleRadians = -DisplayAngleRadians;

	// 각 축마다 다른 BaseAxis 사용
	FVector BaseAxis0, BaseAxis1;
	switch (GizmoDirection)
	{
	case EGizmoDirection::Forward:  // X축: Z→Y
		BaseAxis0 = FVector(0, 0, 1);
		BaseAxis1 = FVector(0, 1, 0);
		break;
	case EGizmoDirection::Right:    // Y축: X→Z
		BaseAxis0 = FVector(1, 0, 0);
		BaseAxis1 = FVector(0, 0, 1);
		break;
	case EGizmoDirection::Up:       // Z축: X→Y
		BaseAxis0 = FVector(1, 0, 0);
		BaseAxis1 = FVector(0, 1, 0);
		break;
	default:
		return;
	}

	// Local 모드면 컴포넌트 회전만 적용
	FQuaternion TotalRotation = FQuaternion::Identity();
	if (!IsWorldMode())
	{
		TotalRotation = StartRotQuat;
	}

	// Z축은 각도 반전 (언리얼 표준)
	float PositionAngleRadians = DisplayAngleRadians;
	if (GizmoDirection == EGizmoDirection::Up)
	{
		PositionAngleRadians = -DisplayAngleRadians;
	}

	// 평면에서 각도 위치의 방향 계산
	const FVector LocalDirection = BaseAxis0 * cosf(PositionAngleRadians) + BaseAxis1 * sinf(PositionAngleRadians);

	// 월드 공간으로 변환
	const FVector AngleDirection = TotalRotation.RotateVector(LocalDirection);

	// 회전 반지름 (기즈모 스케일 적용)
	const float RotateRadius = GetRotateOuterRadius();

	// 원 위의 점 (월드 공간)
	const FVector PointOnCircle = GizmoLocation + AngleDirection * RotateRadius;

	// 스크린 공간으로 투영
	const FCameraConstants& CamConst = InCamera->GetFViewProjConstants();
	const FMatrix ViewProj = CamConst.View * CamConst.Projection;

	FVector4 GizmoScreenPos4 = FVector4(GizmoLocation, 1.0f) * ViewProj;
	FVector4 PointScreenPos4 = FVector4(PointOnCircle, 1.0f) * ViewProj;

	if (GizmoScreenPos4.W <= 0.0f || PointScreenPos4.W <= 0.0f)
	{
		return;
	}

	GizmoScreenPos4 *= 1.0f / GizmoScreenPos4.W;
	PointScreenPos4 *= 1.0f / PointScreenPos4.W;

	// NDC -> 스크린 좌표
	const FVector2 GizmoScreenPos(
		   (GizmoScreenPos4.X * 0.5f + 0.5f) * InViewport.Width,
		   ((-GizmoScreenPos4.Y) * 0.5f + 0.5f) * InViewport.Height
		);

	const FVector2 PointScreenPos(
	   (PointScreenPos4.X * 0.5f + 0.5f) * InViewport.Width,
	   ((-PointScreenPos4.Y) * 0.5f + 0.5f) * InViewport.Height
	);

	// 기즈모 중심에서 점으로 향하는 방향
	FVector2 DirectionToPoint = PointScreenPos - GizmoScreenPos;
	const float DistToPoint = DirectionToPoint.Length();
	if (DistToPoint < 0.01f)
	{
		return;
	}
	DirectionToPoint = DirectionToPoint.GetNormalized();

	// 텍스트 위치 (뷰포트 크기 비례 원 바깥으로 추가 오프셋)
	// 스크린 공간 원 반지름에 비례하는 오프셋 계산
	constexpr float ScreenRadiusRatio = 0.3f;
	const float TextOffset = DistToPoint * ScreenRadiusRatio;
	const float TextX = PointScreenPos.X + DirectionToPoint.X * TextOffset;
	const float TextY = PointScreenPos.Y + DirectionToPoint.Y * TextOffset;

	// 텍스트 포맷 (UI와 일치하도록 각도 표시)
	float DisplayAngleDegrees = FVector::GetRadianToDegree(DisplayAngleRadians);

	// Z축만 각도 부호 반전
	if (GizmoDirection == EGizmoDirection::Up)
	{
		DisplayAngleDegrees = -DisplayAngleDegrees;
	}

	wchar_t AngleText[32];
	(void)swprintf_s(AngleText, L"%.1f°", DisplayAngleDegrees);

	// 텍스트 박스 (중앙 정렬)
	constexpr float TextBoxWidth = 60.0f;
	constexpr float TextBoxHeight = 20.0f;
	D2D1_RECT_F TextRect = D2D1::RectF(
		TextX - TextBoxWidth * 0.5f,
		TextY - TextBoxHeight * 0.5f,
		TextX + TextBoxWidth * 0.5f,
		TextY + TextBoxHeight * 0.5f
	);

	// 반투명 회색 배경
	const D2D1_COLOR_F ColorGrayBG = D2D1::ColorF(0.2f, 0.2f, 0.2f, 0.7f);
	OverlayManager.AddRectangle(TextRect, ColorGrayBG, true);

	// 노란색 텍스트
	const D2D1_COLOR_F ColorYellow = D2D1::ColorF(1.0f, 1.0f, 0.0f);
	OverlayManager.AddText(AngleText, TextRect, ColorYellow, 15.0f, true, true, L"Consolas");
}

/**
 * @brief Translation 드래그 처리
 */
FVector UGizmo::ProcessDragLocation(UCamera* InCamera, FRay& WorldRay, UObjectPicker* InObjectPicker, bool bUseCustomSnap)
{
	if (!InCamera || !InObjectPicker)
	{
		return GetGizmoLocation();
	}

	FVector MouseWorld;
	FVector PlaneOrigin{ GetGizmoLocation() };

	// Snap 설정 결정: 커스텀 또는 ViewportManager
	bool bSnapEnabled = false;
	float SnapValue = 10.0f;
	if (bUseCustomSnap)
	{
		bSnapEnabled = bCustomLocationSnapEnabled;
		SnapValue = CustomLocationSnapValue;
	}
	else
	{
		bSnapEnabled = UViewportManager::GetInstance().IsLocationSnapEnabled();
		SnapValue = UViewportManager::GetInstance().GetLocationSnapValue();
	}

	// Center 구체 드래그 처리
	// UE 기준 카메라 NDC 평면에 평행하게 이동
	if (GetGizmoDirection() == EGizmoDirection::Center)
	{
		// 카메라의 Forward 방향을 평면 법선로 사용
		FVector PlaneNormal = InCamera->GetForward();

		// 드래그 시작 지점의 마우스 위치를 평면 원점으로 사용
		FVector FixedPlaneOrigin = GetDragStartMouseLocation();

		// 레이와 카메라 평면 교차점 계산
		if (InObjectPicker->IsRayCollideWithPlane(WorldRay, FixedPlaneOrigin, PlaneNormal, MouseWorld))
		{
			// 드래그 시작점으로부터 이동 거리 계산
			FVector MouseDelta = MouseWorld - GetDragStartMouseLocation();
			FVector NewLocation = GetDragStartActorLocation() + MouseDelta;

			// Location Snap 적용
			if (bSnapEnabled && SnapValue > 0.0001f)
			{
				NewLocation.X = std::round(NewLocation.X / SnapValue) * SnapValue;
				NewLocation.Y = std::round(NewLocation.Y / SnapValue) * SnapValue;
				NewLocation.Z = std::round(NewLocation.Z / SnapValue) * SnapValue;
			}

			return NewLocation;
		}
		return GetGizmoLocation();
	}

	// 평면 드래그 처리
	if (IsPlaneDirection())
	{
		// 평면 법선 벡터
		FVector PlaneNormal = GetPlaneNormal();

		if (!IsWorldMode())
		{
			FQuaternion q;
			// 본 선택 시: DragStartActorRotationQuat 사용
			// 일반 컴포넌트 선택 시: TargetComponent 회전 사용
			if (bUseFixedLocation)
			{
				q = GetDragStartActorRotationQuat();
			}
			else if (TargetComponent)
			{
				q = TargetComponent->GetWorldRotationAsQuaternion();
			}
			else
			{
				q = GetDragStartActorRotationQuat();
			}
			PlaneNormal = q.RotateVector(PlaneNormal);
		}

		// 레이와 평면 교차점 계산
		if (InObjectPicker->IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
		{
			// 드래그 시작점으로부터 이동 거리 계산
			FVector MouseDelta = MouseWorld - GetDragStartMouseLocation();
			FVector NewLocation = GetDragStartActorLocation() + MouseDelta;

			// Location Snap 적용
			if (bSnapEnabled && SnapValue > 0.0001f)
			{
				NewLocation.X = std::round(NewLocation.X / SnapValue) * SnapValue;
				NewLocation.Y = std::round(NewLocation.Y / SnapValue) * SnapValue;
				NewLocation.Z = std::round(NewLocation.Z / SnapValue) * SnapValue;
			}

			return NewLocation;
		}
		return GetGizmoLocation();
	}

	// 축 드래그 처리
	FVector GizmoAxis = GetGizmoAxis();

	if (!IsWorldMode())
	{
		FQuaternion q;
		// 본 선택 시: DragStartActorRotationQuat 사용
		// 일반 컴포넌트 선택 시: TargetComponent 회전 사용
		if (bUseFixedLocation)
		{
			q = GetDragStartActorRotationQuat();
		}
		else if (TargetComponent)
		{
			q = TargetComponent->GetWorldRotationAsQuaternion();
		}
		else
		{
			q = GetDragStartActorRotationQuat();
		}
		GizmoAxis = q.RotateVector(GizmoAxis);
	}

	// 카메라 방향 벡터를 사용하여 안정적인 평면 계산
	const FVector CamRight = InCamera->GetRight();
	const FVector CamUp = InCamera->GetUp();

	// GizmoAxis에 가장 수직인 카메라 벡터 선택
	FVector PlaneVector;
	const float DotRight = abs(GizmoAxis.Dot(CamRight));
	const float DotUp = abs(GizmoAxis.Dot(CamUp));

	if (DotRight < DotUp)
	{
		PlaneVector = CamRight;
	}
	else
	{
		PlaneVector = CamUp;
	}

	FVector PlaneNormal = GizmoAxis.Cross(PlaneVector);
	PlaneNormal.Normalize();

	// PlaneNormal이 카메라를 향하도록 보장 (드래그 방향 일관성)
	const FVector CameraLocation = InCamera->GetLocation();
	const FVector ToCamera = (CameraLocation - PlaneOrigin).GetNormalized();
	if (PlaneNormal.Dot(ToCamera) < 0.0f)
	{
		PlaneNormal = -PlaneNormal;
	}

	if (InObjectPicker->IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		FVector MouseDistance = MouseWorld - GetDragStartMouseLocation();
		FVector NewLocation = GetDragStartActorLocation() + GizmoAxis * MouseDistance.Dot(GizmoAxis);

		// Location Snap 적용
		if (bSnapEnabled && SnapValue > 0.0001f)
		{
			NewLocation.X = std::round(NewLocation.X / SnapValue) * SnapValue;
			NewLocation.Y = std::round(NewLocation.Y / SnapValue) * SnapValue;
			NewLocation.Z = std::round(NewLocation.Z / SnapValue) * SnapValue;
		}

		return NewLocation;
	}
	return GetGizmoLocation();
}

/**
 * @brief Rotation 드래그 처리
 */
FQuaternion UGizmo::ProcessDragRotation(UCamera* InCamera, FRay& WorldRay, const FRect& ViewportRect, bool bUseCustomSnap)
{
	if (!InCamera)
	{
		return GetComponentRotation();
	}

	const FVector GizmoLocation = GetGizmoLocation();
	const FVector LocalGizmoAxis = GetGizmoAxis();
	const FQuaternion StartRotQuat = GetDragStartActorRotationQuat();

	// 월드 공간 회전축
	FVector WorldRotationAxis = LocalGizmoAxis;
	if (!IsWorldMode())
	{
		WorldRotationAxis = StartRotQuat.RotateVector(LocalGizmoAxis);
	}

	// 스크린 공간 회전 계산
	const float ViewportWidth = static_cast<float>(ViewportRect.Width);
	const float ViewportHeight = static_cast<float>(ViewportRect.Height);
	const float ViewportLeft = static_cast<float>(ViewportRect.Left);
	const float ViewportTop = static_cast<float>(ViewportRect.Top);

	// 기즈모 중심을 스크린 공간으로 투영
	const FCameraConstants& CamConst = InCamera->GetFViewProjConstants();
	const FMatrix ViewProj = CamConst.View * CamConst.Projection;
	FVector4 GizmoScreenPos4 = FVector4(GizmoLocation, 1.0f) * ViewProj;

	if (GizmoScreenPos4.W > 0.0f)
	{
		// NDC로 변환
		GizmoScreenPos4 *= (1.0f / GizmoScreenPos4.W);

		// NDC → 뷰포트 로컬 좌표
		const FVector2 GizmoScreenPos(
			(GizmoScreenPos4.X * 0.5f + 0.5f) * ViewportWidth,
			((-GizmoScreenPos4.Y) * 0.5f + 0.5f) * ViewportHeight
		);

		// 현재 마우스 스크린 좌표
		POINT MousePos;
		GetCursorPos(&MousePos);
		ScreenToClient(GetActiveWindow(), &MousePos);

		const FVector2 CurrentScreenPos(
			static_cast<float>(MousePos.x) - ViewportLeft,
			static_cast<float>(MousePos.y) - ViewportTop
		);

		// UE5 표준: 드래그 시작 지점에서 기즈모로의 방향 (Origin = DragStartPos in UE)
		const FVector2 DragStartScreenPos = GetDragStartScreenPos();
		const FVector2 DirectionToMousePos = (DragStartScreenPos - GizmoScreenPos).GetNormalized();

		// Tangent 방향: DirectionToMousePos에 수직 (시계방향 회전)
		FVector2 TangentDir = FVector2(-DirectionToMousePos.Y, DirectionToMousePos.X);

		// 모든 축에 대해 동일한 부호 보정 적용
		if (GetGizmoDirection() == EGizmoDirection::Right ||
			GetGizmoDirection() == EGizmoDirection::Forward ||
			GetGizmoDirection() == EGizmoDirection::Up)
		{
			TangentDir = -TangentDir;
		}

		// 스크린 공간 드래그 벡터
		const FVector2 PrevScreenPos = GetPreviousScreenPos();
		const FVector2 DragDelta = CurrentScreenPos - PrevScreenPos;
		const FVector2 DragDir = FVector2(DragDelta.X, DragDelta.Y);

		// 마우스가 실제로 움직였는지 체크
		const float DragDistSq = DragDir.LengthSquared();
		constexpr float MinDragDistSq = 0.1f * 0.1f;

		if (DragDistSq > MinDragDistSq)
		{
			// UE5 표준: Dot Product로 회전 각도 계산
			float PixelDelta = TangentDir.Dot(DragDir);

			// 픽셀을 각도(라디안)로 변환 (언리얼에선 1픽셀 = 1도 사용, 어차피 감도 조절용 매직 넘버라 그대로 차용)
			constexpr float PixelsToDegrees = 1.0f;
			float DeltaAngleDegrees = PixelDelta * PixelsToDegrees;
			float DeltaAngle = FVector::GetDegreeToRadian(DeltaAngleDegrees);

			// 카메라 시점 방향에 따른 회전 방향 보정
			// 카메라가 회전축의 반대편에 있으면 부호 반전
			const FVector CamToGizmo = (GizmoLocation - InCamera->GetLocation()).GetNormalized();
			const float AxisDotCam = WorldRotationAxis.Dot(CamToGizmo);
			if (AxisDotCam < 0.0f)
			{
				DeltaAngle = -DeltaAngle;
			}
			// 누적 각도 업데이트
			float NewAngle = GetCurrentRotationAngle() + DeltaAngle;

			// 360deg Clamp
			constexpr float TwoPi = 2.0f * PI;
			if (NewAngle > TwoPi)
			{
				NewAngle = fmodf(NewAngle, TwoPi);
			}
			else if (NewAngle < -TwoPi)
			{
				NewAngle = fmodf(NewAngle, -TwoPi);
			}

			SetCurrentRotationAngle(NewAngle);
		}

		// 현재 스크린 좌표 저장
		SetPreviousScreenPos(CurrentScreenPos);

		// 최종 회전 Quaternion 계산 (스냅 적용)
		float FinalAngle = GetCurrentRotationAngle();

		// Snap 설정 결정: 커스텀 또는 ViewportManager
		bool bSnapEnabled = false;
		float SnapAngleDegrees = 0.0f;
		if (bUseCustomSnap)
		{
			bSnapEnabled = bCustomRotationSnapEnabled;
			SnapAngleDegrees = CustomRotationSnapAngle;
		}
		else
		{
			bSnapEnabled = UViewportManager::GetInstance().IsRotationSnapEnabled();
			SnapAngleDegrees = UViewportManager::GetInstance().GetRotationSnapAngle();
		}

		if (bSnapEnabled)
		{
			const float SnapAngleRadians = FVector::GetDegreeToRadian(SnapAngleDegrees);
			FinalAngle = std::round(GetCurrentRotationAngle() / SnapAngleRadians) * SnapAngleRadians;
		}

		// 기즈모 드래그 방향과 회전 방향을 일치시키기 위해 부호 반전
		const FQuaternion DeltaRotQuat = FQuaternion::FromAxisAngle(LocalGizmoAxis, FinalAngle);
		if (IsWorldMode())
		{
			return DeltaRotQuat * StartRotQuat;
		}
		else
		{
			return StartRotQuat * DeltaRotQuat;
		}
	}

	return GetComponentRotation();
}

/**
 * @brief Scale 드래그 처리
 */
FVector UGizmo::ProcessDragScale(UCamera* InCamera, FRay& WorldRay, UObjectPicker* InObjectPicker)
{
	if (!InCamera || !InObjectPicker)
	{
		return GetComponentScale();
	}

	FVector MouseWorld;
	FVector PlaneOrigin = GetGizmoLocation();
	// 본 선택 시에도 올바른 회전을 사용하기 위해 DragStartActorRotationQuat 사용
	FQuaternion Quat = GetDragStartActorRotationQuat();
	const FVector CameraLocation = InCamera->GetLocation();

	// Center 구체 드래그 처리 (균일 스케일, 모든 축 동일하게)
	if (GetGizmoDirection() == EGizmoDirection::Center)
	{
		// 카메라 Forward 방향의 평면에서 드래그
		FVector PlaneNormal = InCamera->GetForward();

		if (InObjectPicker->IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
		{
			// 드래그 벡터 계산
			const FVector MouseDelta = MouseWorld - GetDragStartMouseLocation();

			// 카메라 Right 방향으로의 드래그 거리 사용 (수평 드래그)
			const FVector CamRight = InCamera->GetRight();
			const float DragDistance = MouseDelta.Dot(CamRight);

			// 스케일 민감도 조정
			const float DistanceToGizmo = (PlaneOrigin - CameraLocation).Length();
			constexpr float BaseSensitivity = 0.01f;  // 민감도 감소
			const float ScaleSensitivity = BaseSensitivity * DistanceToGizmo;
			const float ScaleDelta = DragDistance * ScaleSensitivity;

			// 모든 축에 동일한 스케일 적용 (균일 스케일)
			const FVector DragStartScale = GetDragStartActorScale();
			const float UniformScale = max(1.0f + ScaleDelta / DragStartScale.X, MIN_SCALE_VALUE);

			FVector NewScale;
			NewScale.X = max(DragStartScale.X * UniformScale, MIN_SCALE_VALUE);
			NewScale.Y = max(DragStartScale.Y * UniformScale, MIN_SCALE_VALUE);
			NewScale.Z = max(DragStartScale.Z * UniformScale, MIN_SCALE_VALUE);

			// 스냅 설정 가져오기 (Center는 모든 축에 동일하게 스냅)
			bool bSnapEnabled = false;
			float SnapValue = 1.0f;
			if (bUseCustomScaleSnap)
			{
				bSnapEnabled = bCustomScaleSnapEnabled;
				SnapValue = CustomScaleSnapValue;
			}
			else
			{
				bSnapEnabled = UViewportManager::GetInstance().IsScaleSnapEnabled();
				SnapValue = UViewportManager::GetInstance().GetScaleSnapValue();
			}

			// 충분히 드래그했을 때만 스냅 적용 (클릭만으로는 스냅 안 됨)
			if (bSnapEnabled && SnapValue > 0.0001f)
			{
				// 변화량 계산
				const float DeltaX = std::abs(NewScale.X - DragStartScale.X);
				const float DeltaY = std::abs(NewScale.Y - DragStartScale.Y);
				const float DeltaZ = std::abs(NewScale.Z - DragStartScale.Z);
				const float MaxDelta = std::max({ DeltaX, DeltaY, DeltaZ });

				// 아주 작은 변화(클릭만)가 아니면 스냅 적용
				if (MaxDelta >= 0.01f)
				{
					NewScale.X = std::round(NewScale.X / SnapValue) * SnapValue;
					NewScale.Y = std::round(NewScale.Y / SnapValue) * SnapValue;
					NewScale.Z = std::round(NewScale.Z / SnapValue) * SnapValue;

					NewScale.X = std::max(NewScale.X, MIN_SCALE_VALUE);
					NewScale.Y = std::max(NewScale.Y, MIN_SCALE_VALUE);
					NewScale.Z = std::max(NewScale.Z, MIN_SCALE_VALUE);
				}
			}

			return NewScale;
		}
		return GetComponentScale();
	}

	// 평면 스케일 처리
	if (IsPlaneDirection())
	{
		// 평면 법선과 접선 벡터
		FVector PlaneNormal = GetPlaneNormal();
		FVector Tangent1, Tangent2;
		GetPlaneTangents(Tangent1, Tangent2);

		// 로컬 좌표로 변환
		PlaneNormal = Quat.RotateVector(PlaneNormal);
		FVector WorldTangent1 = Quat.RotateVector(Tangent1);
		FVector WorldTangent2 = Quat.RotateVector(Tangent2);

		// 레이와 평면 교차점 계산
		if (InObjectPicker->IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
		{
			// 평면 내 드래그 벡터 계산
			const FVector MouseDelta = MouseWorld - GetDragStartMouseLocation();

			// 두 접선 방향 드래그 거리 평균
			const float Drag1 = MouseDelta.Dot(WorldTangent1);
			const float Drag2 = MouseDelta.Dot(WorldTangent2);
			const float AvgDrag = (Drag1 + Drag2) * 0.5f;

			// 스케일 민감도 조정
			const float DistanceToGizmo = (PlaneOrigin - CameraLocation).Length();
			constexpr float BaseSensitivity = 0.01f;  // 민감도 감소
			const float ScaleSensitivity = BaseSensitivity * DistanceToGizmo;
			const float ScaleDelta = AvgDrag * ScaleSensitivity;

			const FVector DragStartScale = GetDragStartActorScale();
			FVector NewScale = DragStartScale;

			// 스냅 설정 가져오기
			bool bSnapEnabled = false;
			float SnapValue = 1.0f;
			if (bUseCustomScaleSnap)
			{
				bSnapEnabled = bCustomScaleSnapEnabled;
				SnapValue = CustomScaleSnapValue;
			}
			else
			{
				bSnapEnabled = UViewportManager::GetInstance().IsScaleSnapEnabled();
				SnapValue = UViewportManager::GetInstance().GetScaleSnapValue();
			}

			// 평면의 두 축에 동일한 스케일 적용
			bool bModifiedX = false, bModifiedY = false, bModifiedZ = false;

			if (abs(Tangent1.X) > 0.5f)
			{
				NewScale.X = max(DragStartScale.X + ScaleDelta, MIN_SCALE_VALUE);
				bModifiedX = true;
			}
			if (abs(Tangent1.Y) > 0.5f)
			{
				NewScale.Y = max(DragStartScale.Y + ScaleDelta, MIN_SCALE_VALUE);
				bModifiedY = true;
			}
			if (abs(Tangent1.Z) > 0.5f)
			{
				NewScale.Z = max(DragStartScale.Z + ScaleDelta, MIN_SCALE_VALUE);
				bModifiedZ = true;
			}
			if (abs(Tangent2.X) > 0.5f)
			{
				NewScale.X = max(DragStartScale.X + ScaleDelta, MIN_SCALE_VALUE);
				bModifiedX = true;
			}
			if (abs(Tangent2.Y) > 0.5f)
			{
				NewScale.Y = max(DragStartScale.Y + ScaleDelta, MIN_SCALE_VALUE);
				bModifiedY = true;
			}
			if (abs(Tangent2.Z) > 0.5f)
			{
				NewScale.Z = max(DragStartScale.Z + ScaleDelta, MIN_SCALE_VALUE);
				bModifiedZ = true;
			}

			// 변경된 축만 스냅 적용 (충분히 드래그했을 때만)
			if (bSnapEnabled && SnapValue > 0.0001f)
			{
				if (bModifiedX)
				{
					const float DeltaX = std::abs(NewScale.X - DragStartScale.X);
					if (DeltaX >= 0.01f)
					{
						NewScale.X = std::round(NewScale.X / SnapValue) * SnapValue;
						NewScale.X = std::max(NewScale.X, MIN_SCALE_VALUE);
					}
				}
				if (bModifiedY)
				{
					const float DeltaY = std::abs(NewScale.Y - DragStartScale.Y);
					if (DeltaY >= 0.01f)
					{
						NewScale.Y = std::round(NewScale.Y / SnapValue) * SnapValue;
						NewScale.Y = std::max(NewScale.Y, MIN_SCALE_VALUE);
					}
				}
				if (bModifiedZ)
				{
					const float DeltaZ = std::abs(NewScale.Z - DragStartScale.Z);
					if (DeltaZ >= 0.01f)
					{
						NewScale.Z = std::round(NewScale.Z / SnapValue) * SnapValue;
						NewScale.Z = std::max(NewScale.Z, MIN_SCALE_VALUE);
					}
				}
			}

			return NewScale;
		}
		return GetComponentScale();
	}

	// 축 스케일 처리 (기존 로직)
	FVector CardinalAxis = GetGizmoAxis();
	FVector GizmoAxis = Quat.RotateVector(CardinalAxis);

	// 카메라 방향 벡터를 사용하여 안정적인 평면 계산
	const FVector CamForward = InCamera->GetForward();
	const FVector CamRight = InCamera->GetRight();
	const FVector CamUp = InCamera->GetUp();

	// GizmoAxis에 가장 수직인 카메라 벡터 선택
	FVector PlaneVector;
	const float DotRight = abs(GizmoAxis.Dot(CamRight));
	const float DotUp = abs(GizmoAxis.Dot(CamUp));

	if (DotRight < DotUp)
	{
		PlaneVector = CamRight;
	}
	else
	{
		PlaneVector = CamUp;
	}

	FVector PlaneNormal = GizmoAxis.Cross(PlaneVector);
	PlaneNormal.Normalize();

	// PlaneNormal이 카메라를 향하도록 보장 (드래그 방향 일관성)
	const FVector ToCamera = (CameraLocation - PlaneOrigin).GetNormalized();
	if (PlaneNormal.Dot(ToCamera) < 0.0f)
	{
		PlaneNormal = -PlaneNormal;
	}

	if (InObjectPicker->IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		// UE 방식을 사용, 드래그 거리에 비례하여 스케일 변화
		const FVector MouseDelta = MouseWorld - GetDragStartMouseLocation();
		const float AxisDragDistance = MouseDelta.Dot(GizmoAxis);

		// 카메라 거리에 따른 스케일 민감도 조정
		const float DistanceToGizmo = (PlaneOrigin - CameraLocation).Length();

		// 거리에 비례한 민감도, 기본 배율 적용
		// 가까울수록 정밀하게, 멀수록 빠르게 조정
		constexpr float BaseSensitivity = 0.01f;  // 기본 민감도 (감소)
		const float ScaleSensitivity = BaseSensitivity * DistanceToGizmo;
		const float ScaleDelta = AxisDragDistance * ScaleSensitivity;

		const FVector DragStartScale = GetDragStartActorScale();
		FVector NewScale = DragStartScale;

		// 스냅 설정 가져오기
		bool bSnapEnabled = false;
		float SnapValue = 1.0f;
		if (bUseCustomScaleSnap)
		{
			bSnapEnabled = bCustomScaleSnapEnabled;
			SnapValue = CustomScaleSnapValue;
		}
		else
		{
			bSnapEnabled = UViewportManager::GetInstance().IsScaleSnapEnabled();
			SnapValue = UViewportManager::GetInstance().GetScaleSnapValue();
		}

		// 기즈모는 uniform scale lock 상태와 무관하게 드래그한 축만 스케일
		// X축 (1,0,0)
		if (abs(CardinalAxis.X) > 0.5f)
		{
			NewScale.X = std::max(DragStartScale.X + ScaleDelta, MIN_SCALE_VALUE);
			// 드래그한 축만 스냅 적용 (충분히 드래그했을 때만)
			if (bSnapEnabled && SnapValue > 0.0001f)
			{
				const float Delta = std::abs(NewScale.X - DragStartScale.X);
				if (Delta >= 0.01f)
				{
					NewScale.X = std::round(NewScale.X / SnapValue) * SnapValue;
					NewScale.X = std::max(NewScale.X, MIN_SCALE_VALUE);
				}
			}
		}
		// Y축 (0,1,0)
		else if (abs(CardinalAxis.Y) > 0.5f)
		{
			NewScale.Y = std::max(DragStartScale.Y + ScaleDelta, MIN_SCALE_VALUE);
			// 드래그한 축만 스냅 적용 (충분히 드래그했을 때만)
			if (bSnapEnabled && SnapValue > 0.0001f)
			{
				const float Delta = std::abs(NewScale.Y - DragStartScale.Y);
				if (Delta >= 0.01f)
				{
					NewScale.Y = std::round(NewScale.Y / SnapValue) * SnapValue;
					NewScale.Y = std::max(NewScale.Y, MIN_SCALE_VALUE);
				}
			}
		}
		// Z축 (0,0,1)
		else if (abs(CardinalAxis.Z) > 0.5f)
		{
			NewScale.Z = std::max(DragStartScale.Z + ScaleDelta, MIN_SCALE_VALUE);
			// 드래그한 축만 스냅 적용 (충분히 드래그했을 때만)
			if (bSnapEnabled && SnapValue > 0.0001f)
			{
				const float Delta = std::abs(NewScale.Z - DragStartScale.Z);
				if (Delta >= 0.01f)
				{
					NewScale.Z = std::round(NewScale.Z / SnapValue) * SnapValue;
					NewScale.Z = std::max(NewScale.Z, MIN_SCALE_VALUE);
				}
			}
		}

		return NewScale;
	}
	return GetComponentScale();
}

// ============================================================================
// FGizmoHelper Implementation
// ============================================================================

FVector FGizmoHelper::ProcessDragLocation(UGizmo* Gizmo, UObjectPicker* Picker, UCamera* Camera, FRay& Ray, bool bUseCustomSnap)
{
	if (!Gizmo || !Picker || !Camera)
	{
		return Gizmo ? Gizmo->GetGizmoLocation() : FVector::Zero();
	}

	return Gizmo->ProcessDragLocation(Camera, Ray, Picker, bUseCustomSnap);
}

FQuaternion FGizmoHelper::ProcessDragRotation(UGizmo* Gizmo, UCamera* Camera, FRay& Ray, const FRect& ViewportRect, bool bUseCustomSnap)
{
	if (!Gizmo || !Camera)
	{
		return FQuaternion::Identity();
	}

	return Gizmo->ProcessDragRotation(Camera, Ray, ViewportRect, bUseCustomSnap);
}

FVector FGizmoHelper::ProcessDragScale(UGizmo* Gizmo, UObjectPicker* Picker, UCamera* Camera, FRay& Ray)
{
	if (!Gizmo || !Picker || !Camera)
	{
		return FVector::One();
	}

	return Gizmo->ProcessDragScale(Camera, Ray, Picker);
}
