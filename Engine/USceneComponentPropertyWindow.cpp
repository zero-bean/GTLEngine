#include "stdafx.h"
#include "USceneComponentPropertyWindow.h"


// 활성화(선택) 상태면 버튼색을 Active 계열로 바꿔서 '눌린 버튼'처럼 보이게 하는 헬퍼
static bool ModeButton(const char* label, bool active, const ImVec2& size = ImVec2(0, 0))
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 colBtn = active ? style.Colors[ImGuiCol_ButtonActive] : style.Colors[ImGuiCol_Button];
	ImVec4 colHover = active ? style.Colors[ImGuiCol_ButtonActive] : style.Colors[ImGuiCol_ButtonHovered];
	ImVec4 colActive = style.Colors[ImGuiCol_ButtonActive];

	ImGui::PushStyleColor(ImGuiCol_Button, colBtn);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHover);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
	bool pressed = ImGui::Button(label, size);
	ImGui::PopStyleColor(3);
	return pressed;
}

void USceneComponentPropertyWindow::RenderContent()
{
	if (!Target) return;

	FVector& location = Target->RelativeLocation;
	FVector rotation = Target->GetRotation();
	FVector& scale = Target->RelativeScale3D;

	float locationInput[3] = { location.X, location.Y, location.Z };
	float rotationInput[3] = { rotation.X, rotation.Y, rotation.Z };
	float scaleInput[3] = { scale.X,    scale.Y,    scale.Z };
	bool rotCommited = false;
	// 나머지는 테이블로
	if (ImGui::BeginTable("EditablePropertyTable", 4, ImGuiTableFlags_None))
	{
		ImGui::TableNextRow();
		for (int32 i = 0; i < 3; i++)
		{
			ImGui::TableSetColumnIndex(i);
			ImGui::SetNextItemWidth(-1);
			ImGui::InputFloat(("##tra" + std::to_string(i)).c_str(),
				&locationInput[i], 0.0f, 0.0f, "%.3f");
		}
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("Translation");

		ImGui::TableNextRow();
		for (int32 i = 0; i < 3; i++)
		{
			ImGui::TableSetColumnIndex(i);
			ImGui::SetNextItemWidth(-1);
			ImGui::InputFloat(("##rot" + std::to_string(i)).c_str(),
				&rotationInput[i], 0.0f, 0.0f, "%.3f");
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				rotCommited = true;
			}
		}
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("Rotation");

		ImGui::TableNextRow();
		for (int32 i = 0; i < 3; i++)
		{
			ImGui::TableSetColumnIndex(i);
			ImGui::SetNextItemWidth(-1);
			ImGui::InputFloat(("##scl" + std::to_string(i)).c_str(),
				&scaleInput[i], 0.0f, 0.0f, "%.3f");
		}
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("Scale");

		ImGui::EndTable();
	}

	if (rotCommited)
	{
		Target->SetRotation({ rotationInput[0], rotationInput[1], rotationInput[2] });
	}
	location = FVector(locationInput[0], locationInput[1], locationInput[2]);
	rotation = FVector(rotationInput[0], rotationInput[1], rotationInput[2]);
	scale = FVector(scaleInput[0], scaleInput[1], scaleInput[2]);
}
