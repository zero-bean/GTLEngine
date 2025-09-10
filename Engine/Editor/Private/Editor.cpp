#include "pch.h"

#include "Editor/Public/Editor.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/Axis.h"
#include "Editor/Public/ObjectPicker.h"
#include "Core/Public/Object.h"
#include "Core/Public/AppWindow.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Window/Public/CameraPanelWindow.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Mesh/Public/Actor.h"
#include "Level/Public/Level.h"




UEditor::UEditor()
	:Camera(),
	ObjectPicker(Camera)
{
	if (UCameraPanelWindow* Window =
		dynamic_cast<UCameraPanelWindow*>(UUIManager::GetInstance().FindUIWindow("Camera Control")))
	{
		Window->SetCamera(&Camera);
	}
	ObjectPicker.SetCamera(Camera);
};

UEditor::~UEditor() = default;

void UEditor::Update()
{
	auto& Renderer = URenderer::GetInstance();
	Camera.Update();

	ProcessMouseInput(ULevelManager::GetInstance().GetCurrentLevel(), Gizmo);
	Renderer.UpdateConstant(Camera.GetFViewProjConstants());
}
void UEditor::RenderEditor()
{
	Gizmo.RenderGizmo(ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor(), ObjectPicker);
	Grid.RenderGrid();
	Axis.Render();
}


void UEditor::ProcessMouseInput(ULevel* InLevel, UGizmo& InGizmo)
{
	const UInputManager& InputManager = UInputManager::GetInstance();
	FVector MousePositionNdc = InputManager.GetMouseNDCPosition();

	static EGizmoDirection PreviousGizmoDirection = EGizmoDirection::None;
	AActor* ActorPicked = InLevel->GetSelectedActor();
	FVector4 CollisionPoint;
	float ActorDistance = -1;

	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		InGizmo.EndDrag();
	}
	if (!InGizmo.IsDragging())
	{
		FRay WorldRay = Camera.ConvertToWorldRay(MousePositionNdc.X, MousePositionNdc.Y);
		if (InLevel->GetSelectedActor()) //기즈모가 출력되고있음. 레이캐스팅을 계속 해야함.
		{
			InGizmo.SetGizmoDirection(ObjectPicker.PickGizmo(WorldRay, InGizmo, CollisionPoint));
		}
		if (!ImGui::GetIO().WantCaptureMouse && InputManager.IsKeyPressed(EKeyInput::MouseLeft))
		{
			TArray<UPrimitiveComponent*> Candidate = FindCandidatePrimitives(InLevel);

			UPrimitiveComponent* PrimitiveCollided= ObjectPicker.PickPrimitive( WorldRay, Candidate, &ActorDistance);

			if(PrimitiveCollided)
				ActorPicked = PrimitiveCollided->GetOwner();
		}

		if (InGizmo.GetGizmoDirection() == EGizmoDirection::None) //기즈모에 호버링되거나 클릭되지 않았을 때. Actor 업데이트해줌.
		{
			InLevel->SetSelectedActor(ActorPicked);
			if (PreviousGizmoDirection != EGizmoDirection::None)
			{
				InGizmo.OnMouseRelease(PreviousGizmoDirection);
			}
		}
		//기즈모가 선택되었을 때. Actor가 선택되지 않으면 기즈모도 선택되지 않으므로 이미 Actor가 선택된 상황.
		//SelectedActor를 update하지 않고 마우스 인풋에 따라 hovering or drag
		else
		{
			PreviousGizmoDirection = InGizmo.GetGizmoDirection();
			if (InputManager.IsKeyPressed(EKeyInput::MouseLeft)) //드래그
			{
				InGizmo.OnMouseDragStart(CollisionPoint);
			}
			else
			{
				InGizmo.OnMouseHovering();
			}
		}
	}
}

TArray<UPrimitiveComponent*> UEditor::FindCandidatePrimitives(ULevel* InLevel)
{
	TArray<UPrimitiveComponent*> Candidate;

	for (AActor* Actor : InLevel->GetLevelActors())
	{
		for (auto& ActorComponent : Actor->GetOwnedComponents())
		{
			UPrimitiveComponent* Primitive = dynamic_cast<UPrimitiveComponent*>(ActorComponent);
			if (Primitive)
			{
				Candidate.push_back(Primitive);
			}
		}
	}

	return Candidate;
}
