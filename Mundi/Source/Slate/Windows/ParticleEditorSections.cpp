#include "pch.h"
#include <algorithm>
#include <filesystem>
#include "ParticleEditorSections.h"
#include "SParticleEditorWindow.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorState.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "Source/Runtime/AssetManagement/Texture.h"
#include "Source/Editor/PlatformProcess.h"
#include "Grid/GridActor.h"

void FParticleEditorMenuBarSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Particle System"))
            {
                // TODO: Create new particle system
            }
            if (ImGui::MenuItem("Open..."))
            {
                // TODO: Open particle system
            }
            if (ImGui::MenuItem("Save"))
            {
                // TODO: Save particle system
            }
            if (ImGui::MenuItem("Save As..."))
            {
                // TODO: Save as particle system
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close"))
            {
                Context.Window.Close();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
            {
                // TODO: Undo
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y"))
            {
                // TODO: Redo
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Emitter"))
            {
                // TODO: Delete selected emitter
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emitter"))
        {
            if (ImGui::MenuItem("Add Sprite Emitter"))
            {
                // TODO: Add sprite emitter
            }
            if (ImGui::MenuItem("Add Mesh Emitter"))
            {
                // TODO: Add mesh emitter
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate Emitter"))
            {
                // TODO: Duplicate selected emitter
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ActiveState)
            {
                if (ImGui::MenuItem("Show Grid", nullptr, &ActiveState->bShowGrid))
                {
                    if (ActiveState->World && ActiveState->World->GetGridActor())
                    {
                        ActiveState->World->GetGridActor()->SetGridVisible(ActiveState->bShowGrid);
                    }
                }
                if (ImGui::MenuItem("Show Bounds", nullptr, &ActiveState->bShowBounds))
                {
                    // TODO: Toggle bounds visibility
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Reset Camera"))
                {
                    // TODO: Reset camera
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

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
            // TODO(PYB): 저장 시스템
            // Sample:
            // if (ActiveState)
            // {
            //     const FString SavePath = "Data/ParticleSystems/MySystem.particle";
            //     ParticleEditorSerializer::SaveToFile(*ActiveState, SavePath);
            //     UE_LOG("ParticleEditor: Saved particle system to %s", SavePath.c_str());
            // }
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarOpen", IconLoad, "Open",
            "현재 파티클 시스템 애셋의 위치를 찾습니다.", unifiedButtonWidth))
        {
            const FWideString BaseDir = L"Data/ParticleSystems";
            const FWideString Extension = L".particle";
            const FWideString Description = L"Particle System";
            std::filesystem::path SelectedFile = FPlatformProcess::OpenLoadFileDialog(BaseDir, Extension, Description);
            if (!SelectedFile.empty())
            {
                Context.Window.CreateNewTab();
                // TODO(PYB): 로드 시스템
                // Sample:
                // if (ParticleEditorState* NewState = Context.Window.GetActiveState())
                // {
                //     ParticleEditorSerializer::LoadFromFile(*NewState, SelectedFile);
                //     UE_LOG("ParticleEditor: Loaded particle system %s", SelectedFile.string().c_str());
                // }
            }
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarResetSimul", IconResetSimul, "Restart Sim",
            "뷰포트 창의 시뮬레이션을 리셋시킵니다. ", unifiedButtonWidth))
        {
            // TODO(PYB): 파티클 나오면 아마 만들기 가능함
            // Sample:
            // if (ActiveState && ActiveState->CurrentSystemComponent)
            // {
            //     UParticleSystemComponent* PSC = ActiveState->CurrentSystemComponent;
            //     PSC->ResetSystem();
            //     if (UParticleEmitterInstance* Root = PSC->GetRootEmitter())
            //     {
            //         Root->WarmUp(ActiveState->WarmupTime);
            //     }
            //     UE_LOG("ParticleEditor: Simulation restarted");
            // }
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarResetLevel", IconResetLevel, "Restart Level",
            "레벨에 있는 파티클 시스템과, 해당 시스템의 인스턴스를 리셋시킵니다.", unifiedButtonWidth))
        {
            // TODO(PYB): 파티클 나오면 아마 만들기 가능함
            // Sample:
            // if (ActiveState && ActiveState->World)
            // {
            //     UWorld* PreviewWorld = ActiveState->World;
            //     PreviewWorld->ClearPreviewActors();
            //     if (UParticleSystem* SystemAsset = ActiveState->LoadedSystem)
            //     {
            //         UParticleSystemComponent* PSC = PreviewWorld->SpawnComponent<UParticleSystemComponent>();
            //         PSC->SetTemplate(SystemAsset);
            //         PSC->ActivateSystem(true);
            //         ActiveState->CurrentSystemComponent = PSC;
            //     }
            //     PreviewWorld->ResetPreviewCamera();
            //     ActiveState->ActiveLODLevel = 0;
            //     UE_LOG("ParticleEditor: Preview level reset");
            // }
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
            // TODO(PYB): 파티클 시스템이 연결되면 텍스쳐 이미지 포인터로 생성해서 연결되도록 해야할 것 같은데
            //            생각보다 공을 들여야 할 부분, 예외 처리라던지..
        }

        ImGui::SameLine(0.0f, 24.0f);

        if (DrawIconButton("##ParticleToolbarBound", IconBound, "Bounds",
            "뷰포트 패널에서 파티클 시스템의 현재 바운드 표시 토글입니다.", unifiedButtonWidth))
        {
            // TODO(PYB): 바운드 기능이 연결된다면 bool 값을 통해 On/Off 하는 로직으로 처리
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

void FParticleEditorViewportSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Viewport");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    // ActiveState가 있으면 저장된 배경색 사용, 없으면 기본값 사용
    ImVec4 bgColor = ActiveState ?
        ImVec4(ActiveState->ViewportBackgroundColor[0],
               ActiveState->ViewportBackgroundColor[1],
               ActiveState->ViewportBackgroundColor[2],
               ActiveState->ViewportBackgroundColor[3])
        : ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
    ImGui::BeginChild("ViewportContent", viewportSize, false, ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::Text("Particle Preview Viewport");
        ImGui::Text("Size: %.0fx%.0f", viewportSize.x, viewportSize.y);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void FParticleEditorEmitterSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.50f, 0.35f, 0.8f));
    ImGui::Text("Emitters");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Particle System Emitters:");
    ImGui::Spacing();

    ImGui::BeginChild("EmitterList", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No emitters in the system.");
        ImGui::TextWrapped("Use 'Add Emitter' button to add emitters.");
        ImGui::PopStyleColor();

        // TODO: Display list of emitters once particle systems are wired up
        (void)ActiveState;
    }
    ImGui::EndChild();
}

void FParticleEditorDetailSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.50f, 0.35f, 0.25f, 0.8f));
    ImGui::Text("Details");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Emitter Properties:");
    ImGui::Spacing();

    ImGui::BeginChild("DetailContent", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Select an emitter to edit its properties.");
        ImGui::PopStyleColor();

        // TODO: Display emitter properties when selection is available
        (void)ActiveState;
    }
    ImGui::EndChild();
}

void FParticleEditorCurveSection::Draw(const FParticleEditorSectionContext& Context)
{
    (void)Context;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.25f, 0.50f, 0.8f));
    ImGui::Text("Curve Editor");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Property Curves:");
    ImGui::Spacing();

    ImGui::BeginChild("CurveEditorContent", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Curve editor for particle properties over time.");
        ImGui::TextWrapped("Select a property from Details panel to edit its curve.");
        ImGui::PopStyleColor();

        // TODO: Implement the actual curve editor overlay
    }
    ImGui::EndChild();
}
