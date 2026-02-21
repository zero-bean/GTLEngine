#pragma once

class UGizmo;
class UObjectPicker;
class UCamera;
struct FRay;
struct FRect;

/**
 * @brief 기즈모 드래그 처리를 위한 헬퍼 클래스
 * Editor와 SkeletalMeshViewerWindow 간의 중복 코드를 제거하기 위해 생성
 */
class FGizmoHelper
{
public:
	/**
	 * @brief 기즈모 위치(Translation) 드래그 처리
	 * @param Gizmo 기즈모 객체
	 * @param Picker 오브젝트 피커 (레이-평면 충돌 계산용)
	 * @param Camera 현재 카메라
	 * @param Ray 마우스 레이
	 * @param bUseCustomSnap true면 Gizmo의 커스텀 스냅 사용, false면 ViewportManager 사용
	 * @return 새로운 위치 값
	 */
	static FVector ProcessDragLocation(UGizmo* Gizmo, UObjectPicker* Picker, UCamera* Camera, FRay& Ray, bool bUseCustomSnap = false);

	/**
	 * @brief 기즈모 회전(Rotation) 드래그 처리
	 * @param Gizmo 기즈모 객체
	 * @param Camera 현재 카메라
	 * @param Ray 마우스 레이
	 * @param ViewportRect 뷰포트 영역 (스크린 공간 계산용)
	 * @param bUseCustomSnap true면 Gizmo의 커스텀 스냅 사용, false면 ViewportManager 사용
	 * @return 새로운 회전 값
	 */
	static FQuaternion ProcessDragRotation(UGizmo* Gizmo, UCamera* Camera, FRay& Ray, const FRect& ViewportRect, bool bUseCustomSnap);

	/**
	 * @brief 기즈모 스케일(Scale) 드래그 처리
	 * @param Gizmo 기즈모 객체
	 * @param Picker 오브젝트 피커 (레이-평면 충돌 계산용)
	 * @param Camera 현재 카메라
	 * @param Ray 마우스 레이
	 * @return 새로운 스케일 값
	 */
	static FVector ProcessDragScale(UGizmo* Gizmo, UObjectPicker* Picker, UCamera* Camera, FRay& Ray);
};
