#include "pch.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Core/Public/ObjectIterator.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"

IMPLEMENT_CLASS(UStaticMeshComponentWidget, UWidget)

void UStaticMeshComponentWidget::SetTargetComponent(UStaticMeshComponent* InComponent)
{
	StaticMeshComponent = InComponent;
}

void UStaticMeshComponentWidget::RenderWidget()
{
	if (!StaticMeshComponent)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No static mesh component selected.");
		return;
	}

	if (!StaticMeshComponent->GetStaticMesh())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Static mesh asset is missing.");
		return;
	}

	RenderStaticMeshSelector();
	ImGui::Separator();
	RenderMaterialSections();
	ImGui::Separator();
	RenderOptions();
}

void UStaticMeshComponentWidget::RenderStaticMeshSelector()
{
	// 1. 현재 컴포넌트에 할당된 스태틱 메시를 가져옵니다.
	UStaticMesh* CurrentStaticMesh = StaticMeshComponent->GetStaticMesh();
	const FName PreviewName = CurrentStaticMesh ? CurrentStaticMesh->GetAssetPathFileName() : "None";

	// 2. ImGui::BeginCombo를 사용하여 드롭다운 메뉴를 시작합니다.
	// 첫 번째 인자는 라벨, 두 번째 인자는 닫혀 있을 때 표시될 텍스트입니다.
	if (ImGui::BeginCombo("Static Mesh", PreviewName.ToString().c_str()))
	{
		// 3. TObjectIterator로 모든 UStaticMesh 에셋을 순회합니다.
		for (TObjectIterator<UStaticMesh> It; It; ++It)
		{
			UStaticMesh* MeshInList = *It;
			if (!MeshInList) continue;

			// 현재 선택된 항목인지 확인합니다.
			const bool bIsSelected = (CurrentStaticMesh == MeshInList);

			// 4. ImGui::Selectable로 각 항목을 만듭니다.
			// 사용자가 이 항목을 클릭하면 if문이 true가 됩니다.
			if (ImGui::Selectable(MeshInList->GetAssetPathFileName().ToString().c_str(), bIsSelected))
			{
				// 5. 항목이 선택되면, 컴포넌트의 스태틱 메시를 교체합니다.
				StaticMeshComponent->SetStaticMesh(MeshInList->GetAssetPathFileName());
			}

			// 현재 선택된 항목에 포커스를 맞춰서 드롭다운이 열렸을 때 바로 보이게 합니다.
			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		// 6. 드롭다운 메뉴를 닫습니다.
		ImGui::EndCombo();
	}
}

void UStaticMeshComponentWidget::RenderMaterialSections()
{
	UStaticMesh* CurrentMesh = StaticMeshComponent->GetStaticMesh();
	if (!CurrentMesh)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Static mesh asset is missing.");
		return;
	}

	FStaticMesh* MeshAsset = CurrentMesh->GetStaticMeshAsset();
	if (!MeshAsset)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Static mesh data is not available.");
		return;
	}

	ImGui::Text("Material Slots (%d)", static_cast<int>(MeshAsset->MaterialInfo.size()));

	// 머티리얼 슬롯을 순회합니다.
	for (int32 SlotIndex = 0; SlotIndex < MeshAsset->MaterialInfo.size(); ++SlotIndex)
	{
		// 현재 슬롯에 할당된 머티리얼을 가져옵니다.
		UMaterial* CurrentMaterial = StaticMeshComponent->GetMaterial(SlotIndex);
		FString PreviewName = GetMaterialDisplayName(CurrentMaterial);

		ImGui::PushID(SlotIndex);

		// 머티리얼 선택 콤보박스를 렌더링합니다.
		std::string Label = "Element " + std::to_string(SlotIndex);
		if (ImGui::BeginCombo(Label.c_str(), PreviewName.c_str()))
		{
			RenderAvailableMaterials(SlotIndex);
			ImGui::EndCombo();
		}

		// --- 머티리얼의 텍스처 프리뷰를 렌더링하는 로직 ---
		if (CurrentMaterial)
		{
			// 1. 머티리얼에서 Diffuse 텍스처를 가져옵니다.
			UTexture* DiffuseTexture = CurrentMaterial->GetDiffuseTexture();

			// 2. 텍스처와 렌더 프록시가 유효한지 확인합니다.
			if (DiffuseTexture && DiffuseTexture->GetRenderProxy())
			{
				// 3. 렌더 프록시에서 실제 GPU 리소스(SRV)를 가져옵니다.
				if (ID3D11ShaderResourceView* TextureSRV = DiffuseTexture->GetRenderProxy()->GetSRV())
				{
					ImGui::Spacing();
					// 4. ImGui::Image를 사용해 프리뷰를 표시합니다.
					ImGui::Image((ImTextureID)TextureSRV, ImVec2(128, 128));
				}
			}
		}

		ImGui::PopID();
	}
}

void UStaticMeshComponentWidget::RenderAvailableMaterials(int32 TargetSlotIndex)
{
	// 모든 UMaterial 순회
	for (TObjectIterator<UMaterial> It; It; ++It)
	{
		UMaterial* Mat = *It;
		if (!Mat) continue;

		FString MatName = GetMaterialDisplayName(Mat);
		bool bIsSelected = (StaticMeshComponent->GetMaterial(TargetSlotIndex) == Mat);

		if (ImGui::Selectable(MatName.c_str(), bIsSelected))
		{
			// Material을 직접 참조
			StaticMeshComponent->SetMaterial(TargetSlotIndex, Mat);
		}

		if (bIsSelected)
		{
			ImGui::SetItemDefaultFocus();
		}
	}
}

void UStaticMeshComponentWidget::RenderOptions()
{
	bool bIsScrollEnabled = StaticMeshComponent->IsScrollEnabled();

	if (ImGui::Checkbox("Enable Scroll", &bIsScrollEnabled))
	{
		if (bIsScrollEnabled)
		{
			StaticMeshComponent->EnableScroll();
		}
		else
		{
			StaticMeshComponent->DisableScroll();
		}
	}
}

FString UStaticMeshComponentWidget::GetMaterialDisplayName(UMaterial* Material) const
{
	if (!Material)
	{
		return "None";
	}

	// 1순위: UObject 이름 사용 (기본 "Object_" 형식이 아닌 경우)
	FString ObjectName = Material->GetName().ToString();
	if (!ObjectName.empty() && ObjectName.find("Object_") != 0)
	{
		return ObjectName;
	}

	// 2순위: Diffuse 텍스처 파일 이름 사용
	UTexture* DiffuseTexture = Material->GetDiffuseTexture();
	if (DiffuseTexture)
	{
		FString TexturePath = DiffuseTexture->GetFilePath().ToString();
		if (!TexturePath.empty())
		{
			// 파일 이름만 추출 (확장자 제외)
			size_t LastSlash = TexturePath.find_last_of("/\\");
			size_t LastDot = TexturePath.find_last_of(".");

			if (LastSlash != std::string::npos)
			{
				FString FileName = TexturePath.substr(LastSlash + 1);
				if (LastDot != std::string::npos && LastDot > LastSlash)
				{
					FileName = FileName.substr(0, LastDot - LastSlash - 1);
				}
				return FileName + " (Mat)";
			}
		}
	}

	// 3순위: 다른 텍스처들도 시도
	TArray<UTexture*> Textures = {
		Material->GetAmbientTexture(),
		Material->GetSpecularTexture(),
		Material->GetNormalTexture(),
		Material->GetAlphaTexture(),
		Material->GetBumpTexture()
	};

	for (UTexture* Texture : Textures)
	{
		if (Texture)
		{
			FString TexturePath = Texture->GetFilePath().ToString();
			if (!TexturePath.empty())
			{
				size_t LastSlash = TexturePath.find_last_of("/\\");
				size_t LastDot = TexturePath.find_last_of(".");

				if (LastSlash != std::string::npos)
				{
					FString FileName = TexturePath.substr(LastSlash + 1);
					if (LastDot != std::string::npos && LastDot > LastSlash)
					{
						FileName = FileName.substr(0, LastDot - LastSlash - 1);
					}
					return FileName + " (Mat)";
				}
			}
		}
	}

	// 4순위: UUID 기반 이름 사용
	return "Material_" + std::to_string(Material->GetUUID());
}
