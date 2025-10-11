#include "pch.h"
#include "Render/UI/Widget/Public/PrimitiveSpawnWidget.h"

#include "Actor/Public/BillboardActor.h"
#include "Level/Public/Level.h"
#include "Editor/Public/EditorEngine.h"
#include "Actor/Public/CubeActor.h"
#include "Actor/Public/SphereActor.h"
#include "Actor/Public/SquareActor.h"
#include "Actor/Public/TriangleActor.h"
#include "Actor/Public/DecalActor.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Public/BillboardComponent.h"
#include "Core/public/BVHierarchy.h"

UPrimitiveSpawnWidget::UPrimitiveSpawnWidget()
	: UWidget("Primitive Spawn Widget")
{
}

UPrimitiveSpawnWidget::~UPrimitiveSpawnWidget() = default;

void UPrimitiveSpawnWidget::Initialize()
{
	// Do Nothing Here
}

void UPrimitiveSpawnWidget::Update()
{
	// Do Nothing Here
}

void UPrimitiveSpawnWidget::RenderWidget()
{
	ImGui::Text("Primitive Actor 생성");
	ImGui::Spacing();

	// Primitive 타입 선택 DropDown
	const char* PrimitiveTypes[] = {
		"Sphere",
		"Cube",
		"Triangle",
		"Square",
		"StaticMesh",
		"Billboard",
		"Decal"
	};

	// None을 고려한 Enum 변환 처리
	int TypeNumber = static_cast<int>(SelectedPrimitiveType) - 1;

	ImGui::Text("Primitive Type:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	ImGui::Combo("##PrimitiveType", &TypeNumber, PrimitiveTypes, 7);

	// ImGui가 받은 값을 반영
	SelectedPrimitiveType = static_cast<EPrimitiveType>(TypeNumber + 1);

	// Spawn 버튼과 개수 입력
	ImGui::Text("Number of Spawn:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##NumberOfSpawn", &NumberOfSpawn);
	NumberOfSpawn = max(1, NumberOfSpawn);
	NumberOfSpawn = min(100, NumberOfSpawn);

	ImGui::SameLine();
	if (ImGui::Button("Spawn Actors"))
	{
		SpawnActors();
	}

	// 스폰 범위 설정
	ImGui::Text("Spawn Range:");
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Min##SpawnRange", &SpawnRangeMin, 0.1f, -50.0f, SpawnRangeMax - 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Max##SpawnRange", &SpawnRangeMax, 0.1f, SpawnRangeMin + 0.1f, 50.0f);

	ImGui::Separator();
}

/**
 * @brief Actor 생성 함수
 * 난수를 활용한 Range, Size, Rotion 값 생성으로 Actor Spawn
 */
void UPrimitiveSpawnWidget::SpawnActors() const
{
	// GEngine is now a global pointer
	ULevel* CurrentLevel = GEngine->GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("ControlPanel: Actor를 생성할 레벨이 존재하지 않습니다");
		return;
	}

	UE_LOG("ControlPanel: %s 타입의 Actor를 %d개 생성 시도합니다",
		EnumToString(SelectedPrimitiveType), NumberOfSpawn);

	// 지정된 개수만큼 액터 생성
	for (int32 i = 0; i < NumberOfSpawn; i++)
	{
		AActor* NewActor = nullptr;

		// 타입에 따라 액터 생성
		switch (SelectedPrimitiveType)
		{
		case (EPrimitiveType::Cube):
			NewActor = CurrentLevel->SpawnActorToLevel(ACubeActor::StaticClass());
			break;
		case (EPrimitiveType::Sphere):
			NewActor = CurrentLevel->SpawnActorToLevel(ASphereActor::StaticClass());
			break;
		case (EPrimitiveType::Triangle):
			NewActor = CurrentLevel->SpawnActorToLevel(ATriangleActor::StaticClass());
			break;
		case (EPrimitiveType::Square):
			NewActor = CurrentLevel->SpawnActorToLevel(ASquareActor::StaticClass());
			break;
		case (EPrimitiveType::StaticMesh):
			NewActor = CurrentLevel->SpawnActorToLevel(AStaticMeshActor::StaticClass());
			break;
		case (EPrimitiveType::Billboard):
			NewActor = CurrentLevel->SpawnActorToLevel(ABillboardActor::StaticClass());
			break;
		case (EPrimitiveType::Decal):
			NewActor = CurrentLevel->SpawnActorToLevel(ADecalActor::StaticClass());
			break;
		default:
			break;
		}

		if (NewActor)
		{
			// 범위 내 랜덤 위치
			float RandomX = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomY = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomZ = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);

			NewActor->SetActorLocation(FVector(RandomX, RandomY, RandomZ));

			// 임의의 스케일 (0.5 ~ 2.0 범위)
			float RandomScale = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
			NewActor->SetActorScale3D(FVector(RandomScale, RandomScale, RandomScale));

			TArray<FBVHPrimitive> BVHPrimitives;
			TArray<TObjectPtr<UPrimitiveComponent>> LevelComp =
				GEngine->GetCurrentLevel()->GetLevelPrimitiveComponents();
			UBVHierarchy::GetInstance().ConvertComponentsToBVHPrimitives(LevelComp, BVHPrimitives);
			UBVHierarchy::GetInstance().Build(BVHPrimitives);

			UE_LOG("ControlPanel: (%.2f, %.2f, %.2f) 지점에 Actor를 배치했습니다", RandomX, RandomY, RandomZ);
		}
		else
		{
			UE_LOG("ControlPanel: Actor 배치에 실패했습니다 %d", i);
		}
	}
}
