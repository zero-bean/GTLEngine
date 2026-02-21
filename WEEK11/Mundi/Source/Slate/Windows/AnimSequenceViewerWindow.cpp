#include "pch.h"
#include "USlateManager.h"
#include "AnimSequenceViewerWindow.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"
#include "Source/Runtime/Engine/Animation/AnimSequenceViewerBootstrap.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/AssetManagement/SkeletalMesh.h"
#include "Source/Runtime/Engine/Animation/NotifyDispatcher.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "BoneAnchorComponent.h"
#include "Source/Runtime/Engine/Collision/Picking.h"
#include "Source/Runtime/Engine/GameFramework/CameraActor.h"
#include "SelectionManager.h"
#include "Source/Runtime/Engine/Components/LineComponent.h"
#include "PathUtils.h"

SAnimSequenceViewerWindow::SAnimSequenceViewerWindow()
{
	// ResourceManager에서 모든 애니메이션 파일 경로 가져오기
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	AvailableAnimationPaths = ResourceManager.GetAllFilePaths<UAnimSequence>();

	CurrentSequence = nullptr; // 초기 시퀀스 null (아무것도 선택 안됨)

	//// 더미 시퀀스 생성 (테스트용)
	//CurrentSequence = NewObject<UAnimSequenceBase>();
	//CurrentSequence->SetSequenceLength(5.0f);
	//CurrentSequence->SetLooping(true);

	//// 더미 Notify 데이터 추가
	//FAnimNotifyEvent notify1;
	//notify1.TriggerTime = 1.0f;
	//notify1.Duration = 0.3f;
	//notify1.NotifyName = "Footstep_L";
	//CurrentSequence->AddNotify(notify1);

	//FAnimNotifyEvent notify2;
	//notify2.TriggerTime = 2.0f;
	//notify2.Duration = 0.0f;
	//notify2.NotifyName = "Footstep_R";
	//CurrentSequence->AddNotify(notify2);

	//FAnimNotifyEvent notify3;
	//notify3.TriggerTime = 3.5f;
	//notify3.Duration = 0.5f;  // Duration이 있는 경우
	//notify3.NotifyName = "PlaySound";
	//CurrentSequence->AddNotify(notify3);

	//FAnimNotifyEvent notify4;
	//notify4.TriggerTime = 4.2f;
	//notify4.Duration = 0.0f;
	//notify4.NotifyName = "SpawnEffect";
	//CurrentSequence->AddNotify(notify4);
}

SAnimSequenceViewerWindow::~SAnimSequenceViewerWindow()
{
	// ViewerState 파괴
	AnimSequenceViewerBootstrap::DestroyViewerState(PreviewState);

	// 시퀀스 정리
	if (CurrentSequence)
	{
		CurrentSequence = nullptr;
	}
}

bool SAnimSequenceViewerWindow::Initialize(UWorld* InWorld, ID3D11Device* InDevice)
{
	World = InWorld;
	Device = InDevice;

	// ViewerState 생성 (AnimSequenceViewer 전용 부트스트랩 사용)
	PreviewState = AnimSequenceViewerBootstrap::CreateViewerState("AnimSequencePreview", InWorld, InDevice);
	if (!PreviewState)
	{
		return false;
	}

	return true;
}

void SAnimSequenceViewerWindow::SetSkeletalMeshPath(const char* MeshPath)
{
	if (!PreviewState || !MeshPath || MeshPath[0] == '\0')
	{
		return;
	}

	// MeshPathBuffer에 경로 복사
	strncpy_s(PreviewState->MeshPathBuffer, MeshPath, sizeof(PreviewState->MeshPathBuffer) - 1);
	PreviewState->MeshPathBuffer[sizeof(PreviewState->MeshPathBuffer) - 1] = '\0';

	// 즉시 스켈레탈 메시 로드
	if (PreviewState->PreviewActor)
	{
		ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor);
		if (PreviewActor)
		{
			// 스켈레탈 메시 리소스 로드 및 설정 (SSkeletalMeshViewerWindow 참고)
			USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(MeshPath);
			if (Mesh)
			{
				PreviewActor->SetSkeletalMesh(MeshPath);
				PreviewState->CurrentMesh = Mesh;
				PreviewState->LoadedMeshPath = MeshPath;
				UE_LOG("[AnimSequenceViewer] Skeletal mesh set from outliner: %s", MeshPath);

				// 본 표시가 켜져 있으면 본 라인 구성
				if (bShowBones)
				{
					PreviewActor->RebuildBoneLines(-1); // -1 = 선택된 본 없음
					if (PreviewActor->GetBoneLineComponent())
					{
						PreviewActor->GetBoneLineComponent()->SetLineVisible(true);
					}
				}

				// 현재 애니메이션이 있고 재생 중이면 다시 재생
				if (CurrentSequence && bIsPlaying)
				{
					UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentSequence);
					if (AnimSequence)
					{
						USkeletalMeshComponent* SkeletalMeshComp = PreviewActor->GetSkeletalMeshComponent();
						if (SkeletalMeshComp)
						{
							SkeletalMeshComp->SetVisibility(true);
							SkeletalMeshComp->PlayAnimation(CurrentSequence->GetFilePath(), bLooping);
						}
					}
				}
			}
			else
			{
				UE_LOG("[AnimSequenceViewer] Failed to load skeletal mesh: %s", MeshPath);
			}
		}
	}
}

void SAnimSequenceViewerWindow::LoadAnimSquence(UAnimSequence* Sequence)
{
	if (!Sequence)
	{
		UE_LOG("[AnimSequenceViewer] LoadAnimSequence: Sequence is null");
		return;
	}

	// 기존 시퀀스 교체
	CurrentSequence = Sequence;

	// 시퀀스 정보로 타임라인 업데이트
	PlayLength = Sequence->GetPlayLength();
	TotalFrames = Sequence->GetNumberOfFrames();
	bLooping = Sequence->IsLooping();

	// 재생 상태 초기화
	CurrentTime = 0.0f;
	CurrentFrame = 0;
	bIsPlaying = false;

	// Restore notify tracks from sequence
	NotifyTrackIndices = CurrentSequence->GetNotifyTrackIndices();
	NextNotifyTrackNumber = CurrentSequence->GetNextNotifyTrackNumber();

	// Rebuild notify chips from sequence
	NotifyChips.clear();
	if (CurrentSequence)
	{
		const TArray<FAnimNotifyEvent>& Notifies = CurrentSequence->GetNotifies();
		const TArray<int32>& DisplayTracks = CurrentSequence->GetNotifyDisplayTrackIndices();
		UE_LOG("[AnimSequenceViewer] Rebuilding notify chips. Found %d notifies in sequence '%s'",
			Notifies.size(), CurrentSequence->GetFilePath().c_str());

		for (size_t i = 0; i < Notifies.size(); ++i)
		{
			const FAnimNotifyEvent& Ev = Notifies[i];
            FNotifyChip Chip;
            Chip.Time = Ev.TriggerTime;
            Chip.Duration = Ev.Duration;
            Chip.Name = Ev.NotifyName;
            Chip.TrackIndex = (i < DisplayTracks.size()) ? DisplayTracks[i] : 0; // 저장된 트랙 인덱스 복원
            NotifyChips.Add(Chip);

			UE_LOG("[AnimSequenceViewer]   - Notify: %s at time %.3f on track %d",
				Ev.NotifyName.ToString().c_str(), Ev.TriggerTime, Chip.TrackIndex);
		}

		UE_LOG("[AnimSequenceViewer] Restored %d notify tracks", NotifyTrackIndices.size());
	}

	UE_LOG("[AnimSequenceViewer] Loaded: %s (Length: %.2fs, Frames: %d)",
		Sequence->GetFilePath().c_str(), PlayLength, TotalFrames);

	// PreviewActor에 애니메이션 설정 (아직 재생하지 않음)
	// 참고: 스켈레탈 메시는 이미 SetSkeletalMeshPath()에서 설정되어 있음
	if (PreviewState && PreviewState->PreviewActor)
	{
		ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor);
		if (PreviewActor)
		{
			USkeletalMeshComponent* SkeletalMeshComp = PreviewActor->GetSkeletalMeshComponent();
			if (SkeletalMeshComp)
			{
				SkeletalMeshComp->SetVisibility(true);
				UE_LOG("[AnimSequenceViewer] Animation loaded. Press Play to start.");

				// 본 표시가 켜져 있으면 본 라인 구성
				if (bShowBones)
				{
					PreviewActor->RebuildBoneLines(-1); // -1 = 선택된 본 없음
					if (PreviewActor->GetBoneLineComponent())
					{
						PreviewActor->GetBoneLineComponent()->SetLineVisible(true);
					}
				}
			}
			else
			{
				UE_LOG("[AnimSequenceViewer] WARNING: No skeletal mesh loaded. Please select a skeletal mesh from the outliner first.");
			}
		}
		else
		{
			UE_LOG("[AnimSequenceViewer] ERROR: PreviewActor cast failed");
		}
	}
	else
	{
		UE_LOG("[AnimSequenceViewer] ERROR: PreviewState or PreviewActor is null");
	}
}

void SAnimSequenceViewerWindow::OnRender()
{
	if (!bIsOpen) return;

	// 처음 한 번만 윈도우 위치/크기 설정
	if (!bInitialPlacementDone)
	{
		ImGui::SetNextWindowPos(ImVec2(100, 100));
		ImGui::SetNextWindowSize(ImVec2(1200, 800));  // 크기 조정 (프리뷰를 위해 더 크게)
		bInitialPlacementDone = true;
	}

	// 윈도우 시작 (사용자가 X버튼 누르면 bIsOpen이 false가 됨)
	if (ImGui::Begin("Animation Sequence Viewer", &bIsOpen))
	{
		// 윈도우 Rect 업데이트 (마우스 이벤트 라우팅용)
		ImVec2 WindowPos = ImGui::GetWindowPos();
		ImVec2 WindowSize = ImGui::GetWindowSize();
		Rect.Left = WindowPos.x;
		Rect.Top = WindowPos.y;
		Rect.Right = WindowPos.x + WindowSize.x;
		Rect.Bottom = WindowPos.y + WindowSize.y;
		Rect.UpdateMinMax();

		ImVec2 ContentAvail = ImGui::GetContentRegionAvail();
		float TotalWidth = ContentAvail.x;
		float TotalHeight = ContentAvail.y;

		// ============================================================
		// 레이아웃 비율 계산
		// ============================================================
		float TopPreviewHeightPixels = TotalHeight * TopPreviewHeight;      // 상단 60%
		float BottomPanelHeightPixels = TotalHeight * BottomPanelHeight;    // 하단 40%

		// 하단 패널의 가로 분할
		float RightPanelWidthPixels = TotalWidth * RightPanelWidth;         // 우측 30%

		// 우측 패널의 세로 분할
		float RightTopInfoHeightPixels = BottomPanelHeightPixels * RightTopInfoHeight;      // 40%
		float RightBottomListHeightPixels = BottomPanelHeightPixels * RightBottomListHeight; // 60%

		// ============================================================
		// 상단: 3D 프리뷰 뷰포트 (전체 너비, 60% 높이)
		// ============================================================
		RenderPreviewViewport(TopPreviewHeightPixels);

		// ============================================================
		// 하단: 좌측 (통합 Notify+Timeline) | 우측 (Info+List)
		// ============================================================

		// 스타일: 패널 간 간격 제거
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		// --- 좌측: 통합 Notify+Timeline 패널 (70%) ---
		float LeftCombinedWidth = TotalWidth * 0.70f; // Notify+Timeline 합쳐서 70%
		ImGui::BeginChild("CombinedNotifyTimelinePanel", ImVec2(LeftCombinedWidth, BottomPanelHeightPixels), true);
		{
			// Notify 트랙과 Timeline을 합친 새로운 레이아웃
			RenderCombinedNotifyTimeline();
		}
		ImGui::EndChild();

		ImGui::SameLine(0, 0);

		// --- 우측: Info + List (30%, 세로 분할) ---
		ImGui::BeginChild("RightPanel", ImVec2(RightPanelWidthPixels, BottomPanelHeightPixels), false, ImGuiWindowFlags_NoScrollbar);
		{
			// 상단: Animation Info (40%)
			ImGui::BeginChild("InfoPanel", ImVec2(0, RightTopInfoHeightPixels), true);
			{
				RenderInfoPanel();
			}
			ImGui::EndChild();

			// 하단: Animation List (60%)
			ImGui::BeginChild("AnimList", ImVec2(0, RightBottomListHeightPixels), true);
			{
				RenderAnimationList();
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();

		ImGui::PopStyleVar(); // ItemSpacing 복원
	}
	ImGui::End();

	// 윈도우가 닫히면 정리
	if (!bIsOpen)
	{
		// USlateManager::OnRender()에서 처리됨
	}
}

void SAnimSequenceViewerWindow::OnUpdate(float DeltaSeconds)
{
    // ViewerState 업데이트 (월드 틱)
    if (PreviewState && PreviewState->World)
    {
        PreviewState->World->Tick(DeltaSeconds);

        // Gizmo 모드 전환 처리 (SSkeletalMeshViewerWindow 참고)
        if (PreviewState->World->GetGizmoActor())
        {
            PreviewState->World->GetGizmoActor()->ProcessGizmoModeSwitch();
        }
    }

    // ViewportClient 업데이트 (카메라 컨트롤)
    if (PreviewState && PreviewState->Client)
    {
        PreviewState->Client->Tick(DeltaSeconds);
    }

    // 기즈모로 본을 조작했는지 감지 (SSkeletalMeshViewerWindow 참고)
    // 주의: 애니메이션 재생 중에는 기즈모가 자동으로 움직이므로 사용자 조작만 감지해야 함
    if (PreviewState && PreviewState->World && PreviewState->PreviewActor && PreviewState->SelectedBoneIndex >= 0)
    {
        UBoneAnchorComponent* Anchor = PreviewState->PreviewActor->GetBoneGizmoAnchor();
        if (Anchor && Anchor->IsVisible())
        {
            // 앵커의 월드 트랜스폼이 변경되었는지 체크
            static FTransform LastAnchorTransform;
            FTransform CurrentAnchorTransform = Anchor->GetWorldTransform();

            // 기즈모 조작 감지: Gizmo Actor가 드래그 중이면 사용자 조작으로 판단
            bool bGizmoBeingManipulated = PreviewState->World->GetGizmoActor() &&
                                          PreviewState->World->GetGizmoActor()->GetbIsDragging();

            if (CurrentAnchorTransform != LastAnchorTransform && bGizmoBeingManipulated)
            {
                // 기즈모가 움직였으므로 본 트랜스폼을 업데이트
                if (ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor))
                {
                    if (USkeletalMeshComponent* SkelComp = PreviewActor->GetSkeletalMeshComponent())
                    {
                        // 앵커의 월드 트랜스폼을 본의 로컬 트랜스폼으로 변환하여 적용
                        SkelComp->SetBoneWorldTransform(PreviewState->SelectedBoneIndex, CurrentAnchorTransform);

                        // 이 본을 수동 편집된 본으로 표시 (애니메이션이 덮어쓰지 않도록)
                        SkelComp->MarkBoneAsManuallyEdited(PreviewState->SelectedBoneIndex);

                        // 본 라인 즉시 재구성 (애니메이션 재생 중이 아니어도 업데이트)
                        if (bShowBones && PreviewActor->GetBoneLineComponent())
                        {
                            PreviewActor->RebuildBoneLines(PreviewState->SelectedBoneIndex);
                        }
                    }
                }
            }

            LastAnchorTransform = CurrentAnchorTransform;
        }
    }

    // 타임라인 UI 업데이트 + notify dispatch (viewer-side)
    if (bIsPlaying && CurrentSequence)
    {
        // 시간 증가/감소 (PlayRate에 따라 정재생/역재생)
		PrevTimeForNotify = CurrentTime;
        CurrentTime += DeltaSeconds * PlayRate;

		// 정재생 (PlayRate > 0): 앞으로 재생
		if (PlayRate > 0.0f)
		{
			if (CurrentTime > PlayLength)
			{
				if (bLooping)
				{
					// 루프: 처음으로 되돌림
					// ex) CurrentTime = 5.3, PlayLength = 5.0 -> 0.3 (나머지 반환)
					CurrentTime = fmod(CurrentTime, PlayLength);
				}
				else
				{
					// 루프 아님: 끝에서 정지
					CurrentTime = PlayLength;
					bIsPlaying = false;
				}
			}
		}
		// 역재생 (PlayRate < 0): 뒤로 재생
		else if (PlayRate < 0.0f)
		{
			if (CurrentTime < 0.0f)
			{
				if (bLooping)
				{
					// 루프: 끝으로 이동
					CurrentTime = PlayLength + fmod(CurrentTime, PlayLength);
				}
				else
				{
					// 루프 아님: 처음에서 정지
					CurrentTime = 0.0f;
					bIsPlaying = false;
				}
			}
		}

		// 프레임 업데이트
		CurrentFrame = TimeToFrame(CurrentTime);

		// 애니메이션 포즈를 직접 평가하여 SkeletalMeshComponent에 적용 (재생 중일 때만)
		ApplyAnimationPose();

		// Trigger AnimNotifies using viewer timeline
		//if (UAnimSequence* Seq = Cast<UAnimSequence>(CurrentSequence))
		//{
		//	TArray<FAnimNotifyEvent> Triggered;
		//	Seq->GetAnimNotifiesInRange(PrevTimeForNotify, CurrentTime, Triggered);
		//	const FString SeqKey = CurrentSequence->GetFilePath();

		//	// DEBUG: Log the sequence key and triggered notifies
		//	if (!Triggered.empty())
		//	{
		//		UE_LOG("[AnimSequenceViewer] Triggering notifies for sequence: %s", SeqKey.c_str());
		//	}

		//	for (const FAnimNotifyEvent& Ev : Triggered)
		//	{
		//		UE_LOG("[AnimSequenceViewer] Dispatching notify: %s at time %.3f",
		//			Ev.NotifyName.ToString().c_str(), Ev.TriggerTime);
		//		FNotifyDispatcher::Get().Dispatch(SeqKey, Ev);
		//	}
		//}
	}
}

void SAnimSequenceViewerWindow::ApplyAnimationPose()
{
	if (!CurrentSequence || !PreviewState || !PreviewState->PreviewActor)
	{
		return;
	}

	ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor);
	if (!PreviewActor)
	{
		return;
	}

	USkeletalMeshComponent* SkelComp = PreviewActor->GetSkeletalMeshComponent();
	if (!SkelComp)
	{
		return;
	}

	USkeletalMesh* SkelMesh = SkelComp->GetSkeletalMesh();
	if (!SkelMesh || !SkelMesh->GetSkeletalMeshData())
	{
		return;
	}

	const FSkeleton& Skeleton = SkelMesh->GetSkeletalMeshData()->Skeleton;
	int32 BoneCount = Skeleton.Bones.Num();

    // 애니메이션 시퀀스로부터 포즈 평가
    UAnimSequence* AnimSeq = Cast<UAnimSequence>(CurrentSequence);
    if (AnimSeq)
    {
        // 현재 포즈를 베이스로 가져오기 (수동 편집된 본 포함)
        // 예: [본0의트랜스폼, 본1의트랜스폼(편집됨), 본2의트랜스폼, ...]
        TArray<FTransform> BonePoses = SkelComp->GetPose();

        // 애니메이션 트랙 데이터로 오버라이드 (단, 수동 편집된 본은 제외)
        float FrameRate = AnimSeq->GetFrameRate();
        for (const FBoneAnimationTrack& Track : AnimSeq->GetBoneTracks())
        {
            // 예: Track.BoneIndex = 5 (척추 본)
            // Track.InternalTrack.PosKeys[299개], RotKeys[299개], ScaleKeys[299개]
            if (Track.BoneIndex >= 0 && Track.BoneIndex < BoneCount)
            {
                // 수동으로 편집되지 않은 본만 애니메이션 적용
                if (!SkelComp->IsBoneManuallyEdited(Track.BoneIndex))
                {
                    // 현재 시간(CurrentTime)에 해당하는 트랜스폼 보간
                    // GetTransform(30, 2.5) → 프레임 75의 트랜스폼 반환
                    BonePoses[Track.BoneIndex] = Track.InternalTrack.GetTransform(FrameRate, CurrentTime);
                }
                // 수동 편집된 본은 현재 포즈 유지 (BonePoses에 이미 들어있음)
            }
        }

        // SkeletalMeshComponent에 최종 포즈 적용
        SkelComp->SetLocalSpacePose(BonePoses);

        // 선택된 본이 있으면 기즈모를 해당 본 위치로 업데이트 (애니메이션 재생 중에도 따라다니도록)
        if (PreviewState->SelectedBoneIndex >= 0)
        {
            UBoneAnchorComponent* Anchor = PreviewActor->GetBoneGizmoAnchor();
            if (Anchor && Anchor->IsVisible())
            {
                // 선택된 본의 현재 월드 트랜스폼을 가져와서 기즈모 위치 업데이트
                FTransform BoneWorldTransform = SkelComp->GetBoneWorldTransform(PreviewState->SelectedBoneIndex);
                Anchor->SetWorldTransform(BoneWorldTransform);
            }
        }

        // 본이 표시되어 있으면 본 라인 업데이트
        if (bShowBones && PreviewActor->GetBoneLineComponent())
        {
            // 선택된 본이 있으면 해당 본의 서브트리만 업데이트, 없으면 루트부터 전체 업데이트
            // bBoneLinesDirty가 true일 경우 전체 재구성
            if (PreviewState->bBoneLinesDirty)
            {
                PreviewActor->RebuildBoneLines(PreviewState->SelectedBoneIndex);
                PreviewState->bBoneLinesDirty = false;
            }
            else
            {
                // 단순 트랜스폼 업데이트만 필요한 경우
                PreviewActor->RebuildBoneLines(0); // 루트부터 전체 업데이트
            }
        }
    }
}

void SAnimSequenceViewerWindow::RenderAnimationList()
{
	ImGui::Text("Animation List");
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Available Animations:");
	ImGui::Spacing();

	// 실제 애니메이션 파일 목록 표시
	for (int i = 0; i < AvailableAnimationPaths.size(); i++)
	{
		bool bIsSelected = (SelectedAnimIndex == i);

		// 파일 경로에서 파일명만 추출 (확장자 제거)
		FString FullPath = AvailableAnimationPaths[i];

		// 1. 경로에서 파일명 추출
		size_t LastSlash = FullPath.find_last_of("/\\");
		FString FileName = (LastSlash != FString::npos)
			? FullPath.substr(LastSlash + 1)
			: FullPath;

		// 2. 확장자 제거
		size_t LastDot = FileName.find_last_of(".");
		if (LastDot != FString::npos)
		{
			FileName = FileName.substr(0, LastDot);
		}
		if (ImGui::Selectable(FileName.c_str(), bIsSelected))
		{
			SelectedAnimIndex = i;

			// 실제 애니메이션 시퀀스 로드 (선택된 것)
			UResourceManager& ResourceManager = UResourceManager::GetInstance();
			UAnimSequence* LoadedAnim = ResourceManager.Load<UAnimSequence>(AvailableAnimationPaths[i]);

			if (LoadedAnim)
			{
				// 선택된 애니메이션 시퀀스 로드하기
				LoadAnimSquence(LoadedAnim);
			}
		}
		if (bIsSelected)
		{
			ImGui::SetItemDefaultFocus();
		}
	}
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// 선택 정보
	if (SelectedAnimIndex >= 0 && SelectedAnimIndex < AvailableAnimationPaths.size())
	{
		// 파일명 추출 (확장자 제거)
		FString FullPath = AvailableAnimationPaths[SelectedAnimIndex];
		size_t LastSlash = FullPath.find_last_of("/\\");
		FString FileName = (LastSlash != FString::npos)
			? FullPath.substr(LastSlash + 1)
			: FullPath;

		size_t LastDot = FileName.find_last_of(".");
		if (LastDot != FString::npos)
			FileName = FileName.substr(0, LastDot);

		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
			"Selected: %s", FileName.c_str());
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
			"No animation selected");
	}
	//// 임시 하드코딩된 애니메이션 목록
	//const char* DummyAnims[] = {
	//    "MM_Idle",
	//    "MM_Walk",
	//    "MM_Run",
	//    "MM_Jump"
	//};
	//
	//for (int i = 0; i < 4; i++)
	//{
	//    bool bIsSelected = (SelectedAnimIndex == i);
	//
	//    if (ImGui::Selectable(DummyAnims[i], bIsSelected))
	//    {
	//        SelectedAnimIndex = i;
	//        // Step 4+에서 실제 애니메이션 로드
	//    }
	//
	//    if (bIsSelected)
	//    {
	//        ImGui::SetItemDefaultFocus();
	//    }
	//}
	//
	//ImGui::Spacing();
	//ImGui::Separator();
	//ImGui::Spacing();
	//
	//// 선택 정보
	//if (SelectedAnimIndex >= 0)
	//{
	//    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
	//        "Selected: %s", DummyAnims[SelectedAnimIndex]);
	//}
	//else
	//{
	//    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
	//        "No animation selected");
	//}
}

void SAnimSequenceViewerWindow::RenderInfoPanel()
{
	ImGui::Text("Animation Information");
	ImGui::Separator();
	ImGui::Spacing();

	// Show Bones 체크박스
	if (ImGui::Checkbox("Show Bones", &bShowBones))
	{
		// PreviewActor의 BoneLineComponent 가져오기
		if (PreviewState && PreviewState->PreviewActor)
		{
			ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor);
			if (PreviewActor && PreviewActor->GetBoneLineComponent())
			{
				PreviewActor->GetBoneLineComponent()->SetLineVisible(bShowBones);

                // 본을 켰을 때 본 라인 재구성
                if (bShowBones)
                {
                    PreviewActor->RebuildBoneLines(-1); // -1 = 선택된 본 없음
                }
            }
        }
    }

    // 선택된 본 정보 표시
    if (bShowBones && PreviewState && PreviewState->PreviewActor)
    {
        ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor);
        if (PreviewActor)
        {
            USkeletalMeshComponent* SkelComp = PreviewActor->GetSkeletalMeshComponent();
            if (SkelComp && SkelComp->GetSkeletalMesh() && SkelComp->GetSkeletalMesh()->GetSkeletalMeshData())
            {
                const FSkeleton& Skeleton = SkelComp->GetSkeletalMesh()->GetSkeletalMeshData()->Skeleton;

                ImGui::Spacing();
                if (PreviewState->SelectedBoneIndex >= 0 && PreviewState->SelectedBoneIndex < Skeleton.Bones.Num())
                {
                    const FBone& SelectedBone = Skeleton.Bones[PreviewState->SelectedBoneIndex];
                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Selected Bone:");
                    ImGui::Text("  Index: %d", PreviewState->SelectedBoneIndex);
                    ImGui::Text("  Name: %s", SelectedBone.Name.c_str());

                    // 선택 해제 버튼
                    if (ImGui::Button("Deselect Bone"))
                    {
                        PreviewState->SelectedBoneIndex = -1;
                        PreviewState->bBoneLinesDirty = true;
                        PreviewState->World->GetSelectionManager()->ClearSelection();
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No bone selected");
                    ImGui::TextWrapped("Click on a bone in the viewport to select it");
                }
            }
        }
    }

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Step 2: 플레이스홀더 정보
	if (SelectedAnimIndex >= 0)
	{
		// 실제 시퀀스 정보 표시
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentSequence);

		// 파일명 추출
		FString FilePath = CurrentSequence->GetFilePath();
		size_t LastSlash = FilePath.find_last_of("/\\");
		FString FileName = (LastSlash != FString::npos)
			? FilePath.substr(LastSlash + 1)
			: FilePath;

		size_t LastDot = FileName.find_last_of(".");
		if (LastDot != FString::npos)
			FileName = FileName.substr(0, LastDot);

		ImGui::Text("Name: %s", FileName.c_str());
		ImGui::Text("Length: %.2f seconds", CurrentSequence->GetPlayLength());
		if (AnimSequence)
		{
			ImGui::Text("Frames: %d frames", AnimSequence->GetNumberOfFrames());
			ImGui::Text("FPS: %.2f", AnimSequence->GetFrameRate());
		}

		ImGui::Text("Looping: %s", CurrentSequence->IsLooping() ? "Yes" : "No");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 상세 정보
		ImGui::Text("Details:");
		ImGui::BulletText("File Path: %s", FilePath.c_str());

		if (AnimSequence)
		{
			ImGui::BulletText("Bone Tracks: %d", AnimSequence->GetBoneTracks().size());
		}

		ImGui::BulletText("Notify Events: %d", CurrentSequence->GetNotifies().size());

		//// 임시 정보 표시
		//ImGui::Text("Name: MM_Animation_%d", SelectedAnimIndex);
		//ImGui::Text("Length: 2.50 seconds (placeholder)");
		//ImGui::Text("Frames: 75 frames (placeholder)");
		//ImGui::Text("FPS: 30 (placeholder)");

		//ImGui::Spacing();
		//ImGui::Separator();
		//ImGui::Spacing();

		//// 상세 정보
		//ImGui::Text("Details:");
		//ImGui::BulletText("File Path: (not loaded)");
		//ImGui::BulletText("Bone Tracks: 0");
		//ImGui::BulletText("Notify Events: 0");
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::TextWrapped("Select an animation from the list to view details");
		ImGui::PopStyleColor();
	}
}

void SAnimSequenceViewerWindow::RenderPlaybackControls()
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// 중앙 정렬을 위한 계산
	float WindowWidth = ImGui::GetContentRegionAvail().x;
	float ButtonWidth = 40.0f;
	float Spacing = 8.0f;
	float TotalWidth = (ButtonWidth * 5) + (Spacing * 4); // 5개 버튼 + 4개 간격
	float StartX = (WindowWidth - TotalWidth) * 0.5f;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + StartX);

	// 재생 컨트롤 버튼들
	ImGui::BeginGroup();
	{
		// Previous Frame 버튼
		if (ImGui::Button("|<<", ImVec2(ButtonWidth, 30)))
		{
			if (CurrentFrame > 0)
			{
				CurrentFrame--;
				CurrentTime = FrameToTime(CurrentFrame);
				ApplyAnimationPose(); // 포즈 즉시 적용
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Previous Frame");

		ImGui::SameLine(0, Spacing);

		// Play/Pause 버튼
		const char* playButtonText = bIsPlaying ? "||" : ">";
		if (ImGui::Button(playButtonText, ImVec2(ButtonWidth, 30)))
		{
			if (CurrentSequence)
			{
				bIsPlaying = !bIsPlaying;
				UE_LOG("[AnimSequenceViewer] %s", bIsPlaying ? "Playing" : "Paused");
			}
			else
			{
				UE_LOG("[AnimSequenceViewer] Cannot play: No animation selected");
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(bIsPlaying ? "Pause" : "Play");

		ImGui::SameLine(0, Spacing);

		// Stop 버튼
		if (ImGui::Button("[]", ImVec2(ButtonWidth, 30)))
		{
			// 정지 (처음으로)
			bIsPlaying = false;
			CurrentFrame = 0;
			CurrentTime = 0.0f;
			ApplyAnimationPose(); // 첫 프레임 포즈 적용
			UE_LOG("[AnimSequenceViewer] Stopped and reset to frame 0");
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Stop");

		ImGui::SameLine(0, Spacing);

		// Next Frame 버튼
		if (ImGui::Button(">>|", ImVec2(ButtonWidth, 30)))
		{
			if (CurrentFrame < TotalFrames - 1)
			{
				CurrentFrame++;
				CurrentTime = FrameToTime(CurrentFrame);
				ApplyAnimationPose(); // 포즈 즉시 적용
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Next Frame");

		ImGui::SameLine(0, Spacing);

		// Loop Toggle 버튼
		ImVec4 loopColor = bLooping ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, loopColor);
		if (ImGui::Button("Loop", ImVec2(ButtonWidth, 30)))
		{
			bLooping = !bLooping;
		}
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(bLooping ? "Loop: ON" : "Loop: OFF");
	}
	ImGui::EndGroup();

	ImGui::Spacing();

	// 프레임 정보 표시
	ImGui::Text("Frame: %d / %d  |  Time: %.2fs / %.2fs  |  Speed: %.2fx",
		CurrentFrame, TotalFrames, CurrentTime, PlayLength, PlayRate);

	ImGui::Spacing();

	// 재생 속도 슬라이더
	ImGui::SetNextItemWidth(200.0f);
	ImGui::SliderFloat("Playback Speed", &PlayRate, -1.0f, 2.0f, "%.2fx");
}

// ============================================================
// [REMOVED] Legacy deprecated functions
// - RenderTimeline() - replaced by RenderTimelineColumn()
// - RenderNotifyTrackPanel() - replaced by RenderCombinedNotifyTimeline()
// ============================================================
float SAnimSequenceViewerWindow::TimeToPixel(float Time) const
{
	if (PlayLength <= 0.0f) return 0.0f;
	return (Time / PlayLength) * TimelineWidth;
}

float SAnimSequenceViewerWindow::PixelToTime(float PixelX) const
{
	if (TimelineWidth <= 0.0f) return 0.0f;
	float Time = (PixelX / TimelineWidth) * PlayLength;
	return FMath::Clamp(Time, 0.0f, PlayLength);
}

float SAnimSequenceViewerWindow::FrameToTime(int32 Frame) const
{
	if (TotalFrames <= 0) return 0.0f;
	return ((float)Frame / (float)TotalFrames) * PlayLength;
}

int32 SAnimSequenceViewerWindow::TimeToFrame(float Time) const
{
	if (PlayLength <= 0.0f) return 0;
	return (int32)((Time / PlayLength) * (float)TotalFrames);
}

void SAnimSequenceViewerWindow::RenderPreviewViewport(float Height)
{
	// 프리뷰 뷰포트 영역 정의 (경계선 포함)
	ImGui::BeginChild("PreviewViewport", ImVec2(0, Height), true, ImGuiWindowFlags_NoScrollbar);

	// 현재 영역의 화면 좌표와 크기 저장 (OnRenderViewport에서 사용)
	ImVec2 childPos = ImGui::GetWindowPos();
	ImVec2 childSize = ImGui::GetWindowSize();

	// PreviewRect 업데이트
	PreviewRect.Left = childPos.x;
	PreviewRect.Top = childPos.y;
	PreviewRect.Right = childPos.x + childSize.x;
	PreviewRect.Bottom = childPos.y + childSize.y;
	PreviewRect.UpdateMinMax();

	// 뷰포트가 없으면 플레이스홀더 표시
	if (!PreviewState || !PreviewState->Viewport)
	{
		ImGui::Text("Preview Viewport (No ViewerState)");
	}

	ImGui::EndChild();
}

void SAnimSequenceViewerWindow::OnMouseMove(FVector2D MousePos)
{
	if (!PreviewState || !PreviewState->Viewport) return;

	if (PreviewRect.Contains(MousePos))
	{
		FVector2D LocalPos = MousePos - FVector2D(PreviewRect.Left, PreviewRect.Top);
		PreviewState->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
	}
}

void SAnimSequenceViewerWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
	if (!PreviewState || !PreviewState->Viewport) return;

    if (PreviewRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(PreviewRect.Left, PreviewRect.Top);

        // First, always try gizmo picking (pass to viewport)
        PreviewState->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);

        // Left click: if no gizmo was picked, try bone picking
        if (Button == 0 && PreviewState->PreviewActor && PreviewState->CurrentMesh && PreviewState->Client && PreviewState->World)
        {
            // Check if gizmo was picked by checking selection
            UActorComponent* SelectedComp = PreviewState->World->GetSelectionManager()->GetSelectedComponent();

            // Only do bone picking if gizmo wasn't selected
            if (!SelectedComp || !Cast<UBoneAnchorComponent>(SelectedComp))
            {
                // Get camera from viewport client
                ACameraActor* Camera = PreviewState->Client->GetCamera();
                if (Camera)
                {
                    // Get camera vectors
                    FVector CameraPos = Camera->GetActorLocation();
                    FVector CameraRight = Camera->GetRight();
                    FVector CameraUp = Camera->GetUp();
                    FVector CameraForward = Camera->GetForward();

                    // Calculate viewport-relative mouse position
                    FVector2D ViewportMousePos(MousePos.X - PreviewRect.Left, MousePos.Y - PreviewRect.Top);
                    FVector2D ViewportSize(PreviewRect.GetWidth(), PreviewRect.GetHeight());

                    // Generate ray from mouse position
                    FRay Ray = MakeRayFromViewport(
                        Camera->GetViewMatrix(),
                        Camera->GetProjectionMatrix(PreviewRect.GetWidth() / PreviewRect.GetHeight(), PreviewState->Viewport),
                        CameraPos,
                        CameraRight,
                        CameraUp,
                        CameraForward,
                        ViewportMousePos,
                        ViewportSize
                    );

                    // Try to pick a bone
                    float HitDistance;
                    int32 PickedBoneIndex = PreviewState->PreviewActor->PickBone(Ray, HitDistance);

                    UE_LOG("[AnimSequenceViewer] Bone picking result: %d", PickedBoneIndex);

                    if (PickedBoneIndex >= 0)
                    {
                        // Bone was picked
                        PreviewState->SelectedBoneIndex = PickedBoneIndex;
                        PreviewState->bBoneLinesDirty = true;

                        UE_LOG("[AnimSequenceViewer] Bone %d picked successfully", PickedBoneIndex);

                        // Move gizmo to the selected bone
                        PreviewState->PreviewActor->RepositionAnchorToBone(PickedBoneIndex);
                        if (USceneComponent* Anchor = PreviewState->PreviewActor->GetBoneGizmoAnchor())
                        {
                            PreviewState->World->GetSelectionManager()->SelectActor(PreviewState->PreviewActor);
                            PreviewState->World->GetSelectionManager()->SelectComponent(Anchor);
                        }
                    }
                    else
                    {
                        // No bone was picked - clear selection
                        PreviewState->SelectedBoneIndex = -1;
                        PreviewState->bBoneLinesDirty = true;

                        UE_LOG("[AnimSequenceViewer] No bone picked, clearing selection");

                        // Hide gizmo and clear selection
                        if (UBoneAnchorComponent* Anchor = PreviewState->PreviewActor->GetBoneGizmoAnchor())
                        {
                            Anchor->SetVisibility(false);
                            Anchor->SetEditability(false);
                        }
                        PreviewState->World->GetSelectionManager()->ClearSelection();
                    }
                }
            }
        }
    }
}

void SAnimSequenceViewerWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
	if (!PreviewState || !PreviewState->Viewport) return;

	if (PreviewRect.Contains(MousePos))
	{
		FVector2D LocalPos = MousePos - FVector2D(PreviewRect.Left, PreviewRect.Top);
		PreviewState->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
	}
}

void SAnimSequenceViewerWindow::OnRenderViewport()
{
	// 뷰포트 렌더링 (ImGui 렌더링 전에 호출됨)
	if (PreviewState && PreviewState->Viewport && PreviewRect.GetWidth() > 0 && PreviewRect.GetHeight() > 0)
	{
		const uint32 NewStartX = static_cast<uint32>(PreviewRect.Left);
		const uint32 NewStartY = static_cast<uint32>(PreviewRect.Top);
		const uint32 NewWidth = static_cast<uint32>(PreviewRect.Right - PreviewRect.Left);
		const uint32 NewHeight = static_cast<uint32>(PreviewRect.Bottom - PreviewRect.Top);

		// 뷰포트 크기 조정
		PreviewState->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

		// 뷰포트 렌더링 (3D 씬)
		PreviewState->Viewport->Render();
	}
}

// ============================================================
// 통합 Notify+Timeline 패널 (언리얼 스타일)
// ============================================================

void SAnimSequenceViewerWindow::RenderCombinedNotifyTimeline()
{
	// 전체 패널 크기
	float PanelWidth = ImGui::GetContentRegionAvail().x;
	float PanelHeight = ImGui::GetContentRegionAvail().y;

	// 레이아웃 설정
	float HeaderHeight = 60.0f;         // "Notify [+] [-]" 헤더 영역
	float PlaybackHeight = 80.0f;       // Playback Controls 영역 (줄임)
	float ScrollableHeight = PanelHeight - HeaderHeight - PlaybackHeight;

	float NotifyColumnWidth = PanelWidth * 0.15f;  // Notify 트랙 번호 컬럼 15%
	float TimelineColumnWidth = PanelWidth * 0.85f; // Timeline 컬럼 85%
	float RowHeight = 25.0f; // 각 트랙 행 높이

	// ============================================================
	// 1. 헤더 영역 (고정, 스크롤 안됨)
	// ============================================================
	ImGui::Text("Notify");
	ImGui::SameLine();

	// Add Track 버튼
	if (ImGui::Button("[+] Add Track"))
	{
		ImGui::OpenPopup("AddNotifyTrackMenu");
	}

	ImGui::SameLine();

	// Delete Track 버튼
	bool bCanDelete = (SelectedTrackIndex >= 0 && SelectedTrackIndex < NotifyTrackIndices.Num());
	if (!bCanDelete)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("[-] Delete Track"))
	{
		if (bCanDelete && CurrentSequence)
		{
			// 삭제할 트랙의 실제 번호 (NotifyTrackIndices 배열의 값)
			int32 TrackNumberToDelete = NotifyTrackIndices[SelectedTrackIndex];

			UE_LOG("[AnimSequenceViewer] Deleting track index %d (track number %d)",
				SelectedTrackIndex, TrackNumberToDelete);

			// 1. 해당 트랙에 있는 모든 노티파이 삭제
			const TArray<FAnimNotifyEvent>& Notifies = CurrentSequence->GetNotifies();
			const TArray<int32>& DisplayTracks = CurrentSequence->GetNotifyDisplayTrackIndices();

			// 삭제할 노티파이 이름 수집
			TArray<FName> NotifiesToRemove;
			for (size_t i = 0; i < Notifies.size() && i < DisplayTracks.size(); ++i)
			{
				// DisplayTracks[i]는 트랙 번호 (NotifyTrackIndices의 값)
				if (DisplayTracks[i] == TrackNumberToDelete)
				{
					NotifiesToRemove.Add(Notifies[i].NotifyName);
				}
			}

			// 노티파이 삭제
			for (const FName& NotifyName : NotifiesToRemove)
			{
				CurrentSequence->RemoveNotifiesByName(NotifyName);
				UE_LOG("[AnimSequenceViewer] Removed notify '%s' from track %d",
					NotifyName.ToString().c_str(), TrackNumberToDelete);
			}

			// UI 칩도 삭제
			for (int32 i = static_cast<int32>(NotifyChips.Num()) - 1; i >= 0; --i)
			{
				// NotifyChips[i].TrackIndex는 UI 배열 인덱스
				if (NotifyChips[i].TrackIndex == SelectedTrackIndex)
				{
					NotifyChips.RemoveAt(i);
				}
			}

			// 2. 트랙 삭제
			NotifyTrackIndices.RemoveAt(SelectedTrackIndex);
			SelectedTrackIndex = -1;

			// 3. 시퀀스에 트랙 정보 저장
			CurrentSequence->SetNotifyTrackIndices(NotifyTrackIndices);
			// NextNotifyTrackNumber는 유지 (빈 번호 재사용을 위해)

			// Track delete - save to binary file
			FString OriginalPath = CurrentSequence->GetFilePath();
			FString CachePath = ConvertDataPathToCachePath(OriginalPath);
			FString BinPath = CachePath + ".anim.bin";
			CurrentSequence->SaveBinary(BinPath);
			UE_LOG("[AnimSequenceViewer] Saved track changes to %s", BinPath.c_str());
		}
	}

	if (!bCanDelete)
	{
		ImGui::EndDisabled();
	}

	// 드롭다운 메뉴
	if (ImGui::BeginPopup("AddNotifyTrackMenu"))
	{
		if (ImGui::MenuItem("Add Notify Track"))
		{
			// 1부터 시작해서 NotifyTrackIndices에 없는 가장 작은 번호 찾기
			int32 NewTrackNumber = -1;

			// NextNotifyTrackNumber - 1까지 순회 (NextNotifyTrackNumber는 아직 사용 안 된 번호)
			for (int32 i = 1; i < NextNotifyTrackNumber; ++i)
			{
				bool bExists = false;
				for (int32 ExistingTrack : NotifyTrackIndices)
				{
					if (ExistingTrack == i)
					{
						bExists = true;
						break;
					}
				}

				if (!bExists)
				{
					NewTrackNumber = i;
					break;
				}
			}

			// 빈 번호가 없으면 NextNotifyTrackNumber 사용하고 증가
			if (NewTrackNumber == -1)
			{
				NewTrackNumber = NextNotifyTrackNumber;
				NextNotifyTrackNumber++;
			}

			NotifyTrackIndices.Add(NewTrackNumber);

			UE_LOG("[AnimSequenceViewer] Added track %d (NotifyTrackIndices count: %d, NextNotifyTrackNumber: %d)",
				NewTrackNumber, NotifyTrackIndices.Num(), NextNotifyTrackNumber);

			// 시퀀스에 트랙 정보 저장
			if (CurrentSequence)
			{
				CurrentSequence->SetNotifyTrackIndices(NotifyTrackIndices);
				CurrentSequence->SetNextNotifyTrackNumber(NextNotifyTrackNumber);

				// Track 추가 후 바이너리 파일에 저장
				FString OriginalPath = CurrentSequence->GetFilePath();
				FString CachePath = ConvertDataPathToCachePath(OriginalPath);
				FString BinPath = CachePath + ".anim.bin";
				CurrentSequence->SaveBinary(BinPath);
				UE_LOG("[AnimSequenceViewer] Saved track changes to %s", BinPath.c_str());
			}
		}
		ImGui::EndPopup();
	}

	ImGui::Separator();
	ImGui::Spacing();

	// ============================================================
	// 2. 스크롤 가능한 트랙 영역 (Notify + Timeline 함께 스크롤)
	// ============================================================
	// 부모 스크롤 영역 (실제 스크롤바를 표시)
	ImGui::BeginChild("ScrollableTracks", ImVec2(0, ScrollableHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
	{
		// 실제 트랙 개수
		int32 ActualTrackCount = NotifyTrackIndices.Num();

		// 화면을 채우기 위한 최소 트랙 개수 계산
		int32 MinVisibleTracks = std::max(1, (int32)std::ceil(ScrollableHeight / RowHeight));

		// 화면 표시용 트랙 개수 (최소 MinVisibleTracks개는 표시)
		int32 DisplayTrackCount = std::max(ActualTrackCount, MinVisibleTracks);

		// 실제 스크롤 가능한 콘텐츠 높이 (실제 트랙 개수만큼만)
		float ScrollContentHeight = RowHeight * std::max(ActualTrackCount, 1);

		// 화면 표시용 높이 (빈 공간 포함)
		float DisplayHeight = RowHeight * DisplayTrackCount;

		// 부모의 현재 스크롤 위치 가져오기
		float ScrollY = ImGui::GetScrollY();

		// 좌측 Notify 컬럼과 우측 Timeline 컬럼을 같이 렌더링
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		// 화면 위치 고정을 위해 SetCursorScreenPos 사용
		ImVec2 BasePos = ImGui::GetCursorScreenPos();

		// 좌측: Notify 트랙 번호 컬럼 (고정 위치, 스크롤 오프셋 적용)
		ImGui::SetCursorScreenPos(ImVec2(BasePos.x, BasePos.y));
		ImGui::BeginChild("NotifyColumn", ImVec2(NotifyColumnWidth, DisplayHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
		{
			ImGui::SetCursorPosY(-ScrollY);
			RenderNotifyTrackColumn(NotifyColumnWidth, RowHeight, DisplayTrackCount);
		}
		ImGui::EndChild();

		// 우측: Timeline 컬럼 (고정 위치, 스크롤 오프셋 적용)
		ImGui::SetCursorScreenPos(ImVec2(BasePos.x + NotifyColumnWidth, BasePos.y));
		ImGui::BeginChild("TimelineColumn", ImVec2(TimelineColumnWidth, DisplayHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
		{
			ImGui::SetCursorPosY(-ScrollY);
			RenderTimelineColumn(TimelineColumnWidth, RowHeight, DisplayTrackCount);
		}
		ImGui::EndChild();

		ImGui::PopStyleVar();

		// 더미 아이템으로 스크롤 가능한 영역 설정 (실제 트랙 개수만큼만)
		ImGui::Dummy(ImVec2(0, ScrollContentHeight));
	}
	ImGui::EndChild();

	// ============================================================
	// 3. Playback Controls (하단 고정, 스크롤 안됨)
	// ============================================================
	ImGui::Separator();
	RenderPlaybackControls();
}

void SAnimSequenceViewerWindow::RenderNotifyTrackColumn(float ColumnWidth, float RowHeight, int32 VisibleTrackCount)
{
	// Notify 트랙 번호 표시 (좌측 컬럼)
	for (int i = 0; i < NotifyTrackIndices.Num(); i++)
	{
		int32 TrackNumber = NotifyTrackIndices[i];
		char Label[32];
		sprintf_s(Label, "%d", TrackNumber);

		// 트랙 선택 가능
		bool bSelected = (i == SelectedTrackIndex);
		if (ImGui::Selectable(Label, bSelected, 0, ImVec2(0, RowHeight)))
		{
			SelectedTrackIndex = i;
			UE_LOG("[AnimSequenceViewer] Selected track index %d (track number %d)",
				SelectedTrackIndex, TrackNumber);
		}

		// 호버 감지
		if (ImGui::IsItemHovered())
		{
			HoveredTrackIndex = i;
		}
	}

	// 트랙이 없으면 안내 메시지
	if (NotifyTrackIndices.Num() == 0)
	{
		ImGui::TextDisabled("No tracks");
		ImGui::TextDisabled("Click [+]");
	}
}

void SAnimSequenceViewerWindow::RenderTimelineColumn(float ColumnWidth, float RowHeight, int32 VisibleTrackCount)
{
	// Timeline 영역 계산
	TimelineWidth = ColumnWidth - 20.0f;
	float RulerHeight = 35.0f; // 하단 프레임 눈금 영역
	float TotalHeight = (VisibleTrackCount * RowHeight) + RulerHeight;

	ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 CanvasSize(TimelineWidth, TotalHeight);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// 타임라인 배경
	ImVec4 bgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	DrawList->AddRectFilled(CanvasPos,
		ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
		ImGui::ColorConvertFloat4ToU32(bgColor));

	// 각 트랙별 구분선 그리기
	for (int i = 0; i <= VisibleTrackCount; i++)
	{
		float YPos = CanvasPos.y + (i * RowHeight);
		DrawList->AddLine(
			ImVec2(CanvasPos.x, YPos),
			ImVec2(CanvasPos.x + CanvasSize.x, YPos),
			IM_COL32(80, 80, 80, 255), 1.0f);
	}

	// 적응형 눈금 간격 계산
	int targetRulerCount = 10;
	int FrameInterval = (TotalFrames > 0) ? (TotalFrames / targetRulerCount) : 10;

	if (FrameInterval <= 5)
		FrameInterval = 5;
	else if (FrameInterval <= 10)
		FrameInterval = 10;
	else if (FrameInterval <= 20)
		FrameInterval = 20;
	else if (FrameInterval <= 30)
		FrameInterval = 30;
	else if (FrameInterval <= 50)
		FrameInterval = 50;
	else if (FrameInterval <= 100)
		FrameInterval = 100;
	else
		FrameInterval = ((FrameInterval + 99) / 100) * 100;

	// 프레임 눈금 그리기 (세로선)
	for (int frame = 0; frame <= TotalFrames; frame += FrameInterval)
	{
		float Time = FrameToTime(frame);
		float XPos = CanvasPos.x + TimeToPixel(Time);

		// 큰 눈금선 (전체 높이)
		DrawList->AddLine(
			ImVec2(XPos, CanvasPos.y),
			ImVec2(XPos, CanvasPos.y + CanvasSize.y),
			IM_COL32(100, 100, 100, 255), 1.0f);

		// 프레임 번호 표시 (하단)
		char Label[16];
		sprintf_s(Label, "%d", frame);
		DrawList->AddText(
			ImVec2(XPos - 10, CanvasPos.y + CanvasSize.y - 30),
			IM_COL32(200, 200, 200, 255), Label);
	}

	// 작은 눈금
	int smallInterval = (FrameInterval >= 50) ? (FrameInterval / 5) : (FrameInterval / 2);
	if (smallInterval > 0)
	{
		for (int frame = 0; frame <= TotalFrames; frame += smallInterval)
		{
			if (frame % FrameInterval == 0) continue;

			float Time = FrameToTime(frame);
			float XPos = CanvasPos.x + TimeToPixel(Time);

			DrawList->AddLine(
				ImVec2(XPos, CanvasPos.y),
				ImVec2(XPos, CanvasPos.y + CanvasSize.y - RulerHeight),
				IM_COL32(60, 60, 60, 255), 1.0f);
		}
	}

	// 재생 헤드 (Playhead)
	float PlayheadX = CanvasPos.x + TimeToPixel(CurrentTime);

	// 재생 헤드 라인
	DrawList->AddLine(
		ImVec2(PlayheadX, CanvasPos.y),
		ImVec2(PlayheadX, CanvasPos.y + CanvasSize.y),
		IM_COL32(255, 100, 100, 255), 3.0f);

	// 재생 헤드 상단 삼각형
	ImVec2 triangle[3] = {
		ImVec2(PlayheadX, CanvasPos.y),
		ImVec2(PlayheadX - 6, CanvasPos.y + 10),
		ImVec2(PlayheadX + 6, CanvasPos.y + 10)
	};
	DrawList->AddTriangleFilled(triangle[0], triangle[1], triangle[2],
		IM_COL32(255, 100, 100, 255));

	// (markers drawn as per-track chips below)

    // Track hover state and click state for this frame
    int32 NewHoveredNotifyIndex = -1;
    bool bAnyNotifyChipClicked = false; // Track if any notify chip was clicked

    // Draw notify markers per track FIRST (higher priority for clicks)
    // diamond at TriggerTime + duration rectangle
    for (int i = 0; i < NotifyChips.Num(); ++i)
    {
        const FNotifyChip& Chip = NotifyChips[i];
        float chipX = CanvasPos.x + TimeToPixel(Chip.Time);
        float rowTop = CanvasPos.y + (Chip.TrackIndex * RowHeight);
        float rowBottom = rowTop + RowHeight;
        float centerY = rowTop + RowHeight * 0.5f;

        bool bSelected = (i == SelectedNotifyIndex);
        bool bHovered = (i == HoveredNotifyIndex);

        // Enhanced colors for selected state (brighter and more distinct)
        ImU32 colDiamondFill   = bSelected ? IM_COL32(255, 200, 50, 255) : IM_COL32(200, 200, 200, 255);
        ImU32 colDiamondBorder = bSelected ? IM_COL32(255, 150, 0, 255)  : IM_COL32(120, 120, 120, 255);
        ImU32 colRectFill      = bSelected ? IM_COL32(255, 200, 50, 150) : IM_COL32(180, 180, 80, 90);
        ImU32 colRectBorder    = bSelected ? IM_COL32(255, 150, 0, 200)  : IM_COL32(140, 140, 90, 140);

        // Duration rectangle (from TriggerTime to TriggerTime+Duration)
        if (Chip.Duration > 0.0f)
        {
            float startX = chipX;
            float endX = CanvasPos.x + TimeToPixel(Chip.Time + Chip.Duration);
            if (endX < startX) std::swap(endX, startX);
            float rectTop = rowTop + RowHeight * 0.25f;
            float rectBottom = rowBottom - RowHeight * 0.25f;
            DrawList->AddRectFilled(ImVec2(startX, rectTop), ImVec2(endX, rectBottom), colRectFill, 3.0f);
            DrawList->AddRect(ImVec2(startX, rectTop), ImVec2(endX, rectBottom), colRectBorder, 3.0f, 0, 1.5f);
        }

        // Prepare label and calculate sizes
        std::string label = Chip.Name.ToString();
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        float diamondSize = 5.0f; // diamond half size (reduced from 7.0f)
        float padX = 8.0f;
        float padY = 4.0f;

        // Diamond is centered at chipX (clicked position)
        float diamondCenterX = chipX;

        // Oval starts from the middle of the diamond (at chipX)
        float ovalLeft = chipX;
        float ovalW = textSize.x + padX * 2.0f; // only text + padding
        float ovalH = textSize.y + padY * 2.0f;
        float ovalTop = centerY - ovalH * 0.5f;
        float ovalRight = ovalLeft + ovalW;
        float ovalBottom = ovalTop + ovalH;

        // Enhanced colors for selected/hovered states
        ImU32 ovalFill, ovalBorder, textColor;
        float diamondBorderThickness;

        if (bSelected)
        {
            // Bright orange/gold for selected
            ovalFill = IM_COL32(255, 200, 80, 240);
            ovalBorder = IM_COL32(255, 150, 0, 255);
            textColor = IM_COL32(0, 0, 0, 255);
            diamondBorderThickness = 2.5f;
        }
        else if (bHovered)
        {
            // Light blue for hovered
            ovalFill = IM_COL32(120, 150, 200, 220);
            ovalBorder = IM_COL32(80, 120, 180, 255);
            textColor = IM_COL32(255, 255, 255, 255);
            diamondBorderThickness = 2.0f;
        }
        else
        {
            // Default gray
            ovalFill = IM_COL32(70, 70, 80, 200);
            ovalBorder = IM_COL32(40, 40, 45, 255);
            textColor = IM_COL32(200, 200, 200, 255);
            diamondBorderThickness = 1.5f;
        }

        float radius = ovalH * 0.5f; // oval/pill style

        // Draw oval background first (so diamond appears on top)
        DrawList->AddRectFilled(ImVec2(ovalLeft, ovalTop), ImVec2(ovalRight, ovalBottom), ovalFill, radius);
        DrawList->AddRect(ImVec2(ovalLeft, ovalTop), ImVec2(ovalRight, ovalBottom), ovalBorder, radius, 0, 1.5f);

        // Draw text centered in oval
        float textX = ovalLeft + padX;
        float textY = centerY - textSize.y * 0.5f;
        DrawList->AddText(ImVec2(textX, textY), textColor, label.c_str());

        // Draw diamond marker last (on top of everything)
        ImVec2 p0(diamondCenterX, centerY - diamondSize);
        ImVec2 p1(diamondCenterX + diamondSize, centerY);
        ImVec2 p2(diamondCenterX, centerY + diamondSize);
        ImVec2 p3(diamondCenterX - diamondSize, centerY);

        DrawList->AddQuadFilled(p0, p1, p2, p3, colDiamondFill);
        DrawList->AddQuad(p0, p1, p2, p3, colDiamondBorder, diamondBorderThickness);

        // Click handling on entire notify marker (diamond + oval)
        float totalLeft = diamondCenterX - diamondSize;
        float totalTop = std::min(ovalTop, centerY - diamondSize);
        float totalWidth = (ovalRight - totalLeft);
        float totalHeight = std::max(ovalH, diamondSize * 2.0f);

        ImGui::SetCursorScreenPos(ImVec2(totalLeft, totalTop));
        char idbuf[64];
        sprintf_s(idbuf, "NotifyChip##%d", i);
        ImGui::InvisibleButton(idbuf, ImVec2(totalWidth, totalHeight));

        // Drag handling (only for selected notify)
        if (i == SelectedNotifyIndex && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
        {
            bIsDraggingNotify = true;
            DraggingNotifyIndex = i;
            bAnyNotifyChipClicked = true;

            // Calculate new time from mouse position
            ImVec2 MousePos = ImGui::GetMousePos();
            float MouseX = MousePos.x - CanvasPos.x;
            float NewTime = PixelToTime(MouseX);
            NewTime = FMath::Clamp(NewTime, 0.0f, PlayLength);

            // Update NotifyChip (UI)
            NotifyChips[i].Time = NewTime;

            // Update actual AnimNotifyEvent in sequence
            if (CurrentSequence)
            {
                TArray<FAnimNotifyEvent>& Notifies = const_cast<TArray<FAnimNotifyEvent>&>(CurrentSequence->GetNotifies());
                // Find matching notify by name and approximate time
                for (FAnimNotifyEvent& Ev : Notifies)
                {
                    if (Ev.NotifyName == Chip.Name)
                    {
                        Ev.TriggerTime = NewTime;
                        break; // Assume first match
                    }
                }
            }
        }
        // Update selection on click (only if not dragging)
        else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            SelectedNotifyIndex = i;
            bAnyNotifyChipClicked = true; // Mark that a notify was clicked
        }
        // Right-click: open edit/delete context menu
        else if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            SelectedNotifyIndex = i; // Select on right-click
            bAnyNotifyChipClicked = true; // Prevent timeline right-click from triggering

            // 현재 노티파이 이름을 EditNotifyNameBuffer에 복사
            strncpy_s(EditNotifyNameBuffer, sizeof(EditNotifyNameBuffer),
                      Chip.Name.ToString().c_str(), sizeof(EditNotifyNameBuffer) - 1);
            EditNotifyNameBuffer[sizeof(EditNotifyNameBuffer) - 1] = '\0';

            ImGui::OpenPopup("EditNotifyPopup");
        }

        // Update hover state for next frame
        if (ImGui::IsItemHovered())
        {
            NewHoveredNotifyIndex = i;
        }

        // Reset drag state when mouse released
        if (bIsDraggingNotify && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            bIsDraggingNotify = false;
            DraggingNotifyIndex = -1;
        }
    }

    // Apply new hover state for next frame
    HoveredNotifyIndex = NewHoveredNotifyIndex;

    // ============================================================
    // Edit/Delete Notify Popup (appears when right-clicking a notify)
    // ============================================================
    if (ImGui::BeginPopup("EditNotifyPopup"))
    {
        if (SelectedNotifyIndex >= 0 && SelectedNotifyIndex < NotifyChips.Num())
        {
            FNotifyChip& ChipToEdit = NotifyChips[SelectedNotifyIndex];
            ImGui::Text("Edit Notify");
            ImGui::Separator();
            ImGui::Spacing();

            // 노티파이 이름 수정
            ImGui::Text("Name:");
            ImGui::SetNextItemWidth(250.0f);
            ImGui::InputText("##EditNotifyName", EditNotifyNameBuffer, sizeof(EditNotifyNameBuffer));

            ImGui::Spacing();
            ImGui::Text("Time: %.3f s", ChipToEdit.Time);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Rename 버튼
            if (ImGui::Button("Rename", ImVec2(100, 0)) && EditNotifyNameBuffer[0] != '\0')
            {
                FName OldName = ChipToEdit.Name;
                FName NewName = FName(EditNotifyNameBuffer);

                // 이름이 실제로 변경된 경우만 처리
                if (OldName != NewName)
                {
                    // CurrentSequence에서 노티파이 이름 변경
                    if (CurrentSequence)
                    {
                        TArray<FAnimNotifyEvent>& Notifies = const_cast<TArray<FAnimNotifyEvent>&>(CurrentSequence->GetNotifies());

                        // 해당 노티파이 찾아서 이름 변경
                        for (FAnimNotifyEvent& Ev : Notifies)
                        {
                            if (Ev.NotifyName == OldName)
                            {
                                Ev.NotifyName = NewName;
                                UE_LOG("[AnimSequenceViewer] Renamed notify '%s' to '%s'",
                                    OldName.ToString().c_str(), NewName.ToString().c_str());
                                break;
                            }
                        }

                        // 바이너리 파일에 저장
                        FString OriginalPath = CurrentSequence->GetFilePath();
                        FString CachePath = ConvertDataPathToCachePath(OriginalPath);
                        FString BinPath = CachePath + ".anim.bin";
                        CurrentSequence->SaveBinary(BinPath);
                        UE_LOG("[AnimSequenceViewer] Saved notify changes to %s", BinPath.c_str());
                    }

                    // UI Chip도 업데이트
                    ChipToEdit.Name = NewName;
                }

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            // Delete 버튼
            if (ImGui::Button("Delete", ImVec2(100, 0)))
            {
                // Remove from CurrentSequence (actual data)
                if (CurrentSequence)
                {
                    CurrentSequence->RemoveNotifiesByName(ChipToEdit.Name);
                    UE_LOG("[AnimSequenceViewer] Deleted notify '%s' from sequence '%s'",
                        ChipToEdit.Name.ToString().c_str(), CurrentSequence->GetFilePath().c_str());

                    // Notify 삭제 후 바이너리 파일에 저장
                    FString OriginalPath = CurrentSequence->GetFilePath();
                    FString CachePath = ConvertDataPathToCachePath(OriginalPath);
                    FString BinPath = CachePath + ".anim.bin";
                    CurrentSequence->SaveBinary(BinPath);
                    UE_LOG("[AnimSequenceViewer] Saved notify changes to %s", BinPath.c_str());
                }

                // Remove from NotifyChips (UI)
                NotifyChips.RemoveAt(SelectedNotifyIndex);

                // Reset selection
                SelectedNotifyIndex = -1;

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            // Cancel 버튼
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    // ============================================================
    // Timeline button interaction (AFTER notify chips)
    // Only process timeline clicks if no notify chip was clicked
    // ============================================================

    // Timeline invisible button for playhead dragging and timeline clicks
    // Only process if not dragging a notify
    if (!bIsDraggingNotify)
    {
        ImGui::SetCursorScreenPos(CanvasPos);
        ImGui::InvisibleButton("TimelineButton", CanvasSize);

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
        {
            bIsDraggingPlayhead = true;
            ImVec2 MousePos = ImGui::GetMousePos();
            float ClickX = MousePos.x - CanvasPos.x;
            CurrentTime = PixelToTime(ClickX);
            CurrentFrame = TimeToFrame(CurrentTime);
            bIsPlaying = false;
            ApplyAnimationPose(); // 드래그 중 포즈 실시간 업데이트
        }
        else if (!bAnyNotifyChipClicked && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            // Only move playhead if no notify chip was clicked
            ImVec2 MousePos = ImGui::GetMousePos();
            float ClickX = MousePos.x - CanvasPos.x;
            CurrentTime = PixelToTime(ClickX);
            CurrentFrame = TimeToFrame(CurrentTime);
            bIsPlaying = false;
            ApplyAnimationPose(); // 클릭 시 포즈 즉시 업데이트
        }
        else if (!bAnyNotifyChipClicked && ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            // Open Add Notify popup at clicked time on the hovered track
            // Only if no notify chip was right-clicked
            ImVec2 MousePos = ImGui::GetMousePos();
            float ClickX = MousePos.x - CanvasPos.x;
            PendingNotifyTime = PixelToTime(ClickX);
            // Compute track index from Y (content already offset by SetCursorPosY(-ScrollY))
            float localY = (MousePos.y - CanvasPos.y);
            PendingNotifyTrack = (int32)std::floor(localY / RowHeight);
            strncpy_s(NotifyNameBuffer, sizeof(NotifyNameBuffer), "", 1);
            ImGui::OpenPopup("AddNotifyPopup");
        }
        else
        {
            bIsDraggingPlayhead = false;
        }
    }

    // 커서 이동
    ImGui::SetCursorScreenPos(ImVec2(CanvasPos.x, CanvasPos.y + CanvasSize.y + 5));
    ImGui::Spacing();

    // Add Notify popup UI
    if (ImGui::BeginPopup("AddNotifyPopup"))
    {
        ImGui::Text("Add AnimNotify");
        ImGui::Separator();
        ImGui::Text("Time: %.3f s", PendingNotifyTime);
        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputText("Name", NotifyNameBuffer, sizeof(NotifyNameBuffer));
        if (ImGui::Button("Add") && CurrentSequence && NotifyNameBuffer[0] != '\0')
        {
            // Add a chip on the chosen track for UI placement
            int32 TrackIdx = PendingNotifyTrack;
            if (TrackIdx < 0) TrackIdx = 0;
            // Clamp to actual track count if any
            if (NotifyTrackIndices.Num() == 0)
            {
                // If no tracks exist, create one so chip is visible
                NotifyTrackIndices.Add(NextNotifyTrackNumber++);
                TrackIdx = 0;

                // 시퀀스에 트랙 정보 저장
                if (CurrentSequence)
                {
                    CurrentSequence->SetNotifyTrackIndices(NotifyTrackIndices);
                    CurrentSequence->SetNextNotifyTrackNumber(NextNotifyTrackNumber);
                }
            }
            else if (TrackIdx >= NotifyTrackIndices.Num())
            {
                TrackIdx = NotifyTrackIndices.Num() - 1;
            }

            FAnimNotifyEvent Ev;
            Ev.TriggerTime = std::max(0.0f, std::min(PendingNotifyTime, PlayLength));
            Ev.Duration = 0.0f;
            Ev.NotifyName = FName(NotifyNameBuffer);
            CurrentSequence->AddNotify(Ev, TrackIdx);  // 트랙 인덱스와 함께 추가

            // DEBUG: Log notify addition
            UE_LOG("[AnimSequenceViewer] Added notify '%s' to sequence '%s' at time %.3f on track %d",
                NotifyNameBuffer, CurrentSequence->GetFilePath().c_str(), Ev.TriggerTime, TrackIdx);

            // Notify 추가 후 바이너리 파일에 저장
            FString OriginalPath = CurrentSequence->GetFilePath();
            FString CachePath = ConvertDataPathToCachePath(OriginalPath);
            FString BinPath = CachePath + ".anim.bin";
            CurrentSequence->SaveBinary(BinPath);
            UE_LOG("[AnimSequenceViewer] Saved notify changes to %s", BinPath.c_str());

            FNotifyChip Chip;
            Chip.Time = Ev.TriggerTime;
            Chip.Duration = Ev.Duration;
            Chip.Name = Ev.NotifyName;
            Chip.TrackIndex = TrackIdx;
            NotifyChips.Add(Chip);
            SelectedNotifyIndex = static_cast<int32>(NotifyChips.Num()) - 1;
            // Ensure the newly added chip's row is visible
            float rowTopForScroll = CanvasPos.y + (TrackIdx * RowHeight);
            ImGui::SetScrollFromPosY(rowTopForScroll + RowHeight * 0.5f, 0.5f);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}




