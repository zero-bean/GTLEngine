#include "pch.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"
#include "Editor/Public/EditorEngine.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/BillboardComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Component/Public/TextRenderComponent.h"
#include "Component/Public/LineComponent.h"
#include "Component/Public/DecalComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Component/Mesh/Public/SphereComponent.h"
#include "Component/Mesh/Public/SquareComponent.h"
#include "Component/Mesh/Public/TriangleComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Global/Function.h"
#include "Core/Public/ObjectIterator.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"
#include "Core/Public/BVHierarchy.h"
#include "Core/Public/Object.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>

#include "Component/Public/HeightFogComponent.h"

bool UActorDetailWidget::bAssetsLoaded = false;
TArray<FTextureOption> UActorDetailWidget::BillboardSpriteOptions;
TArray<FTextureOption> UActorDetailWidget::DecalTextureOptions;

UActorDetailWidget::UActorDetailWidget()
	: UWidget("Actor Detail Widget")
{
	if (!bAssetsLoaded) { LoadAssets(); }
	UE_LOG("ActorDetailWidget: Initialized");
}

UActorDetailWidget::~UActorDetailWidget()
{
	ResetStaticMeshWidgetCache();
}

void UActorDetailWidget::Initialize()
{
	UE_LOG("ActorDetailWidget: Initialized");
}

void UActorDetailWidget::Update()
{
	// 특별한 업데이트 로직이 필요하면 여기에 추가
}

void UActorDetailWidget::RenderWidget()
{
	TObjectPtr CurrentLevel = GEngine->GetCurrentLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	TObjectPtr<AActor> SelectedActor = CurrentLevel->GetSelectedActor();
	if (!SelectedActor)
	{
		ImGui::TextUnformatted("No Object Selected");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Detail 확인을 위해 Object를 선택해주세요");
		return;
	}

	// Actor 헤더 렌더링 (이름 + rename 기능)
	RenderActorHeader(SelectedActor);

	ImGui::Separator();

	// 컴포넌트 트리 렌더링
	RenderComponentTree(SelectedActor);
}

void UActorDetailWidget::LoadAssets()
{
	if (bAssetsLoaded) { return; }

	BillboardSpriteOptions = UAssetManager::GetInstance().GetBillboardSpriteOptions();
	DecalTextureOptions = UAssetManager::GetInstance().GetDecalTextureOptions();
	bAssetsLoaded = true;
}

void UActorDetailWidget::ReleaseAssets()
{
	// BillboardSpriteOptions에 있는 UTexture 객체들을 순회하며 메모리 해제
	for (FTextureOption& Option : BillboardSpriteOptions)
	{
		if (Option.Texture)
		{
			delete Option.Texture.Get();
			Option.Texture = nullptr;
		}
	}
	BillboardSpriteOptions.clear();

	// DecalTextureOptions에 있는 UTexture 객체들을 순회하며 메모리 해제
	for (FTextureOption& Option : DecalTextureOptions)
	{
		if (Option.Texture)
		{
			delete Option.Texture.Get();
			Option.Texture = nullptr;
		}
	}
	DecalTextureOptions.clear();

	bAssetsLoaded = false; // 애셋이 해제되었음을 표시
	UE_LOG("ActorDetailWidget: Released all static assets.");
}

void UActorDetailWidget::RenderActorHeader(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor)
	{
		return;
	}

	FName ActorName = InSelectedActor->GetName();
	FString ActorDisplayName = ActorName.ToString();

	ImGui::Text("[A]");
	ImGui::SameLine();

	if (bIsRenamingActor)
	{
		// Rename 모드
		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("##ActorRename", ActorNameBuffer, sizeof(ActorNameBuffer),
		                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			FinishRenamingActor(InSelectedActor);
		}

		// ESC로 취소, InputManager보다 일단 내부 API로 입력 받는 것으로 처리
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			CancelRenamingActor();
		}
	}
	else
	{
		// 더블클릭으로 rename 시작
		ImGui::Text("%s", ActorDisplayName.data());

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			StartRenamingActor(InSelectedActor);
		}

		// 툴팁 UI
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Double-Click to Rename");
		}

		// Duplicate 버튼 추가
		ImGui::SameLine();
		if (ImGui::Button("Duplicate"))
		{
			DuplicateSelectedActor(InSelectedActor);
		}
	}
}

FString UActorDetailWidget::GenerateUniqueComponentName(AActor* InActor, const FString& InBaseName)
{
	int Suffix = 1;
	std::string NewName = InBaseName;

	auto& Components = InActor->GetOwnedComponents();
	bool bNameExists = true;

	while (bNameExists)
	{
		bNameExists = false;
		for (const auto& Comp : Components)
		{
			if (Comp && Comp->GetName() == NewName)
			{
				++Suffix;
				NewName = InBaseName + std::to_string(Suffix);
				bNameExists = true;
				break;
			}
		}
	}

	return NewName;
}

/**
 * @brief 컴포넌트들을 트리 형태로 표시하는 함수
 * @param InSelectedActor 선택된 Actor
 */
void UActorDetailWidget::RenderComponentTree(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor)
	{
		return;
	}

	const TArray<TObjectPtr<UActorComponent>>& Components = InSelectedActor->GetOwnedComponents();
	USceneComponent* RootSceneComponentRaw = InSelectedActor->GetRootComponent();
	TObjectPtr<USceneComponent> RootSceneComponent = TObjectPtr(RootSceneComponentRaw);
	TObjectPtr<UActorComponent> RootAsActorComponent = Cast<UActorComponent>(RootSceneComponent);
	TObjectPtr<UActorComponent> SelectedComponent = InSelectedActor->GetSelectedComponent();

	ImGui::Text("Components (%d)", static_cast<int>(Components.size()));
	ImGui::SameLine();

	if (ImGui::Button(" + Add "))
	{
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Adds a new component to this actor");
	}

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		auto AddComponentToActor = [&](UActorComponent* NewComponent)
			{
				if (!NewComponent) return;

				// 1. 고유 이름 설정
				FString BaseName = NewComponent->GetClass()->GetClassTypeName().ToString();
				FString UniqueName = GenerateUniqueComponentName(InSelectedActor, BaseName);
				NewComponent->SetName(UniqueName);

				// 2. SceneComponent인 경우 계층 구조 설정
				if (USceneComponent* NewSceneComponent = Cast<USceneComponent>(NewComponent))
				{
					// SetRootComponent가 소유권 등록을 직접 처리하므로 수동 등록을 생략합니다.
					if (!InSelectedActor->GetRootComponent())
					{
						InSelectedActor->SetRootComponent(NewSceneComponent);
					}
					else
					{
						if (TObjectPtr<USceneComponent> ParentComponent = Cast<USceneComponent>(SelectedComponent))
						{
							InSelectedActor->AddComponent(NewComponent, ParentComponent);
						}
					}
				}
			};

		if (ImGui::MenuItem("Text Render Component"))
		{
			AddComponentToActor(new UTextRenderComponent());
		}
		if (ImGui::MenuItem("Billboard Component"))
		{
			AddComponentToActor(new UBillboardComponent());
		}
		if (ImGui::MenuItem("Decal Component"))
		{
			AddComponentToActor(new UDecalComponent());
			InSelectedActor->SetActorTickEnabled(true);
			InSelectedActor->SetTickInEditor(true);
		}
		if (ImGui::MenuItem("SpotLight Component"))
		{
			AddComponentToActor(new USpotLightComponent());
			UBillboardComponent* Billboard = new UBillboardComponent();
			Billboard->SetSprite(ELightType::Spotlight);
			AddComponentToActor(std::move(Billboard));
		}
		if (ImGui::MenuItem("HeightFog Component"))
		{
			AddComponentToActor(new UHeightFogComponent());
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Cube Component"))
		{
			AddComponentToActor(new UCubeComponent());
		}
		if (ImGui::MenuItem("Sphere Component"))
		{
			AddComponentToActor(new USphereComponent());
		}
		if (ImGui::MenuItem("Square Component"))
		{
			AddComponentToActor(new USquareComponent());
		}
		if (ImGui::MenuItem("Triangle Component"))
		{
			AddComponentToActor(new UTriangleComponent());
		}
		if (ImGui::MenuItem("Static Mesh Component"))
		{
			AddComponentToActor(new UStaticMeshComponent());
		}

		ImGui::EndPopup();
	}

	ImGui::SameLine();
	static float RemoveMessageTimer = 0.0f;

	if (ImGui::Button(" - Remove "))
	{
		if (!SelectedComponent || RootAsActorComponent == SelectedComponent)
		{
			RemoveMessageTimer = 2.0f;
		}
		else
		{
			InSelectedActor->RemoveComponent(SelectedComponent);
			SelectedComponent = RootAsActorComponent;
			return;
		}
	}

	if (RemoveMessageTimer > 0.0f)
	{
		ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Cannot remove root component.");
		RemoveMessageTimer -= ImGui::GetIO().DeltaTime;
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Removes the selected component from this actor");
	}

	ImGui::Separator();

	USceneComponent* RootComponentRaw = InSelectedActor->GetRootComponent();
	const TArray<TObjectPtr<UActorComponent>>& AllComponents = InSelectedActor->GetOwnedComponents();

	if (!RootComponentRaw && AllComponents.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No components");
		return;
	}

	// 0. 렌더링된 컴포넌트를 추적하여 중복 렌더링 방지
	TSet<UActorComponent*> RenderedComponents;

	// 1. 루트 컴포넌트부터 시작하여 전체 계층 구조를 재귀적으로 렌더링
	if (RootComponentRaw)
	{
		RenderHierarchyNode(RootComponentRaw, RenderedComponents, InSelectedActor);
	}

	// 2. 계층 구조에 포함되지 않은 나머지 컴포넌트들을 렌더링
	for (const auto& Component : AllComponents)
	{
		if (Component && RenderedComponents.find(Component.Get()) == RenderedComponents.end())
		{
			RenderFlatNode(Component.Get(), InSelectedActor);
		}
	}

	ImGui::Separator();

	if (SelectedComponent)
	{
		RenderComponentDetails(SelectedComponent);
	}
}

/**
 * @brief SceneComponent 계층 구조를 재귀적으로 렌더링하는 함수
 * @param InSceneComponent 렌더링할 SceneComponent
 * @param OutRenderedComponents 렌더링된 컴포넌트를 추적하기 위한 set
 */
void UActorDetailWidget::RenderHierarchyNode(USceneComponent* InSceneComponent,
	TSet<UActorComponent*>& OutRenderedComponents, TObjectPtr<AActor> InSelectedActor)
{
	if (!InSceneComponent) return;

	OutRenderedComponents.insert(InSceneComponent);

	// 컴포넌트 타입에 따른 아이콘 설정
	FString ComponentIcon = "[S]"; // SceneComponent 기본 아이콘
	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(InSceneComponent);
	if (PrimitiveComponent)
	{
		ComponentIcon = "[P]"; // PrimitiveComponent 아이콘
	}

	const bool bIsRootComponent = (InSceneComponent->GetOwner() && InSceneComponent->GetOwner()->GetRootComponent() == InSceneComponent);

	// 노드 라벨 구성
	FString NodeLabel = bIsRootComponent ? "[Root] " : ComponentIcon + " ";
	NodeLabel += InSceneComponent->GetName().ToString();

	// ImGui TreeNode 플래그 설정
	const auto& Children = InSceneComponent->GetChildren();
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
	if (Children.empty())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf; // 자식이 없으면 Leaf 노드로 처리
	}
	if (InSelectedActor->GetSelectedComponent() == InSceneComponent)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	// 루트 컴포넌트는 다른 색상으로 표시
	if (bIsRootComponent)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.82f, 0.25f, 1.0f));
	}

	// TreeNode 렌더링
	bool bNodeIsOpen = ImGui::TreeNodeEx(NodeLabel.c_str(), NodeFlags);

	if (bIsRootComponent)
	{
		ImGui::PopStyleColor();
	}

	// 클릭 시 선택 처리
	if (ImGui::IsItemClicked())
	{
		InSelectedActor->SetSelectedComponent(InSceneComponent);
	}

	// 툴팁 표시
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Component Type: %s", InSceneComponent->GetClass()->GetClassTypeName().ToString().data());
	}

	// PrimitiveComponent의 경우 Visible/Hidden 상태 표시
	if (PrimitiveComponent)
	{
		ImGui::SameLine();
		ImGui::TextColored(
			PrimitiveComponent->IsVisible() ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
			PrimitiveComponent->IsVisible() ? "[Visible]" : "[Hidden]"
		);
	}

	// 노드가 열려있으면 자식 노드들을 재귀적으로 렌더링
	if (bNodeIsOpen)
	{
		for (USceneComponent* Child : Children)
		{
			RenderHierarchyNode(Child, OutRenderedComponents, InSelectedActor);
		}
		ImGui::TreePop(); // 현재 노드를 닫음
	}
}

/**
 * @brief 계층 구조에 속하지 않는 일반 컴포넌트를 렌더링하는 함수
 * @param InComponent 렌더링할 ActorComponent
 */
void UActorDetailWidget::RenderFlatNode(UActorComponent* InComponent, TObjectPtr<AActor> InSelectedActor)
{
	if (!InComponent) return;

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (InSelectedActor->GetSelectedComponent() == InComponent)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	FString NodeLabel = "[C] " + InComponent->GetName().ToString(); // 일반 컴포넌트 아이콘

	ImGui::TreeNodeEx(NodeLabel.c_str(), NodeFlags);

	if (ImGui::IsItemClicked())
	{
		InSelectedActor->SetSelectedComponent(InComponent);
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Component Type: %s", InComponent->GetClass()->GetClassTypeName().ToString().data());
	}
}

void UActorDetailWidget::RenderComponentDetails(TObjectPtr<UActorComponent> InComponent)
{
	if (!InComponent) return;

	FName TypeName = InComponent->GetClass()->GetClassTypeName();
	ImGui::Text("Details for: %s", TypeName.ToString().data());

	if (InComponent->IsA(UTextRenderComponent::StaticClass()))
	{
		UTextRenderComponent* TextComp = Cast<UTextRenderComponent>(InComponent);
		static char TextBuffer[256];
		strncpy_s(TextBuffer, TextComp->GetText().c_str(), sizeof(TextBuffer)-1);

		bool bShowUUID = TextComp->bIsUUIDText;
		if (ImGui::Checkbox("Show UUID", &bShowUUID))
		{
			TextComp->bIsUUIDText = bShowUUID;
		}

		if (!bShowUUID)
		{
			if (ImGui::InputText("Text", TextBuffer, sizeof(TextBuffer)))
			{
				TextComp->SetText(TextBuffer);
			}
		}
	}
	else if (InComponent->IsA(UStaticMeshComponent::StaticClass()))
	{
		UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(InComponent);
		if (UStaticMeshComponentWidget* StaticMeshWidget = GetOrCreateStaticMeshWidget(StaticMesh))
		{
			StaticMeshWidget->SetTargetComponent(StaticMesh);
			StaticMeshWidget->RenderWidget();
		}
	}
	else if (InComponent->IsA(UBillboardComponent::StaticClass()))
	{
		UBillboardComponent* Billboard = Cast<UBillboardComponent>(InComponent);

		auto GetTextureDisplayName = [](UTexture* InTexture) -> FString
			{
				if (!InTexture) return "None";

				FString DisplayName = InTexture->GetName().ToString();
				return "Texture_" + std::to_string(InTexture->GetUUID());
			};

		UTexture* CurrentSprite = Billboard->GetSprite();
		FString PreviewName = GetTextureDisplayName(CurrentSprite);


		if (CurrentSprite)
		{
			const auto Found = std::find_if(BillboardSpriteOptions.begin(),
				BillboardSpriteOptions.end(), [CurrentSprite](const FTextureOption& Option)
				{
					return Option.Texture.Get() == CurrentSprite;
				});

			if (Found != BillboardSpriteOptions.end())
			{
				PreviewName = (*Found).DisplayName;
			}
		}

		if (BillboardSpriteOptions.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No icon textures found under Engine/Asset/Icon");
		}

		if (ImGui::BeginCombo("Sprite", PreviewName.c_str()))
		{
			for (const FTextureOption& Option : BillboardSpriteOptions)
			{
				bool bIsSelected = (Option.Texture.Get() == CurrentSprite);
				if (ImGui::Selectable(Option.DisplayName.c_str(), bIsSelected))
				{
					Billboard->SetSprite(Option.Texture.Get());
				}

				if (bIsSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (CurrentSprite && CurrentSprite->GetRenderProxy())
		{
			if (ID3D11ShaderResourceView* TextureSRV = CurrentSprite->GetRenderProxy()->GetSRV())
			{
				ImGui::Spacing();
				ImGui::Image((ImTextureID)TextureSRV, ImVec2(128, 128));

			}
		}
	}
	else if (InComponent->IsA(UDecalComponent::StaticClass()))
	{
		UDecalComponent* Decal = Cast<UDecalComponent>(InComponent);
		UMaterial* CurrentMaterial = Decal->GetDecalMaterial();
		UTexture* CurrentTexture = CurrentMaterial ? CurrentMaterial->GetDiffuseTexture() : nullptr;

		ImGui::Text("Decal Texture");

		FString PreviewName = "None";
		if (CurrentTexture)
		{
			const auto& Found = std::find_if(DecalTextureOptions.begin(), DecalTextureOptions.end(),
				[CurrentTexture](const FTextureOption& Option)
				{
					// Option에 저장된 파일 경로와 현재 텍스처의 파일 경로를 비교
					return Option.FilePath == CurrentTexture->GetFilePath().ToString();
				});

			if (Found != DecalTextureOptions.end())
			{
				PreviewName = (*Found).DisplayName;
			}
			else
			{
				// Fallback if texture is not in the cached list
				PreviewName = CurrentTexture->GetName().ToString();
			}
		}

		if (DecalTextureOptions.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No textures found in Asset/Texture/");
		}

		if (ImGui::BeginCombo("Texture", PreviewName.c_str()))
		{
			UAssetManager& AssetManager = UAssetManager::GetInstance();

			// 'None' 선택 옵션 추가
			if (ImGui::Selectable("None", CurrentTexture == nullptr))
			{
				// 텍스처가 없는 빈 머티리얼로 설정
				UMaterial* EmptyMaterial = AssetManager.CreateMaterial(FName("None_DecalMaterial"));
				Decal->SetDecalMaterial(EmptyMaterial);
			}

			for (const FTextureOption& Option : DecalTextureOptions)
			{
				bool bIsSelected = (Option.Texture.Get() == CurrentTexture);
				if (ImGui::Selectable(Option.DisplayName.c_str(), bIsSelected))
				{
					UMaterial* NewMaterial = AssetManager.CreateMaterial(Option.DisplayName, Option.FilePath);
					Decal->SetDecalMaterial(NewMaterial);
				}
				if (bIsSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (CurrentTexture)
		{
			if (ID3D11ShaderResourceView* TextureId = CurrentTexture->GetRenderProxy()->GetSRV())
			{
				ImGui::Spacing();
				ImGui::Image((ImTextureID)(intptr_t)TextureId, ImVec2(128, 128));
			}
		}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Fade Effect", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 컴포넌트로부터 페이드 속성에 대한 참조를 직접 가져옵니다.
			FDecalFadeProperty& FadeProps = Decal->GetFadeProperty();

			// Fade Style 콤보박스 추가
			const char* StyleItems[] = { "Standard", "Wipe Left to Right", "Dissolve", "Iris" };
			int CurrentStyle = static_cast<int>(FadeProps.FadeStyle);
			if (ImGui::Combo("Fade Style", &CurrentStyle, StyleItems, IM_ARRAYSIZE(StyleItems)))
			{
				FadeProps.FadeStyle = static_cast<EFadeStyle>(CurrentStyle);
			}

			ImGui::PushItemWidth(120.f); // UI 정렬을 위해 너비 조절

			ImGui::DragFloat("Fade In Duration", &FadeProps.FadeInDuration, 0.01f, 0.0f, 30.0f, "%.2f s");
			ImGui::DragFloat("Fade In Delay", &FadeProps.FadeInStartDelay, 0.01f, 0.0f, 30.0f, "%.2f s");
			ImGui::DragFloat("Fade Out Duration", &FadeProps.FadeDuration, 0.01f, 0.0f, 30.0f, "%.2f s");
			ImGui::DragFloat("Fade Out Delay", &FadeProps.FadeStartDelay, 0.01f, 0.0f, 30.0f, "%.2f s");

			ImGui::PopItemWidth();

			ImGui::Checkbox("Destroy on Fade Out", &FadeProps.bDestroyedAfterFade);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("If checked, the actor will be destroyed after the fade out is complete.");
			}

			// 페이드 효과를 테스트하기 위한 버튼
			if (ImGui::Button("Start Fade In"))
			{
				Decal->StartFadeIn(FadeProps.FadeInDuration, FadeProps.FadeInStartDelay, FadeProps.FadeStyle);
			}
			ImGui::SameLine();
			if (ImGui::Button("Start Fade Out"))
			{
				Decal->StartFadeOut(FadeProps.FadeDuration, FadeProps.FadeStartDelay, FadeProps.bDestroyedAfterFade, FadeProps.FadeStyle);
			}

			ImGui::Separator();

			// 현재 알파 값을 시각적으로 보여주는 프로그레스 바
			ImGui::ProgressBar(Decal->GetFadeAlpha(), ImVec2(-1.0f, 0.0f));
			ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text("Current Alpha");
		}

	}
	else if (InComponent->IsA(USpotLightComponent::StaticClass()))
	{
		USpotLightComponent* Spot = Cast<USpotLightComponent>(InComponent);
		ImGui::Text("Spot Light");

		// --- Color ---
		// 엔진이 선형색(FLinearColor)을 쓴다고 가정. ImGui는 [0..1] float 입력.
		FVector4 Cur = Spot->GetLightColor();
		float ColorRGBA[4] = { Cur.X, Cur.Y, Cur.Z, Cur.W };
		// 알파는 보통 사용 안 하므로 ColorEdit3로도 충분. 필요하면 ColorEdit4로 교체.
		if (ImGui::ColorEdit3("Color", ColorRGBA, ImGuiColorEditFlags_Float))
		{
			Spot->UpdateLightColor(FVector4(ColorRGBA[0], ColorRGBA[1], ColorRGBA[2], 1.0f));
		}
		ImGui::Separator();
	}
	else if (InComponent->IsA(UHeightFogComponent::StaticClass()))
	{
		UHeightFogComponent* Fog = Cast<UHeightFogComponent>(InComponent);
		ImGui::Text("Height Fog");

		FHeightFogConstants FogConstants = Fog->BuildFogConstants();
		FVector4 Cur = Fog->GetFogInscatteringColor(); // 꼭 Linear/HDR!
		float ColorRGBA[4] = { Cur.X, Cur.Y, Cur.Z, Cur.W };

		if (ImGui::ColorEdit3("Color", ColorRGBA, ImGuiColorEditFlags_Float))
		{
			Fog->SetFogInscatteringColor(FVector4(ColorRGBA[0], ColorRGBA[1], ColorRGBA[2], 1.0f));
		}

		float Density = FogConstants.FogDensity;
		ImGui::DragFloat("Density", &Density, 0.01f, 0.0f, 1.0f, "%.2f s");
		Fog->SetFogDensity(Density);

		float HeightFalloff = FogConstants.FogHeightFalloff;
		ImGui::DragFloat("HeightFalloff", &HeightFalloff, 0.01f, 0.0f, 10.0f, "%.2f s");
		Fog->SetFogHeightFalloff(HeightFalloff);

		float StartDistance = FogConstants.StartDistance;
		ImGui::DragFloat("StartDistance", &StartDistance, 0.01f, 0.0f, 100.0f, "%.2f s");
		Fog->SetStartDistance(StartDistance);

		float FogCutoffDistance = FogConstants.FogCutoffDistance;
		ImGui::DragFloat("FogCutoffDistance", &FogCutoffDistance, 0.01f, 0.0f, 100.0f, "%.2f s");
		Fog->SetFogCutoffDistance(FogCutoffDistance);

		float FogMaxOpacity = FogConstants.FogMaxOpacity;
		ImGui::DragFloat("FogMaxOpacity", &FogMaxOpacity, 0.01f, 0.0f, 1.0f, "%.2f s");
		Fog->SetFogMaxOpacity(FogMaxOpacity);

		float FogHeight = FogConstants.FogHeight;
		ImGui::DragFloat("FogHeight", &FogHeight, 0.01f, 0.0f, 100.0f, "%.2f s");
		Fog->SetFogHeight(FogHeight);

		ImGui::Separator();
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.0f), "No detail view for this component type.");
	}

	ImGui::Separator();
	ImGui::Text("Component Transform");

	TObjectPtr<USceneComponent> SceneComponent = Cast<USceneComponent>(InComponent);
	if (!SceneComponent)
	{
		return;
	}
	AActor* OwningActor = SceneComponent->GetOwner();
	const bool bIsRootComponent = (OwningActor && OwningActor->GetRootComponent() == SceneComponent.Get());
	if (bIsRootComponent)
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Root component uses actor transform; relative adjustments are disabled.");
		ImGui::BeginDisabled();
	}
	bool bTransformChanged = false;

	FVector RelativeLocation = SceneComponent->GetRelativeLocation();
	float LocationArr[3] = { RelativeLocation.X, RelativeLocation.Y, RelativeLocation.Z };

	if (ImGui::DragFloat3("Relative Location", LocationArr, 0.1f))
	{
		SceneComponent->SetRelativeLocation(FVector(LocationArr[0], LocationArr[1], LocationArr[2]));
		bTransformChanged = true;
	}

	FVector RelativeRotation = SceneComponent->GetRelativeRotation();
	float RotationArr[3] = { RelativeRotation.X, RelativeRotation.Y, RelativeRotation.Z };
	if (ImGui::DragFloat3("Relative Rotation", RotationArr, 0.1f))
	{
		SceneComponent->SetRelativeRotation(FVector(RotationArr[0], RotationArr[1], RotationArr[2]));
		bTransformChanged = true;
	}

	bool bUniformScale = SceneComponent->IsUniformScale();
	FVector RelativeScale = SceneComponent->GetRelativeScale3D();

	if (bUniformScale)
	{
		float UniformScale = RelativeScale.X;
		if (ImGui::DragFloat("Relative Scale", &UniformScale, 0.01f, 0.01f, 10.0f))
		{
			SceneComponent->SetRelativeScale3D(FVector(UniformScale, UniformScale, UniformScale));
			bTransformChanged = true;
		}
	}
	else
	{
		float ScaleArr[3] = { RelativeScale.X, RelativeScale.Y, RelativeScale.Z };
		if (ImGui::DragFloat3("Relative Scale", ScaleArr, 0.01f))
		{
			SceneComponent->SetRelativeScale3D(FVector(ScaleArr[0], ScaleArr[1], ScaleArr[2]));
			bTransformChanged = true;
		}
	}

	if (ImGui::Checkbox("Relative Uniform Scale", &bUniformScale))
	{
		SceneComponent->SetUniformScale(bUniformScale);
		bTransformChanged = true;

		if (bUniformScale)
		{
			float UniformScale = SceneComponent->GetRelativeScale3D().X;
			SceneComponent->SetRelativeScale3D(FVector(UniformScale, UniformScale, UniformScale));
		}
	}


	if (bIsRootComponent)
	{
		ImGui::EndDisabled();
	}
	if (bTransformChanged && InComponent->IsA(UPrimitiveComponent::StaticClass()))
	{
		UBVHierarchy::GetInstance().Refit();
	}
}

void UActorDetailWidget::StartRenamingActor(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	bIsRenamingActor = true;
	FString CurrentName = InActor->GetName().ToString();
	strncpy_s(ActorNameBuffer, CurrentName.data(), sizeof(ActorNameBuffer) - 1);
	ActorNameBuffer[sizeof(ActorNameBuffer) - 1] = '\0';

	UE_LOG("ActorDetailWidget: '%s' 에 대한 이름 변경 시작", CurrentName.data());
}

void UActorDetailWidget::FinishRenamingActor(TObjectPtr<AActor> InActor)
{
	if (!InActor || !bIsRenamingActor)
	{
		return;
	}

	FString NewName = ActorNameBuffer;
	if (!NewName.empty() && NewName != InActor->GetName().ToString())
	{
		// Actor 이름 변경
		InActor->SetDisplayName(NewName);
		UE_LOG_SUCCESS("ActorDetailWidget: Actor의 이름을 '%s' (으)로 변경하였습니다", NewName.data());
	}

	bIsRenamingActor = false;
	ActorNameBuffer[0] = '\0';
}

void UActorDetailWidget::CancelRenamingActor()
{
	bIsRenamingActor = false;
	ActorNameBuffer[0] = '\0';
	UE_LOG_WARNING("ActorDetailWidget: 이름 변경 취소");
}

void UActorDetailWidget::DuplicateSelectedActor(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	ULevel* CurrentLevel = GEngine->GetCurrentLevel();
	if (!CurrentLevel)
	{
		return;
	}

	AActor* NewActor = DuplicateObject(InActor, CurrentLevel, FName::GetNone());

	if (NewActor)
	{
		//FVector Location = NewActor->GetActorLocation();
		//NewActor->SetActorLocation(Location + FVector(1.0f, 0.0f, 0.0f)); // Offset by 100 on X

		CurrentLevel->RegisterDuplicatedActor(NewActor);
	}
}

UStaticMeshComponentWidget* UActorDetailWidget::GetOrCreateStaticMeshWidget(UStaticMeshComponent* InComponent)
{
	if (!InComponent)
	{
		return nullptr;
	}

	AActor* OwningActor = InComponent->GetOwner();
	if (StaticMeshWidgetOwner != OwningActor)
	{
		ResetStaticMeshWidgetCache();
		StaticMeshWidgetOwner = OwningActor;
	}

	if (OwningActor)
	{
		PruneInvalidStaticMeshWidgets(OwningActor->GetOwnedComponents());
	}

	TObjectPtr<UStaticMeshComponent> ComponentPtr = InComponent;
	if (auto It = StaticMeshWidgetMap.find(ComponentPtr); It != StaticMeshWidgetMap.end())
	{
		return It->second;
	}

	UStaticMeshComponentWidget* NewWidget = new UStaticMeshComponentWidget();
	NewWidget->SetTargetComponent(InComponent);
	StaticMeshWidgetMap.emplace(ComponentPtr, NewWidget);
	return NewWidget;
}

void UActorDetailWidget::ResetStaticMeshWidgetCache()
{
	for (auto& Pair : StaticMeshWidgetMap)
	{
		if (Pair.second)
		{
			SafeDelete(Pair.second);
		}
	}
	StaticMeshWidgetMap.clear();
	StaticMeshWidgetOwner = nullptr;
}

void UActorDetailWidget::PruneInvalidStaticMeshWidgets(const TArray<TObjectPtr<UActorComponent>>& InComponents)
{
	if (StaticMeshWidgetMap.empty())
	{
		return;
	}

	TSet<UStaticMeshComponent*> ValidComponents;
	ValidComponents.reserve(InComponents.size());

	for (const TObjectPtr<UActorComponent>& ComponentPtr : InComponents)
	{
		if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(ComponentPtr.Get()))
		{
			ValidComponents.insert(StaticMeshComponent);
		}
	}

	for (auto It = StaticMeshWidgetMap.begin(); It != StaticMeshWidgetMap.end();)
	{
		UStaticMeshComponent* Component = It->first.Get();
		if (!Component || ValidComponents.find(Component) == ValidComponents.end())
		{
			if (It->second)
			{
				SafeDelete(It->second);
			}
			It = StaticMeshWidgetMap.erase(It);
		}
		else
		{
			++It;
		}
	}
}
