#pragma once
#include "Render/UI/Window/Public/UIWindow.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"

class FViewport;
class FViewportClient;
class UCamera;
class USkeletalMeshComponent;
class USkeletalMeshViewerToolbarWidget;
class UAmbientLightComponent;
class UDirectionalLightComponent;
class UWorld;
class AActor;
class UGizmo;
class UObjectPicker;
struct FHitProxyId;

/**
 * @brief SkeletalMesh 뷰어 윈도우
 * Editor 내부에서 복수의 SkeletalMesh 리소스를 View & Edit 할 수 있는 내부 Viewer
 *
 * Layout:
 * - 좌측: Skeleton Tree (본 계층 구조) [TODO]
 * - 중앙: 3D Viewport (독립 카메라) [TODO]
 * - 우측: Edit Tools (Transform, Gizmo 설정 등) [TODO]
 *
 * @note 현재는 기본 레이아웃 구조만 구현됨. 각 패널의 실제 기능은 추후 구현 예정
 *
 * TODO: SkeletalMeshComponent의 디테일 패널에서도 이 뷰어를 열 수 있는 기능 추가
 */
class USkeletalMeshViewerWindow : public UUIWindow
{
	DECLARE_CLASS(USkeletalMeshViewerWindow, UUIWindow);

public:
	USkeletalMeshViewerWindow();
	virtual ~USkeletalMeshViewerWindow() override;

	void Initialize() override;
	void Cleanup() override;

	void OpenViewer(USkeletalMeshComponent* InSkeletalMeshComponent);
	bool OnWindowClose() override;

	void DuplicateSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent);

protected:
	void OnPreRenderWindow(float MenuBarOffset) override;
	void OnPostRenderWindow() override;
	/**
	 * @brief 3-패널 레이아웃을 렌더링하는 함수
	 * 좌측(25%): 스켈레톤 트리, 중앙(50%): 3D 뷰, 우측(25%): 편집 툴
	 */
	void RenderLayout();

	/**
	 * @brief 좌측 패널: Skeleton Tree 영역 렌더링 (Placeholder)
	 * RenderBoneTreeNode 호출	
	 */
	void RenderSkeletonTreePanel(const USkeletalMesh* InSkeletalMesh, const FReferenceSkeleton& InRefSkeleton, const int32 InNumBones);

	/**
	 * @brief 개별 본 노드를 재귀적으로 렌더링하는 헬퍼 함수
	 * @param BoneIndex 렌더링할 본의 인덱스
	 * @param RefSkeleton 레퍼런스 스켈레톤
	 * @param SearchFilter 검색 필터 문자열
	 * @param bHasSearchFilter 검색 필터가 활성화되어 있는지 여부
	 */
	void RenderBoneTreeNode(int32 BoneIndex, const FReferenceSkeleton& RefSkeleton, const FString& SearchFilter, bool bHasSearchFilter);

	/**
	 * @brief 중앙 패널: 3D Viewport 영역 렌더링 (Placeholder)
	 * TODO: 독립적인 카메라를 가진 3D 렌더링 뷰포트 구현
	 * TODO: 선택된 본의 Transform Gizmo 렌더링
	 */
	void Render3DViewportPanel();

	/**
	 * @brief Render3DViewportPanel의 세부 함수들 (리팩토링)
	 */

	/** @brief 툴바 업데이트 및 기즈모 동기화 처리 */
	void UpdateToolbarAndGizmoSync();
	/** @brief 뷰포트 크기 변경 시 렌더 타겟 업데이트 */
	void UpdateViewportRenderTarget(uint32 NewWidth, uint32 NewHeight);
	/** @brief 뷰포트 렌더 타겟에 3D 씬 렌더링 */
	void RenderToViewportTexture();
	/** @brief 뷰포트 이미지 표시 및 정보 오버레이 렌더링 */
	void DisplayViewportImage(const ImVec2& ViewportSize);
	/** @brief 뷰포트 입력 처리 (키보드, 마우스, 기즈모, 카메라) */
	void ProcessViewportInput(bool bViewerHasFocus, const ImVec2& ViewportWindowPos);


	/**
	 * @brief 우측 패널: Edit Tools 영역 렌더링 (Placeholder)
	 * TODO: Transform 편집 UI, Gizmo 설정, 본 프로퍼티 등 구현
	 */
	void RenderEditToolsPanel(const USkeletalMesh* InSkeletalMesh, const FReferenceSkeleton& InRefSkeleton, const int32 InNumBones);

	/**
	 * @brief 수직 Splitter (구분선) 렌더링 및 드래그 처리
	 * @param SplitterID 고유 ID
	 * @param Ratio 현재 비율 (0.0 ~ 1.0)
	 * @param MinRatio 최소 비율
	 * @param MaxRatio 최대 비율
	 * @param bInvertDirection true면 드래그 방향 반전 (우측 패널용)
	 */
	void RenderVerticalSplitter(const char* SplitterID, float& Ratio, float MinRatio, float MaxRatio, bool bInvertDirection = false);

	/**
	 * @brief 카메라 컨트롤 UI 렌더링
	 * @param InCamera 카메라 객체
	 */
	void RenderCameraControls(UCamera& InCamera);

	/**
	 * @brief SkeletalMeshComponent 유효성 검사 및 로그 출력
	 * @return 유효한 경우 true, 그렇지 않으면 false
	 */
	bool CheckSkeletalValidity(USkeletalMesh*& OutSkeletalMesh, FReferenceSkeleton& OutRefSkeleton, int32& OutNumBones, bool bLogging) const;

private:
	// 패널 크기 비율 (드래그 가능)
	float LeftPanelWidthRatio = 0.25f;
	float RightPanelWidthRatio = 0.25f;

	// Splitter 설정
	static constexpr float SplitterWidth = 4.0f;
	static constexpr float MinPanelRatio = 0.1f;
	static constexpr float MaxPanelRatio = 0.8f;

	// 독립적인 뷰포트 및 카메라
	FViewport* ViewerViewport = nullptr;
	FViewportClient* ViewerViewportClient = nullptr;

	// 독립적인 렌더 타겟
	ID3D11Texture2D* ViewerRenderTargetTexture = nullptr;
	ID3D11RenderTargetView* ViewerRenderTargetView = nullptr;
	ID3D11ShaderResourceView* ViewerShaderResourceView = nullptr;
	ID3D11DepthStencilView* ViewerDepthStencilView = nullptr;
	ID3D11Texture2D* ViewerDepthStencilTexture = nullptr;

	// HitProxy 렌더 타겟 (마우스 피킹용)
	ID3D11Texture2D* ViewerHitProxyTexture = nullptr;
	ID3D11RenderTargetView* ViewerHitProxyRTV = nullptr;
	ID3D11ShaderResourceView* ViewerHitProxySRV = nullptr;

	// D2D 렌더 타겟 (오버레이용)
	ID2D1RenderTarget* ViewerD2DRenderTarget = nullptr;

	uint32 ViewerWidth = 800;
	uint32 ViewerHeight = 600;

	// 초기화 및 정리 상태 플래그
	bool bIsInitialized = false;
	bool bIsCleanedUp = false;

	// 렌더링할 SkeletalMeshComponent
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;

	// 원본 SkeletalMeshComponent
	USkeletalMeshComponent* OriginalSkeletalMeshComponent = nullptr;

	bool bDirtyBoneTransforms = false;

	// 선택된 본 인덱스
	int32 SelectedBoneIndex = INDEX_NONE;

	// 임시 본 트랜스폼 (편집용)
	TArray<FTransform> TempBoneSpaceTransforms;

	// 독립적인 BatchLines (Grid 렌더링용)
	class UBatchLines* ViewerBatchLines = nullptr;

	// 툴바 위젯
	USkeletalMeshViewerToolbarWidget* ToolbarWidget = nullptr;

	// 마우스 드래그 상태 플래그
	bool bIsDraggingRightButton = false;
	bool bIsDraggingMiddleButton = false;

	// 그리드 설정
	float GridCellSize = 10.0f;

	bool bShowAllBones = false;

	// 본 스케일 설정
	bool bUniformBoneScale = false;

	bool bShowAllBoneNames = false; // tree node에 모든 본 이름 표시 여부

	// Lock/Unlock 아이콘
	class UTexture* LockIcon = nullptr;
	class UTexture* UnlockIcon = nullptr;

	// Preview World (EWorldType::EditorPreview)
	UWorld* PreviewWorld = nullptr;

	// Preview World에 스폰된 Actor들
	AActor* PreviewSkeletalMeshActor = nullptr;
	AActor* PreviewAmbientLightActor = nullptr;
	AActor* PreviewDirectionalLightActor = nullptr;

	// 기즈모 및 오브젝트 피커
	UGizmo* ViewerGizmo = nullptr;
	UObjectPicker* ViewerObjectPicker = nullptr;

public:
	// 그리드 설정 접근자
	float GetGridCellSize() const { return GridCellSize; }
	void SetGridCellSize(float NewCellSize);

	// Gizmo 접근자 (툴바 위젯에서 World/Local 모드 토글용)
	UGizmo* GetGizmo() const { return ViewerGizmo; }

private:
	/**
	 * @brief 렌더 타겟 생성
	 */
	void CreateRenderTarget(uint32 Width, uint32 Height);

	/**
	 * @brief 렌더 타겟 해제
	 */
	void ReleaseRenderTarget();

	/**
	 * @brief HitProxy 패스 렌더링 (본 피킹용)
	 */
	void RenderHitProxyPassForViewer();

	/**
	 * @brief 뷰어의 특정 위치에서 HitProxy ID 읽기
	 * @param X 뷰어 로컬 X 좌표
	 * @param Y 뷰어 로컬 Y 좌표
	 * @return HitProxy ID
	 */
	FHitProxyId ReadHitProxyAtViewerLocation(int32 X, int32 Y);

	/**
	 * @brief 본 좌표 변환 헬퍼 함수들
	 * UI 슬라이더와 동일한 방식으로 본의 로컬 트랜스폼을 직접 수정하되,
	 * 기즈모의 월드 좌표를 로컬 좌표로 변환하는 기능을 제공합니다.
	 */

	/**
	 * @brief 본의 월드 스페이스 위치 계산
	 * @param BoneIndex 본 인덱스
	 * @return 본의 월드 스페이스 위치
	 */
	FVector GetBoneWorldLocation(int32 BoneIndex) const;

	/**
	 * @brief 본의 월드 스페이스 회전 계산
	 * @param BoneIndex 본 인덱스
	 * @return 본의 월드 스페이스 회전 (쿼터니언)
	 */
	FQuaternion GetBoneWorldRotation(int32 BoneIndex) const;

	/**
	 * @brief 월드 스페이스 위치를 본의 로컬 스페이스 위치로 변환
	 * @param BoneIndex 본 인덱스
	 * @param WorldLocation 월드 스페이스 위치
	 * @return 본의 로컬 스페이스 위치 (부모 본 기준)
	 */
	FVector WorldToLocalBoneTranslation(int32 BoneIndex, const FVector& WorldLocation) const;

	/**
	 * @brief 월드 스페이스 회전을 본의 로컬 스페이스 회전으로 변환
	 * @param BoneIndex 본 인덱스
	 * @param WorldRotation 월드 스페이스 회전 (쿼터니언)
	 * @return 본의 로컬 스페이스 회전 (부모 본 기준)
	 */
	FQuaternion WorldToLocalBoneRotation(int32 BoneIndex, const FQuaternion& WorldRotation) const;
};
