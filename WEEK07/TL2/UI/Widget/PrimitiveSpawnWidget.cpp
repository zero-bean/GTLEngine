#include "pch.h"
#include "PrimitiveSpawnWidget.h"
#include "../UIManager.h"
#include "../../ImGui/imgui.h"
#include "../../World.h"
#include "../../StaticMeshActor.h"
#include "../../DecalActor.h"
#include "../../DecalSpotLightActor.h"
#include "../../Vector.h"
#include "ExponentialHeightFog.h"
#include "FXAAActor.h"
#include "ObjManager.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include "SpotLightActor.h"
#include "ObjectIterator.h"

//// UE_LOG 대체 매크로
//#define UE_LOG(fmt, ...)

// std 함수들 정의
using std::max;
using std::min;
using std::to_string;

static inline FString GetBaseNameNoExt(const FString& Path)
{
	const size_t Sep = Path.find_last_of("/\\");
	const size_t Start = (Sep == FString::npos) ? 0 : Sep + 1;

	const FString Ext = ".obj";
	size_t End = Path.size();
	if (End >= Ext.size() && Path.compare(End - Ext.size(), Ext.size(), Ext) == 0)
	{
		End -= Ext.size();
	}

	if (Start <= End) 
	{
		return Path.substr(Start, End - Start);
	}

	return Path;
}

UPrimitiveSpawnWidget::UPrimitiveSpawnWidget()
	: UWidget("Primitive Spawn Widget")
	, UIManager(&UUIManager::GetInstance())
{
	// 랜덤 시드 초기화
	srand(static_cast<unsigned int>(time(nullptr)));
}

UPrimitiveSpawnWidget::~UPrimitiveSpawnWidget() = default;

void UPrimitiveSpawnWidget::Initialize()
{
	// UIManager 참조 확보
	UIManager = &UUIManager::GetInstance();

    // Reflection을 통해 스폰 가능한 모든 Actor 자동 탐색
    if (SpawnableClasses.empty())
    {
        SpawnableClasses = UClassRegistry::Get().GetSpawnableClasses(AActor::StaticClass());
        UE_LOG("PrimitiveSpawnWidget: Found %d spawnable actor classes", SpawnableClasses.size());
    }
}

void UPrimitiveSpawnWidget::Update()
{
	// 필요시 업데이트 로직 추가
}

UWorld* UPrimitiveSpawnWidget::GetCurrentWorld() const
{
	if (!UIManager)
		return nullptr;
		
	return UIManager->GetWorld();
}

const char* UPrimitiveSpawnWidget::GetPrimitiveTypeName(int32 TypeIndex) const
{
	switch (TypeIndex)
	{
	case 0: return "StaticMesh";
	default: return "Unknown";
	}
}

FVector UPrimitiveSpawnWidget::GenerateRandomLocation() const
{
	float RandomX = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
	float RandomY = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
	float RandomZ = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
	
	return FVector(RandomX, RandomY, RandomZ);
}

float UPrimitiveSpawnWidget::GenerateRandomScale() const
{
	if (!bRandomScale)
		return 1.0f;
		
	return MinScale + (static_cast<float>(rand()) / RAND_MAX) * (MaxScale - MinScale);
}

FQuat UPrimitiveSpawnWidget::GenerateRandomRotation() const
{
	if (!bRandomRotation)
		return FQuat::Identity();
	
	// 랜덤 오일러 각도 생성 (도 단위)
	float RandomPitch = (static_cast<float>(rand()) / RAND_MAX) * 360.0f - 180.0f;
	float RandomYaw = (static_cast<float>(rand()) / RAND_MAX) * 360.0f - 180.0f;
	float RandomRoll = (static_cast<float>(rand()) / RAND_MAX) * 360.0f - 180.0f;
	
	return FQuat::MakeFromEuler(FVector(RandomPitch, RandomYaw, RandomRoll));
}

void UPrimitiveSpawnWidget::RenderWidget()
{
    ImGui::Text("Actor Spawner");
    ImGui::Spacing();

    if (SpawnableClasses.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No spawnable actors found!");
        return;
    }

    // 스폰 가능한 Actor 타입 목록 생성
    TArray<const char*> ActorDisplayNames;
    for (UClass* ActorClass : SpawnableClasses)
    {
        ActorDisplayNames.push_back(ActorClass->GetDisplayName());
    }

    // 선택된 Actor 타입
    ImGui::Text("Actor Type:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    ImGui::Combo("##ActorType", &SelectedClassIndex, ActorDisplayNames.data(), static_cast<int>(ActorDisplayNames.size()));

    UClass* SelectedClass = (SelectedClassIndex >= 0 && SelectedClassIndex < static_cast<int>(SpawnableClasses.size()))
        ? SpawnableClasses[SelectedClassIndex]
        : nullptr;

    // Static Mesh Actor인 경우 메시 선택 UI 표시
    if (SelectedClass && SelectedClass->IsChildOf(AStaticMeshActor::StaticClass()))
    {
        auto& ResourceManager = UResourceManager::GetInstance();

        // 최신 목록 갱신 (캐시 보관)
        CachedMeshFilePaths = ResourceManager.GetAllStaticMeshFilePaths();

        // 표시용 이름(파일명 스템)과 ImGui 아이템 배열 구성
        TArray<FString> DisplayNames;
        DisplayNames.reserve(CachedMeshFilePaths.size());
        for (const FString& Path : CachedMeshFilePaths)
        {
            DisplayNames.push_back(GetBaseNameNoExt(Path));
        }

        TArray<const char*> Items;
        Items.reserve(DisplayNames.size());
        for (const FString& Name : DisplayNames)
        {
            Items.push_back(Name.c_str());
        }

        // 기본 선택: Cube가 있으면 자동 선택
        if (SelectedMeshIndex == -1)
        {
            for (int32 i = 0; i < static_cast<int32>(CachedMeshFilePaths.size()); ++i)
            {
                if (GetBaseNameNoExt(CachedMeshFilePaths[i]) == "Sphere" ||
                    CachedMeshFilePaths[i] == "Data/Sphere.obj")
                {
                    SelectedMeshIndex = i;
                    break;
                }
            }
        }

        ImGui::Text("Static Mesh:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220);
        ImGui::Combo("##StaticMeshList", &SelectedMeshIndex, Items.data(), static_cast<int>(Items.size()));
        ImGui::SameLine();
        if (ImGui::Button("Clear##StaticMesh"))
        {
            SelectedMeshIndex = -1;
        }

        // Optional: 디버그용 트리
        if (ImGui::TreeNode("Registered Static Meshes"))
        {
            for (const FString& Path : CachedMeshFilePaths)
                ImGui::BulletText("%s", Path.c_str());
            ImGui::TreePop();
        }
    }
    else
    {
        ImGui::Text("%s does not require additional resources.", SelectedClass ? SelectedClass->GetDisplayName() : "Unknown");
    }

    // 스폰 개수 설정
    ImGui::Text("Number of Spawn:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputInt("##NumberOfSpawn", &NumberOfSpawn);
    NumberOfSpawn = max(1, min(100, NumberOfSpawn));

    // 스폰 버튼
    ImGui::SameLine();
    if (ImGui::Button("Spawn Actors"))
    {
        if (SelectedClass)
        {
            SpawnActor(SelectedClass);
        }
    }

	////Obj Parser 테스트용
	//static std::string fileName;  // 입력값 저장용
	//// 입력창
	//char buffer[256];
	//strncpy_s(buffer, fileName.c_str(), sizeof(buffer));
	//buffer[sizeof(buffer) - 1] = '\0';

	//if (ImGui::InputText("file name", buffer, sizeof(buffer))) {
	//	fileName = buffer;  // std::string으로 갱신
	//}
	//// 버튼
	//if (ImGui::Button("Spawn Dice Test")) {
	//	FObjManager::LoadObjStaticMesh("spaceCompound.obj");
	//}

	ImGui::Spacing();
	ImGui::Separator();

    // 스폰 범위 설정
    ImGui::Text("Spawn Settings");

    ImGui::Text("Position Range:");
    ImGui::SetNextItemWidth(100);
    ImGui::DragFloat("Min##SpawnRange", &SpawnRangeMin, 0.1f, -50.0f, SpawnRangeMax - 0.1f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::DragFloat("Max##SpawnRange", &SpawnRangeMax, 0.1f, SpawnRangeMin + 0.1f, 50.0f);

    // 랜덤 회전 옵션
    ImGui::Checkbox("Random Rotation", &bRandomRotation);

    // 랜덤 스케일 옵션
    ImGui::Checkbox("Random Scale", &bRandomScale);

    if (bRandomScale)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("Min##Scale", &MinScale, 0.01f, 0.1f, MaxScale - 0.01f);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("Max##Scale", &MaxScale, 0.01f, MinScale + 0.01f, 10.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 월드 상태 정보
    UWorld* World = GetCurrentWorld();
    if (World)
    {
        ImGui::Text("World Status: Connected");
        ImGui::Text("Current Actors: %zu", World->GetActors().size());
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "World Status: Not Available");
    }

    ImGui::Spacing();

    // 빠른 스폰 버튼들
    ImGui::Text("Quick Spawn:");
    if (ImGui::Button("Spawn 1 Cube"))
    {
        // Static Mesh Actor 찾기
        UClass* StaticMeshClass = nullptr;
        for (int32 i = 0; i < static_cast<int32>(SpawnableClasses.size()); ++i)
        {
            if (SpawnableClasses[i]->IsChildOf(AStaticMeshActor::StaticClass()))
            {
                StaticMeshClass = SpawnableClasses[i];
                SelectedClassIndex = i;
                break;
            }
        }

        if (StaticMeshClass)
        {
            NumberOfSpawn = 1;
            // 기본 선택을 Cube로 강제
            if (!CachedMeshFilePaths.empty())
            {
                SelectedMeshIndex = -1;
                for (int32 i = 0; i < static_cast<int32>(CachedMeshFilePaths.size()); ++i)
                {
                    if (GetBaseNameNoExt(CachedMeshFilePaths[i]) == "Cube" ||
                        CachedMeshFilePaths[i] == "Data/Cube.obj")
                    {
                        SelectedMeshIndex = i;
                        break;
                    }
                }
            }
            SpawnActor(StaticMeshClass);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Spawn 20 Random"))
    {
        // Static Mesh Actor 찾기
        UClass* StaticMeshClass = nullptr;
        for (int32 i = 0; i < static_cast<int32>(SpawnableClasses.size()); ++i)
        {
            if (SpawnableClasses[i]->IsChildOf(AStaticMeshActor::StaticClass()))
            {
                StaticMeshClass = SpawnableClasses[i];
                SelectedClassIndex = i;
                break;
            }
        }

        if (StaticMeshClass)
        {
            NumberOfSpawn = 20;
            SpawnActor(StaticMeshClass);
        }
    }
}

void UPrimitiveSpawnWidget::SpawnActor(UClass* ActorClass) const
{
    UWorld* World = GetCurrentWorld();
    if (!World)
    {
        UE_LOG("PrimitiveSpawn: No World available for spawning");
        return;
    }

    if (!ActorClass || !ActorClass->CreateInstance)
    {
        UE_LOG("ActorSpawn: Failed - Invalid actor class");
        return;
    }

    UE_LOG("ActorSpawn: Spawning %d %s actors", NumberOfSpawn, ActorClass->GetDisplayName());

    int32 SuccessCount = 0;
    for (int32 i = 0; i < NumberOfSpawn; i++)
    {
        FVector SpawnLocation = FVector(0, 0, 0);
        FQuat   SpawnRotation = FQuat::Identity();
        float   SpawnScale = 1.0f;
        FVector SpawnScaleVec(SpawnScale, SpawnScale, SpawnScale);
        FTransform SpawnTransform(SpawnLocation, SpawnRotation, SpawnScaleVec);

        // 특정 Actor 타입에 대한 기본 회전 설정
        if (ActorClass->IsChildOf(ADecalSpotLightActor::StaticClass()))
        {
            SpawnTransform.Rotation = FQuat::MakeFromEuler(FVector(0, 89.5, 0));
        }
        else if (ActorClass->IsChildOf(ASpotLightActor::StaticClass()))
        {
            SpawnTransform.Rotation = FQuat::MakeFromEuler(FVector(0.0f, 89.5f, 0.0f));
        }

        // ObjectFactory::NewObject를 통해 생성 (GUObjectArray에 등록됨)
        AActor* NewActor = Cast<AActor>(NewObject(ActorClass));
        if (NewActor)
        {
            // World에 스폰 (SetWorld, InitEmptyActor, 컴포넌트 등록 포함)
            // 이 시점에서 RootComponent가 생성됨
            World->SpawnActor(NewActor);

            // Static Mesh Actor인 경우 메시 설정
            FString ActorName;
            if (ActorClass->IsChildOf(AStaticMeshActor::StaticClass()))
            {
                if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(NewActor))
                {
                    // 드롭다운에서 선택한 리소스가 있으면 그걸 사용, 아니면 Cube로 기본 설정
                    FString MeshPath = "Data/Cube.obj";
                    const bool bHasResourceSelection =
                        (SelectedMeshIndex >= 0) &&
                        (SelectedMeshIndex < static_cast<int32>(CachedMeshFilePaths.size()));

                    if (bHasResourceSelection)
                    {
                        MeshPath = CachedMeshFilePaths[SelectedMeshIndex];
                    }

                    if (auto* StaticMeshComp = MeshActor->GetStaticMeshComponent())
                    {
                        StaticMeshComp->SetStaticMesh(MeshPath);
                    }

                    ActorName = World->GenerateUniqueActorName(
                        bHasResourceSelection ? GetBaseNameNoExt(MeshPath).c_str() : ActorClass->GetDisplayName()
                    );
                    MeshActor->SetName(ActorName);
                }
            }
            // 다른 Actor 타입
            else
            {
                ActorName = World->GenerateUniqueActorName(ActorClass->GetDisplayName());
                NewActor->SetName(ActorName);
            }

            // Transform 설정 (SpawnActor 후, RootComponent가 생성된 후)
            NewActor->SetActorTransform(SpawnTransform);

            UE_LOG("ActorSpawn: Created %s at (%.2f, %.2f, %.2f)", ActorClass->GetDisplayName(), SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
            SuccessCount++;
        }
        else
        {
            UE_LOG("ActorSpawn: Failed - CreateInstance returned nullptr for type %s.", ActorClass->Name);
        }
    }
    UE_LOG("ActorSpawn: Successfully spawned %d/%d actors", SuccessCount, NumberOfSpawn);
}