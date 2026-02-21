#pragma once
class ViewerState;
class UWorld;
struct ID3D11Device;

/**
 * @brief AnimSequenceViewer 전용 뷰어 스테이트 생성/파괴 헬퍼
 * - 애니메이션 프리뷰에 최적화된 설정
 * - 전신이 보이도록 카메라 위치 조정
 * - 기본 FViewportClient 사용 (에디터 기능 불필요)
 */
class AnimSequenceViewerBootstrap
{
public:
    static ViewerState* CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice);
    static void DestroyViewerState(ViewerState*& State);
};
