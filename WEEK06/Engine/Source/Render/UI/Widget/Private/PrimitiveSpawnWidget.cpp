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
#include "Actor/Public/SpotLightActor.h"
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
		"Sphere", "Cube", "Triangle", "Square", "StaticMesh", "Billboard", "Decal", "SpotLight",
	};
	int TypeNumber = static_cast<int>(SelectedPrimitiveType) - 1;

	ImGui::Text("Primitive Type:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	ImGui::Combo("##PrimitiveType", &TypeNumber, PrimitiveTypes, 8);
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

	ImGui::Separator();

	// 스폰 범위 설정
	ImGui::Text("Spawn Range:");
	// XYZ 일괄 조정 체크박스
	ImGui::Checkbox("Uniform XYZ Range", &bUseUniformSpawnRange);

	if (bUseUniformSpawnRange)
	{
		// [통합 모드] XYZ를 함께 조절합니다.
		// 임시 변수를 사용하여 ImGui 위젯을 제어하고, 변경 시 모든 축에 값을 적용합니다.
		float MinRange = SpawnRangeMin.X;
		float MaxRange = SpawnRangeMax.X;

		ImGui::SetNextItemWidth(80);
		if (ImGui::DragFloat("Min##Uniform", &MinRange, 0.1f, -50.0f, MaxRange - 0.1f))
		{
			// 값이 변경되면 모든 축에 동일하게 적용합니다.
			SpawnRangeMin = FVector({ MinRange, MinRange, MinRange });
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80);
		if (ImGui::DragFloat("Max##Uniform", &MaxRange, 0.1f, MinRange + 0.1f, 50.0f))
		{
			// 값이 변경되면 모든 축에 동일하게 적용합니다.
			SpawnRangeMax = FVector({ MaxRange, MaxRange, MaxRange });
		}
	}
	else
	{
		// [개별 모드] X, Y, Z를 각각 조절합니다.
		const float LabelColumnWidth = 25.0f; // 라벨 정렬을 위한 너비

		ImGui::Text("X");
		ImGui::SameLine(LabelColumnWidth);
		ImGui::SetNextItemWidth(80);
		ImGui::DragFloat("Min##X", &SpawnRangeMin.X, 0.1f, -50.0f, SpawnRangeMax.X - 0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80);
		ImGui::DragFloat("Max##X", &SpawnRangeMax.X, 0.1f, SpawnRangeMin.X + 0.1f, 50.0f);

		ImGui::Text("Y");
		ImGui::SameLine(LabelColumnWidth);
		ImGui::SetNextItemWidth(80);
		ImGui::DragFloat("Min##Y", &SpawnRangeMin.Y, 0.1f, -50.0f, SpawnRangeMax.Y - 0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80);
		ImGui::DragFloat("Max##Y", &SpawnRangeMax.Y, 0.1f, SpawnRangeMin.Y + 0.1f, 50.0f);

		ImGui::Text("Z");
		ImGui::SameLine(LabelColumnWidth);
		ImGui::SetNextItemWidth(80);
		ImGui::DragFloat("Min##Z", &SpawnRangeMin.Z, 0.1f, -50.0f, SpawnRangeMax.Z - 0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80);
		ImGui::DragFloat("Max##Z", &SpawnRangeMax.Z, 0.1f, SpawnRangeMin.Z + 0.1f, 50.0f);
	}

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
		case (EPrimitiveType::Spotlight):
			NewActor = CurrentLevel->SpawnActorToLevel(ASpotLightActor::StaticClass());
			break;
		default:
			break;
		}

		if (NewActor)
		{
			// 범위 내 랜덤 위치
			float RandomX = SpawnRangeMin.X + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax.X - SpawnRangeMin.X);
			float RandomY = SpawnRangeMin.Y + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax.Y - SpawnRangeMin.Y);
			float RandomZ = SpawnRangeMin.Z + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax.Z - SpawnRangeMin.Z);

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
