#include "pch.h"
#include "ParticleEditorSections.h"
#include "ParticleEditorState.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleSize.h"
#include "ParticleModuleColor.h"
#include "ParticleModuleVelocity.h"
#include "ParticleModuleLifetime.h"
#include "ParticleModuleRotation.h"
#include "Distribution.h"
#include <algorithm>

void FParticleEditorCurveSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.25f, 0.50f, 0.8f));
    ImGui::Text("Curve Editor");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (!ActiveState || !ActiveState->HasParticleSystem())
    {
        ImGui::BeginChild("CurveEditorContent", ImVec2(0, 0), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No particle system loaded.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }

    UParticleEmitter* SelectedEmitter = ActiveState->GetSelectedEmitter();
    if (!SelectedEmitter || !SelectedEmitter->LODLevels.Num())
    {
        ImGui::BeginChild("CurveEditorContent", ImVec2(0, 0), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No emitter selected.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }

    UParticleLODLevel* LODLevel = SelectedEmitter->LODLevels[0];

    ImGui::BeginChild("CurveEditorContent", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        ImGui::TextWrapped("편집 가능한 속성 커브:");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // 선택된 모듈만 커브 에디터에 표시
        if (ActiveState->SelectedModuleSelection == EParticleDetailSelection::Module &&
            ActiveState->SelectedModuleIndex >= 0 &&
            ActiveState->SelectedModuleIndex < LODLevel->Modules.Num())
        {
            UParticleModule* SelectedModule = LODLevel->Modules[ActiveState->SelectedModuleIndex];

            // Size 모듈
            if (UParticleModuleSize* SizeModule = Cast<UParticleModuleSize>(SelectedModule))
            {
                DrawVectorDistributionCurveEditor(&SizeModule->StartSize, "Size", SizeModule, 0.0f, 10.0f);
            }
            // Color 모듈 (RGB 채널)
            else if (UParticleModuleColor* ColorModule = Cast<UParticleModuleColor>(SelectedModule))
            {
                static const char* ColorAxisNames[3] = { "R", "G", "B" };
                DrawVectorDistributionCurveEditor(&ColorModule->StartColor, "Color", ColorModule, 0.0f, 1.0f, ColorAxisNames);
            }
            // Velocity 모듈
            else if (UParticleModuleVelocity* VelocityModule = Cast<UParticleModuleVelocity>(SelectedModule))
            {
                DrawVectorDistributionCurveEditor(&VelocityModule->StartVelocity, "Velocity", VelocityModule, -10.0f, 10.0f);
            }
            // Rotation 모듈
            else if (UParticleModuleRotation* RotationModule = Cast<UParticleModuleRotation>(SelectedModule))
            {
                DrawVectorDistributionCurveEditor(&RotationModule->StartRotation, "Rotation", RotationModule, 0.0f, 360.0f);
            }
            // Lifetime 모듈 (Float)
            else if (UParticleModuleLifetime* LifetimeModule = Cast<UParticleModuleLifetime>(SelectedModule))
            {
                DrawFloatDistributionCurveEditor(&LifetimeModule->LifeTime, "Lifetime", LifetimeModule, 0.0f, 10.0f);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("선택된 모듈은 커브 편집을 지원하지 않습니다.");
                ImGui::TextWrapped("지원 모듈: Size, Color, Velocity, Rotation, Lifetime");
                ImGui::PopStyleColor();
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("Details 패널에서 모듈을 선택하세요.");
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0.0f, 12.0f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 12.0f));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("- 더블클릭: 키프레임 추가");
        ImGui::TextWrapped("- 드래그: 키프레임 이동");
        ImGui::TextWrapped("- Alt+클릭: 키프레임 삭제");
        ImGui::TextWrapped("- 마우스 휠: Shift=최소값, Ctrl=최대값, 기본=줌/확대");
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}

void FParticleEditorCurveSection::DrawCurveEditor(FCurve* Curve, const char* PropertyName, float MinValue, float MaxValue)
{
    if (!Curve) { return; }

    // 이 커브의 드래그 상태 가져오기
    std::string CurveKey = PropertyName;
    auto RangeIt = CurveRangeMap.find(CurveKey);
    if (RangeIt == CurveRangeMap.end())
    {
        FCurveViewRange NewRange;
        NewRange.MinValue = MinValue;
        NewRange.MaxValue = MaxValue;
        NewRange.DefaultMin = MinValue;
        NewRange.DefaultMax = MaxValue;
        RangeIt = CurveRangeMap.emplace(CurveKey, NewRange).first;
    }

    auto ClampRange = [](FCurveViewRange& Range)
    {
        const float MinSpan = 0.0001f;
        if (Range.MaxValue < Range.MinValue)
        {
            std::swap(Range.MaxValue, Range.MinValue);
        }
        if ((Range.MaxValue - Range.MinValue) < MinSpan)
        {
            float Center = (Range.MaxValue + Range.MinValue) * 0.5f;
            Range.MinValue = Center - MinSpan * 0.5f;
            Range.MaxValue = Center + MinSpan * 0.5f;
        }
    };

    auto GetDisplayRange = [](const FCurveViewRange& Range)
    {
        return std::max(Range.MaxValue - Range.MinValue, 0.0001f);
    };

    FCurveViewRange& ViewRange = RangeIt->second;
    ClampRange(ViewRange);
    float ValueRange = GetDisplayRange(ViewRange);

    int32& DraggingKeyIndex = DraggingKeyIndexMap[CurveKey];
    bool& bIsDragging = IsDraggingMap[CurveKey];

    const float GraphHeight = 250.0f;
    const float GraphWidth = ImGui::GetContentRegionAvail().x - 20.0f;
    const float Padding = 10.0f;

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    ImVec2 GraphMin = ImVec2(CursorPos.x + Padding, CursorPos.y + Padding);
    ImVec2 GraphMax = ImVec2(CursorPos.x + GraphWidth - Padding, CursorPos.y + GraphHeight - Padding);

    // 배경
    DrawList->AddRectFilled(ImVec2(CursorPos.x, CursorPos.y),
                           ImVec2(CursorPos.x + GraphWidth, CursorPos.y + GraphHeight),
                           IM_COL32(30, 30, 35, 255));
    DrawList->AddRect(ImVec2(CursorPos.x, CursorPos.y),
                     ImVec2(CursorPos.x + GraphWidth, CursorPos.y + GraphHeight),
                     IM_COL32(100, 100, 120, 255));

    // 그리드
    const int GridLines = 5;
    const ImU32 GridLineColor = IM_COL32(60, 60, 70, 150);
    const ImU32 GridLabelColor = IM_COL32(190, 190, 200, 255);
    for (int i = 0; i <= GridLines; ++i)
    {
        float t = (float)i / GridLines;
        float x = GraphMin.x + (GraphMax.x - GraphMin.x) * t;
        float y = GraphMin.y + (GraphMax.y - GraphMin.y) * t;
        DrawList->AddLine(ImVec2(x, GraphMin.y), ImVec2(x, GraphMax.y), GridLineColor, 1.0f);
        DrawList->AddLine(ImVec2(GraphMin.x, y), ImVec2(GraphMax.x, y), GridLineColor, 1.0f);

        // Y축 값 표시 (내부 왼쪽)
        float valueAlpha = 1.0f - t;
        float gridValue = ViewRange.MinValue + ValueRange * valueAlpha;
        char ValueLabel[32];
        sprintf_s(ValueLabel, "%.2f", gridValue);
        ImVec2 ValueSize = ImGui::CalcTextSize(ValueLabel);
        ImVec2 ValuePos = ImVec2(GraphMin.x + 4.0f, y - ValueSize.y * 0.5f);
        DrawList->AddText(ValuePos, GridLabelColor, ValueLabel);

        // X축 값 표시 (하단)
        char TimeLabel[16];
        sprintf_s(TimeLabel, "%.2f", t);
        ImVec2 TimeSize = ImGui::CalcTextSize(TimeLabel);
        ImVec2 TimePos = ImVec2(x - TimeSize.x * 0.5f, GraphMax.y + 4.0f);
        DrawList->AddText(TimePos, GridLabelColor, TimeLabel);
    }

    // 커브 라인 그리기 (Cubic Bezier 또는 Linear)
    if (Curve->Keys.size() >= 2)
    {
        const int SampleCount = 100;
        for (int i = 0; i < SampleCount; ++i)
        {
            float t0 = (float)i / SampleCount;
            float t1 = (float)(i + 1) / SampleCount;

            float val0 = Curve->Evaluate(t0);
            float val1 = Curve->Evaluate(t1);

            // 값을 0~1 범위로 정규화
            float norm0 = (val0 - ViewRange.MinValue) / ValueRange;
            float norm1 = (val1 - ViewRange.MinValue) / ValueRange;
            norm0 = std::clamp(norm0, 0.0f, 1.0f);
            norm1 = std::clamp(norm1, 0.0f, 1.0f);

            ImVec2 p0 = ImVec2(
                GraphMin.x + (GraphMax.x - GraphMin.x) * t0,
                GraphMax.y - (GraphMax.y - GraphMin.y) * norm0  // Y축 반전
            );
            ImVec2 p1 = ImVec2(
                GraphMin.x + (GraphMax.x - GraphMin.x) * t1,
                GraphMax.y - (GraphMax.y - GraphMin.y) * norm1
            );

            DrawList->AddLine(p0, p1, IM_COL32(100, 200, 255, 255), 2.0f);
        }
    }

    // Bezier 제어점 핸들은 Auto 모드라서 시각화 안 함
    // (필요하면 나중에 User/Break 모드에서 추가)

    // 키프레임 그리기 및 상호작용
    ImGui::SetCursorScreenPos(CursorPos);
    ImGui::InvisibleButton(PropertyName, ImVec2(GraphWidth, GraphHeight));
    bool bIsHovered = ImGui::IsItemHovered();

    if (bIsHovered && !bIsDragging)
    {
        ImGuiIO& IO = ImGui::GetIO();
        if (IO.MouseWheel != 0.0f)
        {
            float WheelDelta = IO.MouseWheel;
            float RangeStep = std::max(ValueRange * 0.05f, 0.001f);

            if (IO.KeyShift)
            {
                ViewRange.MinValue -= WheelDelta * RangeStep;
            }
            else if (IO.KeyCtrl)
            {
                ViewRange.MaxValue += WheelDelta * RangeStep;
            }
            else
            {
                float Center = (ViewRange.MaxValue + ViewRange.MinValue) * 0.5f;
                float ZoomFactor = 1.0f - WheelDelta * 0.12f;
                ZoomFactor = std::clamp(ZoomFactor, 0.2f, 5.0f);
                float HalfSpan = std::max(0.0005f, (ValueRange * ZoomFactor) * 0.5f);
                ViewRange.MinValue = Center - HalfSpan;
                ViewRange.MaxValue = Center + HalfSpan;
            }

            ClampRange(ViewRange);
            ValueRange = GetDisplayRange(ViewRange);
        }
    }

    for (size_t i = 0; i < Curve->Keys.size(); ++i)
    {
        FCurveKey& Key = Curve->Keys[i];

        float normValue = (Key.Value - ViewRange.MinValue) / ValueRange;
        normValue = std::clamp(normValue, 0.0f, 1.0f);

        ImVec2 KeyPos = ImVec2(
            GraphMin.x + (GraphMax.x - GraphMin.x) * Key.Time,
            GraphMax.y - (GraphMax.y - GraphMin.y) * normValue
        );

        // 키프레임 원 그리기
        bool bIsSelected = (DraggingKeyIndex == (int32)i);
        ImU32 KeyColor = bIsSelected ? IM_COL32(255, 200, 100, 255) : IM_COL32(255, 255, 255, 255);
        DrawList->AddCircleFilled(KeyPos, 5.0f, KeyColor);
        DrawList->AddCircle(KeyPos, 5.0f, IM_COL32(0, 0, 0, 255), 12, 1.5f);

        // 키프레임 드래그
        ImVec2 MousePos = ImGui::GetMousePos();
        float DistToMouse = sqrtf(
            (MousePos.x - KeyPos.x) * (MousePos.x - KeyPos.x) +
            (MousePos.y - KeyPos.y) * (MousePos.y - KeyPos.y)
        );

        if (DistToMouse < 8.0f && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().KeyAlt)
        {
            DraggingKeyIndex = (int32)i;
            bIsDragging = true;
        }

        // Alt + 클릭으로 삭제 (첫/마지막 키는 삭제 불가)
        if (DistToMouse < 8.0f && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyAlt)
        {
            if (i > 0 && i < Curve->Keys.size() - 1)
            {
                Curve->RemoveKey((int32)i);
                break;  // 벡터가 변경되므로 루프 탈출
            }
        }
    }

    // 드래그 중
    if (bIsDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        if (DraggingKeyIndex >= 0 && DraggingKeyIndex < (int32)Curve->Keys.size())
        {
            ImVec2 MousePos = ImGui::GetMousePos();

            // 마우스 위치를 0~1 범위로 변환
            float NewTime = (MousePos.x - GraphMin.x) / (GraphMax.x - GraphMin.x);
            float NewValueNorm = 1.0f - (MousePos.y - GraphMin.y) / (GraphMax.y - GraphMin.y);

            NewTime = std::clamp(NewTime, 0.0f, 1.0f);
            NewValueNorm = std::clamp(NewValueNorm, 0.0f, 1.0f);

            // 첫/마지막 키는 Time 고정
            if (DraggingKeyIndex == 0)
                NewTime = 0.0f;
            else if (DraggingKeyIndex == (int32)Curve->Keys.size() - 1)
                NewTime = 1.0f;

            float NewValue = ViewRange.MinValue + ValueRange * NewValueNorm;

            Curve->UpdateKey(DraggingKeyIndex, NewTime, NewValue);
        }
    }

    // 드래그 종료
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        bIsDragging = false;
        DraggingKeyIndex = -1;
    }

    // 더블클릭으로 키프레임 추가
    if (bIsHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        ImVec2 MousePos = ImGui::GetMousePos();

        float NewTime = (MousePos.x - GraphMin.x) / (GraphMax.x - GraphMin.x);
        float NewValueNorm = 1.0f - (MousePos.y - GraphMin.y) / (GraphMax.y - GraphMin.y);

        NewTime = std::clamp(NewTime, 0.0f, 1.0f);
        NewValueNorm = std::clamp(NewValueNorm, 0.0f, 1.0f);

        float NewValue = ViewRange.MinValue + ValueRange * NewValueNorm;

        Curve->AddKey(NewTime, NewValue);
    }

    ImGui::SetCursorScreenPos(ImVec2(CursorPos.x, CursorPos.y + GraphHeight + 5.0f));
}

void FParticleEditorCurveSection::DrawVectorDistributionCurveEditor(
    FRawDistributionVector* Distribution,
    const char* ModuleName,
    void* ModulePtr,
    float MinValue,
    float MaxValue,
    const char* AxisNames[3])
{
    if (!Distribution) return;

    if (Distribution->Operation != EDistributionMode::DOP_Curve)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("%s 모듈의 Distribution을 'Curve' 모드로 설정하세요.", ModuleName);
        ImGui::PopStyleColor();
        return;
    }

    // 기본 축 이름
    static const char* DefaultAxisNames[3] = { "X", "Y", "Z" };
    if (!AxisNames)
        AxisNames = DefaultAxisNames;

    ImGui::Text("%s Over Lifetime", ModuleName);
    ImGui::Spacing();

    // 고유 ID 생성
    char CurveIDBuffer[128];
    sprintf_s(CurveIDBuffer, "%s_%p", ModuleName, ModulePtr);
    std::string CurveID = CurveIDBuffer;

    if (SelectedAxisIndexMap.find(CurveID) == SelectedAxisIndexMap.end())
    {
        SelectedAxisIndexMap[CurveID] = 0; // 기본값 첫 번째 축
    }
    int32& SelectedAxisIndex = SelectedAxisIndexMap[CurveID];

    // 축 선택 버튼 (빨강, 초록, 파랑)
    ImVec4 ButtonColors[3] = {
        ImVec4(0.8f, 0.2f, 0.2f, 0.8f),  // X/R - 빨강
        ImVec4(0.2f, 0.8f, 0.2f, 0.8f),  // Y/G - 초록
        ImVec4(0.2f, 0.2f, 0.8f, 0.8f)   // Z/B - 파랑
    };
    ImVec4 ButtonHoverColors[3] = {
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
        ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
        ImVec4(0.3f, 0.3f, 1.0f, 1.0f)
    };
    ImVec4 ButtonActiveColors[3] = {
        ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
        ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
        ImVec4(0.4f, 0.4f, 1.0f, 1.0f)
    };

    for (int i = 0; i < 3; ++i)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ButtonColors[i]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ButtonHoverColors[i]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ButtonActiveColors[i]);

        char ButtonLabel[32];
        sprintf_s(ButtonLabel, "%s##%sAxis%d", AxisNames[i], ModuleName, i);
        if (ImGui::Button(ButtonLabel, ImVec2(50, 25)))
            SelectedAxisIndex = i;

        ImGui::PopStyleColor(3);

        if (i < 2)
            ImGui::SameLine(0.0f, 10.0f);
    }

    ImGui::SameLine(0.0f, 20.0f);

    // 보간 모드 버튼
    bool bIsLinear = !Distribution->CurveX.bUseCubicInterpolation;
    if (bIsLinear)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    if (ImGui::Button("Linear##Mode", ImVec2(70, 25)))
    {
        Distribution->CurveX.bUseCubicInterpolation = false;
        Distribution->CurveY.bUseCubicInterpolation = false;
        Distribution->CurveZ.bUseCubicInterpolation = false;
    }
    if (bIsLinear)
        ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 5.0f);

    bool bIsCubic = Distribution->CurveX.bUseCubicInterpolation;
    if (bIsCubic)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    if (ImGui::Button("Cubic##Mode", ImVec2(70, 25)))
    {
        Distribution->CurveX.bUseCubicInterpolation = true;
        Distribution->CurveY.bUseCubicInterpolation = true;
        Distribution->CurveZ.bUseCubicInterpolation = true;
    }
    if (bIsCubic)
        ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 12.0f));
    ImGui::Separator();

    // 선택된 축의 커브 표시
    FCurve* SelectedCurve = nullptr;
    ImVec4 TextColor;
    char UniqueID[256];

    if (SelectedAxisIndex == 0)
    {
        SelectedCurve = &Distribution->CurveX;
        TextColor = ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
        sprintf_s(UniqueID, "%s.%s_%p", ModuleName, AxisNames[0], ModulePtr);
    }
    else if (SelectedAxisIndex == 1)
    {
        SelectedCurve = &Distribution->CurveY;
        TextColor = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
        sprintf_s(UniqueID, "%s.%s_%p", ModuleName, AxisNames[1], ModulePtr);
    }
    else if (SelectedAxisIndex == 2)
    {
        SelectedCurve = &Distribution->CurveZ;
        TextColor = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);
        sprintf_s(UniqueID, "%s.%s_%p", ModuleName, AxisNames[2], ModulePtr);
    }

    if (SelectedCurve)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, TextColor);
        ImGui::Text("%s Axis", AxisNames[SelectedAxisIndex]);
        ImGui::PopStyleColor();
        DrawCurveEditor(SelectedCurve, UniqueID, MinValue, MaxValue);
    }

    ImGui::Dummy(ImVec2(0.0f, 12.0f));
}

void FParticleEditorCurveSection::DrawFloatDistributionCurveEditor(
    FRawDistributionFloat* Distribution,
    const char* ModuleName,
    void* ModulePtr,
    float MinValue,
    float MaxValue)
{
    if (!Distribution) return;

    if (Distribution->Operation != EDistributionMode::DOP_Curve)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("%s 모듈의 Distribution을 'Curve' 모드로 설정하세요.", ModuleName);
        ImGui::PopStyleColor();
        return;
    }

    ImGui::Text("%s Over Lifetime", ModuleName);
    ImGui::Spacing();

    // 보간 모드 버튼
    bool bIsLinear = !Distribution->Curve.bUseCubicInterpolation;
    if (bIsLinear)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    if (ImGui::Button("Linear##Mode", ImVec2(70, 25)))
    {
        Distribution->Curve.bUseCubicInterpolation = false;
    }
    if (bIsLinear)
        ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 5.0f);

    bool bIsCubic = Distribution->Curve.bUseCubicInterpolation;
    if (bIsCubic)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    if (ImGui::Button("Cubic##Mode", ImVec2(70, 25)))
    {
        Distribution->Curve.bUseCubicInterpolation = true;
    }
    if (bIsCubic)
        ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 고유 ID 생성
    char UniqueID[256];
    sprintf_s(UniqueID, "%s_%p", ModuleName, ModulePtr);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.0f, 1.0f));
    ImGui::Text("Value");
    ImGui::PopStyleColor();
    DrawCurveEditor(&Distribution->Curve, UniqueID, MinValue, MaxValue);

    ImGui::Spacing();
}
