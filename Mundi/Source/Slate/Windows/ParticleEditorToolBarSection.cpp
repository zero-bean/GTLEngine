#include "pch.h"
#include <algorithm>
#include <filesystem>
#include "ParticleEditorSections.h"
#include "SParticleEditorWindow.h"
#include "ParticleEditorState.h"
#include "World.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "PlatformProcess.h"
#include "Grid/GridActor.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleSpawn.h"
#include "Source/Slate/Widgets/PropertyRenderer.h"

FParticleEditorToolBarSection::FParticleEditorToolBarSection() = default;

void FParticleEditorToolBarSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    EnsureIconsLoaded();

    if (ActiveState && ActiveState->World && ActiveState->World->GetGridActor())
    {
        ActiveState->World->GetGridActor()->SetAxisVisible(ActiveState->bShowAxis);
        ActiveState->World->GetGridActor()->SetGridVisible(ActiveState->bShowGrid);
    }

    const float labelHeight = ImGui::GetFontSize();
    const float toolbarHeight = IconSize + labelHeight + 18.0f;

    // 모든 라벨의 최대 너비를 계산
    const char* labels[] = { "Save", "Open", "Restart Sim",
        "Restart Level", "Background", "Thumbnail", "Bounds", "Axis", "Grid" };
    float maxLabelWidth = 0.0f;
    for (const char* label : labels)
    {
        float width = ImGui::CalcTextSize(label).x;
        maxLabelWidth = std::max(maxLabelWidth, width);
    }
    const float unifiedButtonWidth = std::max(IconSize, maxLabelWidth) + 16.0f;

    ImGui::BeginChild("ToolBar", ImVec2(0, toolbarHeight), true, ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(24.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));

        if (DrawIconButton("##ParticleToolbarSave", IconSave, "Save",
            "현재 파티클 시스템 애셋을 저장합니다.", unifiedButtonWidth))
        {
            if (ActiveState && ActiveState->CurrentParticleSystem)
            {
                // Save As 다이얼로그 표시
                const FWideString BaseDir = L"Data/Particles";
                const FWideString Extension = L".particle";
                const FWideString Description = L"Particle System";

                // 기존 파일명이 있으면 그것을 기본값으로 사용
                FString DefaultName = "NewParticleSystem";
                if (!ActiveState->LoadedParticleSystemPath.empty())
                {
                    std::filesystem::path p(ActiveState->LoadedParticleSystemPath);
                    DefaultName = p.stem().string();
                }
                else
                {
                    FString TabName = ActiveState->Name.ToString();
                    if (!TabName.empty() && TabName != "Untitled Particle System")
                    {
                        DefaultName = TabName;
                    }
                }

                std::wstring wDefaultName(DefaultName.begin(), DefaultName.end());
                std::filesystem::path SelectedFile = FPlatformProcess::OpenSaveFileDialog(BaseDir, Extension, Description, wDefaultName);
                if (!SelectedFile.empty())
                {
                    FString SavePath = SelectedFile.string();
                    if (ActiveState->CurrentParticleSystem->Save(SavePath))
                    {
                        ActiveState->LoadedParticleSystemPath = SavePath;

                        // 탭 이름을 파일명으로 업데이트
                        std::filesystem::path filePath(SavePath);
                        ActiveState->Name = filePath.stem().string();
                        UPropertyRenderer::RefreshParticleSystemCache();

                        UE_LOG("Particle system saved successfully: %s", SavePath.c_str());
                    }
                    else
                    {
                        UE_LOG("Failed to save particle system: %s", SavePath.c_str());
                    }
                }
            }
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarOpen", IconLoad, "Open",
            "파티클 시스템 애셋을 불러옵니다.", unifiedButtonWidth))
        {
            const FWideString BaseDir = L"Data/Particles";
            const FWideString Extension = L".particle";
            const FWideString Description = L"Particle System";
            std::filesystem::path SelectedFile = FPlatformProcess::OpenLoadFileDialog(BaseDir, Extension, Description);
            if (!SelectedFile.empty())
            {
                FString LoadPath = SelectedFile.string();

                // 새 파티클 시스템 생성
                UParticleSystem* LoadedSystem = NewObject<UParticleSystem>();
                if (LoadedSystem && LoadedSystem->Load(LoadPath, nullptr))
                {
                    // 새 탭 생성 - CreateNewTab이 내부적으로 ActiveState를 업데이트함
                    Context.Window.CreateNewTab();

                    // 새로 생성된 탭의 ActiveState 가져오기
                    ParticleEditorState* NewState = Context.Window.GetActiveState();
                    if (NewState)
                    {
                        // 기존 시스템이 있다면 제거 (소유권이 있는 경우만)
                        if (NewState->CurrentParticleSystem && NewState->bOwnsParticleSystem)
                        {
                            DeleteObject(NewState->CurrentParticleSystem);
                        }

                        NewState->CurrentParticleSystem = LoadedSystem;
                        NewState->bOwnsParticleSystem = true; // 로드한 시스템은 우리가 소유
                        NewState->LoadedParticleSystemPath = LoadPath;

                        // 첫 번째 Emitter 선택 (있으면)
                        if (LoadedSystem->Emitters.Num() > 0)
                        {
                            NewState->SelectedEmitterIndex = 0;
                        }
                        else
                        {
                            NewState->SelectedEmitterIndex = -1;
                        }

                        // 탭 이름을 파일명으로 업데이트
                        std::filesystem::path filePath(LoadPath);
                        NewState->Name = filePath.stem().string();

                        // 프리뷰 액터 업데이트
                        NewState->UpdatePreviewParticleSystem();

                        UE_LOG("Particle system loaded successfully: %s", LoadPath.c_str());
                    }
                    else
                    {
                        // 탭 생성 실패 시 로드된 시스템 정리
                        DeleteObject(LoadedSystem);
                        UE_LOG("Failed to create new tab for particle system");
                    }
                }
                else
                {
                    // 로드 실패 시 생성된 오브젝트 정리
                    if (LoadedSystem)
                    {
                        DeleteObject(LoadedSystem);
                    }
                    UE_LOG("Failed to load particle system: %s", LoadPath.c_str());
                }
            }
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarResetSimul", IconResetSimul, "Restart Sim",
            "뷰포트 창의 시뮬레이션을 리셋시킵니다. ", unifiedButtonWidth))
        {
            // TODO(PYB): 파티클 시뮬레이션 리셋
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarResetLevel", IconResetLevel, "Restart Level",
            "레벨에 있는 파티클 시스템과, 해당 시스템의 인스턴스를 리셋시킵니다.", unifiedButtonWidth))
        {
            // TODO(PYB): 레벨 리셋
        }

        ImGui::SameLine(0.0f, 24.0f);

        ImVec2 colorButtonMin(0.0f, 0.0f);
        ImVec2 colorButtonMax(0.0f, 0.0f);
        if (DrawIconButton("##ParticleToolbarColor", IconColor, "Background",
            "뷰포트 패널 카메라에서 보는 화면의 배경색을 조정합니다",
            unifiedButtonWidth, &colorButtonMin, &colorButtonMax))
        {
            if (Context.bShowColorPicker)
            {
                *Context.bShowColorPicker = true;
            }
            FVector2D spawnPos(colorButtonMin.x, colorButtonMax.y + 4.0f);
            Context.Window.SetColorPickerSpawnPosition(spawnPos);
            Context.Window.RequestColorPickerFocus();
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarThumbnail", IconThumbnail, "Thumbnail",
            "뷰포트 패널 카메라에서 보는 화면을 콘텐츠 브라우저에서 파티클 시스템에 쓸 썸네일 이미지로 저장합니다",
            unifiedButtonWidth))
        {
            // TODO(PYB): 썸네일 저장
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarBound", IconBound, "Bounds",
            "뷰포트 패널에서 파티클 시스템의 현재 바운드 표시 토글입니다.", unifiedButtonWidth))
        {
            // TODO(PYB): 바운드 토글
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarAxis", IconAxis, "Axis",
            "파티클 뷰포트 창에서의 원점 축 표시 토글입니다.", unifiedButtonWidth))
        {
            if (ActiveState)
            {
                ActiveState->bShowAxis = !ActiveState->bShowAxis;
                if (ActiveState->World && ActiveState->World->GetGridActor())
                {
                    ActiveState->World->GetGridActor()->SetAxisVisible(ActiveState->bShowAxis);
                }
            }
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarLOD", IconLOD, "LOD",
            "LOD를 선택할 수 있습니다.", unifiedButtonWidth))
        {
            ImGui::OpenPopup("ParticleEditorLODPopup");
        }

        if (ActiveState && ImGui::BeginPopup("ParticleEditorLODPopup"))
        {
            const char* LODLabels[] = { "LOD 0 (Highest)", "LOD 1", "LOD 2", "LOD 3 (Lowest)" };
            const int32 LODCount = static_cast<int32>(IM_ARRAYSIZE(LODLabels));
            for (int32 lodIndex = 0; lodIndex < LODCount; ++lodIndex)
            {
                const bool bIsActiveLOD = (ActiveState->ActiveLODLevel == lodIndex);
                if (bIsActiveLOD)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));
                }

                if (ImGui::Selectable(LODLabels[lodIndex], bIsActiveLOD))
                {
                    ActiveState->ActiveLODLevel = lodIndex;
                    // TODO(PYB): LOD 버전 별로 구현이 되면, 선택이 되도록 만들어야 함
                }

                if (bIsActiveLOD)
                {
                    ImGui::PopStyleColor();
                }
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
    }
    ImGui::EndChild();
}

void FParticleEditorToolBarSection::EnsureIconsLoaded()
{
    UResourceManager& ResourceManager = UResourceManager::GetInstance();
    if (!IconLoad)
    {
        IconLoad = ResourceManager.Load<UTexture>("Data/Icon/Toolbar_Load.png");
    }
    if (!IconSave)
    {
        IconSave = ResourceManager.Load<UTexture>("Data/Icon/Toolbar_Save.png");
    }
    if (!IconResetSimul)
    {
        IconResetSimul = ResourceManager.Load<UTexture>("Data/Icon/Viewport_Toolbar_ReStart.png");
    }
    if (!IconResetLevel)
    {
        IconResetLevel = ResourceManager.Load<UTexture>("Data/Icon/Viewport_Toolbar_Rotate.png");
    }
    if (!IconColor)
    {
        IconColor = ResourceManager.Load<UTexture>("Data/Icon/Viewport_Toolbar_Color.png");
    }
    if (!IconThumbnail)
    {
        IconThumbnail = ResourceManager.Load<UTexture>("Data/Icon/Viewport_Toolbar_Thumbnail.png");
    }
    if (!IconBound)
    {
        IconBound = ResourceManager.Load<UTexture>("Data/Icon/Viewport_Toolbar_Bound.png");
    }
    if (!IconAxis)
    {
        IconAxis = ResourceManager.Load<UTexture>("Data/Icon/Viewport_Toolbar_Grid.png");
    }
    if (!IconLOD)
    {
        IconLOD = ResourceManager.Load<UTexture>("Data/Icon/Viewport_Toolbar_LOD.png");
    }
}

bool FParticleEditorToolBarSection::DrawIconButton(const char* Id, UTexture* Icon, const char* Label, const char* Tooltip, float ButtonWidth, ImVec2* OutRectMin, ImVec2* OutRectMax)
{
    bool bClicked = false;
    const bool bHasLabel = (Label && *Label);

    // 통일된 버튼 너비 사용
    const float columnWidth = ButtonWidth;

    // 텍스트 길이 측정
    float textWidth = 0.0f;
    if (bHasLabel)
    {
        textWidth = ImGui::CalcTextSize(Label).x;
    }

    ImGui::BeginGroup();
    const ImVec2 startPos = ImGui::GetCursorPos();

    // 아이콘을 중앙에서 살짝 왼쪽으로 이동
    const float iconCenterOffset = (columnWidth - IconSize) * 0.5f;
    const float iconOffsetX = iconCenterOffset * 0.85f;
    ImGui::SetCursorPos(ImVec2(startPos.x + iconOffsetX, startPos.y));
    const ImVec2 iconSize(IconSize, IconSize);
    if (Icon && Icon->GetShaderResourceView())
    {
        bClicked = ImGui::ImageButton(Id, (void*)Icon->GetShaderResourceView(), iconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0));
    }
    else
    {
        bClicked = ImGui::Button(Id, iconSize);
    }
    if (OutRectMin)
    {
        *OutRectMin = ImGui::GetItemRectMin();
    }
    if (OutRectMax)
    {
        *OutRectMax = ImGui::GetItemRectMax();
    }

    if (Tooltip && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", Tooltip);
    }

    float usedHeight = IconSize;
    if (bHasLabel)
    {
        // 라벨을 아이콘 아래에 배치
        const float labelY = startPos.y + IconSize + 4.0f;

        // 텍스트를 정확히 중앙 정렬
        const float labelOffsetX = (columnWidth - textWidth) * 0.5f;
        ImGui::SetCursorPos(ImVec2(startPos.x + labelOffsetX, labelY));
        ImGui::Text("%s", Label);

        const ImVec2 textSize = ImGui::GetItemRectSize();
        usedHeight += 4.0f + textSize.y;
    }

    ImGui::SetCursorPos(startPos);
    ImGui::Dummy(ImVec2(columnWidth, usedHeight));
    ImGui::EndGroup();
    return bClicked;
}
