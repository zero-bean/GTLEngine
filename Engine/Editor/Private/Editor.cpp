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
#include "Render/UI/Widget/Public/CameraControlWidget.h"


UEditor::UEditor()
	: ObjectPicker(Camera)
{
	ObjectPicker.SetCamera(Camera);

	// Set Camera to Control Panel
	auto& UIManager = UUIManager::GetInstance();
	auto* CameraControlWidget =
		reinterpret_cast<UCameraControlWidget*>(UIManager.FindWidget("Camera Control Widget"));
	CameraControlWidget->SetCamera(&Camera);

};

UEditor::~UEditor() = default;

void UEditor::Update()
{
	auto& Renderer = URenderer::GetInstance();
	Camera.Update();

	ProcessMouseInput(ULevelManager::GetInstance().GetCurrentLevel());


	Renderer.UpdateConstant(Camera.GetFViewProjConstants());
}
void UEditor::RenderEditor()
{
	Gizmo.RenderGizmo(ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor());
	Grid.RenderGrid();
	Axis.Render();
}


void UEditor::ProcessMouseInput(ULevel* InLevel)
{
	FRay WorldRay;
	const UInputManager& InputManager = UInputManager::GetInstance();
	FVector MousePositionNdc = InputManager.GetMouseNDCPosition();

	static EGizmoDirection PreviousGizmoDirection = EGizmoDirection::None;
	AActor* ActorPicked = InLevel->GetSelectedActor();
	FVector CollisionPoint;
	float ActorDistance = -1;

	//로컬 기즈모, 쿼터니언 구현 후 사용
	/*if (InputManager.IsKeyPressed(EKeyInput::Tab))
	{
		if (Gizmo.IsWorld())
			Gizmo.SetLocal();
		else
			Gizmo.SetWorld();
	}*/
	if (InputManager.IsKeyPressed(EKeyInput::Space))
	{
		Gizmo.ChangeGizmoMode();
	}
	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		Gizmo.EndDrag();
	}
	WorldRay = Camera.ConvertToWorldRay(MousePositionNdc.X, MousePositionNdc.Y);

	if(Gizmo.IsDragging())
	{
		switch (Gizmo.GetGizmoMode())
		{
		case EGizmoMode::Translate:
		{
			FVector GizmoDragLocation = GetGizmoDragLocation(WorldRay);
			Gizmo.SetLocation(GizmoDragLocation); break;
		}
		case EGizmoMode::Rotate:
		{
			FVector GizmoDragRotation = GetGizmoDragRotation(WorldRay);
			Gizmo.SetActorRotation(GizmoDragRotation); break;
		}
		case EGizmoMode::Scale:
		{
			FVector GizmoDragScale = GetGizmoDragScale(WorldRay);
			Gizmo.SetActorScale(GizmoDragScale);
		}
		}
		
	}
	else
	{
		if (InLevel->GetSelectedActor()) //기즈모가 출력되고있음. 레이캐스팅을 계속 해야함.
		{
			ObjectPicker.PickGizmo(WorldRay, Gizmo, CollisionPoint);
		}
		if (!ImGui::GetIO().WantCaptureMouse && InputManager.IsKeyPressed(EKeyInput::MouseLeft))
		{
			TArray<UPrimitiveComponent*> Candidate = FindCandidatePrimitives(InLevel);

			UPrimitiveComponent* PrimitiveCollided= ObjectPicker.PickPrimitive( WorldRay, Candidate, &ActorDistance);

			if (PrimitiveCollided)
				ActorPicked = PrimitiveCollided->GetOwner();
			else
				ActorPicked = nullptr;
		}

		if (Gizmo.GetGizmoDirection() == EGizmoDirection::None) //기즈모에 호버링되거나 클릭되지 않았을 때. Actor 업데이트해줌.
		{
			InLevel->SetSelectedActor(ActorPicked);
			if (PreviousGizmoDirection != EGizmoDirection::None)
			{
				Gizmo.OnMouseRelease(PreviousGizmoDirection);
			}
		}
		//기즈모가 선택되었을 때. Actor가 선택되지 않으면 기즈모도 선택되지 않으므로 이미 Actor가 선택된 상황.
		//SelectedActor를 update하지 않고 마우스 인풋에 따라 hovering or drag
		else
		{
			PreviousGizmoDirection = Gizmo.GetGizmoDirection();
			if (InputManager.IsKeyPressed(EKeyInput::MouseLeft)) //드래그
			{
				Gizmo.OnMouseDragStart(CollisionPoint);
			}
			else
			{
				Gizmo.OnMouseHovering();
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

FVector UEditor::GetGizmoDragLocation(FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	//로컬 기즈모, 쿼터니언 구현 후 사용
	//if (!Gizmo.IsWorld())	//local
	//{
	//	FVector4 GizmoAxis4{ GizmoAxis.X,GizmoAxis.Y ,GizmoAxis.Z, 0.0f };
	//	GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(Gizmo.GetActorRotation());
	//}
	
	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, Camera.CalculatePlaneNormal(GizmoAxis), MouseWorld))
	{
		FVector MouseDistance = MouseWorld - Gizmo.GetDragStartMouseLocation();
		return Gizmo.GetDragStartActorLocation() + GizmoAxis * MouseDistance.Dot(GizmoAxis);
	}
	else
		return Gizmo.GetGizmoLocation();
	
}

FVector UEditor::GetGizmoDragRotation(FRay& WorldRay)
{

	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	//로컬 기즈모, 쿼터니언 구현 후 사용
	//if (!Gizmo.IsWorld())	//local
	//{
	//	FVector4 GizmoAxis4{ GizmoAxis.X,GizmoAxis.Y ,GizmoAxis.Z, 0.0f };
	//	GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(Gizmo.GetActorRotation());
	//}
	
	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, GizmoAxis, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		PlaneOriginToMouse.Normalize();
		PlaneOriginToMouseStart.Normalize();
		float Angle = acosf((PlaneOriginToMouseStart).Dot(PlaneOriginToMouse));	//플레인 중심부터 마우스까지 벡터 이용해서 회전각도 구하기
		if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(GizmoAxis) < 0) // 회전축 구하기
		{
			Angle = -Angle;
		}
		return Gizmo.GetDragStartActorRotation() + GizmoAxis *FVector::GetRadianToDegree(Angle);

	}
	else
		return Gizmo.GetActorRotation();
	
}

FVector UEditor::GetGizmoDragScale(FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	//로컬 기즈모, 쿼터니언 구현 후 사용
	//if (!Gizmo.IsWorld())	//local
	//{
	//	FVector4 GizmoAxis4{ GizmoAxis.X,GizmoAxis.Y ,GizmoAxis.Z, 0.0f };
	//	GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(Gizmo.GetActorRotation());
	//}

	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, Camera.CalculatePlaneNormal(GizmoAxis), MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		float DragStartAxisDistance = PlaneOriginToMouseStart.Dot(GizmoAxis);
		float DragAxisDistance = PlaneOriginToMouse.Dot(GizmoAxis);
		float ScaleFactor = 1.0f;
		if (abs(DragStartAxisDistance) > 0.001f)
		{
			ScaleFactor = DragAxisDistance / DragStartAxisDistance;
		}
		
		FVector DragStartScale = Gizmo.GetDragStartActorScale();
		if (ScaleFactor > MinScale)
		{
			if (Gizmo.GetSelectedActor()->IsUniformScale())
			{
				float UniformValue = DragStartScale.Dot(GizmoAxis);
				return FVector(UniformValue, UniformValue, UniformValue) + FVector(1, 1, 1) * (ScaleFactor - 1);
			}
			else
				return DragStartScale + GizmoAxis * (ScaleFactor - 1) * DragStartScale.Dot(GizmoAxis);
		}
		else
			return Gizmo.GetActorScale();
	}
	else
		return Gizmo.GetActorScale();
}
