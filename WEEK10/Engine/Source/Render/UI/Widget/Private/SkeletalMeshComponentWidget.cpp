#include "pch.h"

#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Core/Public/ObjectIterator.h"
#include "Level/Public/Level.h"
#include "Render/UI/Widget/Public/SkeletalMeshComponentWidget.h"
#include "Runtime/Engine/Public/SkeletalMesh.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"

IMPLEMENT_CLASS(USkeletalMeshComponentWidget, UWidget)

void USkeletalMeshComponentWidget::RenderWidget()
{
	ULevel* CurrentLevel = GWorld->GetLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	UActorComponent* Component = GEditor->GetEditorModule()->GetSelectedComponent();
	if (!Component)
	{
		ImGui::TextUnformatted("No Object Selected");
		return;
	}
	SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component);

	if (!SkeletalMeshComponent)
	{
		return;
	}

	if (!SkeletalMeshComponent->GetSkeletalMeshAsset())
	{
		return;
	}

	// 모든 입력 필드를 검은색으로 설정
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

	RenderSkeletalMeshSelector();
	ImGui::Separator();
	RenderMaterialSections();
	ImGui::Separator();
	RenderOptions();

	ImGui::PopStyleColor(5);
}

void USkeletalMeshComponentWidget::RenderSkeletalMeshSelector()
{
	USkeletalMesh* CurrentSkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	UStaticMesh* CurrentStaticMesh = CurrentSkeletalMesh->GetStaticMesh();
	const FName PreviewName = CurrentStaticMesh ? CurrentStaticMesh->GetAssetPathFileName() : "None";

	if (ImGui::BeginCombo("Skeletal Mesh", PreviewName.ToString().c_str()))
	{
		for (TObjectIterator<USkeletalMesh> It; It; ++It)
		{
			USkeletalMesh* MeshInList = *It;
			UStaticMesh* StaticMeshInList = MeshInList->GetStaticMesh();
			if (!MeshInList) continue;

			const bool bIsSelected = (CurrentSkeletalMesh == MeshInList);

			if (ImGui::Selectable(StaticMeshInList->GetAssetPathFileName().ToString().c_str(), bIsSelected))
			{
				SkeletalMeshComponent->SetSkeletalMeshAsset(MeshInList);
			}

			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

void USkeletalMeshComponentWidget::RenderMaterialSections()
{
	USkeletalMesh* CurrentMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!CurrentMesh || !CurrentMesh->GetStaticMesh())
	{
		return;
	}
	FStaticMesh* MeshAsset = CurrentMesh->GetStaticMesh()->GetStaticMeshAsset();

	ImGui::Text("Material Slots (%d)", static_cast<int>(MeshAsset->MaterialInfo.Num()));

	for (int32 SlotIndex = 0; SlotIndex < MeshAsset->MaterialInfo.Num(); ++SlotIndex)
	{
		UMaterial* CurrentMaterial = SkeletalMeshComponent->GetMaterial(SlotIndex);

		if (!CurrentMaterial)
		{
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Material Slot %d: (None)", SlotIndex);
			ImGui::Dummy(ImVec2(1.0f, 10.0f));
			continue;
		}

		FString PreviewName = CurrentMaterial ? GetMaterialDisplayName(CurrentMaterial) : "None";
		ImGui::PushID(SlotIndex);

		std::string Label = "Element " + std::to_string(SlotIndex);
		ImGui::TextUnformatted(Label.c_str());

		const float PreviewSize = 64.0f;
		UTexture* MaterialPreviewTexture = GetPreviewTextureForMaterial(CurrentMaterial);
		ID3D11ShaderResourceView* ShaderResourceView = nullptr;
		if (MaterialPreviewTexture != nullptr)
		{
			ShaderResourceView = MaterialPreviewTexture->GetTextureSRV();
		}

		if (ShaderResourceView != nullptr)
		{
			ImGui::Image((ImTextureID)ShaderResourceView, ImVec2(PreviewSize, PreviewSize), ImVec2(0, 0), ImVec2(1, 1));
		}
		else
		{
			ImGui::Dummy(ImVec2(PreviewSize, PreviewSize));
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

		std::string ComboId = "##MaterialSlotCombo_" + std::to_string(SlotIndex);
		if (ImGui::BeginCombo(ComboId.c_str(), PreviewName.c_str()))
		{
			RenderAvailableMaterials(SlotIndex);
			ImGui::EndCombo();
		}

		auto RenderColorPicker = [](const char* Label, FVector& Color, UMaterial* Material, void (UMaterial::*SetColor)(FVector&)) {
			float ColorRGB[3] = { Color.X * 255.0f, Color.Y * 255.0f, Color.Z * 255.0f };
			bool ColorChanged = false;
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			float BoxWidth = 65.0f;

			ImGui::SetNextItemWidth(BoxWidth);
			ImVec2 PosR = ImGui::GetCursorScreenPos();
			std::string IDR = std::string("##") + Label + "R";
			ColorChanged |= ImGui::DragFloat(IDR.c_str(), &ColorRGB[0], 1.0f, 0.0f, 255.0f, "R: %.0f");
			ImVec2 SizeR = ImGui::GetItemRectSize();
			DrawList->AddLine(ImVec2(PosR.x + 5, PosR.y + 2), ImVec2(PosR.x + 5, PosR.y + SizeR.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
			ImGui::SameLine();

			ImGui::SetNextItemWidth(BoxWidth);
			ImVec2 PosG = ImGui::GetCursorScreenPos();
			std::string IDG = std::string("##") + Label + "G";
			ColorChanged |= ImGui::DragFloat(IDG.c_str(), &ColorRGB[1], 1.0f, 0.0f, 255.0f, "G: %.0f");
			ImVec2 SizeG = ImGui::GetItemRectSize();
			DrawList->AddLine(ImVec2(PosG.x + 5, PosG.y + 2), ImVec2(PosG.x + 5, PosG.y + SizeG.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
			ImGui::SameLine();

			ImGui::SetNextItemWidth(BoxWidth);
			ImVec2 PosB = ImGui::GetCursorScreenPos();
			std::string IDB = std::string("##") + Label + "B";
			ColorChanged |= ImGui::DragFloat(IDB.c_str(), &ColorRGB[2], 1.0f, 0.0f, 255.0f, "B: %.0f");
			ImVec2 SizeB = ImGui::GetItemRectSize();
			DrawList->AddLine(ImVec2(PosB.x + 5, PosB.y + 2), ImVec2(PosB.x + 5, PosB.y + SizeB.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
			ImGui::SameLine();

			float Color01[3] = { ColorRGB[0] / 255.0f, ColorRGB[1] / 255.0f, ColorRGB[2] / 255.0f };
			if (ImGui::ColorEdit3(Label, Color01, ImGuiColorEditFlags_NoInputs))
			{
				ColorRGB[0] = Color01[0] * 255.0f;
				ColorRGB[1] = Color01[1] * 255.0f;
				ColorRGB[2] = Color01[2] * 255.0f;
				ColorChanged = true;
			}

			if (ColorChanged)
			{
				Color.X = ColorRGB[0] / 255.0f;
				Color.Y = ColorRGB[1] / 255.0f;
				Color.Z = ColorRGB[2] / 255.0f;
				(Material->*SetColor)(Color);
			}
		};

		FVector Ambient = CurrentMaterial->GetAmbientColor();
		FVector Diffuse = CurrentMaterial->GetDiffuseColor();
		FVector Specular = CurrentMaterial->GetSpecularColor();

		RenderColorPicker("Ambient", Ambient, CurrentMaterial, &UMaterial::SetAmbientColor);
		RenderColorPicker("Diffuse", Diffuse, CurrentMaterial, &UMaterial::SetDiffuseColor);
		RenderColorPicker("Specular", Specular, CurrentMaterial, &UMaterial::SetSpecularColor);

		ImGui::PopID();
	}
}

void USkeletalMeshComponentWidget::RenderAvailableMaterials(int32 TargetSlotIndex)
{
	for (TObjectIterator<UMaterial> It; It; ++It)
	{
		UMaterial* Mat = *It;
		if (!Mat) continue;
		if (Mat->GetDiffuseTexture() == nullptr)
		{
			continue;
		}

		FString MatName = GetMaterialDisplayName(Mat);
		bool bIsSelected = (SkeletalMeshComponent->GetMaterial(TargetSlotIndex) == Mat);
		const float RowPreviewSize = 20.0f;
		UTexture* RowPreviewTexture = GetPreviewTextureForMaterial(Mat);
		ID3D11ShaderResourceView* RowShaderResourceView = nullptr;
		if (RowPreviewTexture != nullptr)
		{
			RowShaderResourceView = RowPreviewTexture->GetTextureSRV();
		}

		if (RowShaderResourceView != nullptr)
		{
			ImGui::Image((ImTextureID)RowShaderResourceView,
				ImVec2(RowPreviewSize, RowPreviewSize), ImVec2(0, 0), ImVec2(1, 1));
		}
		else
		{
			ImGui::Dummy(ImVec2(RowPreviewSize, RowPreviewSize));
		}
		ImGui::SameLine();

		if (ImGui::Selectable(MatName.c_str(), bIsSelected))
		{
			SkeletalMeshComponent->SetMaterial(TargetSlotIndex, Mat);
		}

		if (bIsSelected)
		{
			ImGui::SetItemDefaultFocus();
		}
	}
}

void USkeletalMeshComponentWidget::RenderOptions()
{
	bool IsNormalMapEnabledLocal = SkeletalMeshComponent->IsNormalMapEnabled();
	if (ImGui::Checkbox("Enable Normal Map", &IsNormalMapEnabledLocal))
	{
		if (IsNormalMapEnabledLocal)
		{
			SkeletalMeshComponent->EnableNormalMap();
		}
		else
		{
			SkeletalMeshComponent->DisableNormalMap();
		}
	}
}

FString USkeletalMeshComponentWidget::GetMaterialDisplayName(UMaterial* Material) const
{
	if (!Material)
	{
		return "None";
	}

	FString ObjectName = Material->GetName().ToString();
	if (!ObjectName.empty() && ObjectName.find("Object_") != 0)
	{
		return ObjectName;
	}

	UTexture* DiffuseTexture = Material->GetDiffuseTexture();
	if (DiffuseTexture)
	{
		FString TexturePath = DiffuseTexture->GetFilePath().ToString();
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

	return "Material_" + std::to_string(Material->GetUUID());
}
UTexture* USkeletalMeshComponentWidget::GetPreviewTextureForMaterial(UMaterial* Material) const
{
	if (Material == nullptr)
	{
		return nullptr;
	}

	UTexture* PreviewTexture = nullptr;

	PreviewTexture = Material->GetDiffuseTexture();
	if (PreviewTexture != nullptr) { return PreviewTexture; }

	PreviewTexture = Material->GetAmbientTexture();
	if (PreviewTexture != nullptr) { return PreviewTexture; }

	PreviewTexture = Material->GetSpecularTexture();
	if (PreviewTexture != nullptr) { return PreviewTexture; }

	PreviewTexture = Material->GetNormalTexture();
	if (PreviewTexture != nullptr) { return PreviewTexture; }

	PreviewTexture = Material->GetAlphaTexture();
	if (PreviewTexture != nullptr) { return PreviewTexture; }

	PreviewTexture = Material->GetBumpTexture();
	return PreviewTexture;
}
