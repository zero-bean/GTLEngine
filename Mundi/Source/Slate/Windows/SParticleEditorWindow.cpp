#include "pch.h"
#include <algorithm>
#include "SParticleEditorWindow.h"
#include "ParticleEditorSections.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "UIManager.h"
#include "ParticleEditorState.h"
#include "ParticleEditorBootstrap.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule/ParticleModuleRequired.h"
#include "ParticleModule/ParticleModuleSpawn.h"
#include "ParticleModule/ParticleModuleLifetime.h"
#include "ParticleModule/ParticleModuleSize.h"
#include "ParticleModule/ParticleModuleColor.h"
#include "ResourceManager.h"
#include "Material.h"
#include "Shader.h"

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

        FParticleEditorSectionContext SectionContext{ *this, ActiveState, &bShowColorPicker };

        // Update window rect
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        Rect.Left = pos.x;
        Rect.Top = pos.y;
        Rect.Right = pos.x + size.x;
        Rect.Bottom = pos.y + size.y;
        Rect.UpdateMinMax();

        /* Section 2: 툴바 */
        ToolBarSection.Draw(SectionContext);

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

        // --- Section 3 & 4: Left Column (Viewport / Details) ---
        const float leftColumnWidthForDraw = leftWidth > 0.0f ? leftWidth : CollapsedPanelEpsilon;
        const float totalHeightForDraw = totalHeight > 0.0f ? totalHeight : SplitterThickness;
        ImGui::BeginChild("LeftColumn", ImVec2(leftColumnWidthForDraw, totalHeightForDraw), false, ImGuiWindowFlags_NoScrollbar);
        {
            const float availableLeftHeight = std::max(0.0f, totalHeight - SplitterThickness);
            float viewportHeight = availableLeftHeight * LeftColumnSplitRatio;
            float detailHeight = availableLeftHeight - viewportHeight;

            if (availableLeftHeight >= MinRowHeight * 2.0f)
            {
                viewportHeight = std::clamp(viewportHeight, MinRowHeight, availableLeftHeight - MinRowHeight);
                detailHeight = availableLeftHeight - viewportHeight;
            }
            else if (availableLeftHeight > 0.0f)
            {
                viewportHeight = availableLeftHeight * 0.5f;
                detailHeight = availableLeftHeight - viewportHeight;
            }
            else
            {
                viewportHeight = detailHeight = 0.0f;
            }

            /* Section 3: 뷰포트 패널 - 현재 파티클 시스템을 표시합니다. 툴바를 통해 시뮬레이션 속도 결정 가능 */
            const bool bViewportCollapsed = (viewportHeight <= 0.0f) || (leftWidth <= 0.0f);
            const float viewportDrawHeight = bViewportCollapsed ? CollapsedPanelEpsilon : viewportHeight;
            ImGui::BeginChild("ViewportPanel", ImVec2(0, viewportDrawHeight), true, ImGuiWindowFlags_NoScrollbar);
            {
                if (!bViewportCollapsed)
                {
                    ViewportSection.Draw(SectionContext);

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

            if (DrawSplitter("ViewportDetailSplitter", false, leftWidth, viewportHeight, detailHeight, MinRowHeight, MinRowHeight))
            {
                float total = viewportHeight + detailHeight;
                if (total > 0.0f)
                {
                    LeftColumnSplitRatio = viewportHeight / total;
                }
            }

            /* Section 4: 이미터 패널 - 현재 파티클 시스템의 모든 이미터 패널과 각 이미터의 모든 모듈 리스트가 포함 */
            const bool bDetailCollapsed = (detailHeight <= 0.0f) || (leftWidth <= 0.0f);
            const float detailDrawHeight = bDetailCollapsed ? CollapsedPanelEpsilon : detailHeight;
            ImGui::BeginChild("DetailPanel", ImVec2(0, detailDrawHeight), true, ImGuiWindowFlags_NoScrollbar);
            {
                if (!bDetailCollapsed)
                {
                    DetailSection.Draw(SectionContext);

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
            float emitterHeight = availableRightHeight * RightColumnSplitRatio;
            float curveHeight = availableRightHeight - emitterHeight;

            if (availableRightHeight >= MinRowHeight * 2.0f)
            {
                emitterHeight = std::clamp(emitterHeight, MinRowHeight, availableRightHeight - MinRowHeight);
                curveHeight = availableRightHeight - emitterHeight;
            }
            else if (availableRightHeight > 0.0f)
            {
                emitterHeight = availableRightHeight * 0.5f;
                curveHeight = availableRightHeight - emitterHeight;
            }
            else
            {
                emitterHeight = curveHeight = 0.0f;
            }

            // Section 5: Detail Panel
            const bool bEmitterCollapsed = (emitterHeight <= 0.0f) || (rightWidth <= 0.0f);
            const float emitterDrawHeight = bEmitterCollapsed ? CollapsedPanelEpsilon : emitterHeight;
            ImGui::BeginChild("EmitterPanel", ImVec2(0, emitterDrawHeight), true, ImGuiWindowFlags_NoScrollbar);
            {
                if (!bEmitterCollapsed)
                {
                    EmitterSection.Draw(SectionContext);

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

            if (DrawSplitter("EmitterCurveSplitter", false, rightWidth, emitterHeight, curveHeight, MinRowHeight, MinRowHeight))
            {
                float total = emitterHeight + curveHeight;
                if (total > 0.0f)
                {
                    RightColumnSplitRatio = emitterHeight / total;
                }
            }

            // Section 6: Curve Editor Panel
            const bool bCurveCollapsed = (curveHeight <= 0.0f) || (rightWidth <= 0.0f);
            const float curveDrawHeight = bCurveCollapsed ? CollapsedPanelEpsilon : curveHeight;
            ImGui::BeginChild("CurveEditorPanel", ImVec2(0, curveDrawHeight), true, ImGuiWindowFlags_NoScrollbar);
            {
                if (!bCurveCollapsed)
                {
                    CurveSection.Draw(SectionContext);

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

        // 배경색 피커 윈도우 (메인 윈도우 끝나기 전에, 다른 패널에 가려지지 않도록)
        if (bShowColorPicker && ActiveState)
        {
            if (bColorPickerSpawnPosValid)
            {
                ImGui::SetNextWindowPos(ImVec2(ColorPickerSpawnPos.X, ColorPickerSpawnPos.Y), ImGuiCond_Appearing);
            }
            if (bColorPickerFocusRequested)
            {
                ImGui::SetNextWindowFocus();
                bColorPickerFocusRequested = false;
            }
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGuiWindowFlags PickerFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
            const bool bPickerWindowOpen = ImGui::Begin("Viewport Background Color", &bShowColorPicker, PickerFlags);
            if (bPickerWindowOpen)
            {
                const bool bPickerHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
                UUIManager::GetInstance().RegisterOverlayWindow("Viewport Background Color", bPickerHovered);
                ImGui::ColorPicker4("##ViewportBgColor", ActiveState->ViewportBackgroundColor,
                    ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar);
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        }
        else
        {
            bColorPickerFocusRequested = false;
        }
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

        if (ActiveState->Client)
        {
            ActiveState->Client->SetViewportBackgroundColor(FVector4(
                ActiveState->ViewportBackgroundColor[0],
                ActiveState->ViewportBackgroundColor[1],
                ActiveState->ViewportBackgroundColor[2],
                ActiveState->ViewportBackgroundColor[3]));
        }

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

bool SParticleEditorWindow::ExistName(const FString& InName)
{
    for (int32 Index = 0; Index < Tabs.Num(); Index++)
    {
        if (InName == Tabs[Index]->Name)
        {
            return true;
        }
    }
    return false;
}

void SParticleEditorWindow::CreateNewTab()
{
    char label[32];
    sprintf_s(label, "Particle Editor %d", Tabs.Num() + 1);
    OpenNewTab(label);
}

void SParticleEditorWindow::RequestColorPickerFocus()
{
    bColorPickerFocusRequested = true;
}

void SParticleEditorWindow::SetColorPickerSpawnPosition(const FVector2D& ScreenPos)
{
    ColorPickerSpawnPos = ScreenPos;
    bColorPickerSpawnPosValid = true;
}

void SParticleEditorWindow::LoadParticleSystem(UParticleSystem* ParticleSystem)
{
    if (!ActiveState)
    {
        return;
    }

    // 기존 파티클 시스템 제거 (단, 우리가 소유한 경우만)
    if (ActiveState->CurrentParticleSystem && ActiveState->bOwnsParticleSystem)
    {
        DeleteObject(ActiveState->CurrentParticleSystem);
        ActiveState->CurrentParticleSystem = nullptr;
        ActiveState->bOwnsParticleSystem = false;
    }
    else
    {
        // 소유하지 않은 경우 참조만 제거
        ActiveState->CurrentParticleSystem = nullptr;
        ActiveState->bOwnsParticleSystem = false;
    }

    // None이 전달된 경우 기본 빈 파티클 시스템 생성
    if (!ParticleSystem)
    {
        // ParticleEditorBootstrap::CreateEditorState와 동일한 방식으로 기본 시스템 생성
        ActiveState->CurrentParticleSystem = NewObject<UParticleSystem>();
        ActiveState->bOwnsParticleSystem = true; // 우리가 생성했으므로 소유권 설정
        ActiveState->LoadedParticleSystemPath = "";

        // 기본 Sprite Emitter 생성
        UParticleEmitter* DefaultEmitter = NewObject<UParticleEmitter>();

        // LOD Level 0 생성
        UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
        LODLevel->Level = 0;
        LODLevel->bEnabled = true;
        DefaultEmitter->LODLevels.Add(LODLevel);

        // Required Module (필수)
        UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();

        // 기본 Material 생성
        UMaterial* DefaultMaterial = NewObject<UMaterial>();
        UShader* BillboardShader = UResourceManager::GetInstance().Load<UShader>("Shaders/UI/Billboard.hlsl");
        if (BillboardShader)
        {
            DefaultMaterial->SetShader(BillboardShader);
        }
        Required->Material = DefaultMaterial;
        Required->EmitterDuration = 2.0f;
        Required->EmitterLoops = 0;
        LODLevel->RequiredModule = Required;

        // Spawn Module (필수)
        UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
        SpawnModule->SpawnRate.Operation = EDistributionMode::DOP_Constant;
        SpawnModule->SpawnRate.Constant = 30.0f;
        LODLevel->SpawnModule = SpawnModule;

        // 기본 모듈들 추가
        UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
        LifetimeModule->LifeTime.Operation = EDistributionMode::DOP_Uniform;
        LifetimeModule->LifeTime.Min = 1.0f;
        LifetimeModule->LifeTime.Max = 2.0f;

        UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
        SizeModule->StartSize.Operation = EDistributionMode::DOP_Constant;
        SizeModule->StartSize.Constant = FVector(1.0f, 1.0f, 1.0f);

        UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
        ColorModule->StartColor.Operation = EDistributionMode::DOP_Constant;
        ColorModule->StartColor.Constant = FVector(1.0f, 1.0f, 1.0f);

        // Modules 배열에 추가
        LODLevel->Modules.Add(LifetimeModule);
        LODLevel->Modules.Add(SizeModule);
        LODLevel->Modules.Add(ColorModule);

        // SpawnModules 배열에 추가
        LODLevel->SpawnModules.Add(LifetimeModule);
        LODLevel->SpawnModules.Add(SizeModule);
        LODLevel->SpawnModules.Add(ColorModule);

        LODLevel->TypeDataModule = nullptr;

        // Emitter 정보 캐싱
        DefaultEmitter->CacheEmitterModuleInfo();

        // ParticleSystem에 기본 Emitter 추가
        ActiveState->CurrentParticleSystem->Emitters.Add(DefaultEmitter);

        // 첫 번째 Emitter 자동 선택
        ActiveState->SelectedEmitterIndex = 0;
    }
    else
    {
        // 파티클 시스템 참조 설정 (외부에서 전달받은 것이므로 소유권 없음)
        ActiveState->CurrentParticleSystem = ParticleSystem;
        ActiveState->bOwnsParticleSystem = false; // 외부 참조이므로 소유권 없음
        ActiveState->LoadedParticleSystemPath = "";

        // 첫 번째 Emitter 선택 (있으면)
        if (ParticleSystem->Emitters.Num() > 0)
        {
            ActiveState->SelectedEmitterIndex = 0;
        }
        else
        {
            ActiveState->SelectedEmitterIndex = -1;
        }
    }

    // 프리뷰 업데이트
    ActiveState->UpdatePreviewParticleSystem();
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
