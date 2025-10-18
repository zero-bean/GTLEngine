#include "pch.h"
#include "StaticMeshComponent.h"
#include "StaticMesh.h"
#include "Shader.h"
#include "Texture.h"
#include "ResourceManager.h"
#include "ObjManager.h"
#include"CameraActor.h"
#include "SceneLoader.h"
#include "ImGui/imgui.h"
#include <filesystem>

// Helper function to extract base filename without extension
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

// Helper function to get normal map files from Data/textures/NormalMap folder
static TArray<FString> GetNormalMapFiles()
{
    TArray<FString> normalMapFiles;
    try
    {
        std::filesystem::path normalMapPath = "Data/textures/NormalMap";
        if (std::filesystem::exists(normalMapPath) && std::filesystem::is_directory(normalMapPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(normalMapPath))
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
        UE_LOG("Failed to scan normal map directory: %s", e.what());
    }
    return normalMapFiles;
}

UStaticMeshComponent::UStaticMeshComponent()
{
    //SetMaterial("StaticMeshShader.hlsl");
    SetMaterial("UberLit.hlsl");
}

UStaticMeshComponent::~UStaticMeshComponent()
{

}

void UStaticMeshComponent::Render(URenderer* Renderer, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, const EEngineShowFlags ShowFlags)
{
    if (HasShowFlag(ShowFlags, EEngineShowFlags::SF_StaticMeshes) == false)
    {
        return;
    }
    if (StaticMesh)
    {
        if (Cast<AGizmoActor>(this->GetOwner()))
        {
            Renderer->OMSetDepthStencilState(EComparisonFunc::Always);
        }
        else
        {
            Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqual);
        }

        Renderer->RSSetNoCullState();

        // Normal transformation을 위한 inverse transpose matrix 계산
        FMatrix WorldMatrix = GetWorldMatrix();
        FMatrix NormalMatrix = WorldMatrix.Inverse().Transpose();

        ModelBufferType ModelBuffer;
        ModelBuffer.Model = WorldMatrix;
        ModelBuffer.UUID = this->InternalIndex;
        ModelBuffer.NormalMatrix = NormalMatrix;

        Renderer->UpdateSetCBuffer(ModelBuffer);
        
        Renderer->PrepareShader(GetMaterial()->GetShader());
        Renderer->DrawIndexedPrimitiveComponent(GetStaticMesh(), D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, MaterailSlots);
    }
}

void UStaticMeshComponent::SetStaticMesh(const FString& PathFileName)
{
	StaticMesh = FObjManager::LoadObjStaticMesh(PathFileName);
    
    const TArray<FGroupInfo>& GroupInfos = StaticMesh->GetMeshGroupInfo();
    if (MaterailSlots.size() < GroupInfos.size())
    {
        MaterailSlots.resize(GroupInfos.size());
    }

    // MaterailSlots.size()가 GroupInfos.size() 보다 클 수 있기 때문에, GroupInfos.size()로 설정
    for (int i = 0; i < GroupInfos.size(); ++i) 
    {
        if (MaterailSlots[i].bChangedByUser == false)
        {
            MaterailSlots[i].MaterialName = GroupInfos[i].InitialMaterialName;
        }
    }
}

void UStaticMeshComponent::Serialize(bool bIsLoading, FPrimitiveData& InOut)
{
    // 0) 트랜스폼 직렬화/역직렬화는 상위(UPrimitiveComponent)에서 처리
    UPrimitiveComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        // 1) 신규 포맷: ObjStaticMeshAsset가 있으면 우선 사용
        if (!InOut.ObjStaticMeshAsset.empty())
        {
            SetStaticMesh(InOut.ObjStaticMeshAsset);
            return;
        }

        // 2) 레거시 호환: Type을 "Data/<Type>.obj"로 매핑
        if (!InOut.Type.empty())
        {
            const FString LegacyPath = "Data/" + InOut.Type + ".obj";
            SetStaticMesh(LegacyPath);
        }
    }
    else
    {
        // 저장 시: 현재 StaticMesh가 있다면 실제 에셋 경로를 기록
        if (UStaticMesh* Mesh = GetStaticMesh())
        {
            InOut.ObjStaticMeshAsset = Mesh->GetAssetPathFileName();
        }
        else
        {
            InOut.ObjStaticMeshAsset.clear();
        }
        // Type은 상위(월드/액터) 정책에 따라 별도 기록 (예: "StaticMeshComp")
    }
}

void UStaticMeshComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // 0) 트랜스폼 직렬화/역직렬화는 상위(UPrimitiveComponent)에서 처리
    UPrimitiveComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        // StaticMesh 로드
        if (!InOut.StaticMesh.empty())
        {
            SetStaticMesh(InOut.StaticMesh);
        }
        // TODO: Materials 로드
    }
    else
    {
        // StaticMesh 저장
        if (UStaticMesh* Mesh = GetStaticMesh())
        {
            InOut.StaticMesh = Mesh->GetAssetPathFileName();
        }
        else
        {
            InOut.StaticMesh.clear();
        }
        // TODO: Materials 저장
    }
}

void UStaticMeshComponent::SetMaterialByUser(const uint32 InMaterialSlotIndex, const FString& InMaterialName)
{
    assert((0 <= InMaterialSlotIndex && InMaterialSlotIndex < MaterailSlots.size()) && "out of range InMaterialSlotIndex");

    if (0 <= InMaterialSlotIndex && InMaterialSlotIndex < MaterailSlots.size())
    {
        MaterailSlots[InMaterialSlotIndex].MaterialName = InMaterialName;
        MaterailSlots[InMaterialSlotIndex].bChangedByUser = true;
    }
    else
    {
        UE_LOG("out of range InMaterialSlotIndex: %d", InMaterialSlotIndex);
    }

    assert(MaterailSlots[InMaterialSlotIndex].bChangedByUser == true);
}
const FAABB UStaticMeshComponent::GetWorldAABB() const
{
    if (StaticMesh == nullptr)
    {
        return FAABB();
    }
    
    //LocalAABB의 8점에 WorldMatrix를 곱해 AABB제작
    const FMatrix& WorldMatrix = GetWorldMatrix();
    const FAABB& LocalAABB = StaticMesh->GetAABB();
    FVector WorldVertex[8];
    for (int i = 0; i < 8; i++)
    {
        WorldVertex[i] = (FVector4(LocalAABB.GetVertex(i), 1) * WorldMatrix).GetVt3();
    }

    return FAABB(WorldVertex, 8);    
}
const FAABB& UStaticMeshComponent::GetLocalAABB() const
{
    if (StaticMesh == nullptr)
    {
        return FAABB();
    }
    return StaticMesh->GetAABB();
}
UObject* UStaticMeshComponent::Duplicate()
{
    UStaticMeshComponent* DuplicatedComponent = Cast<UStaticMeshComponent>(NewObject(GetClass()));

    // 공통 속성 복사 (Transform, AttachChildren) - 부모 헬퍼 사용
    CopyCommonProperties(DuplicatedComponent);

    // StaticMeshComponent 전용 속성 복사
    DuplicatedComponent->Material = this->Material;
    DuplicatedComponent->StaticMesh = this->StaticMesh;
    DuplicatedComponent->MaterailSlots = this->MaterailSlots;

    DuplicatedComponent->DuplicateSubObjects();
    return DuplicatedComponent;
}

void UStaticMeshComponent::DuplicateSubObjects()
{
    // 부모의 깊은 복사 수행 (AttachChildren 재귀 복제)
    Super_t::DuplicateSubObjects();
}

void UStaticMeshComponent::RenderDetails()
{
	ImGui::Text("Static Mesh Override");
	if (!this)
	{
		ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), "StaticMeshComponent not found.");
	}
	else
	{
		// 현재 메시 경로 표시
		FString CurrentPath;
		UStaticMesh* CurMesh = GetStaticMesh();
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
		}
		else
		{
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
					SetStaticMesh(NewPath);

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

		// Material 설정

		const TArray<FString> MaterialNames = UResourceManager::GetInstance().GetAllFilePaths<UMaterial>();
		// ImGui 콤보 아이템 배열
		TArray<const char*> MaterialNamesCharP;
		MaterialNamesCharP.reserve(MaterialNames.size());
		for (const FString& n : MaterialNames)
			MaterialNamesCharP.push_back(n.c_str());

		if (CurMesh)
		{
			const uint64 MeshGroupCount = CurMesh->GetMeshGroupCount();

			if (0 < MeshGroupCount)
			{
				ImGui::Separator();
			}

			static TArray<int32> SelectedMaterialIdxAt; // i번 째 Material Slot이 가지고 있는 MaterialName이 MaterialNames의 몇번쩨 값인지.
			if (SelectedMaterialIdxAt.size() < MeshGroupCount)
			{
				SelectedMaterialIdxAt.resize(MeshGroupCount);
			}

			// 현재 SMC의 MaterialSlots 정보를 UI에 반영
			const TArray<FMaterialSlot>& MaterialSlots = GetMaterailSlots();
			for (uint64 MaterialSlotIndex = 0; MaterialSlotIndex < MeshGroupCount; ++MaterialSlotIndex)
			{
				for (uint32 MaterialIndex = 0; MaterialIndex < MaterialNames.size(); ++MaterialIndex)
				{
					if (MaterialSlots[MaterialSlotIndex].MaterialName == MaterialNames[MaterialIndex])
					{
						SelectedMaterialIdxAt[MaterialSlotIndex] = MaterialIndex;
					}
				}
			}

			// Material 선택
			for (uint64 MaterialSlotIndex = 0; MaterialSlotIndex < MeshGroupCount; ++MaterialSlotIndex)
			{
				ImGui::PushID(static_cast<int>(MaterialSlotIndex));
				if (ImGui::Combo("Material", &SelectedMaterialIdxAt[MaterialSlotIndex], MaterialNamesCharP.data(), static_cast<int>(MaterialNamesCharP.size())))
				{
					SetMaterialByUser(static_cast<uint32>(MaterialSlotIndex), MaterialNames[SelectedMaterialIdxAt[MaterialSlotIndex]]);
				}

				// Normal Map 설정
				UMaterial* Material = UResourceManager::GetInstance().Load<UMaterial>(MaterialSlots[MaterialSlotIndex].MaterialName);
				if (Material)
				{
					static TArray<FString> NormalMapTexturePaths = GetNormalMapFiles();

					// 표시용 이름 배열 생성 (경로, 확장자 제거)
					TArray<FString> DisplayNames;
					DisplayNames.reserve(NormalMapTexturePaths.size());
					for (const FString& path : NormalMapTexturePaths)
					{
						DisplayNames.push_back(std::filesystem::path(path).stem().string());
					}

					// 현재 선택된 텍스처 찾기
					UTexture* CurrentNormalTexture = Material->GetNormalTexture();
					const char* CurrentTextureDisplayName = "None";
					if (CurrentNormalTexture)
					{
						FString CurrentPath = CurrentNormalTexture->GetFilePath();
						for (size_t i = 0; i < NormalMapTexturePaths.size(); ++i)
						{
							if (NormalMapTexturePaths[i] == CurrentPath)
							{
								CurrentTextureDisplayName = DisplayNames[i].c_str();
								break;
							}
						}
					}

					// 콤보박스 시작
					if (ImGui::BeginCombo("Normal Map", CurrentTextureDisplayName))
					{
						// "None" 옵션
						bool is_none_selected = (CurrentNormalTexture == nullptr);
						if (ImGui::Selectable("None", is_none_selected))
						{
							Material->SetNormalTexture(nullptr);
						}
						if (is_none_selected)
							ImGui::SetItemDefaultFocus();

						// 나머지 텍스처 옵션들
						for (size_t i = 0; i < NormalMapTexturePaths.size(); ++i)
						{
							bool is_selected = (CurrentNormalTexture && CurrentNormalTexture->GetFilePath() == NormalMapTexturePaths[i]);
							if (ImGui::Selectable(DisplayNames[i].c_str(), is_selected))
							{
								UTexture* SelectedTexture = UResourceManager::GetInstance().Load<UTexture>(NormalMapTexturePaths[i]);
								Material->SetNormalTexture(SelectedTexture);
							}

							// 콤보박스 항목 호버 시 미리보기
							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								UTexture* HoveredTexture = UResourceManager::GetInstance().Load<UTexture>(NormalMapTexturePaths[i]);
								if (HoveredTexture && HoveredTexture->GetShaderResourceView())
								{
									ImGui::Image(HoveredTexture->GetShaderResourceView(), ImVec2(128, 128));
								}
								ImGui::TextUnformatted(DisplayNames[i].c_str());
								ImGui::EndTooltip();
							}

							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					// 현재 선택된 텍스처 미리보기
					if (CurrentNormalTexture && CurrentNormalTexture->GetShaderResourceView())
					{
						ImGui::Image(CurrentNormalTexture->GetShaderResourceView(), ImVec2(128, 128));
					}
				}

				ImGui::PopID();
			}
		}
	}
}
