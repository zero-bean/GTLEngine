#include "pch.h"
#include "Render/UI/Window/Public/CurveEditorWindow.h"
#include "Manager/UI/Public/UIManager.h"
#include "Level/Public/CurveLibrary.h"
#include "Level/Public/Level.h"
#include "Level/Public/World.h"
#include "Editor/Public/EditorEngine.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UCurveEditorWindow, UUIWindow)

UCurveEditorWindow::UCurveEditorWindow()
{
	// Curve editor window configuration
	FUIWindowConfig Config;
	Config.WindowTitle = "Curve Editor";
	Config.DefaultSize = ImVec2(500, 700);
	Config.DefaultPosition = ImVec2(100, 100);
	Config.MinSize = ImVec2(400, 550);
	Config.InitialState = EUIWindowState::Hidden;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.Priority = 10;
	Config.UpdateWindowFlags();
	SetConfig(Config);
	SetWindowState(EUIWindowState::Hidden); // Apply initial hidden state

	// Initialize with default curve
	CurrentCurve = FCurve::EaseInOut();
	SelectedCurveName = "";
}

UCurveEditorWindow::~UCurveEditorWindow() = default;

void UCurveEditorWindow::Initialize()
{
	// Subscribe to level change events
	if (GWorld)
	{
		GWorld->OnLevelChanged.AddDynamic(this, &UCurveEditorWindow::OnLevelChanged);
	}

	// Load first curve if available
	UCurveLibrary* Library = GetCurveLibrary();
	if (Library && Library->GetCurveCount() > 0)
	{
		TArray<FString> Names = Library->GetCurveNames();
		if (Names.Num() > 0)
		{
			SelectedCurveName = Names[0];
			LoadSelectedCurve();
		}
	}
}

UCurveLibrary* UCurveEditorWindow::GetCurveLibrary() const
{
	if (GWorld && GWorld->GetLevel())
	{
		return GWorld->GetLevel()->GetCurveLibrary();
	}
	return nullptr;
}

void UCurveEditorWindow::LoadSelectedCurve()
{
	UCurveLibrary* Library = GetCurveLibrary();
	if (!Library || SelectedCurveName.empty())
		return;

	const FCurve* Curve = Library->GetCurve(SelectedCurveName);
	if (Curve)
	{
		CurrentCurve = *Curve;
	}
}

void UCurveEditorWindow::SaveCurrentCurve()
{
	UCurveLibrary* Library = GetCurveLibrary();
	if (!Library || SelectedCurveName.empty())
		return;

	CurrentCurve.CurveName = SelectedCurveName;
	Library->AddOrUpdateCurve(SelectedCurveName, CurrentCurve);
}

void UCurveEditorWindow::OnLevelChanged()
{
	// Reset selection when level changes
	SelectedCurveName.clear();

	// Load first available curve from new level
	UCurveLibrary* Library = GetCurveLibrary();
	if (Library && Library->GetCurveCount() > 0)
	{
		TArray<FString> Names = Library->GetCurveNames();
		if (Names.Num() > 0)
		{
			SelectedCurveName = Names[0];
			LoadSelectedCurve();
		}
	}
	else
	{
		// No curves available, reset to default
		CurrentCurve = FCurve::EaseInOut();
	}
}

void UCurveEditorWindow::OnPreRenderWindow(float MenuBarOffset)
{
	// No special pre-render needed
}

void UCurveEditorWindow::OnPostRenderWindow()
{
	UCurveLibrary* Library = GetCurveLibrary();
	if (!Library)
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "No CurveLibrary available!");
		ImGui::Text("Make sure a level is loaded.");
		return;
	}

	// Curve selection
	RenderCurveSelection();

	ImGui::Separator();

	// Curve operations (New, Delete, Rename)
	RenderCurveOperations();

	ImGui::Separator();

	// Curve canvas
	RenderCurveCanvas();

	ImGui::Separator();

	// Manual input
	RenderManualInput();
}

void UCurveEditorWindow::RenderCurveSelection()
{
	UCurveLibrary* Library = GetCurveLibrary();
	if (!Library)
		return;

	ImGui::Text("Select Curve:");

	TArray<FString> CurveNames = Library->GetCurveNames();

	if (CurveNames.Num() == 0)
	{
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "No curves available. Create one below.");
		return;
	}

	// Dropdown
	if (ImGui::BeginCombo("##CurveSelect", SelectedCurveName.empty() ? "(None)" : SelectedCurveName.c_str()))
	{
		for (const FString& Name : CurveNames)
		{
			bool bIsSelected = (Name == SelectedCurveName);
			if (ImGui::Selectable(Name.c_str(), bIsSelected))
			{
				SelectedCurveName = Name;
				LoadSelectedCurve();
			}

			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

void UCurveEditorWindow::RenderCurveOperations()
{
	UCurveLibrary* Library = GetCurveLibrary();
	if (!Library)
		return;

	ImGui::Text("Curve Operations:");

	// New Curve Name Input
	ImGui::InputText("##NewCurveName", NewCurveNameBuffer, sizeof(NewCurveNameBuffer));

	// Buttons on same line
	if (ImGui::Button("New Curve"))
	{
		FString NewName(NewCurveNameBuffer);
		if (!NewName.empty())
		{
			if (!Library->HasCurve(NewName))
			{
				FCurve NewCurve = FCurve::Linear();
				NewCurve.CurveName = NewName;
				Library->AddOrUpdateCurve(NewName, NewCurve);
				SelectedCurveName = NewName;
				LoadSelectedCurve();
				NewCurveNameBuffer[0] = '\0'; // Clear input
			}
			else
			{
				// TODO: Show error message
			}
		}
	}

	// Delete and Rename on same line
	if (!SelectedCurveName.empty())
	{
		ImGui::SameLine();
		if (ImGui::Button("Delete Current"))
		{
			Library->RemoveCurve(SelectedCurveName);
			SelectedCurveName.clear();

			// Select first curve if available
			TArray<FString> Names = Library->GetCurveNames();
			if (Names.Num() > 0)
			{
				SelectedCurveName = Names[0];
				LoadSelectedCurve();
			}
		}

		ImGui::SameLine();

		// Rename Curve
		if (ImGui::Button("Rename"))
		{
			ImGui::OpenPopup("RenameCurvePopup");
			strcpy_s(RenameCurveBuffer, SelectedCurveName.c_str());
		}
	}

	// Rename Popup
	if (ImGui::BeginPopup("RenameCurvePopup"))
	{
		ImGui::Text("Rename Curve:");
		ImGui::InputText("##RenameInput", RenameCurveBuffer, sizeof(RenameCurveBuffer));

		if (ImGui::Button("OK"))
		{
			FString NewName(RenameCurveBuffer);
			if (!NewName.empty() && NewName != SelectedCurveName)
			{
				if (Library->RenameCurve(SelectedCurveName, NewName))
				{
					SelectedCurveName = NewName;
				}
			}
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

void UCurveEditorWindow::RenderCurveCanvas()
{
	ImGui::Text("Curve Editor:");

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 CanvasMax = ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y);

	// Draw canvas background
	DrawList->AddRectFilled(CanvasPos, CanvasMax, IM_COL32(50, 50, 50, 255));
	DrawList->AddRect(CanvasPos, CanvasMax, IM_COL32(255, 255, 255, 255));

	// Highlight the 0~1 range (main area of interest)
	// Y range is -0.5 ~ 1.5, so 0~1 is from 25% to 75% of canvas height
	const float YRangeMin = -0.5f;
	const float YRangeMax = 1.5f;
	const float YRangeTotal = YRangeMax - YRangeMin;

	float Y0Normalized = (1.0f - YRangeMin) / YRangeTotal;  // Position of Y=0 in canvas
	float Y1Normalized = (1.0f - 1.0f - YRangeMin) / YRangeTotal;  // Position of Y=1 in canvas (inverted)

	ImVec2 MainRangeMin(CanvasPos.x, CanvasPos.y + CanvasSize.y * Y1Normalized);
	ImVec2 MainRangeMax(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y * Y0Normalized);
	DrawList->AddRectFilled(MainRangeMin, MainRangeMax, IM_COL32(60, 60, 70, 255));

	// Draw grid (0~1 range lines are brighter)
	for (int i = 0; i <= GridDivisions; ++i)
	{
		float t = static_cast<float>(i) / GridDivisions;
		ImVec2 VertStart(CanvasPos.x + CanvasSize.x * t, CanvasPos.y);
		ImVec2 VertEnd(CanvasPos.x + CanvasSize.x * t, CanvasPos.y + CanvasSize.y);

		// Horizontal grid lines at different Y values
		float YValue = YRangeMax - t * YRangeTotal;  // Y value at this grid line
		float YPixel = t;
		ImVec2 HorizStart(CanvasPos.x, CanvasPos.y + CanvasSize.y * YPixel);
		ImVec2 HorizEnd(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y * YPixel);

		// Highlight Y=0 and Y=1 lines
		ImU32 GridColor;
		if (std::abs(YValue - 0.0f) < 0.01f || std::abs(YValue - 1.0f) < 0.01f)
			GridColor = IM_COL32(255, 255, 255, 150);
		else if (i == 0 || i == GridDivisions)
			GridColor = IM_COL32(255, 255, 255, 100);
		else
			GridColor = IM_COL32(100, 100, 100, 50);

		DrawList->AddLine(VertStart, VertEnd, GridColor);
		DrawList->AddLine(HorizStart, HorizEnd, GridColor);
	}

	// Push clip rect to prevent drawing outside canvas
	DrawList->PushClipRect(CanvasPos, CanvasMax, true);

	// Draw curve using proper Bezier evaluation
	const int Steps = 100;
	for (int i = 0; i < Steps; ++i)
	{
		float t1 = static_cast<float>(i) / Steps;
		float t2 = static_cast<float>(i + 1) / Steps;

		float x1, y1, x2, y2;
		CurrentCurve.EvaluateXY(t1, x1, y1);
		CurrentCurve.EvaluateXY(t2, x2, y2);

		// Don't clamp - allow overshoot values
		ImVec2 p1 = CurveToCanvas(ImVec2(x1, y1), CanvasPos, CanvasSize);
		ImVec2 p2 = CurveToCanvas(ImVec2(x2, y2), CanvasPos, CanvasSize);

		DrawList->AddLine(p1, p2, IM_COL32(0, 255, 0, 255), 2.0f);
	}

	// Don't clamp control points - show actual values (even if outside visible range)
	float CP1X = CurrentCurve.ControlPoint1X;
	float CP1Y = CurrentCurve.ControlPoint1Y;
	float CP2X = CurrentCurve.ControlPoint2X;
	float CP2Y = CurrentCurve.ControlPoint2Y;

	// Draw bezier control lines
	ImVec2 P0 = CurveToCanvas(ImVec2(0, 0), CanvasPos, CanvasSize);
	ImVec2 P1 = CurveToCanvas(ImVec2(CP1X, CP1Y), CanvasPos, CanvasSize);
	ImVec2 P2 = CurveToCanvas(ImVec2(CP2X, CP2Y), CanvasPos, CanvasSize);
	ImVec2 P3 = CurveToCanvas(ImVec2(1, 1), CanvasPos, CanvasSize);

	DrawList->AddLine(P0, P1, IM_COL32(255, 255, 0, 150), 1.0f);
	DrawList->AddLine(P2, P3, IM_COL32(255, 255, 0, 150), 1.0f);

	// Draw start/end points
	DrawList->AddCircleFilled(P0, 5.0f, IM_COL32(255, 255, 255, 255));
	DrawList->AddCircleFilled(P3, 5.0f, IM_COL32(255, 255, 255, 255));

	// Draw control points (read-only display)
	const float ControlPointRadius = 7.0f;
	DrawList->AddCircleFilled(P1, ControlPointRadius, IM_COL32(255, 255, 0, 255));
	DrawList->AddCircle(P1, ControlPointRadius, IM_COL32(0, 0, 0, 255), 0, 1.5f);
	DrawList->AddCircleFilled(P2, ControlPointRadius, IM_COL32(255, 255, 0, 255));
	DrawList->AddCircle(P2, ControlPointRadius, IM_COL32(0, 0, 0, 255), 0, 1.5f);

	// Pop clip rect
	DrawList->PopClipRect();

	// Dummy to reserve space for canvas
	ImGui::Dummy(CanvasSize);
}

void UCurveEditorWindow::RenderManualInput()
{
	ImGui::Text("Control Points (Manual Input):");

	// Control Point 1
	ImGui::Text("CP1:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("##CP1_X", &CurrentCurve.ControlPoint1X, 0.01f, 0.0f, 1.0f, "X: %.2f"))
	{
		SaveCurrentCurve();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("##CP1_Y", &CurrentCurve.ControlPoint1Y, 0.01f, -0.5f, 1.5f, "Y: %.2f"))
	{
		SaveCurrentCurve();
	}

	// Control Point 2
	ImGui::Text("CP2:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("##CP2_X", &CurrentCurve.ControlPoint2X, 0.01f, 0.0f, 1.0f, "X: %.2f"))
	{
		SaveCurrentCurve();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("##CP2_Y", &CurrentCurve.ControlPoint2Y, 0.01f, -0.5f, 1.5f, "Y: %.2f"))
	{
		SaveCurrentCurve();
	}

	// Evaluation test
	ImGui::Separator();
	ImGui::Text("Test Evaluation:");
	static float TestT = 0.5f;
	ImGui::SetNextItemWidth(150);
	if (ImGui::SliderFloat("t", &TestT, 0.0f, 1.0f))
	{
		// Just update slider
	}
	float EvalY = CurrentCurve.Evaluate(TestT);
	ImGui::Text("Value at t=%.2f: %.3f", TestT, EvalY);
}

ImVec2 UCurveEditorWindow::CurveToCanvas(ImVec2 CurvePos, ImVec2 CanvasPos, ImVec2 CanvasSize) const
{
	// Y range: -0.5 ~ 1.5 (to support overshoot effects)
	const float YRangeMin = -0.5f;
	const float YRangeMax = 1.5f;
	const float YRangeTotal = YRangeMax - YRangeMin;

	// Normalize Y to 0~1 range
	float NormalizedY = (CurvePos.y - YRangeMin) / YRangeTotal;

	// Flip Y axis (canvas Y goes down, curve Y goes up)
	return ImVec2(
		CanvasPos.x + CurvePos.x * CanvasSize.x,
		CanvasPos.y + (1.0f - NormalizedY) * CanvasSize.y
	);
}

ImVec2 UCurveEditorWindow::CanvasToCurve(ImVec2 CanvasPixelPos, ImVec2 CanvasPos, ImVec2 CanvasSize) const
{
	// Y range: -0.5 ~ 1.5
	const float YRangeMin = -0.5f;
	const float YRangeMax = 1.5f;
	const float YRangeTotal = YRangeMax - YRangeMin;

	// Flip Y axis and denormalize
	float NormalizedY = 1.0f - (CanvasPixelPos.y - CanvasPos.y) / CanvasSize.y;
	float CurveY = NormalizedY * YRangeTotal + YRangeMin;

	return ImVec2(
		(CanvasPixelPos.x - CanvasPos.x) / CanvasSize.x,
		CurveY
	);
}
