#pragma once

class UWorld; class FViewport; class FViewportClient; class ASkeletalMeshActor; class USkeletalMesh;

class ViewerState
{
public:
    FName Name;
    UWorld* World = nullptr;
    FViewport* Viewport = nullptr;
    FViewportClient* Client = nullptr;
    
    // Have a pointer to the currently selected mesh to render in the viewer
    ASkeletalMeshActor* PreviewActor = nullptr;
    USkeletalMesh* CurrentMesh = nullptr;
    FString LoadedMeshPath;  // Track loaded mesh path for unloading
    int32 SelectedBoneIndex = -1;
    bool bShowMesh = true;
    bool bShowBones = true;
    // Bone line rebuild control
    bool bBoneLinesDirty = true;      // true면 본 라인 재구성
    int32 LastSelectedBoneIndex = -1; // 색상 갱신을 위한 이전 선택 인덱스
    // UI path buffer per-tab
    char MeshPathBuffer[260] = {0};
    std::set<int32> ExpandedBoneIndices;

    // 본 트랜스폼 편집 관련
    FVector EditBoneLocation;
    FVector EditBoneRotation;  // Euler angles in degrees
    FVector EditBoneScale;

    bool bBoneTransformChanged = false;
    bool bBoneRotationEditing = false;

    // 애니메이션 재생 관련 (Step 6: AnimSequenceViewer 연동)
    float CurrentTime = 0.0f;
    bool bIsPlaying = false;
    float PlayRate = 1.0f;
};
