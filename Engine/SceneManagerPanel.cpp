#include "stdafx.h"
#include "SceneManagerPanel.h"
#include "UScene.h"
#include "UObject.h"
#include "USceneComponent.h"
#include "UPrimitiveComponent.h"

USceneManagerPanel::USceneManagerPanel(USceneManager* InSceneManager, TFunction<void(UPrimitiveComponent*)> InOnPrimitiveSelected)
	: ImGuiWindowWrapper("Scene Manager", ImVec2(276, 0), ImVec2(200, 300)), SceneManager(InSceneManager), OnPrimitiveSelected(InOnPrimitiveSelected)
{
}

void USceneManagerPanel::RenderContent()
{
	if (!SceneManager)
	{
		ImGui::Text("SceneManager not available.");
		return;
	}

	UScene* Scene = SceneManager->GetScene();
	if (!Scene)
	{
		ImGui::Text("No active scene.");
		return;
	}

	const TArray<USceneComponent*>& Objects = Scene->GetObjects();

	ImGui::Text("Objects (%u)", (unsigned)Objects.size());
	ImGui::Separator();

	if (ImGui::BeginChild("SceneObjectList", ImVec2(0, 0), true))
	{
		for (USceneComponent* Obj : Objects)
		{
			if (!Obj) continue;

			const FString& Label = Obj->Name.ToString();

			// UPrimitiveComponent만 선택 가능, 그 외는 비활성 표시
			if (UPrimitiveComponent* Prim = Obj->Cast<UPrimitiveComponent>())
			{
				// 클릭 순간에만 ImGui가 하이라이트를 보여주도록 selected=false 고정
				if (ImGui::Selectable(Label.c_str(), false))
				{
					if (OnPrimitiveSelected)
					{
						OnPrimitiveSelected(Prim);
					}
				}
			}
			else
			{
				ImGui::BeginDisabled();
				ImGui::Selectable(Label.c_str(), false);
				ImGui::EndDisabled();
			}
		}
		ImGui::EndChild();
	}
}