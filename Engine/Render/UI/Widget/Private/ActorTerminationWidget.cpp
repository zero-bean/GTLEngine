#include "pch.h"
#include "Render/UI/Widget/Public/ActorTerminationWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Level/Public/LevelManager.h"


UActorTerminationWidget::UActorTerminationWidget()
	: UWidget("Actor Termination Widget")
	  , SelectedActor(nullptr)
{
}

UActorTerminationWidget::~UActorTerminationWidget() = default;

void UActorTerminationWidget::Initialize()
{
	// Do Nothing Here
}

void UActorTerminationWidget::Update()
{
	// 매 프레임 Level의 선택된 Actor를 확인해서 정보 반영
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (CurrentLevel)
	{
		AActor* CurrentSelectedActor = CurrentLevel->GetSelectedActor();

		// Update Current Selected Actor
		if (SelectedActor != CurrentSelectedActor)
		{
			SelectedActor = CurrentSelectedActor;
		}

		// null이어도 갱신 필요
		if (!CurrentSelectedActor)
		{
			SelectedActor = nullptr;
		}
	}
}

void UActorTerminationWidget::RenderWidget()
{
	auto& InputManager = UInputManager::GetInstance();

	// ImGui::Text("Actor Management");

	if (SelectedActor)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Selected: %s (%p)",
		                   SelectedActor->GetName().c_str(), SelectedActor);

		if (ImGui::Button("Delete Selected") || InputManager.IsKeyDown(EKeyInput::Delete))
		{
			DeleteSelectedActor();
		}
	}
	else
	{
		// ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Actor Selected For Deletion");
	}
}

/**
 * @brief Selected Actor 삭제 함수
 */
void UActorTerminationWidget::DeleteSelectedActor()
{
	if (!SelectedActor)
	{
		UE_LOG("ActorTerminationWidget: No Actor Selected For Deletion");
		return;
	}

	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("ActorTerminationWidget: No Current Level To Delete Actor From");
		return;
	}

	UE_LOG("ActorTerminationWidget: Marking Selected Actor For Deletion: %s (%p)",
	       SelectedActor->GetName().empty() ? "UnNamed" : SelectedActor->GetName().c_str(),
	       SelectedActor);

	// 지연 삭제를 사용하여 안전하게 다음 틱에서 삭제
	CurrentLevel->MarkActorForDeletion(SelectedActor);

	// MarkActorForDeletion에서 선택 해제도 처리하므로 여기에서는 단순히 nullptr로 설정
	SelectedActor = nullptr;
	UE_LOG("ActorTerminationWidget: Actor Marked For Deletion In Next Tick");
}
