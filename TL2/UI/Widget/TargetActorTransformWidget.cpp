#include "pch.h"
#include "TargetActorTransformWidget.h"
#include "UI/UIManager.h"
#include "ImGui/imgui.h"
#include "Actor.h"
#include "World.h"
#include "Vector.h"
#include "GizmoActor.h"
#include "SelectionManager.h"
#include <string>
#include"RotationMovementComponent.h"
#include"ProjectileMovementComponent.h"
#include "SceneComponent.h"

#include <vector>

using namespace std;

//// UE_LOG 대체 매크로
//#define UE_LOG(fmt, ...)

// ★ 고정 오더: ZYX (Yaw-Pitch-Roll) - 기즈모의 Delta 곱(Z * Y * X)과 동일
static inline FQuat QuatFromEulerZYX_Deg(const FVector& Deg)
{
	const float Rx = DegreeToRadian(Deg.X); // Roll (X)
	const float Ry = DegreeToRadian(Deg.Y); // Pitch (Y)
	const float Rz = DegreeToRadian(Deg.Z); // Yaw (Z)

	const FQuat Qx = MakeQuatFromAxisAngle(FVector(1, 0, 0), Rx);
	const FQuat Qy = MakeQuatFromAxisAngle(FVector(0, 1, 0), Ry);
	const FQuat Qz = MakeQuatFromAxisAngle(FVector(0, 0, 1), Rz);
	return Qz * Qy * Qx; // ZYX
}

static inline FVector EulerZYX_DegFromQuat(const FQuat& Q)
{
	// ZYX Euler 변환: Quat → (Roll, Pitch, Yaw)
	// atan2를 사용하여 전체 360도 범위 지원
	const float w = Q.W, x = Q.X, y = Q.Y, z = Q.Z;

	// Roll (X축 회전): -180 ~ 180도
	const float sinr_cosp = 2.0f * (w * x + y * z);
	const float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
	float roll = std::atan2(sinr_cosp, cosr_cosp);

	// Pitch (Y축 회전): atan2 사용으로 전체 범위 지원
	const float sinp = 2.0f * (w * y - z * x);
	const float cosp = std::sqrt(1.0f - sinp * sinp);
	float pitch = std::atan2(sinp, cosp);

	// Yaw (Z축 회전): -180 ~ 180도
	const float siny_cosp = 2.0f * (w * z + x * y);
	const float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
	float yaw = std::atan2(siny_cosp, cosy_cosp);

	return FVector(RadianToDegree(roll), RadianToDegree(pitch), RadianToDegree(yaw));
}


UTargetActorTransformWidget::UTargetActorTransformWidget()
	: UWidget("Target Actor Transform Widget")
	, UIManager(&UUIManager::GetInstance())
	, SelectionManager(&USelectionManager::GetInstance())
{

}

UTargetActorTransformWidget::~UTargetActorTransformWidget() = default;

void UTargetActorTransformWidget::OnSelectedActorCleared()
{
	// 즉시 내부 캐시/플래그 정리
	//SelectedActor = nullptr;
	//CachedActorName.clear();
	ResetChangeFlags();
}

void UTargetActorTransformWidget::Initialize()
{
	// UIManager 참조 확보
	UIManager = &UUIManager::GetInstance();

	// Transform 위젯을 UIManager에 등록하여 선택 해제 브로드캐스트를 받을 수 있게 함
	if (UIManager)
	{
		UIManager->RegisterTargetTransformWidget(this);
	}
}

//AActor* UTargetActorTransformWidget::GetCurrentSelectedActor() const
//{
//	if (!UIManager)
//		return nullptr;
//
//	return UIManager->GetSelectedActor();
//}

void UTargetActorTransformWidget::Update()
{
	// UIManager를 통해 현재 선택된 액터 가져오기

	//if (SelectedActor != CurrentSelectedActor)
	//{
	//	SelectedActor = CurrentSelectedActor;
	//	// 새로 선택된 액터의 이름 캐시
	//	if (SelectedActor)
	//	{
	//		try
	//		{
	//			// 새로운 액터가 선택되면, 선택된 컴포넌트를 해당 액터의 루트 컴포넌트로 초기화합니다.
	//			SelectedComponent = SelectedActor->GetRootComponent();

	//			CachedActorName = SelectedActor->GetName().ToString();
	//		}
	//		catch (...)
	//		{
	//			CachedActorName = "[Invalid Actor]";
	//			SelectedActor = nullptr;
	//			SelectedComponent = nullptr;
	//		}
	//	}
	//	else
	//	{
	//		CachedActorName = "";
	//		SelectedComponent = nullptr;
	//	}
	//}

	//if (SelectedActor)
	//{
	//	// 액터가 선택되어 있으면 항상 트랜스폼 정보를 업데이트하여
	//	// 기즈모 조작을 실시간으로 UI에 반영합니다.
	//	UpdateTransformFromActor();
	//}

	UpdateTransformFromActor();
}

/**
 * @brief Actor 복제 테스트 함수
 */
void UTargetActorTransformWidget::DuplicateTarget(AActor* SelectedActor) const
{
	if (SelectedActor)
	{
		AActor* NewActor = Cast<AActor>(SelectedActor->Duplicate());
		
		// 초기 트랜스폼 적용
		NewActor->SetActorTransform(SelectedActor->GetActorTransform());

		// TODO(KHJ): World 접근?
		UWorld* World = SelectedActor->GetWorld();
		
		World->SpawnActor(NewActor);
	}
}

void UTargetActorTransformWidget::RenderWidget()
{
	// +-+-+ Get Selection Info +-+-+
	AActor* SelectedActor = SelectionManager->GetSelectedActor();
	if (!SelectedActor)
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Actor Selected");
		ImGui::TextUnformatted("Select an actor to edit its transform.");
		ImGui::Separator();
		return;
	}
	UActorComponent* SelectedComponent = SelectionManager->GetSelectedComponent();

	// +-+-+ Show Selected Actor Info +-+-+
	ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Selected: %s", SelectedActor->GetName().ToString().c_str());	// Actor Name (cached name)
	ImGui::Text("UUID: %u", static_cast<unsigned int>(SelectedActor->UUID));	// Show Selected Actor UUID (Global Unique ID)
	ImGui::Spacing();

	// 추가 가능한 컴포넌트 타입 목록 (자동 수집)
	static TArray<TPair<FString, UClass*>> AddableSceneComponentTypes;
	static bool bComponentTypesInitialized = false;

	if (!bComponentTypesInitialized)
	{
		// USceneComponent를 상속받은 모든 클래스를 자동으로 수집
		TArray<UClass*> DerivedClasses = UClassRegistry::Get().GetDerivedClasses(USceneComponent::StaticClass());

		for (UClass* Class : DerivedClasses)
		{
			// 추상 클래스(인스턴스 생성 불가) 제외
			if (Class->CreateInstance == nullptr)
			{
				continue;
			}

			// Gizmo 컴포넌트와 같은 에디터 전용 컴포넌트 제외
			FString ClassName = Class->Name;
			if (ClassName.find("Gizmo") == FString::npos &&
				ClassName.find("Grid") == FString::npos &&
				ClassName.find("Line") == FString::npos &&
				ClassName.find("Shape") == FString::npos &&
				ClassName.find("Cube") == FString::npos &&
				ClassName.find("Sphere") == FString::npos &&
				ClassName.find("Triangle") == FString::npos &&
				ClassName.find("BoundingBox") == FString::npos)
			{
				AddableSceneComponentTypes.push_back({ ClassName, Class });
			}
		}

		// 이름순 정렬
		std::sort(AddableSceneComponentTypes.begin(), AddableSceneComponentTypes.end(),
			[](const TPair<FString, UClass*>& A, const TPair<FString, UClass*>& B)
			{
				return A.first < B.first;
			});

		bComponentTypesInitialized = true;
	}
	// 추가 가능한 컴포넌트 타입 목록 (임시 하드코딩)
	
	static const TArray<TPair<FString, UClass*>> AddableActorComponentTypes = {
		{ "Rotation Movement Component", URotationMovementComponent::StaticClass() },
		{ "Projectile Movement Component", UProjectileMovementComponent::StaticClass() }
	};

	// +-+-+ Component Add/Delete Button +-+-+
	if (SelectedComponent)
	{
		if (ImGui::Button("+추가"))
		{
			ImGui::OpenPopup("AddComponentPopup");
		}
		ImGui::SameLine();

		if (ImGui::Button("-삭제"))
		{
			// Check if the component to delete is a SceneComponent
			if (USceneComponent* SceneCompToDelete = Cast<USceneComponent>(SelectedComponent))
			{
				if (SceneCompToDelete == SelectedActor->GetRootComponent())
				{
					UE_LOG("루트 컴포넌트는 UI에서 직접 삭제할 수 없습니다.");
				}
				else
				{
					USceneComponent* ParentComponent = SceneCompToDelete->GetAttachParent();
					USelectionManager::GetInstance().ClearSelection();

					if (SelectedActor->DeleteComponent(SceneCompToDelete))
					{
						if (ParentComponent)
						{
							USelectionManager::GetInstance().SelectComponent(ParentComponent);
							SelectedComponent = ParentComponent;
						}
						else
						{
							// 컴포넌트 삭제 시 상위 컴포넌트로 선택되도록 설정
							SelectedComponent = SelectedActor->GetRootComponent();
						}
					}
				}
			}
			else	// For non-SceneComponents
			{
				USelectionManager::GetInstance().ClearSelection();
				
				if (SelectedActor->DeleteComponent(SelectedComponent))
				{
					USceneComponent* Root = SelectedActor->GetRootComponent();
					if (Root)
					{
						USelectionManager::GetInstance().SelectComponent(Root);
						SelectedComponent = Root;
					}
					else
					{
						SelectedComponent = nullptr;
					}
				}
			}
		}

		// "Add Component" 버튼에 대한 팝업 메뉴 정의
		if (ImGui::BeginPopup("AddComponentPopup"))
		{
			ImGui::BeginChild("ComponentListScroll", ImVec2(240.0f, 200.0f), true);

			// +-+-+ Show Scene Component List +-+-+
			ImGui::SetWindowFontScale(0.8f);
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Scene Components");
			ImGui::SetWindowFontScale(1.0f);
			for (const TPair<FString, UClass*>& Item : AddableSceneComponentTypes)
			{
				if (ImGui::Selectable(Item.first.c_str()))
				{
					USceneComponent* ParentComponent = Cast<USceneComponent>(SelectedComponent);
					// 컴포넌트를 누르면 생성 함수를 호출합니다.
					USceneComponent* NewSceneComponent = SelectedActor->CreateAndAttachComponent(ParentComponent, Item.second);
					// SelectedComponent를 생성된 컴포넌트로 교체합니다
					SelectedComponent = NewSceneComponent;
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 0.1f));

			// +-+-+ Show Actor Component List +-+-+
			ImGui::SetWindowFontScale(0.8f);
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Movement Components");
			ImGui::SetWindowFontScale(1.0f);
			for (const TPair<FString, UClass*>& Item : AddableActorComponentTypes)
			{
				if (ImGui::Selectable(Item.first.c_str()))
				{
					SelectedActor->AddComponentByClass(Item.second);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndChild();
			ImGui::EndPopup();
		}
	}

	// 컴포넌트 계층 구조 표시
	ImGui::BeginChild("ComponentHierarchy", ImVec2(0, 240), true);
	if (SelectedActor)
	{
		const FName ActorName = SelectedActor->GetName();

		// 1. 최상위 액터 노드는 클릭해도 접을 수 없습니다.
		ImGui::TreeNodeEx(
			ActorName.ToString().c_str(),
			ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
		);

		// 2. 수동으로 들여쓰기를 추가합니다.
		ImGui::Indent();

		// +-+-+ Scene Components (Tree) +-+-+
		USceneComponent* RootComponent = SelectedActor->GetRootComponent();
		if (RootComponent)
		{
			RenderComponentHierarchy(RootComponent);
		}
		ImGui::Separator();

		// +-+-+ Actor Components (Ownedlist) +-+-+
		for (UActorComponent* Comp : SelectedActor->GetComponents())
		{
			if (Comp && !Comp->IsA(USceneComponent::StaticClass()))
			{
				const bool bIsSelected = (SelectedComponent == Comp);

				if (ImGui::Selectable(Comp->GetName().c_str(), bIsSelected))
				{
					SelectionManager->SelectComponent(Comp);
				}
			}
		}

		// 4. 들여쓰기를 해제합니다.
		ImGui::Unindent();
	}
	else
	{
		ImGui::Text("No actor selected.");
	}
	ImGui::EndChild();

	// Location 편집
	if (ImGui::DragFloat3("Location", &EditLocation.X, 0.1f))
	{
		bPositionChanged = true;
	}

	// Rotation 편집 (Euler angles)
	if (ImGui::DragFloat3("Rotation", &EditRotation.X, 0.5f))
	{
		bRotationChanged = true;
	}

	// Scale 편집
	ImGui::Checkbox("Uniform Scale", &bUniformScale);

	if (bUniformScale)
	{
		float UniformScale = EditScale.X;
		if (ImGui::DragFloat("Scale", &UniformScale, 0.01f, 0.01f, 10.0f))
		{
			EditScale = FVector(UniformScale, UniformScale, UniformScale);
			bScaleChanged = true;
		}
	}
	else
	{
		if (ImGui::DragFloat3("Scale", &EditScale.X, 0.01f, 0.01f, 10.0f))
		{
			bScaleChanged = true;
		}
	}

	ImGui::Spacing();

	//// 실시간 적용 버튼
	//if (ImGui::Button("Apply Transform"))
	//{
	//	ApplyTransformToActor();
	//}

	//ImGui::SameLine();
	//if (ImGui::Button("Reset Transform"))
	//{
	//	UpdateTransformFromActor();
	//	ResetChangeFlags();
	//}

	// TODO(KHJ): 테스트용, 완료 후 지울 것
	if (ImGui::Button("Duplicate Test Button"))
	{
		DuplicateTarget(SelectedActor);
	}

	ImGui::Spacing();
	ImGui::Separator();

	// 컴포넌트별 디테일 렌더링 (객체지향 방식)
	if (SelectedComponent)
	{
		// 각 컴포넌트가 자신의 디테일을 렌더링
		SelectedComponent->RenderDetails();
	}
	ImGui::Separator();
}

// 재귀적으로 모든 하위 컴포넌트를 트리 형태로 렌더링
void UTargetActorTransformWidget::RenderComponentHierarchy(USceneComponent* SceneComponent)
{
	if (!SceneComponent || !SceneComponent->IsEditable())
	{
		return;
	}

	/*if (!SelectedActor || !SelectedComponent)
	{
		return;
	}*/
	AActor* SelectedActor = SelectionManager->GetSelectedActor();
	UActorComponent* SelectedComponent = SelectionManager->GetSelectedComponent();

	const bool bIsRootComponent = SelectedActor->GetRootComponent() == SceneComponent;
	const FString ComponentName = SceneComponent->GetName() + (bIsRootComponent ? " (Root)" : "");
	const TArray<USceneComponent*>& AttachedChildren = SceneComponent->GetAttachChildren();
	
	bool bHasEditableChild = false;

	for (USceneComponent* ChildComponent : AttachedChildren)
	{
		bHasEditableChild = ChildComponent->IsEditable();
	}

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
		| ImGuiTreeNodeFlags_SpanAvailWidth
		| ImGuiTreeNodeFlags_DefaultOpen;

	// 현재 그리고 있는 SceneComponent가 SelectedComponent와 일치하는지 확인
	const bool bIsSelected = (SelectedComponent == SceneComponent);
	if (bIsSelected)
	{
		// 일치하면 Selected 플래그를 추가하여 하이라이트 효과를 줍니다.
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
		
	}
	//Editable한 자식이 존재하지 않는 경우만 NonLeaf노드로 처리
	if (!bHasEditableChild)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf;
	}
	

	const bool bNodeIsOpen = ImGui::TreeNodeEx(
		(void*)SceneComponent,
		NodeFlags,
		"%s",
		ComponentName.c_str()
	);

	// 방금 그린 TreeNode가 클릭되었는지 확인합니다.
	if (ImGui::IsItemClicked())
	{
		// 클릭되었다면, 멤버 변수인 SelectedComponent를 현재 컴포넌트로 업데이트합니다.
		USelectionManager::GetInstance().SelectComponent(SceneComponent);
	}

	if (bNodeIsOpen)
	{
		for (USceneComponent* ChildComponent : AttachedChildren)
		{
			RenderComponentHierarchy(ChildComponent);
		}
		ImGui::TreePop();
	}
}

void UTargetActorTransformWidget::PostProcess()
{
	// 자동 적용이 활성화된 경우 변경사항을 즉시 적용
	if (bPositionChanged || bRotationChanged || bScaleChanged)
	{
		UActorComponent* SelectedComp = SelectionManager->GetSelectedComponent();
		if (USceneComponent* SelectedSceneComp = Cast<USceneComponent>(SelectedComp))
		{
			ApplyTransformToComponent(SelectedSceneComp);
			ResetChangeFlags(); // 적용 후 플래그 리셋
		}
	}
}

void UTargetActorTransformWidget::UpdateTransformFromActor()
{
	UActorComponent* SelectedComp = SelectionManager->GetSelectedComponent();

	if (USceneComponent* SelectedSceneComp = Cast<USceneComponent>(SelectedComp))
	{
		// 컴포넌트 선택이 바뀌었는지 확인
		bool bComponentChanged = (LastReadComponent != SelectedSceneComp);
		if (bComponentChanged)
		{
			LastReadComponent = SelectedSceneComp;
		}

		// 액터의 현재 트랜스폼을 UI 변수로 복사
		EditLocation = SelectedSceneComp->GetRelativeLocation();

		// Rotation: 컴포넌트가 바뀌었을 때만 Quat에서 Euler로 변환
		// (같은 컴포넌트라면 사용자 입력값 유지하여 짐벌락 회피)
		if (bComponentChanged)
		{
			EditRotation = EulerZYX_DegFromQuat(SelectedSceneComp->GetRelativeRotation());
			PrevEditRotation = EditRotation;
		}

		EditScale = SelectedSceneComp->GetRelativeScale();
	}
	else
	{
		// 컴포넌트가 없으면 초기화
		LastReadComponent = nullptr;
	}

	ResetChangeFlags();
}

void UTargetActorTransformWidget::ApplyTransformToComponent(USceneComponent* SelectedComponent)
{
	if (!SelectedComponent)
		return;

	// 변경사항이 있는 경우에만 적용
	if (bPositionChanged)
	{
		SelectedComponent->SetRelativeLocation(EditLocation);
		UE_LOG("Transform: Applied location (%.2f, %.2f, %.2f)",
			EditLocation.X, EditLocation.Y, EditLocation.Z);
	}

	if (bRotationChanged)
	{
		// 커스텀 ZYX Euler → Quat 변환 사용 (기즈모 회전 순서와 일치)
		FQuat NewRotation = QuatFromEulerZYX_Deg(EditRotation);
		SelectedComponent->SetRelativeRotation(NewRotation);
		PrevEditRotation = EditRotation; // 적용 후 이전 값 갱신
		UE_LOG("Transform: Applied rotation (%.1f, %.1f, %.1f)",
			EditRotation.X, EditRotation.Y, EditRotation.Z);
	}

	if (bScaleChanged)
	{
		SelectedComponent->SetRelativeScale(EditScale);
		UE_LOG("Transform: Applied scale (%.2f, %.2f, %.2f)",
			EditScale.X, EditScale.Y, EditScale.Z);
	}
}


void UTargetActorTransformWidget::ResetChangeFlags()
{
	bPositionChanged = false;
	bRotationChanged = false;
	bScaleChanged = false;
}
