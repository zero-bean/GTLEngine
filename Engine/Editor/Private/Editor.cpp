#include "pch.h"
#include "Editor/Public/Editor.h"

#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/Axis.h"
#include "Editor/Public/ObjectPicker.h"
#include "Level/Public/Level.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Widget/Public/Widget.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Widget/Public/CameraControlWidget.h"

UEditor::UEditor()
	: ObjectPicker(Camera)
{
	ObjectPicker.SetCamera(Camera);

	// Set Camera to Control Panel
	auto& UIManager =UUIManager::GetInstance();
	auto* CameraControlWidget =
		reinterpret_cast<UCameraControlWidget*>(UIManager.FindWidget("Camera Control Widget"));
	CameraControlWidget->SetCamera(&Camera);
}

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
			Gizmo.SetActorRotation(GizmoDragRotation);
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

			if(PrimitiveCollided)
				ActorPicked = PrimitiveCollided->GetOwner();
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
	FVector PointOnPlane;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	switch (Gizmo.GetGizmoDirection())
	{
	case EGizmoDirection::Right:
	{
		if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, Camera.CalculatePlaneNormal(FVector{1,0,0}), PointOnPlane))
		{
			FVector MouseDistance = PointOnPlane - Gizmo.GetDragStartMouseLocation();
			return Gizmo.GetDragStartActorLocation() + FVector(1, 0, 0) * MouseDistance.Dot(FVector(1, 0, 0));
		}
		else
			return Gizmo.GetGizmoLocation();
	}
	case EGizmoDirection::Forward:
	{
		if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, Camera.CalculatePlaneNormal(FVector{ 0,0,1 }), PointOnPlane))
		{
			FVector MouseDistance = PointOnPlane - Gizmo.GetDragStartMouseLocation();	//현재 오브젝트 위치부터 충돌점까지 거리벡터
			return Gizmo.GetDragStartActorLocation() + FVector(0, 0, 1) * MouseDistance.Dot(FVector(0, 0, 1));
		}
		else
			return Gizmo.GetGizmoLocation();

	}
	case EGizmoDirection::Up:
		if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, Camera.CalculatePlaneNormal(FVector{ 0,1,0 }), PointOnPlane))
		{
			FVector MouseDistance = PointOnPlane - Gizmo.GetDragStartMouseLocation();	//현재 오브젝트 위치부터 충돌점까지 거리벡터
			return Gizmo.GetDragStartActorLocation() + FVector(0, 1, 0) * MouseDistance.Dot(FVector(0, 1, 0));
		}
		else
			return Gizmo.GetGizmoLocation();
	}

	return Gizmo.GetGizmoLocation();
}

FVector UEditor::GetGizmoDragRotation(FRay& WorldRay)
{

	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	switch (Gizmo.GetGizmoDirection())
	{
	case EGizmoDirection::Right:
	{
		if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, FVector{ 1,0,0 }, MouseWorld))
		{
			FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
			FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
			PlaneOriginToMouse.Normalize();
			PlaneOriginToMouseStart.Normalize();
			float Angle = acosf((PlaneOriginToMouseStart).Dot(PlaneOriginToMouse));	//플레인 중심부터 마우스까지 벡터 이용해서 회전각도 구하기
			if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(FVector{ 1,0,0 }) < 0) // 회전축 구하기
			{
				Angle = -Angle;
			}
			return Gizmo.GetDragStartActorRotation() + FVector{ 1,0,0 }*FVector::GetRadianToDegree(Angle);

		}
		else
			return Gizmo.GetActorRotation();
	}
	case EGizmoDirection::Forward:
	{
		if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, FVector{ 0,0,1 }, MouseWorld))
		{
			FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
			FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
			PlaneOriginToMouse.Normalize();
			PlaneOriginToMouseStart.Normalize();
			float Angle = acosf((PlaneOriginToMouseStart).Dot(PlaneOriginToMouse));	//플레인 중심부터 마우스까지 벡터 이용해서 회전각도 구하기
			if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(FVector{ 0,0,1 }) < 0) // 회전축 구하기
			{
				Angle = -Angle;
			}
			return Gizmo.GetDragStartActorRotation() + FVector{ 0,0,1 }*FVector::GetRadianToDegree(Angle);

		}
		else
			return Gizmo.GetActorRotation();

	}
	case EGizmoDirection::Up:
		if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, FVector{ 0,1,0 }, MouseWorld))
		{
			FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
			FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
			PlaneOriginToMouse.Normalize();
			PlaneOriginToMouseStart.Normalize();
			float Angle = acosf((PlaneOriginToMouseStart).Dot(PlaneOriginToMouse));	//플레인 중심부터 마우스까지 벡터 이용해서 회전각도 구하기
			if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(FVector{ 0,1,0 }) < 0) // 회전축 구하기
			{
				Angle = -Angle;
			}
			return Gizmo.GetDragStartActorRotation() + FVector{ 0,1,0 }*FVector::GetRadianToDegree(Angle);

		}
		else
			return Gizmo.GetActorRotation();
	}

	return Gizmo.GetGizmoLocation();
}
