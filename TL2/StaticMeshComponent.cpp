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

void FImGuiDisplayCache::BuildCache()
{
    if (bResourceCheckDone && !bCacheBuilt)
    {
        bCacheBuilt = true;
        UE_LOG("Building ImGui display cache from ResourceManager lists...");

        auto& ResourceManager = UResourceManager::GetInstance();

        // --- 1. Static Mesh ---
        const auto& MeshPaths = ResourceManager.GetFilteredObjPaths();
        MeshDisplayNames.reserve(MeshPaths.size());
        MeshItems.reserve(MeshPaths.size());
        for (const FString& p : MeshPaths)
        {
            MeshDisplayNames.push_back(GetBaseNameNoExt(p));
            MeshItems.push_back(MeshDisplayNames.back().c_str());
        }

        // --- 2. Material ---
        const auto& MatPaths = ResourceManager.GetFilteredMaterialPaths();
        MaterialItems.reserve(MatPaths.size());
        for (const FString& p : MatPaths)
        {
            MaterialItems.push_back(p.c_str());
        }

        // --- 3. Normal Map ---
        const auto& NormalPaths = ResourceManager.GetFilteredNormalMapPaths();
        NormalDisplayNames.reserve(NormalPaths.size());
        for (const FString& path : NormalPaths)
        {
            NormalDisplayNames.push_back(std::filesystem::path(path).stem().string());
        }
    }
    else if (!bResourceCheckDone)
    {
        auto& RM = UResourceManager::GetInstance();
        if (!RM.GetFilteredObjPaths().empty() && 
            !RM.GetFilteredMaterialPaths().empty() && 
            !RM.GetFilteredNormalMapPaths().empty())
        {
            bResourceCheckDone = true;
        }
    }
}

UStaticMeshComponent::UStaticMeshComponent()
{
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

        if (UShader* shader = GetMaterial()->GetShader())
        {
            // 각 머티리얼 슬롯마다 다른 노멀맵 설정이 있을 수 있으므로
            // DrawIndexedPrimitiveComponent 내부에서 처리하거나
            // 여기서 기본값 설정

            // 첫 번째 슬롯의 노멀맵 상태를 기준으로 설정
            if (!MaterailSlots.empty())
            {
                bool hasNormalMap = false;

                if (MaterailSlots[0].bOverrideNormalTexture)
                {
                    // 오버라이드가 있으면, 오버라이드 텍스처 존재 여부 확인
                    hasNormalMap = (MaterailSlots[0].NormalTextureOverride != nullptr);
                }
                else
                {
                    // 오버라이드가 없으면, 원본 머티리얼의 노멀맵 확인
                    if (UMaterial* Material = UResourceManager::GetInstance().Get<UMaterial>(MaterailSlots[0].MaterialName))
                    {
                        hasNormalMap = (Material->GetNormalTexture() != nullptr);
                    }
                }

                shader->SetActiveNormalMode(hasNormalMap ? ENormalMapMode::HasNormalMap : ENormalMapMode::NoNormalMap);
            }
            else
            {
                shader->SetActiveNormalMode(ENormalMapMode::NoNormalMap);
            }
        }
        
        Renderer->PrepareShader(GetMaterial()->GetShader());
        Renderer->DrawIndexedPrimitiveComponent(GetStaticMesh(), D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, MaterailSlots);
    }
}

void UStaticMeshComponent::SetStaticMesh(const FString& PathFileName)
{
	StaticMesh = FObjManager::LoadObjStaticMesh(PathFileName);
    
    const TArray<FGroupInfo>& GroupInfos = StaticMesh->GetMeshGroupInfo();

    // 기존 오버라이드 정보를 유지하기 위해 임시 백업합니다.
    TArray<FMaterialSlot> OldSlots = std::move(MaterailSlots);

    // 슬롯 배열 크기 재설정합니다.
    MaterailSlots.clear();
    MaterailSlots.resize(GroupInfos.size());

    if (MaterailSlots.size() < GroupInfos.size())
    {
        MaterailSlots.resize(GroupInfos.size());
    }

    // MaterailSlots.size()가 GroupInfos.size() 보다 클 수 있기 때문에, GroupInfos.size()로 설정
    for (int i = 0; i < GroupInfos.size(); ++i)
    {
        // 이전 슬롯 정보가 있고, 유저가 변경한 적이 있다면 그대로 복원합니다.
        if (i < OldSlots.size() && OldSlots[i].bChangedByUser)
        {
            MaterailSlots[i] = OldSlots[i];
        }
        // 아니라면, 새 메시의 기본 머티리얼로 설정합니다.
        else 
        {
            MaterailSlots[i].MaterialName = GroupInfos[i].InitialMaterialName;
            MaterailSlots[i].bChangedByUser = false;
            MaterailSlots[i].bOverrideNormalTexture = false;
            MaterailSlots[i].NormalTextureOverride = nullptr; 
        }
    }
}

void UStaticMeshComponent::SetNormalTextureOverride(int32 SlotIndex, UTexture* InTexture)
{
    if (SlotIndex >= 0 && SlotIndex < MaterailSlots.size())
    {
        MaterailSlots[SlotIndex].NormalTextureOverride = InTexture;
        MaterailSlots[SlotIndex].bOverrideNormalTexture = true;
        MaterailSlots[SlotIndex].bChangedByUser = true;
    }
}

void UStaticMeshComponent::ClearNormalTextureOverride(int32 SlotIndex)
{
    if (SlotIndex >= 0 && SlotIndex < MaterailSlots.size())
    {
        MaterailSlots[SlotIndex].NormalTextureOverride = nullptr;
        MaterailSlots[SlotIndex].bOverrideNormalTexture = false;
        // bChangedByUser는 되돌리지 않습니다.
        // 원본 머티리얼은 여전히 유저가 설정한 것일 수 있습니다.
    }
}

UTexture* UStaticMeshComponent::GetNormalTextureOverride(int32 SlotIndex) const
{
    if (SlotIndex >= 0 && SlotIndex < MaterailSlots.size())
    {
        // 오버라이드가 활성화된 경우에만 텍스처 반환
        if (MaterailSlots[SlotIndex].bOverrideNormalTexture)
        {
            return MaterailSlots[SlotIndex].NormalTextureOverride;
        }
    }

    return nullptr; // 오버라이드가 활성화되지 않았으면 nullptr 반환
}

bool UStaticMeshComponent::HasNormalTextureOverride(int32 SlotIndex) const
{
    if (SlotIndex >= 0 && SlotIndex < MaterailSlots.size())
    {
        return MaterailSlots[SlotIndex].bOverrideNormalTexture;
    }

    return false;
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

        // 씬 파일에 저장된 슬롯 수와 현재 메시의 슬롯 수 중 작은 값을 기준으로 복원 (안전성)
        size_t SlotCountToLoad = std::min((size_t)MaterailSlots.size(), InOut.Materials.size());

        for (size_t i = 0; i < SlotCountToLoad; ++i)
        {
            // 1. 원본 머티리얼 설정
            MaterailSlots[i].MaterialName = InOut.Materials[i];
            MaterailSlots[i].bChangedByUser = true; // 씬에서 로드했으므로 유저가 변경한 것으로 간주

            // 2. 오버라이드 정보 로드 
            if (i < InOut.MaterialNormalMapOverrides.size())
            {
                const FString& OverridePath = InOut.MaterialNormalMapOverrides[i];

                if (OverridePath.empty())
                {
                    // 오버라이드 없음
                    MaterailSlots[i].bOverrideNormalTexture = false;
                    MaterailSlots[i].NormalTextureOverride = nullptr;
                }
                else if (OverridePath == "NONE") 
                {
                    // '없음'으로 오버라이드
                    MaterailSlots[i].bOverrideNormalTexture = true;
                    MaterailSlots[i].NormalTextureOverride = nullptr;
                }
                else 
                {
                    // 텍스처 경로로 오버라이드
                    MaterailSlots[i].bOverrideNormalTexture = true;
                    MaterailSlots[i].NormalTextureOverride = UResourceManager::GetInstance().Load<UTexture>(OverridePath);
                }
            }
            else
            {
                // 레거시 씬 파일 (오버라이드 정보가 없음)
                MaterailSlots[i].bOverrideNormalTexture = false;
                MaterailSlots[i].NormalTextureOverride = nullptr;
            }
        }
    }
    else // (bIsLoading == false)
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

        InOut.Materials.clear();
        InOut.MaterialNormalMapOverrides.clear();

        for (const FMaterialSlot& Slot : MaterailSlots)
        {
            // 1. 원본 머티리얼 저장
            InOut.Materials.push_back(Slot.MaterialName);

            // 2. 오버라이드 정보 저장
            if (!Slot.bOverrideNormalTexture)
            {
                // 오버라이드 없음 -> 빈 문자열
                InOut.MaterialNormalMapOverrides.push_back("");
            }
            else
            {
                if (Slot.NormalTextureOverride == nullptr)
                {
                    // '없음'으로 오버라이드 -> "NONE"
                    InOut.MaterialNormalMapOverrides.push_back("NONE");
                }
                else
                {
                    // 텍스처 경로로 오버라이드
                    InOut.MaterialNormalMapOverrides.push_back(Slot.NormalTextureOverride->GetFilePath());
                }
            }
        }
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
		return;
	}

	GetDisplayCache().BuildCache();

	RenderStaticMeshSection();
	RenderMaterialSection();
}

FImGuiDisplayCache& UStaticMeshComponent::GetDisplayCache()
{
	static FImGuiDisplayCache cache;
	return cache;
}

void UStaticMeshComponent::RenderStaticMeshSection()
{
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

	auto& Cache = GetDisplayCache();
	if (!Cache.bCacheBuilt)
	{
		ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "Resources loading...");
	}
	else if (Cache.MeshItems.empty())
	{
		ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), "No .obj resources found.");
	}
	else
	{
		static int SelectedMeshIdx = -1;
		if (SelectedMeshIdx == -1)
		{
			for (int i = 0; i < static_cast<int>(Cache.MeshDisplayNames.size()); ++i)
			{
				if (Cache.MeshDisplayNames[i] == "Cube")
				{
					SelectedMeshIdx = i;
					break;
				}
			}
		}

		ImGui::SetNextItemWidth(240);
		ImGui::Combo("StaticMesh", &SelectedMeshIdx, Cache.MeshItems.data(), static_cast<int>(Cache.MeshItems.size()));
		ImGui::SameLine();
		if (ImGui::Button("Apply Mesh"))
		{
			auto& RM = UResourceManager::GetInstance();
			if (SelectedMeshIdx >= 0 && SelectedMeshIdx < static_cast<int>(RM.GetFilteredObjPaths().size()))
			{
				const FString& NewPath = RM.GetFilteredObjPaths()[SelectedMeshIdx];
				SetStaticMesh(NewPath);
				UE_LOG("Applied StaticMesh: %s", NewPath.c_str());
			}
		}
	}
}

void UStaticMeshComponent::RenderMaterialSection()
{
    UStaticMesh* CurMesh = GetStaticMesh();
    if (CurMesh)
    {
        auto& RM = UResourceManager::GetInstance();
        auto& Cache = GetDisplayCache();
        const TArray<FString>& FilteredMaterialPaths = RM.GetFilteredMaterialPaths();
        const TArray<const char*>& MaterialPathsCharP = Cache.MaterialItems;

        const uint64 MeshGroupCount = CurMesh->GetMeshGroupCount();
        if (0 < MeshGroupCount)
        {
            ImGui::Separator();
        }

        static TArray<int32> SelectedMaterialIdxAt;
        if (SelectedMaterialIdxAt.size() < MeshGroupCount)
        {
            SelectedMaterialIdxAt.resize(MeshGroupCount);
        }

        const TArray<FMaterialSlot>& MaterialSlots = GetMaterailSlots();

        // ✅ Material 이름으로 먼저 찾고, 없으면 경로로 찾기
        for (uint64 MaterialSlotIndex = 0; MaterialSlotIndex < MeshGroupCount; ++MaterialSlotIndex)
        {
            // Material 이름으로 찾기 시도
            UMaterial* Material = RM.Get<UMaterial>(MaterialSlots[MaterialSlotIndex].MaterialName);

            // 못 찾았으면 FilteredMaterialPaths에서 매칭되는 경로 찾기
            if (!Material)
            {
                for (uint32 MaterialIndex = 0; MaterialIndex < FilteredMaterialPaths.size(); ++MaterialIndex)
                {
                    UMaterial* PathMaterial = RM.Get<UMaterial>(FilteredMaterialPaths[MaterialIndex]);
                    if (PathMaterial)
                    {
                        // 이 Material의 이름이 슬롯의 MaterialName과 일치하는지 확인
                        // (MaterialInfo에서 MaterialName 비교)
                        SelectedMaterialIdxAt[MaterialSlotIndex] = MaterialIndex;
                        break;
                    }
                }
            }
            else
            {
                // Material을 찾았으면 해당하는 경로 인덱스 찾기
                for (uint32 MaterialIndex = 0; MaterialIndex < FilteredMaterialPaths.size(); ++MaterialIndex)
                {
                    if (MaterialSlots[MaterialSlotIndex].MaterialName == FilteredMaterialPaths[MaterialIndex])
                    {
                        SelectedMaterialIdxAt[MaterialSlotIndex] = MaterialIndex;
                        break;
                    }
                }
            }
        }

        for (uint64 MaterialSlotIndex = 0; MaterialSlotIndex < MeshGroupCount; ++MaterialSlotIndex)
        {
            ImGui::PushID(static_cast<int>(MaterialSlotIndex));
            if (ImGui::Combo("Material", &SelectedMaterialIdxAt[MaterialSlotIndex], MaterialPathsCharP.data(), static_cast<int>(MaterialPathsCharP.size())))
            {
                SetMaterialByUser(static_cast<uint32>(MaterialSlotIndex), FilteredMaterialPaths[SelectedMaterialIdxAt[MaterialSlotIndex]]);
            }

            RenderNormalMapSection(MaterialSlotIndex);

            ImGui::PopID();
        }
    }
}

void UStaticMeshComponent::RenderNormalMapSection(uint64 MaterialSlotIndex)
{
	auto& RM = UResourceManager::GetInstance();
	auto& Cache = GetDisplayCache();
	const TArray<FMaterialSlot>& MaterialSlots = GetMaterailSlots();

    UMaterial* Material = RM.Get<UMaterial>(MaterialSlots[MaterialSlotIndex].MaterialName);

	if (Material)
	{
		const TArray<FString>& NormalMapTexturePaths = RM.GetFilteredNormalMapPaths();
		const TArray<FString>& DisplayNames = Cache.NormalDisplayNames;

		bool bIsOverridden = this->HasNormalTextureOverride(MaterialSlotIndex);
		UTexture* CurrentNormalTexture = nullptr;

		if (bIsOverridden)
		{
			CurrentNormalTexture = this->GetNormalTextureOverride(MaterialSlotIndex);
		}
		else
		{
			CurrentNormalTexture = Material->GetNormalTexture();
		}

		const char* CurrentTextureNameOnly = "None";
		if (CurrentNormalTexture)
		{
			FString CurrentPath = CurrentNormalTexture->GetFilePath();
			for (size_t i = 0; i < NormalMapTexturePaths.size(); ++i)
			{
                if (NormalMapTexturePaths[i] == CurrentPath)
                {
                    if (i < DisplayNames.size())
                    {
                        CurrentTextureNameOnly = DisplayNames[i].c_str();
                    }
                    break;
                }
            }
		}

		FString DisplayLabel;
		if (bIsOverridden)
		{
			if (CurrentNormalTexture == nullptr)
				DisplayLabel = "None";
			else
				DisplayLabel = FString(CurrentTextureNameOnly);
		}
		else
		{
			if (CurrentNormalTexture == nullptr)
				DisplayLabel = "Default";
			else
				DisplayLabel = FString("Default (") + CurrentTextureNameOnly + ")";
		}

		if (ImGui::BeginCombo("Normal", DisplayLabel.c_str()))
		{
			ImGui::Separator();

			for (size_t i = 0; i < NormalMapTexturePaths.size(); ++i)
			{
				bool is_selected = (bIsOverridden && CurrentNormalTexture && CurrentNormalTexture->GetFilePath() == NormalMapTexturePaths[i]);

				if (ImGui::Selectable(DisplayNames[i].c_str(), is_selected))
				{
					UTexture* SelectedTexture = RM.Load<UTexture>(NormalMapTexturePaths[i]);
					this->SetNormalTextureOverride(MaterialSlotIndex, SelectedTexture);
				}

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					UTexture* HoveredTexture = RM.Load<UTexture>(NormalMapTexturePaths[i]);
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

		if (CurrentNormalTexture && CurrentNormalTexture->GetShaderResourceView())
		{
			ImGui::Image(CurrentNormalTexture->GetShaderResourceView(), ImVec2(128, 128));
		}
	}
}
