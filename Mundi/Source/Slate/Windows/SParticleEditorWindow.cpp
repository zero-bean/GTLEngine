#include "pch.h"
#include <algorithm>
#include "SParticleEditorWindow.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorState.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorBootstrap.h"

SParticleEditorWindow::SParticleEditorWindow()
{
    ViewportRect = FRect(0, 0, 0, 0);
    EmitterRect = FRect(0, 0, 0, 0);
    DetailRect = FRect(0, 0, 0, 0);
    CurveEditorRect = FRect(0, 0, 0, 0);
}

SParticleEditorWindow::~SParticleEditorWindow()
{
    // Clean up tabs if any
    for (int i = 0; i < Tabs.Num(); ++i)
    {
        ParticleEditorState* State = Tabs[i];
        ParticleEditorBootstrap::DestroyEditorState(State);
    }
    Tabs.Empty();
    ActiveState = nullptr;
}

bool SParticleEditorWindow::Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice)
{
    World = InWorld;
    Device = InDevice;

    SetRect(StartX, StartY, StartX + Width, StartY + Height);

    // Create first tab/state
    OpenNewTab("Particle Editor 1");
    if (ActiveState && ActiveState->Viewport)
    {
        ActiveState->Viewport->Resize((uint32)StartX, (uint32)StartY, (uint32)Width, (uint32)Height);
    }

    bRequestFocus = true;
    return true;
}

void SParticleEditorWindow::OnRender()
{
    // If window is closed, don't render any UI and clear viewport rect so render thread won't draw leftovers
    if (!bIsOpen)
    {
        ViewportRect = FRect(0, 0, 0, 0);
        ViewportRect.UpdateMinMax();
        return;
    }

    // Parent window with solid background
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings;

    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(Rect.Left, Rect.Top));
        ImGui::SetNextWindowSize(ImVec2(Rect.GetWidth(), Rect.GetHeight()));
        bInitialPlacementDone = true;
    }

    if (bRequestFocus)
    {
        ImGui::SetNextWindowFocus();
    }

    bool bEditorVisible = false;
    if (ImGui::Begin("Particle Editor", &bIsOpen, flags))
    {
        bEditorVisible = true;

        // Render tab bar and switch active state
        if (ImGui::BeginTabBar("ParticleEditorTabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable))
        {
            for (int i = 0; i < Tabs.Num(); ++i)
            {
                ParticleEditorState* State = Tabs[i];
                bool open = true;
                if (ImGui::BeginTabItem(State->Name.ToString().c_str(), &open))
                {
                    ActiveTabIndex = i;
                    ActiveState = State;
                    ImGui::EndTabItem();
                }
                if (!open)
                {
                    CloseTab(i);
                    break;
                }
            }
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            {
                char label[32];
                sprintf_s(label, "Particle Editor %d", Tabs.Num() + 1);
                OpenNewTab(label);
            }
            ImGui::EndTabBar();
        }

        /* Section 1: 메뉴바 */
        DrawMenuBar();

        // Update window rect
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        Rect.Left = pos.x;
        Rect.Top = pos.y;
        Rect.Right = pos.x + size.x;
        Rect.Bottom = pos.y + size.y;
        Rect.UpdateMinMax();

        /* Section 2: 툴바 */
        DrawToolBar();

        // Get available content region
        ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        float totalWidth = std::max(contentAvail.x, 0.0f);
        float totalHeight = std::max(contentAvail.y, 0.0f);

        // Resolve initial sizes from ratios and clamp
        const float availableWidth = std::max(0.0f, totalWidth - SplitterThickness);
        float leftWidth = availableWidth * VerticalSplitRatio;
        float rightWidth = availableWidth - leftWidth;

        const float minColumns = MinColumnWidth * 2.0f;
        if (availableWidth >= minColumns)
        {
            leftWidth = std::clamp(leftWidth, MinColumnWidth, availableWidth - MinColumnWidth);
            rightWidth = availableWidth - leftWidth;
        }
        else if (availableWidth > 0.0f)
        {
            leftWidth = availableWidth * 0.5f;
            rightWidth = availableWidth - leftWidth;
        }
        else
        {
            leftWidth = rightWidth = 0.0f;
        }

        // Remove spacing between panels
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // --- Section 3 & 4: Left Column (Viewport / Emitters) ---
        const float leftColumnWidthForDraw = leftWidth > 0.0f ? leftWidth : CollapsedPanelEpsilon;
        const float totalHeightForDraw = totalHeight > 0.0f ? totalHeight : SplitterThickness;
        ImGui::BeginChild("LeftColumn", ImVec2(leftColumnWidthForDraw, totalHeightForDraw), false, ImGuiWindowFlags_NoScrollbar);
        {
            const float availableLeftHeight = std::max(0.0f, totalHeight - SplitterThickness);
            float viewportHeight = availableLeftHeight * LeftColumnSplitRatio;
            float emitterHeight = availableLeftHeight - viewportHeight;

            if (availableLeftHeight >= MinRowHeight * 2.0f)
            {
                viewportHeight = std::clamp(viewportHeight, MinRowHeight, availableLeftHeight - MinRowHeight);
                emitterHeight = availableLeftHeight - viewportHeight;
            }
            else if (availableLeftHeight > 0.0f)
            {
                viewportHeight = availableLeftHeight * 0.5f;
                emitterHeight = availableLeftHeight - viewportHeight;
            }
            else
            {
                viewportHeight = emitterHeight = 0.0f;
            }

            /* Section 3: 뷰포트 패널 - 현재 파티클 시스템을 표시합니다. 툴바를 통해 시뮬레이션 속도 결정 가능 */
            const bool bViewportCollapsed = (viewportHeight <= 0.0f) || (leftWidth <= 0.0f);
            const float viewportDrawHeight = bViewportCollapsed ? CollapsedPanelEpsilon : viewportHeight;
            ImGui::BeginChild("ViewportPanel", ImVec2(0, viewportDrawHeight), true, ImGuiWindowFlags_NoScrollbar);
            {
                if (!bViewportCollapsed)
                {
                    DrawViewportPanel();

                    // Cache viewport rect for input handling
                    ImVec2 childPos = ImGui::GetWindowPos();
                    ImVec2 childSize = ImGui::GetWindowSize();
                    ViewportRect.Left = childPos.x;
                    ViewportRect.Top = childPos.y;
                    ViewportRect.Right = childPos.x + childSize.x;
                    ViewportRect.Bottom = childPos.y + childSize.y;
                    ViewportRect.UpdateMinMax();
                }
                else
                {
                    ViewportRect = FRect(0, 0, 0, 0);
                    ViewportRect.UpdateMinMax();
                }
            }
            ImGui::EndChild();

            if (DrawSplitter("ViewportEmitterSplitter", false, leftWidth, viewportHeight, emitterHeight, MinRowHeight, MinRowHeight))
            {
                float total = viewportHeight + emitterHeight;
                if (total > 0.0f)
                {
                    LeftColumnSplitRatio = viewportHeight / total;
                }
            }

            /* Section 4: 이미터 패널 - 현재 파티클 시스템의 모든 이미터 패널과 각 이미터의 모든 모듈 리스트가 포함 */
            const bool bEmitterCollapsed = (emitterHeight <= 0.0f) || (leftWidth <= 0.0f);
            const float emitterDrawHeight = bEmitterCollapsed ? CollapsedPanelEpsilon : emitterHeight;
            ImGui::BeginChild("EmitterPanel", ImVec2(0, emitterDrawHeight), true, ImGuiWindowFlags_NoScrollbar);
            {
                if (!bEmitterCollapsed)
                {
                    DrawEmitterPanel();

                    // Cache emitter rect
                    ImVec2 childPos = ImGui::GetWindowPos();
                    ImVec2 childSize = ImGui::GetWindowSize();
                    EmitterRect.Left = childPos.x;
                    EmitterRect.Top = childPos.y;
                    EmitterRect.Right = childPos.x + childSize.x;
                    EmitterRect.Bottom = childPos.y + childSize.y;
                    EmitterRect.UpdateMinMax();
                }
                else
                {
                    EmitterRect = FRect(0, 0, 0, 0);
                    EmitterRect.UpdateMinMax();
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0); // No spacing between columns

        if (DrawSplitter("MainVerticalSplitter", true, totalHeight, leftWidth, rightWidth, MinColumnWidth, MinColumnWidth))
        {
            float total = leftWidth + rightWidth;
            if (total > 0.0f)
            {
                VerticalSplitRatio = leftWidth / total;
            }
        }

        ImGui::SameLine(0, 0); // Continue with right column

        /* Section 5: 디테일 패널 - 현재 파티클 시스템, 이미터, 모듈의 프로퍼티를 확인 및 수정 가능 */
        /* Section 6: 커브 에디터 - 상대 시간 혹은 절대 시간에 걸쳐 수정되는 프로퍼티가 표시되며,
                                    커브 에디터에 모듈이 추가되면 표시 컨트롤도 추가됨              */
        const float rightColumnWidthForDraw = rightWidth > 0.0f ? rightWidth : CollapsedPanelEpsilon;
        ImGui::BeginChild("RightColumn", ImVec2(rightColumnWidthForDraw, totalHeightForDraw), false, ImGuiWindowFlags_NoScrollbar);
        {
            const float availableRightHeight = std::max(0.0f, totalHeight - SplitterThickness);
            float detailHeight = availableRightHeight * RightColumnSplitRatio;
            float curveHeight = availableRightHeight - detailHeight;

            if (availableRightHeight >= MinRowHeight * 2.0f)
            {
                detailHeight = std::clamp(detailHeight, MinRowHeight, availableRightHeight - MinRowHeight);
                curveHeight = availableRightHeight - detailHeight;
            }
            else if (availableRightHeight > 0.0f)
            {
                detailHeight = availableRightHeight * 0.5f;
                curveHeight = availableRightHeight - detailHeight;
            }
            else
            {
                detailHeight = curveHeight = 0.0f;
            }

            // Section 5: Detail Panel
            const bool bDetailCollapsed = (detailHeight <= 0.0f) || (rightWidth <= 0.0f);
            const float detailDrawHeight = bDetailCollapsed ? CollapsedPanelEpsilon : detailHeight;
            ImGui::BeginChild("DetailPanel", ImVec2(0, detailDrawHeight), true, ImGuiWindowFlags_NoScrollbar);
            {
                if (!bDetailCollapsed)
                {
                    DrawDetailPanel();

                    // Cache detail rect
                    ImVec2 childPos = ImGui::GetWindowPos();
                    ImVec2 childSize = ImGui::GetWindowSize();
                    DetailRect.Left = childPos.x;
                    DetailRect.Top = childPos.y;
                    DetailRect.Right = childPos.x + childSize.x;
                    DetailRect.Bottom = childPos.y + childSize.y;
                    DetailRect.UpdateMinMax();
                }
                else
                {
                    DetailRect = FRect(0, 0, 0, 0);
                    DetailRect.UpdateMinMax();
                }
            }
            ImGui::EndChild();

            if (DrawSplitter("DetailCurveSplitter", false, rightWidth, detailHeight, curveHeight, MinRowHeight, MinRowHeight))
            {
                float total = detailHeight + curveHeight;
                if (total > 0.0f)
                {
                    RightColumnSplitRatio = detailHeight / total;
                }
            }

            // Section 6: Curve Editor Panel
            const bool bCurveCollapsed = (curveHeight <= 0.0f) || (rightWidth <= 0.0f);
            const float curveDrawHeight = bCurveCollapsed ? CollapsedPanelEpsilon : curveHeight;
            ImGui::BeginChild("CurveEditorPanel", ImVec2(0, curveDrawHeight), true, ImGuiWindowFlags_NoScrollbar);
            {
                if (!bCurveCollapsed)
                {
                    DrawCurveEditorPanel();

                    // Cache curve editor rect
                    ImVec2 childPos = ImGui::GetWindowPos();
                    ImVec2 childSize = ImGui::GetWindowSize();
                    CurveEditorRect.Left = childPos.x;
                    CurveEditorRect.Top = childPos.y;
                    CurveEditorRect.Right = childPos.x + childSize.x;
                    CurveEditorRect.Bottom = childPos.y + childSize.y;
                    CurveEditorRect.UpdateMinMax();
                }
                else
                {
                    CurveEditorRect = FRect(0, 0, 0, 0);
                    CurveEditorRect.UpdateMinMax();
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        // Pop the ItemSpacing style
        ImGui::PopStyleVar();
    }
    ImGui::End();

    // If collapsed or not visible, clear the viewport rect so we don't render a floating viewport
    if (!bEditorVisible)
    {
        ViewportRect = FRect(0, 0, 0, 0);
        ViewportRect.UpdateMinMax();
    }

    bRequestFocus = false;
}

void SParticleEditorWindow::OnUpdate(float DeltaSeconds)
{
    if (!ActiveState) return;

    if (ActiveState->Client)
    {
        ActiveState->Client->Tick(DeltaSeconds);
    }

    if (ActiveState->World)
    {
        ActiveState->World->Tick(DeltaSeconds);
        if (ActiveState->World->GetGizmoActor())
        {
            ActiveState->World->GetGizmoActor()->ProcessGizmoModeSwitch();
        }
    }

    // TODO: Update particle system playback when implemented
    // if (ActiveState->bIsPlaying && ActiveState->CurrentParticleSystem)
    // {
    //     ActiveState->CurrentTime += DeltaSeconds * ActiveState->PlaybackSpeed;
    //     // Handle looping, etc.
    // }
}

void SParticleEditorWindow::OnMouseMove(FVector2D MousePos)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (ViewportRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);
        ActiveState->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SParticleEditorWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (ViewportRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);
        ActiveState->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SParticleEditorWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (ViewportRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);
        ActiveState->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SParticleEditorWindow::OnRenderViewport()
{
    if (!bIsOpen)
    {
        return;
    }

    if (ActiveState && ActiveState->Viewport && ViewportRect.GetWidth() > 0 && ViewportRect.GetHeight() > 0)
    {
        const uint32 NewStartX = static_cast<uint32>(ViewportRect.Left);
        const uint32 NewStartY = static_cast<uint32>(ViewportRect.Top);
        const uint32 NewWidth = static_cast<uint32>(ViewportRect.Right - ViewportRect.Left);
        const uint32 NewHeight = static_cast<uint32>(ViewportRect.Bottom - ViewportRect.Top);

        ActiveState->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);
        ActiveState->Viewport->Render();
    }
}

FViewport* SParticleEditorWindow::GetViewport() const
{
    return ActiveState ? ActiveState->Viewport : nullptr;
}

FViewportClient* SParticleEditorWindow::GetViewportClient() const 
{ 
    return ActiveState ? ActiveState->Client : nullptr; 
}

void SParticleEditorWindow::OpenNewTab(const char* Name)
{
    ParticleEditorState* State = ParticleEditorBootstrap::CreateEditorState(Name, World, Device);
    if (!State) return;

    Tabs.Add(State);
    ActiveTabIndex = Tabs.Num() - 1;
    ActiveState = State;
}

void SParticleEditorWindow::CloseTab(int Index)
{
    if (Index < 0 || Index >= Tabs.Num()) return;

    ParticleEditorState* State = Tabs[Index];
    ParticleEditorBootstrap::DestroyEditorState(State);
    Tabs.RemoveAt(Index);

    if (Tabs.Num() == 0)
    {
        ActiveTabIndex = -1;
        ActiveState = nullptr;
    }
    else
    {
        ActiveTabIndex = std::min(Index, Tabs.Num() - 1);
        ActiveState = Tabs[ActiveTabIndex];
    }
}

void SParticleEditorWindow::DrawMenuBar()
{
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
                bIsOpen = false;
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
                    // TODO: Toggle grid visibility
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

void SParticleEditorWindow::DrawToolBar()
{
    ImGui::BeginChild("ToolBar", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.40f, 0.55f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.50f, 0.70f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.35f, 0.50f, 1.0f));

        if (ImGui::Button("New System", ImVec2(100, 30)))
        {
            // TODO: Create new particle system
        }
        ImGui::SameLine();

        if (ImGui::Button("Add Emitter", ImVec2(100, 30)))
        {
            // TODO: Add emitter to system
        }
        ImGui::SameLine();

        if (ImGui::Button("Save", ImVec2(80, 30)))
        {
            // TODO: Save particle system
        }
        ImGui::SameLine();

        ImGui::Separator();
        ImGui::SameLine();

        if (ActiveState)
        {
            if (ImGui::Button(ActiveState->bIsPlaying ? "Stop" : "Play", ImVec2(80, 30)))
            {
                ActiveState->bIsPlaying = !ActiveState->bIsPlaying;
                // TODO: Toggle particle system playback
            }
            ImGui::SameLine();

            if (ImGui::Button("Reset", ImVec2(80, 30)))
            {
                ActiveState->CurrentTime = 0.0f;
                // TODO: Reset particle system
            }
        }

        ImGui::PopStyleColor(3);
    }
    ImGui::EndChild();
}

void SParticleEditorWindow::DrawViewportPanel()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Viewport");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Viewport rendering will be handled in OnRenderViewport
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::BeginChild("ViewportContent", viewportSize, false, ImGuiWindowFlags_NoScrollbar);
    {
        // Viewport is rendered separately
        ImGui::Text("Particle Preview Viewport");
        ImGui::Text("Size: %.0fx%.0f", viewportSize.x, viewportSize.y);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void SParticleEditorWindow::DrawEmitterPanel()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.50f, 0.35f, 0.8f));
    ImGui::Text("Emitters");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Particle System Emitters:");
    ImGui::Spacing();

    // Placeholder for emitter list
    ImGui::BeginChild("EmitterList", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No emitters in the system.");
        ImGui::TextWrapped("Use 'Add Emitter' button to add emitters.");
        ImGui::PopStyleColor();

        // TODO: Display list of emitters when particle system is loaded
        // Example structure:
        // if (ActiveState && ActiveState->CurrentParticleSystem)
        // {
        //     auto& emitters = ActiveState->CurrentParticleSystem->GetEmitters();
        //     for (int i = 0; i < emitters.size(); ++i)
        //     {
        //         bool selected = (ActiveState->SelectedEmitterIndex == i);
        //         if (ImGui::Selectable(emitters[i]->GetName().c_str(), selected))
        //         {
        //             ActiveState->SelectedEmitterIndex = i;
        //         }
        //     }
        // }
    }
    ImGui::EndChild();
}

void SParticleEditorWindow::DrawDetailPanel()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.50f, 0.35f, 0.25f, 0.8f));
    ImGui::Text("Details");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Emitter Properties:");
    ImGui::Spacing();

    // Placeholder for emitter properties
    ImGui::BeginChild("DetailContent", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Select an emitter to edit its properties.");
        ImGui::PopStyleColor();

        // TODO: Display emitter properties when an emitter is selected
        // if (ActiveState && ActiveState->SelectedEmitterIndex >= 0)
        // {
        //     auto* emitter = ActiveState->CurrentParticleSystem->GetEmitter(ActiveState->SelectedEmitterIndex);
        //
        //     ImGui::Text("Spawn Rate");
        //     ImGui::DragFloat("##SpawnRate", &emitter->SpawnRate, 0.1f, 0.0f, 1000.0f);
        //
        //     ImGui::Text("Particle Lifetime");
        //     ImGui::DragFloat("##Lifetime", &emitter->ParticleLifetime, 0.1f, 0.0f, 100.0f);
        //
        //     // etc.
        // }
    }
    ImGui::EndChild();
}

void SParticleEditorWindow::DrawCurveEditorPanel()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.25f, 0.50f, 0.8f));
    ImGui::Text("Curve Editor");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Property Curves:");
    ImGui::Spacing();

    // Placeholder for curve editor
    ImGui::BeginChild("CurveEditorContent", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Curve editor for particle properties over time.");
        ImGui::TextWrapped("Select a property from Details panel to edit its curve.");
        ImGui::PopStyleColor();

        // TODO: Implement curve editor
        // - Display curves for selected property
        // - Allow adding/removing keyframes
        // - Allow editing curve tangents
        // - Support for Size over Lifetime, Color over Lifetime, etc.
    }
    ImGui::EndChild();
}

bool SParticleEditorWindow::DrawSplitter(const char* Id, bool bVertical, float Length, 
    float& SizeA, float& SizeB, float MinSizeA, float MinSizeB)
{
    float drawLength = std::max(Length, SplitterThickness);
    ImVec2 splitterSize = bVertical ? ImVec2(SplitterThickness, drawLength) : ImVec2(drawLength, SplitterThickness);
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_SeparatorHovered]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_SeparatorActive]);
    ImGui::InvisibleButton(Id, splitterSize);
    ImGui::PopStyleColor(3);

    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const bool bActive = ImGui::IsItemActive();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    ImU32 fillColor = ImGui::GetColorU32(style.Colors[ImGuiCol_Separator]);
    if (bActive)
    {
        fillColor = ImGui::GetColorU32(style.Colors[ImGuiCol_SeparatorActive]);
    }
    else if (bHovered)
    {
        fillColor = ImGui::GetColorU32(style.Colors[ImGuiCol_SeparatorHovered]);
    }
    drawList->AddRectFilled(min, max, fillColor);

    if (bHovered || bActive)
    {
        drawList->AddRect(min, max, ImGui::GetColorU32(style.Colors[ImGuiCol_Border]), 0.0f, 0, 1.0f);
        ImGui::SetMouseCursor(bVertical ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
    }

    bool bChanged = false;
    if (bActive)
    {
        float delta = bVertical ? ImGui::GetIO().MouseDelta.x : ImGui::GetIO().MouseDelta.y;
        if (delta != 0.0f)
        {
            const float total = SizeA + SizeB;
            if (total <= 0.0f)
            {
                return false;
            }

            float effectiveMinA = MinSizeA;
            float effectiveMinB = MinSizeB;
            const float combinedMin = MinSizeA + MinSizeB;
            if (combinedMin > 0.0f && total < combinedMin)
            {
                const float scale = total / combinedMin;
                effectiveMinA *= scale;
                effectiveMinB *= scale;
            }

            float newSizeA = SizeA + delta;
            float newSizeB = SizeB - delta;

            if (newSizeA < effectiveMinA)
            {
                newSizeA = effectiveMinA;
                newSizeB = total - newSizeA;
            }
            else if (newSizeB < effectiveMinB)
            {
                newSizeB = effectiveMinB;
                newSizeA = total - newSizeB;
            }

            newSizeA = std::max(newSizeA, 0.0f);
            newSizeB = std::max(newSizeB, 0.0f);

            if (newSizeA + newSizeB > 0.0f)
            {
                SizeA = newSizeA;
                SizeB = newSizeB;
                bChanged = true;
            }
        }
    }

    return bChanged;
}
