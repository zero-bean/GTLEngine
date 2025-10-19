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

#include "DecalComponent.h"
#include "BillboardComponent.h"
#include "StaticMeshActor.h"    
#include "StaticMeshComponent.h"
#include "ResourceManager.h"    
#include "SceneComponent.h"    
#include "TextRenderComponent.h"    
#include "DecalComponent.h"
#include "MeshComponent.h"
#include "RotationMovementComponent.h"
#include "ProjectileMovementComponent.h"
#include "ExponentialHeightFogComponent.h"
#include "FXAAComponent.h"
#include"PointLightComponent.h"
#include "SpotLightComponent.h"
#include "Texture.h"
#include "DirectionalLightComponent.h"

#include <filesystem>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

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

// 파일명 스템(Cube 등) 추출 + .obj 확장자 제거
static inline FString GetBaseNameNoExt(const FString& Path)
{
	const size_t sep = Path.find_last_of("/\\");
	const size_t start = (sep == FString::npos) ? 0 : sep + 1;

	const FString ext = ".obj";
	size_t end = Path.size();
	if (end >= ext.size() && Path.compare(end - ext.size(), ext.size(), ext) == 0)
	{
		end -= ext.size();
	}
	if (start <= end) return Path.substr(start, end - start);
	return Path;
}

// Editor/Icon 폴더에서 모든 .dds 파일을 동적으로 찾아서 반환
static TArray<FString> GetIconFiles()
{
	TArray<FString> iconFiles;
	try
	{
		fs::path iconPath = "Editor/Icon";
		if (fs::exists(iconPath) && fs::is_directory(iconPath))
		{
			for (const auto& entry : fs::directory_iterator(iconPath))
			{
				if (entry.is_regular_file())
				{
					auto filename = entry.path().filename().string();
					// .dds 확장자만 포함
					if (filename.ends_with(".dds"))
					{
						// 상대경로 포맷으로 저장 (Editor/Icon/filename.dds)
						FString relativePath = "Editor/Icon/" + filename;
						iconFiles.push_back(relativePath);
					}
				}
			}
		}
	}
	catch (const std::exception&)
	{
		// 파일 시스템 오류 발생 시 기본값으로 폴백
		iconFiles.push_back("Editor/Icon/Pawn_64x.dds");
		iconFiles.push_back("Editor/Icon/PointLight_64x.dds");
		iconFiles.push_back("Editor/Icon/SpotLight_64x.dds");
	}
	return iconFiles;
}

// Data/Texture/NormalMap 폴더에서 모든 텍스처 파일을 동적으로 찾아서 반환
static TArray<FString> GetNormalMapFiles()
{
    TArray<FString> normalMapFiles;
    try
    {
        fs::path normalMapPath = "Data/textures/NormalMap";
        if (fs::exists(normalMapPath) && fs::is_directory(normalMapPath))
        {
            for (const auto& entry : fs::directory_iterator(normalMapPath))
            {
                if (entry.is_regular_file())
                {
                    auto path = entry.path();
                    FString filename = path.filename().string();
                    FString extension = path.extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                    if (extension == ".dds" || extension == ".png" || extension == ".jpg" || extension == ".jpeg")
                    {
                        FString relativePath = "Data/textures/NormalMap/" + filename;
                        normalMapFiles.push_back(relativePath);
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        // 에러 로그 (선택 사항)
        UE_LOG("Failed to scan normal map directory: %s", e.what());
    }
    return normalMapFiles;
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

	// NOTE: 추후 컴포넌트별 위젯 따로 추가
	// Actor가 AStaticMeshActor인 경우 StaticMesh 변경 UI
	if (SelectedComponent)
	{
		if (UExponentialHeightFogComponent* FogComponent = Cast<UExponentialHeightFogComponent>(SelectedComponent))
		{
			RenderExponentialHeightFogComponentDetails(FogComponent);

		}
		if (UStaticMeshComponent* Comp = Cast<UStaticMeshComponent>(SelectedComponent))
		{
			RenderStaticMeshComponentDetails(Comp);
		}
		else if (UBillboardComponent* Comp = Cast<UBillboardComponent>(SelectedComponent))
		{
			RenderBillboardComponentDetails(Comp);
		}
		else if (UTextRenderComponent* Comp = Cast<UTextRenderComponent>(SelectedComponent))
		{
			RenderTextRenderComponentDetails(Comp);
		}
		else if (UPointLightComponent* Comp = Cast<UPointLightComponent>(SelectedComponent))
		{
			RenderPointLightComponentDetails(Comp);
		}	
		else if (UDecalComponent* Comp = Cast<UDecalComponent>(SelectedComponent))
		{
			RenderDecalComponentDetails(Comp);
		}
		else if (URotationMovementComponent* Comp = Cast<URotationMovementComponent>(SelectedComponent))
		{
			RenderRotationMovementComponentDetails(Comp);
		}
		else if (UProjectileMovementComponent* Comp = Cast<UProjectileMovementComponent>(SelectedComponent))
		{
			RenderProjectileMovementComponentDetails(Comp);
		}
		else if (UFXAAComponent* FXAAComp = Cast<UFXAAComponent>(SelectedComponent))
		{
			RenderFXAAComponentDetails(FXAAComp);
		}
		else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(SelectedComponent))
		{
			RenderSpotLightComponentDetails(SpotLightComp);
		}
		else if (UDirectionalLightComponent* DirectionalLightComp = Cast<UDirectionalLightComponent>(SelectedComponent))
		{
			RenderDirectionalLightComponentDetails(DirectionalLightComp);
		}

		else
		{
			ImGui::Text("Selected component is not a supported type.");
		}
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

void UTargetActorTransformWidget::RenderExponentialHeightFogComponentDetails(UExponentialHeightFogComponent* InComponent)
{
	UExponentialHeightFogComponent::FFogInfo FogInfo = InComponent->GetFogInfo();
	ImGui::Text("Exponential Height Fog Component");

	ImGui::DragFloat("Fog Density", &FogInfo.FogDensity, 0.001f, 0.0f, 10.0f);
	ImGui::DragFloat("Fog Height Falloff", &FogInfo.FogHeightFalloff, 0.0001f, 0.0f, 10.0f);
	ImGui::DragFloat("Start Distance", &FogInfo.StartDistance, 0.1f, 0.0f);
	ImGui::DragFloat("Fog Max Opacity", &FogInfo.FogMaxOpacity, 0.001f, 0.0f, 1.0f);
	ImGui::DragFloat("Fog Max Opacity Distance", &FogInfo.FogMaxOpacityDistance, 100.0f, 0.0f);
	ImGui::DragFloat("Fog Cutoff Distance", &FogInfo.FogCutoffDistance, 100.0f, 0.0f);
	float Color[3]{ FogInfo.FogInscatteringColor.R,FogInfo.FogInscatteringColor.G ,FogInfo.FogInscatteringColor.B };
	if (ImGui::ColorEdit3("Fog Inscattering Color", Color))
	{
		FogInfo.FogInscatteringColor.R = Color[0];
		FogInfo.FogInscatteringColor.G = Color[1];
		FogInfo.FogInscatteringColor.B = Color[2];
	}
	//ImGui::DragFloat3("Fog Inscattering Color", &FogInfo.FogInscatteringColor, 0.1f, 0.0f, 10.0f);
	bool bIsRender = InComponent->IsRender();
	ImGui::Checkbox("Render", &bIsRender);
	InComponent->SetShowflag(bIsRender);
	InComponent->SetFogInfo(FogInfo);
}

void UTargetActorTransformWidget::RenderStaticMeshComponentDetails(UStaticMeshComponent* InComponent)
{
	ImGui::Text("StaticMesh Component");

	if (!InComponent)
	{
		ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), "StaticMeshComponent not found.");
		return;
	}

	// 1. 스태틱 메시 선택 UI 렌더링
	RenderStaticMeshSelector(InComponent);

	// 2. 머티리얼 슬롯 UI 렌더링
	RenderMaterialSlots(InComponent);
}

void UTargetActorTransformWidget::RenderStaticMeshSelector(UStaticMeshComponent* InComponent)
{
	// 현재 메시 경로 표시
	FString CurrentPath;
	UStaticMesh* CurMesh = InComponent->GetStaticMesh();
	if (CurMesh)
	{
		CurrentPath = CurMesh->GetFilePath();
		ImGui::Text("Current: %s", CurrentPath.c_str());
	}
	else
	{
		ImGui::Text("Current: <None>");
	}

	// 리소스 매니저에서 로드된 모든 StaticMesh 경로 수집
	auto& RM = UResourceManager::GetInstance();
	TArray<FString> Paths = RM.GetAllStaticMeshFilePaths();

	if (Paths.empty())
	{
		ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), "No StaticMesh resources loaded.");
		return;
	}

	// 표시용 이름(파일명 스템)
	TArray<FString> DisplayNames;
	DisplayNames.reserve(Paths.size());
	for (const FString& p : Paths)
		DisplayNames.push_back(GetBaseNameNoExt(p));

	// ImGui 콤보 아이템 배열
	TArray<const char*> Items;
	Items.reserve(DisplayNames.size());
	for (const FString& n : DisplayNames)
		Items.push_back(n.c_str());

	// 선택 인덱스 유지
	static int SelectedMeshIdx = -1;

	// 기본 선택: Cube가 있으면 자동 선택
	if (SelectedMeshIdx == -1)
	{
		for (int i = 0; i < static_cast<int>(Paths.size()); ++i)
		{
			if (DisplayNames[i] == "Cube" || Paths[i] == "Data/Cube.obj")
			{
				SelectedMeshIdx = i;
				break;
			}
		}
	}

	ImGui::SetNextItemWidth(240);
	ImGui::Combo("StaticMesh", &SelectedMeshIdx, Items.data(), static_cast<int>(Items.size()));
	ImGui::SameLine();
	if (ImGui::Button("Apply Mesh"))
	{
		if (SelectedMeshIdx >= 0 && SelectedMeshIdx < static_cast<int>(Paths.size()))
		{
			const FString& NewPath = Paths[SelectedMeshIdx];
			InComponent->SetStaticMesh(NewPath);

			UE_LOG("Applied StaticMesh: %s", NewPath.c_str());
		}
	}

	// 현재 메시로 선택 동기화 버튼 (옵션)
	ImGui::SameLine();
	if (ImGui::Button("Select Current"))
	{
		SelectedMeshIdx = -1;
		if (!CurrentPath.empty())
		{
			for (int i = 0; i < static_cast<int>(Paths.size()); ++i)
			{
				if (Paths[i] == CurrentPath ||
					DisplayNames[i] == GetBaseNameNoExt(CurrentPath))
				{
					SelectedMeshIdx = i;
					break;
				}
			}
		}
	}
}

void UTargetActorTransformWidget::RenderMaterialSlots(UStaticMeshComponent* InComponent)
{
	UStaticMesh* CurMesh = InComponent->GetStaticMesh();
	if (!CurMesh)
	{
		return; // 표시할 머티리얼 슬롯이 없음
	}

	const uint64 MeshGroupCount = CurMesh->GetMeshGroupCount();
	if (MeshGroupCount == 0)
	{
		return; // 메시 그룹이 없음
	}

	ImGui::Separator();

	// 모든 머티리얼 이름 로드
	const TArray<FString> MaterialNames = UResourceManager::GetInstance().GetAllFilePaths<UMaterial>();
	TArray<const char*> MaterialNamesCharP;
	MaterialNamesCharP.reserve(MaterialNames.size());
	for (const FString& n : MaterialNames)
		MaterialNamesCharP.push_back(n.c_str());

	if (MaterialNamesCharP.empty())
	{
		ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), "No Material resources loaded.");
		return;
	}

	// 각 슬롯별 선택된 머티리얼 인덱스
	static TArray<int32> SelectedMaterialIdxAt;
	if (SelectedMaterialIdxAt.size() < MeshGroupCount)
	{
		SelectedMaterialIdxAt.resize(MeshGroupCount);
	}

	// 현재 SMC의 MaterialSlots 정보를 UI에 반영
	const TArray<FMaterialSlot>& MaterialSlots = InComponent->GetMaterailSlots();
	for (uint64 MaterialSlotIndex = 0; MaterialSlotIndex < MeshGroupCount; ++MaterialSlotIndex)
	{
		// 이 동기화 로직은 매 프레임 실행되어야 현재 상태를 올바르게 반영합니다.
		SelectedMaterialIdxAt[MaterialSlotIndex] = -1; // 기본값
		for (uint32 MaterialIndex = 0; MaterialIndex < MaterialNames.size(); ++MaterialIndex)
		{
			if (MaterialSlots[MaterialSlotIndex].MaterialName == MaterialNames[MaterialIndex])
			{
				SelectedMaterialIdxAt[MaterialSlotIndex] = MaterialIndex;
				break;
			}
		}
	}

	// Material 슬롯별 UI 렌더링
	for (uint64 MaterialSlotIndex = 0; MaterialSlotIndex < MeshGroupCount; ++MaterialSlotIndex)
	{
		ImGui::PushID(static_cast<int>(MaterialSlotIndex));

		// 머티리얼 콤보박스
		if (ImGui::Combo("Material", &SelectedMaterialIdxAt[MaterialSlotIndex], MaterialNamesCharP.data(), static_cast<int>(MaterialNamesCharP.size())))
		{
			if (SelectedMaterialIdxAt[MaterialSlotIndex] >= 0)
			{
				InComponent->SetMaterialByUser(static_cast<uint32>(MaterialSlotIndex), 
					MaterialNames[SelectedMaterialIdxAt[MaterialSlotIndex]]);
				InComponent->ClearNormalTextureOverride(static_cast<int32>(MaterialSlotIndex));
			}
		}

		RenderNormalMapSelector(InComponent, static_cast<int32>(MaterialSlotIndex));

		ImGui::PopID();
	}
}

// UTargetActorTransformWidget.cpp (혹은 .h에 선언부가 있다면 .h에도)

/**
 * 헬퍼 함수 3: 노멀 맵 선택 UI (컴포넌트 오버라이드 방식)
 */
void UTargetActorTransformWidget::RenderNormalMapSelector(UStaticMeshComponent* InComponent, int32 MaterialSlotIndex)
{
	// 1. 현재 슬롯의 원본 머티리얼 로드
	const FMaterialSlot& Slot = InComponent->GetMaterailSlots()[MaterialSlotIndex];
	UMaterial* BaseMaterial = UResourceManager::GetInstance().Load<UMaterial>(Slot.MaterialName);

	// 원본 머티리얼이 없으면 UI를 그릴 수 없음
	if (!BaseMaterial)
	{
		ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), "Base Material not found.");
		return;
	}

	// 2. 노멀 맵 리소스 로드 (매번 호출해도 괜찮도록 static으로 관리)
	static TArray<FString> NormalMapTexturePaths;
	static TArray<FString> DisplayNames;
	static bool bNormalMapsInitialized = false;

	if (!bNormalMapsInitialized)
	{
		NormalMapTexturePaths = GetNormalMapFiles(); // 이 함수는 제공된 코드에 없었으므로, 이미 구현되어 있다고 가정
		DisplayNames.reserve(NormalMapTexturePaths.size());
		for (const FString& path : NormalMapTexturePaths)
		{
			DisplayNames.push_back(std::filesystem::path(path).stem().string());
		}
		bNormalMapsInitialized = true;
	}

	// 3. 현재 적용된 텍스처 결정
	const bool bHasOverride = InComponent->HasNormalTextureOverride(MaterialSlotIndex);

	UTexture* CurrentTexture = nullptr;
	if (bHasOverride)
	{
		// 오버라이드 상태: 컴포넌트의 오버라이드 텍스처를 가져옴 (None 오버라이드 시 nullptr일 수 있음)
		CurrentTexture = InComponent->GetNormalTextureOverride(MaterialSlotIndex);
	}
	else
	{
		// 기본 상태: 원본 머티리얼의 텍스처를 가져옴
		CurrentTexture = BaseMaterial->GetNormalTexture();
	}

	// 4. 콤보박스 라벨(표시될 이름) 설정
	const char* CurrentTextureDisplayName = "None";
	if (CurrentTexture)
	{
		FString CurrentPath = CurrentTexture->GetFilePath();
		for (size_t i = 0; i < NormalMapTexturePaths.size(); ++i)
		{
			if (NormalMapTexturePaths[i] == CurrentPath)
			{
				CurrentTextureDisplayName = DisplayNames[i].c_str();
				break;
			}
		}
		// 목록에 없는 텍스처(예: 원본 머티리얼의 기본값)는 "None"으로 표시될 수 있음
	}

	// 상태 표시: (Default) 또는 (Override)
	char ComboLabel[256];
	snprintf(ComboLabel, 256, "%s %s", CurrentTextureDisplayName, bHasOverride ? "(Override)" : "(Default)");

	// 5. 콤보박스 렌더링
	if (ImGui::BeginCombo("Normal Map", ComboLabel))
	{
		// "None (Override)" 옵션: 명시적으로 텍스처 없음을 오버라이드
		bool is_none_selected = (bHasOverride && CurrentTexture == nullptr);
		if (ImGui::Selectable("None (Override)", is_none_selected))
		{
			InComponent->SetNormalTextureOverride(MaterialSlotIndex, nullptr);
		}
		if (is_none_selected)
			ImGui::SetItemDefaultFocus();

		// 리소스 텍스처 옵션들
		for (size_t i = 0; i < NormalMapTexturePaths.size(); ++i)
		{
			UTexture* OptionTexture = UResourceManager::GetInstance().Load<UTexture>(NormalMapTexturePaths[i]);
			bool is_selected = (bHasOverride && CurrentTexture == OptionTexture);

			if (ImGui::Selectable(DisplayNames[i].c_str(), is_selected))
			{
				// 컴포넌트의 오버라이드 설정
				InComponent->SetNormalTextureOverride(MaterialSlotIndex, OptionTexture);
			}

			// (툴팁 미리보기)
			if (ImGui::IsItemHovered() && OptionTexture && OptionTexture->GetShaderResourceView())
			{
				ImGui::BeginTooltip();
				ImGui::Image(OptionTexture->GetShaderResourceView(), ImVec2(128, 128));
				ImGui::TextUnformatted(DisplayNames[i].c_str());
				ImGui::EndTooltip();
			}

			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// 6. "기본값으로" 되돌리기 버튼 (오버라이드 중일 때만 표시)
	if (bHasOverride)
	{
		ImGui::SameLine();
		if (ImGui::Button("Revert to Default"))
		{
			// 컴포넌트의 오버라이드 제거
			InComponent->ClearNormalTextureOverride(MaterialSlotIndex);
		}
	}

	// 7. 현재 적용된 텍스처 미리보기
	if (CurrentTexture && CurrentTexture->GetShaderResourceView())
	{
		ImGui::Image(CurrentTexture->GetShaderResourceView(), ImVec2(128, 128));
	}
}

void UTargetActorTransformWidget::RenderBillboardComponentDetails(UBillboardComponent* InComponent)
{
	// Billboard Component가 선택된 경우 Sprite UI
	ImGui::Separator();
	ImGui::Text("Billboard Component Settings");

	// Sprite 텍스처 경로 표시 및 변경
	FString CurrentTexture = InComponent->GetTexturePath();
	ImGui::Text("Current Sprite: %s", CurrentTexture.c_str());

	// Editor/Icon 폴더에서 동적으로 스프라이트 옵션 로드
	static TArray<FString> SpriteOptions;
	static bool bSpriteOptionsLoaded = false;
	static int currentSpriteIndex = 0; // 현재 선택된 스프라이트 인덱스

	if (!bSpriteOptionsLoaded)
	{
		// Editor/Icon 폴더에서 .dds 파일들을 찾아서 추가
		SpriteOptions = GetIconFiles();
		bSpriteOptionsLoaded = true;

		// 현재 텍스처와 일치하는 인덱스 찾기
		FString currentTexturePath = InComponent->GetTexturePath();
		for (int i = 0; i < SpriteOptions.size(); ++i)
		{
			if (SpriteOptions[i] == currentTexturePath)
			{
				currentSpriteIndex = i;
				break;
			}
		}
	}

	// 스프라이트 선택 드롭다운 메뉴
	ImGui::Text("Sprite Texture:");
	FString currentDisplayName = (currentSpriteIndex >= 0 && currentSpriteIndex < SpriteOptions.size())
		? GetBaseNameNoExt(SpriteOptions[currentSpriteIndex])
		: "Select Sprite";

	if (ImGui::BeginCombo("##SpriteCombo", currentDisplayName.c_str()))
	{
		for (int i = 0; i < SpriteOptions.size(); ++i)
		{
			FString displayName = GetBaseNameNoExt(SpriteOptions[i]);
			bool isSelected = (currentSpriteIndex == i);

			if (ImGui::Selectable(displayName.c_str(), isSelected))
			{
				currentSpriteIndex = i;
				InComponent->SetTexture(SpriteOptions[i]);
			}

			// 현재 선택된 항목에 포커스 설정
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// 새로고침 버튼 (같은 줄에)
	ImGui::SameLine();
	if (ImGui::Button("Refresh"))
	{
		bSpriteOptionsLoaded = false; // 다음에 다시 로드하도록
		currentSpriteIndex = 0; // 인덱스 리셋
	}

	ImGui::Spacing();

	// Screen Size Scaled 체크박스
	// bool bIsScreenSizeScaled = BBC->IsScreenSizeScaled();
	// if (ImGui::Checkbox("Is Screen Size Scaled", &bIsScreenSizeScaled))
	// {
	// 	BBC->SetScreenSizeScaled(bIsScreenSizeScaled);
	// }

	// Screen Size (Is Screen Size Scaled가 true일 때만 활성화)
	if (false) // (bIsScreenSizeScaled)
	{
		float screenSize = InComponent->GetScreenSize();
		if (ImGui::DragFloat("Screen Size", &screenSize, 0.0001f, 0.0001f, 0.1f, "%.4f"))
		{
			InComponent->SetScreenSize(screenSize);
		}
	}
	//else
	//{
	//	// Billboard Size (Is Screen Size Scaled가 false일 때)
	//	float billboardWidth = BBC->GetBillboardWidth();
	//	float billboardHeight = BBC->GetBillboardHeight();
	//	
	//	if (ImGui::DragFloat("Width", &billboardWidth, 0.1f, 0.1f, 100.0f))
	//	{
	//		BBC->SetBillboardSize(billboardWidth, billboardHeight);
	//	}
	//	
	//	if (ImGui::DragFloat("Height", &billboardHeight, 0.1f, 0.1f, 100.0f))
	//	{
	//		BBC->SetBillboardSize(billboardWidth, billboardHeight);
	//	}
	//}

	ImGui::Spacing();

	// UV 좌표 설정
	ImGui::Text("UV Coordinates");

	float u = InComponent->GetU();
	float v = InComponent->GetV();
	float ul = InComponent->GetUL();
	float vl = InComponent->GetVL();

	bool uvChanged = false;

	if (ImGui::DragFloat("U", &u, 0.01f))
		uvChanged = true;

	if (ImGui::DragFloat("V", &v, 0.01f))
		uvChanged = true;

	if (ImGui::DragFloat("UL", &ul, 0.01f))
		uvChanged = true;

	if (ImGui::DragFloat("VL", &vl, 0.01f))
		uvChanged = true;

	if (uvChanged)
	{
		InComponent->SetUVCoords(u, v, ul, vl);
	}
}

void UTargetActorTransformWidget::RenderTextRenderComponentDetails(UTextRenderComponent* InComponent)
{
	ImGui::Separator();
	ImGui::Text("TextRender Component Settings");

	static char textBuffer[256];
	static UTextRenderComponent* lastSelected = nullptr;
	if (lastSelected != InComponent)
	{
		strncpy_s(textBuffer, sizeof(textBuffer), InComponent->GetText().c_str(), sizeof(textBuffer) - 1);
		lastSelected = InComponent;
	}

	ImGui::Text("Text Content");

	if (ImGui::InputText("##TextContent", textBuffer, sizeof(textBuffer)))
	{
		// 실시간으로 SetText 함수 호출
		InComponent->SetText(FString(textBuffer));
	}

	ImGui::Spacing();

	//// 4. 텍스트 색상을 편집하는 Color Picker를 추가합니다.
	//FLinearColor currentColor = TextRenderComponent->GetTextColor();
	//float color[3] = { currentColor.R, currentColor.G, currentColor.B }; // ImGui는 float 배열 사용

	//ImGui::Text("Text Color");
	//if (ImGui::ColorEdit3("##TextColor", color))
	//{
	//	// 색상이 변경되면 컴포넌트의 SetTextColor 함수를 호출
	//	TextRenderComponent->SetTextColor(FLinearColor(color[0], color[1], color[2]));
	//}
}

void UTargetActorTransformWidget::RenderPointLightComponentDetails(UPointLightComponent* InComponent)
{
	ImGui::Separator();
	ImGui::Text("PointLight Component Settings");

	// 🔸 색상 설정 (RGB Color Picker)
	float TintColor[3] = { InComponent->GetTintColor().R, InComponent->GetTintColor().G, InComponent->GetTintColor().B};
	if (ImGui::ColorEdit3("Color", TintColor))
	{
		InComponent->SetTintColor(FLinearColor(TintColor[0], TintColor[1], TintColor[2], 1.0f));
	}

	ImGui::Spacing();

	// 색 온도 설정
	float Temperature = InComponent->GetColorTemperature();
	if (ImGui::DragFloat("Temperature", &Temperature, 11.029f, 1000.0f, 15000.0f))
	{
		InComponent->SetColorTemperature(Temperature);
	}

	// 🔸 밝기 (Intensity)
	float intensity = InComponent->GetIntensity();
	if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
	{
		InComponent->SetIntensity(intensity);
	}

	// 🔸 반경 (Radius)
	float radius = InComponent->GetRadius();
	if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 1000.0f))
	{
		InComponent->SetRadius(radius);
	}

	// 🔸 감쇠 정도 (FallOff)
	float falloff = InComponent->GetRadiusFallOff();
	if (ImGui::DragFloat("FallOff", &falloff, 0.05f, 0.1f, 10.0f))
	{
		InComponent->SetRadiusFallOff(falloff);
	}

	ImGui::Spacing();

	// 🔸 시각적 미리보기용 Sphere 표시 (선택된 경우)
	ImGui::Text("Preview:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(TintColor[0], TintColor[1], TintColor[2], 1.0f), "● PointLight Active");
}

void UTargetActorTransformWidget::RenderDecalComponentDetails(UDecalComponent* InComponent)
{
	ImGui::Text("Decal Component Settings");

	// Decal Texture Setting
	ImGui::Separator();

	// Editor/Icon 폴더에서 동적으로 스프라이트 옵션 로드
	static TArray<FString> SpriteOptions;
	static bool bSpriteOptionsLoaded = false;
	static int currentSpriteIndex = 0; // 현재 선택된 스프라이트 인덱스

	if (!bSpriteOptionsLoaded)
	{
		// Editor/Icon 폴더에서 .dds 파일들을 찾아서 추가
		SpriteOptions = GetIconFiles();
		bSpriteOptionsLoaded = true;

		// 현재 텍스처와 일치하는 인덱스 찾기
		FString CurrentTexture = InComponent->GetTexturePath();
		for (int i = 0; i < SpriteOptions.size(); ++i)
		{
			if (SpriteOptions[i] == CurrentTexture)
			{
				currentSpriteIndex = i;
				break;
			}
		}
	}

	// 스프라이트 선택 드롭다운 메뉴
	ImGui::Text("Sprite Texture:");
	FString currentDisplayName = (currentSpriteIndex >= 0 && currentSpriteIndex < SpriteOptions.size())
		? GetBaseNameNoExt(SpriteOptions[currentSpriteIndex])
		: "Select Sprite";

	if (ImGui::BeginCombo("##SpriteCombo", currentDisplayName.c_str()))
	{
		for (int i = 0; i < SpriteOptions.size(); ++i)
		{
			FString displayName = GetBaseNameNoExt(SpriteOptions[i]);
			bool isSelected = (currentSpriteIndex == i);

			if (ImGui::Selectable(displayName.c_str(), isSelected))
			{
				currentSpriteIndex = i;
				InComponent->SetDecalTexture(SpriteOptions[i]);
			}

			// 현재 선택된 항목에 포커스 설정
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// 새로고침 버튼 (같은 줄에)
	ImGui::SameLine();
	if (ImGui::Button("Refresh"))
	{
		bSpriteOptionsLoaded = false; // 다음에 다시 로드하도록
		currentSpriteIndex = 0; // 인덱스 리셋
	}

	ImGui::Separator();

	int32 SortOrder = InComponent->GetSortOrder();
	if (ImGui::DragInt("Sort Order", &SortOrder));
	{
		InComponent->SetSortOrder(SortOrder);
	}

	ImGui::Separator();

	// Decal Fade In/Out
	float FadeScreenSize = InComponent->GetFadeScreenSize();
	if (ImGui::DragFloat("Fade Screen Size", &FadeScreenSize, 0.01f, 0.0f, 100.0f))
	{
		InComponent->SetFadeScreenSize(FadeScreenSize);
	}

	float FadeStartDelay = InComponent->GetFadeStartDelay();
	if (ImGui::DragFloat("Fade Start Delay", &FadeStartDelay, 0.1f))
	{
		InComponent->SetFadeStartDelay(FadeStartDelay);
	}

	float FadeDuration = InComponent->GetFadeDuration();
	if (ImGui::DragFloat("Fade Duration", &FadeDuration, 0.1f))
	{
		InComponent->SetFadeDuration(FadeDuration);
	}

	float FadeInStartDelay = InComponent->GetFadeInStartDelay();
	if (ImGui::DragFloat("Fade In StartDelay", &FadeInStartDelay, 0.1f))
	{
		InComponent->SetFadeInStartDelay(FadeInStartDelay);
	}

	float FadeInDuration = InComponent->GetFadeInDuration();
	if (ImGui::DragFloat("Fade In Duration", &FadeInDuration, 0.1f))
	{
		InComponent->SetFadeInDuration(FadeInDuration);
	}

	ImGui::Separator();

	// Decal UV Tiling
	FVector2D Tiling = InComponent->GetUVTiling();
	if (ImGui::DragFloat2("UV Tiling", &Tiling.X, 0.1f, 1.0f, 10.0f))
	{
		InComponent->SetUVTiling(Tiling);
	}
	FVector DecalSize = InComponent->GetDecalSize();
	if (ImGui::DragFloat3("Decal Size", &DecalSize.X, 0.1f, 1.0f, 10.0f))
	{
		InComponent->SetDecalSize(DecalSize);
	}
}

void UTargetActorTransformWidget::RenderRotationMovementComponentDetails(URotationMovementComponent* InComponent)
{
	ImGui::Text("Rotation Movement Component Settings");
	ImGui::Separator();

	// Rotate in Local Space
	bool bLocalRotation = InComponent->GetRotationInLocalSpace();
	if (ImGui::Checkbox("Rotate in Local Space", &bLocalRotation))
	{
		InComponent->SetRotationInLocalSpace(bLocalRotation);
	}

	// Rotation Rate
	FVector RotationRate = InComponent->GetRotationRate();
	if (ImGui::DragFloat3("Rotation Rate", &RotationRate.X, 0.1f))
	{
		InComponent->SetRotationRate(RotationRate);
	}

	// Pivot Translation
	FVector PivotTranslation = InComponent->GetPivotTranslation();
	if (ImGui::DragFloat3("Pivot Translation", &PivotTranslation.X, 0.1f))
	{
		InComponent->SetPivotTranslation(PivotTranslation);
	}
}

void UTargetActorTransformWidget::RenderProjectileMovementComponentDetails(UProjectileMovementComponent* InComponent)
{
	ImGui::Text("Projectile Movement Component Settings");
	ImGui::Separator();

	// Initial Speed
	float InitialSpeed = InComponent->GetInitialSpeed();
	if (ImGui::DragFloat("InitialSpeed", &InitialSpeed, 1.0f))
	{
		InComponent->SetInitialSpeed(InitialSpeed);
	}
	// Max Speed
	float MaxSpeed = InComponent->GetMaxSpeed();
	if (ImGui::DragFloat("MaxSpeed", &MaxSpeed, 1.0f))
	{
		InComponent->SetMaxSpeed(MaxSpeed);
	}
	// Gravity Scale
	float GravityScale = InComponent->GetGravityScale();
	if (ImGui::DragFloat("GravityScale", &GravityScale, 1.0f, 0.0f, 10000.0f))
	{
		InComponent->SetGravityScale(GravityScale);
	}
}

void UTargetActorTransformWidget::RenderFXAAComponentDetails(UFXAAComponent* InComponent)
{
	float SlideX = InComponent->GetSlideX();
	float SpanMax = InComponent->GetSpanMax();
	int ReduceMin = InComponent->GetReduceMin();
	float ReduceMul = InComponent->GetReduceMul();
	if (ImGui::DragFloat("SlideX", &SlideX, 0.01f, 0, 1))
	{
		InComponent->SetSlideX(SlideX);
	}
	if (ImGui::DragFloat("SpanMax", &SpanMax, 0.01f, 0, 12))
	{
		InComponent->SetSpanMax(SpanMax);
	}
	if (ImGui::DragInt("ReduceMin", &ReduceMin, 1.0f, 0, 128))
	{
		InComponent->SetReduceMin(ReduceMin);
	}
	if (ImGui::DragFloat("ReduceMul", &ReduceMul, 0.01f, 0, 1))
	{
		InComponent->SetReduceMul(ReduceMul);
	}
}

void UTargetActorTransformWidget::RenderSpotLightComponentDetails(USpotLightComponent* InComponent)
{
	ImGui::Separator();
	ImGui::Text("SpotLight Component Settings");
	
	// 🔸 색상 설정 (RGB Color Picker)
	float color[3] = { InComponent->GetTintColor().R, InComponent->GetTintColor().G, InComponent->GetTintColor().B};
	if (ImGui::ColorEdit3("Color", color))
	{
		InComponent->SetTintColor(FLinearColor(color[0], color[1], color[2], 1.0f));
	}
	
	ImGui::Spacing();

	// 색 온도 설정
	float Temperature = InComponent->GetColorTemperature();
	if (ImGui::DragFloat("Temperature", &Temperature, 11.029f, 1000.0f, 15000.0f))
	{
		InComponent->SetColorTemperature(Temperature);
	}

	// 🔸 반지름
	float radius = InComponent->GetRadius();
	if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.0f, 20.0f))
	{
		InComponent->SetRadius(radius);
	}
	
	// 🔸 밝기 (Intensity)
	float intensity = InComponent->GetIntensity();
	if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
	{
		InComponent->SetIntensity(intensity);
	}
	
	// 🔸 InnerConeAngle
	float InnerRadius = InComponent->GetInnerConeAngle();
	if (ImGui::DragFloat("InnerConeAngle", &InnerRadius, 1.0f, 0.0f, 90.0f))
	{
		InComponent->SetInnerConeAngle(InnerRadius );
	}
	
	// 🔸 OuterConeAngle
	float OuterRadius = InComponent->GetOuterConeAngle();
	if (ImGui::DragFloat("OuterConeAngle", &OuterRadius, 1.0f, 0.0f, 90.0f))
	{
		InComponent->SetOuterConeAngle(OuterRadius );
	}
	
	// 🔸 감쇠 정도 (FallOff)
	float falloff = InComponent->GetRadiusFallOff();
  	if (ImGui::DragFloat("FallOff", &falloff, 0.05f, 0.1f, 10.0f))
	{
		InComponent->SetRadiusFallOff(falloff);
	}
	FVector attFactor = InComponent->GetAttFactor();
  	if (ImGui::DragFloat3("Attenuation Factor", &attFactor.X, 1.0f, 0.0f, 10.0f))
	{
		InComponent->SetAttFactor(attFactor);
	}
	
	// 🔸 inner 원과 outter 원과 smooth하게 섞임
	float smooth = InComponent->GetInAndOutSmooth();
	if (ImGui::DragFloat("In&Out Smooth Factor", &smooth, 1.0f, 1.0f, 10.0f))
	{ 
		InComponent->SetInAndOutSmooth(smooth);
	}

	// Circle vertex count for end-cap visualization
	int segs = InComponent->GetCircleSegments();
	if (ImGui::DragInt("Circle Segments", &segs, 1.0f, 3, 512))
	{
		segs = FMath::Clamp(segs, 3, 512);
		InComponent->SetCircleSegments(segs);
	}
	ImGui::Spacing();
	
	// 🔸 시각적 미리보기용 Sphere 표시 (선택된 경우)
	ImGui::Text("Preview:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(color[0], color[1], color[2], 1.0f), "● PointLight Active");

}

void UTargetActorTransformWidget::RenderDirectionalLightComponentDetails(UDirectionalLightComponent* InComponent)
{
	ImGui::Separator();
	ImGui::Text("PointLight Component Settings");

	// 🔸 색상 설정 (RGB Color Picker)
	float color[3] = { InComponent->GetTintColor().R, InComponent->GetTintColor().G, InComponent->GetTintColor().B};
	if (ImGui::ColorEdit3("Color", color))
	{
		InComponent->SetTintColor(FLinearColor(color[0], color[1], color[2], 1.0f));
	}

	ImGui::Spacing();

	// 색 온도 설정
	float Temperature = InComponent->GetColorTemperature();
	if (ImGui::DragFloat("Temperature", &Temperature, 11.029f, 1000.0f, 15000.0f))
	{
		InComponent->SetColorTemperature(Temperature);
	}

	// 🔸 밝기 (Intensity)
	float intensity = InComponent->GetIntensity();
	if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
	{
		InComponent->SetIntensity(intensity);
	}

	bool bEnableSpecular = InComponent->IsEnabledSpecular() && true;
	if (ImGui::Checkbox("Specular Enable", &bEnableSpecular))
	{
		InComponent->SetSpecularEnable(bEnableSpecular);
	}
	
	ImGui::Spacing();

	// 🔸 시각적 미리보기용 Sphere 표시 (선택된 경우)
	ImGui::Text("Preview:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(color[0], color[1], color[2], 1.0f), "● DirectionalLight Active");
}
