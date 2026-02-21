#pragma once
#include "Widget.h"
#include "Editor/Public/GizmoTypes.h"

class FViewportClient;
class UCamera;
class UTexture;
class UGizmo;

/**
 * @brief 뷰포트 툴바 위젯의 베이스 클래스
 *
 * ViewportControlWidget과 SkeletalMeshViewerToolbarWidget의 공통 기능을 제공합니다.
 * - Gizmo Mode 버튼 (Select/Translate/Rotate/Scale)
 * - World/Local Space 토글
 * - Location Snap 컨트롤
 * - Rotation Snap 컨트롤
 * - Scale Snap 컨트롤
 * - ViewType 버튼 (Perspective/Ortho)
 * - Camera Settings 버튼
 * - ViewMode 버튼
 */
class UViewportToolbarWidgetBase : public UWidget
{
public:
	UViewportToolbarWidgetBase();
	virtual ~UViewportToolbarWidgetBase();

protected:
	// ========================================
	// Common Icon Loading
	// ========================================

	/**
	 * @brief 공통 아이콘들을 로드합니다
	 */
	void LoadCommonIcons();

	// ========================================
	// Common Rendering Functions
	// ========================================

	/**
	 * @brief 아이콘 버튼 렌더링 (공통 로직)
	 */
	bool DrawIconButton(const char* ID, UTexture* Icon, bool bActive, const char* Tooltip, float ButtonSize = 24.0f, float IconSize = 16.0f);

	/**
	 * @brief Gizmo Mode 버튼들 렌더링 (QWER)
	 * @param CurrentGizmoMode 현재 기즈모 모드
	 * @param bSelectModeActive Select 모드 활성화 여부
	 * @param OutGizmoMode 변경된 기즈모 모드 (출력)
	 * @param OutSelectMode 변경된 Select 모드 (출력)
	 */
	void RenderGizmoModeButtons(EGizmoMode CurrentGizmoMode, bool bSelectModeActive, EGizmoMode& OutGizmoMode, bool& OutSelectMode);

	/**
	 * @brief World/Local Space 토글 버튼 렌더링
	 * @param Gizmo 기즈모 객체
	 */
	void RenderWorldLocalToggle(UGizmo* Gizmo);

	/**
	 * @brief Location Snap 컨트롤 렌더링
	 * @param bSnapEnabled 스냅 활성화 여부 (입출력)
	 * @param SnapValue 스냅 값 (입출력)
	 */
	void RenderLocationSnapControls(bool& bSnapEnabled, float& SnapValue);

	/**
	 * @brief Rotation Snap 컨트롤 렌더링
	 * @param bSnapEnabled 스냅 활성화 여부 (입출력)
	 * @param SnapAngle 스냅 각도 (입출력)
	 */
	void RenderRotationSnapControls(bool& bSnapEnabled, float& SnapAngle);

	/**
	 * @brief Scale Snap 컨트롤 렌더링
	 * @param bSnapEnabled 스냅 활성화 여부 (입출력)
	 * @param SnapValue 스냅 값 (입출력)
	 */
	void RenderScaleSnapControls(bool& bSnapEnabled, float& SnapValue);

	/**
	 * @brief ViewType 버튼 렌더링
	 * @param Client ViewportClient
	 * @param ViewTypeLabels ViewType 레이블 배열
	 * @param ButtonWidth 버튼 너비
	 * @param bIsPilotMode 파일럿 모드 여부 (기본값: false)
	 * @param PilotedActorName 파일럿 액터 이름 (기본값: "")
	 */
	void RenderViewTypeButton(FViewportClient* Client, const char** ViewTypeLabels, float ButtonWidth = 110.0f, bool bIsPilotMode = false, const char* PilotedActorName = "");

	/**
	 * @brief Camera Settings 팝업 내용 렌더링
	 * @param Camera 카메라 객체
	 * @param bIsPerspective Perspective 뷰 여부
	 * @param bEnableSharedOrthoZoom 공유 Ortho Zoom 활성화 여부
	 * @param OnOrthoZoomChanged Ortho Zoom 변경 콜백
	 */
	void RenderCameraSettingsPopup(UCamera* Camera, bool bIsPerspective, bool bEnableSharedOrthoZoom = false, void(*OnOrthoZoomChanged)(float) = nullptr);

	/**
	 * @brief Camera Speed 버튼 렌더링
	 * @param Camera 카메라 객체
	 * @param ButtonWidth 버튼 너비
	 */
	void RenderCameraSpeedButton(UCamera* Camera, float ButtonWidth = 70.0f);

	/**
	 * @brief ViewMode 버튼 렌더링
	 * @param Client ViewportClient
	 * @param ViewModeLabels ViewMode 레이블 배열
	 */
	void RenderViewModeButton(FViewportClient* Client, const char** ViewModeLabels);

protected:
	// ========================================
	// Common Icons
	// ========================================

	// ViewType 아이콘들
	UTexture* IconPerspective = nullptr;
	UTexture* IconTop = nullptr;
	UTexture* IconBottom = nullptr;
	UTexture* IconLeft = nullptr;
	UTexture* IconRight = nullptr;
	UTexture* IconFront = nullptr;
	UTexture* IconBack = nullptr;

	// Gizmo Mode 아이콘들
	UTexture* IconSelect = nullptr;
	UTexture* IconTranslate = nullptr;
	UTexture* IconRotate = nullptr;
	UTexture* IconScale = nullptr;

	// World/Local Space 아이콘들
	UTexture* IconWorldSpace = nullptr;
	UTexture* IconLocalSpace = nullptr;

	// Snap 아이콘들
	UTexture* IconSnapLocation = nullptr;
	UTexture* IconSnapRotation = nullptr;
	UTexture* IconSnapScale = nullptr;

	// ViewMode 아이콘
	UTexture* IconLitCube = nullptr;

	// 카메라 설정 아이콘
	UTexture* IconCamera = nullptr;

	bool bIconsLoaded = false;
};
