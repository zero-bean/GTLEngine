#include "pch.h"
#include "ParticleEditorSections.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorState.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleSpawn.h"
#include "ParticleModuleColor.h"
#include "ParticleModuleLifetime.h"
#include "ParticleModuleLocation.h"
#include "ParticleModuleSize.h"
#include "ParticleModuleVelocity.h"
#include "ParticleModuleRotation.h"
#include "ParticleModuleTypeDataMesh.h"
#include "ResourceManager.h"
#include "Material.h"
#include <algorithm>
#include <cctype>
#include <string>
#include "ParticleModuleTypeDataBeam.h"

namespace
{
    ImVec4 AdjustChipColor(const ImVec4& BaseColor, float Delta)
    {
        return ImVec4(
            std::clamp(BaseColor.x + Delta, 0.0f, 1.0f),
            std::clamp(BaseColor.y + Delta, 0.0f, 1.0f),
            std::clamp(BaseColor.z + Delta, 0.0f, 1.0f),
            std::clamp(BaseColor.w + Delta * 0.5f, 0.2f, 1.0f));
    }

    bool DrawModuleChipButton(const char* Label, const ImVec4& BaseColor, bool bSelected, float Width = -1.0f)
    {
        const ImVec4 FillColor = bSelected ? AdjustChipColor(BaseColor, 0.20f) : BaseColor;
        ImGui::PushStyleColor(ImGuiCol_Button, FillColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, AdjustChipColor(FillColor, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, AdjustChipColor(FillColor, -0.08f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        const bool bPressed = ImGui::Button(Label, ImVec2(Width, 0.0f));
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        return bPressed;
    }

    void DrawLODModuleStack(ParticleEditorState* State, UParticleEmitter* Emitter, UParticleLODLevel* LODLevel, int32 EmitterIndex)
    {
        if (!LODLevel)
        {
            ImGui::TextDisabled("No data for this LOD.");
            return;
        }

        const ImVec4 TypeDataColor(1.0f, 0.3f, 0.3f, 0.9f);
        const ImVec4 RequiredColor(0.85f, 0.55f, 0.15f, 0.9f);
        const ImVec4 SpawnColor(0.20f, 0.45f, 0.95f, 0.9f);
        const ImVec4 ModuleColor(0.18f, 0.65f, 0.50f, 0.9f);

        bool bHasChip = false;
        const float chipWidth = ImGui::GetContentRegionAvail().x;
        bool bIsActiveEmitter = (State && State->SelectedEmitterIndex == EmitterIndex);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 6.0f));
        auto EmitChip = [&](const char* Label, const ImVec4& Color, EParticleDetailSelection SlotType, int32 ModuleIndex, int32 UniqueId)
        {
            if (bHasChip)
            {
                ImGui::Spacing();
            }
            ImGui::PushID(UniqueId);
            bool bSelected = false;
            if (bIsActiveEmitter && State && State->SelectedModuleSelection == SlotType)
            {
                if (SlotType == EParticleDetailSelection::Module)
                {
                    bSelected = (State->SelectedModuleIndex == ModuleIndex);
                }
                else
                {
                    bSelected = true;
                }
            }

            if (DrawModuleChipButton(Label, Color, bSelected, chipWidth) && State)
            {
                if (!bIsActiveEmitter)
                {
                    State->SelectedEmitterIndex = EmitterIndex;
                    bIsActiveEmitter = true;
                }
                State->SelectedModuleSelection = SlotType;
                State->SelectedModuleIndex = (SlotType == EParticleDetailSelection::Module) ? ModuleIndex : -1;
            }
            ImGui::PopID();
            bHasChip = true;
        };

        if (LODLevel->TypeDataModule)
        {
            EmitChip("TypeData", TypeDataColor, EParticleDetailSelection::MeshType, -1, 1002);
        }

        if (LODLevel->RequiredModule)
        {
            EmitChip("Required", RequiredColor, EParticleDetailSelection::Required, -1, 1000);
        }

        if (LODLevel->SpawnModule)
        {
            EmitChip("Spawn", SpawnColor, EParticleDetailSelection::Spawn, -1, 1001);
        }

        for (int32 moduleIndex = 0; moduleIndex < LODLevel->Modules.Num(); ++moduleIndex)
        {
            UParticleModule* Module = LODLevel->Modules[moduleIndex];
            if (!Module)
            {
                continue;
            }

            const char* ClassName = Module->GetClass()->Name;
            char ChipLabel[96];
            sprintf_s(ChipLabel, "%s", ClassName ? ClassName : "Module");
            EmitChip(ChipLabel, ModuleColor, EParticleDetailSelection::Module, moduleIndex, 2000 + moduleIndex);
        }

        if (!bHasChip)
        {
            ImGui::TextDisabled("No modules assigned.");
        }

        ImGui::PopStyleVar();
    }
}

static UParticleLODLevel* GetActiveLODLevel(ParticleEditorState* State, UParticleEmitter* Emitter)
{
    if (!State || !Emitter || Emitter->LODLevels.Num() == 0)
    {
        return nullptr;
    }

    const int32 LodIndex = FMath::Clamp(State->ActiveLODLevel, 0, Emitter->LODLevels.Num() - 1);
    if (LodIndex < 0 || LodIndex >= Emitter->LODLevels.Num())
    {
        return nullptr;
    }

    return Emitter->LODLevels[LodIndex];
}

static void SelectDefaultModule(ParticleEditorState* State, UParticleEmitter* Emitter)
{
    if (!State)
    {
        return;
    }

    UParticleLODLevel* LODLevel = GetActiveLODLevel(State, Emitter);
    if (LODLevel && LODLevel->RequiredModule)
    {
        State->SelectedModuleSelection = EParticleDetailSelection::Required;
        State->SelectedModuleIndex = -1;
        return;
    }

    if (LODLevel && LODLevel->SpawnModule)
    {
        State->SelectedModuleSelection = EParticleDetailSelection::Spawn;
        State->SelectedModuleIndex = -1;
        return;
    }

    if (LODLevel && LODLevel->Modules.Num() > 0)
    {
        State->SelectedModuleSelection = EParticleDetailSelection::Module;
        State->SelectedModuleIndex = 0;
        return;
    }

    State->SelectedModuleSelection = EParticleDetailSelection::None;
    State->SelectedModuleIndex = -1;
}

void FParticleEditorEmitterSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.50f, 0.35f, 0.8f));
    ImGui::Text("Emitters");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    static char EmitterFilter[64] = "";
    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##EmitterFilter", "Filter emitters or modules...", EmitterFilter, IM_ARRAYSIZE(EmitterFilter));
    ImGui::PopItemWidth();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    if (ActiveState && ActiveState->HasParticleSystem())
    {
        const float spacing = 8.0f;
        const float buttonWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5f;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));

        if (ImGui::Button("+ Add Emitter", ImVec2(buttonWidth, 0)))
        {
            ImGui::OpenPopup("AddEmitterPopup");
        }

        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopup("AddEmitterPopup"))
        {
            if (ImGui::MenuItem("Sprite Emitter"))
            {
                CreateNewEmitter(ActiveState, EEmitterType::Sprite);
            }
            else if (ImGui::MenuItem("Mesh Emitter"))
            {
                CreateNewEmitter(ActiveState, EEmitterType::Mesh);
            }
            else if (ImGui::MenuItem("Beam Emitter"))
            {
                CreateNewEmitter(ActiveState, EEmitterType::Beam);
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine(0.0f, spacing);

        const bool bHasSelection = (ActiveState->SelectedEmitterIndex >= 0 &&
            ActiveState->SelectedEmitterIndex < ActiveState->GetEmitterCount());

        if (!bHasSelection)
        {
            ImGui::BeginDisabled();
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));

        if (ImGui::Button("- Delete Emitter", ImVec2(buttonWidth, 0)))
        {
            DeleteSelectedEmitter(ActiveState);
        }

        ImGui::PopStyleColor(3);

        if (!bHasSelection)
        {
            ImGui::EndDisabled();
        }
    }
    else
    {
        ImGui::BeginDisabled();
        const float disabledWidth = (ImGui::GetContentRegionAvail().x - 8.0f) * 0.5f;
        ImGui::Button("+ Add Emitter", ImVec2(disabledWidth, 0));
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::Button("- Delete Emitter", ImVec2(disabledWidth, 0));
        ImGui::EndDisabled();
    }

    ImGui::Spacing();
    ImGui::Text("Emitter Stack");
    ImGui::Spacing();

    ImGui::BeginChild("EmitterList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        if (!ActiveState || !ActiveState->HasParticleSystem())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("No particle system loaded.");
            ImGui::TextWrapped("Use 'File > Open' or 'File > New' to get started.");
            ImGui::PopStyleColor();
        }
        else
        {
            const int32 EmitterCount = ActiveState->GetEmitterCount();

            if (EmitterCount == 0)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("No emitters in the system.");
                ImGui::TextWrapped("Use 'Emitter > Add Sprite Emitter' to add emitters.");
                ImGui::PopStyleColor();
            }
            else
            {
                std::string FilterToken;
                if (EmitterFilter[0] != '\0')
                {
                    FilterToken = EmitterFilter;
                    std::transform(FilterToken.begin(), FilterToken.end(), FilterToken.begin(),
                        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                }

                auto MatchesFilter = [&](const std::string& Text) -> bool
                {
                    if (FilterToken.empty())
                    {
                        return true;
                    }

                    std::string LowerText = Text;
                    std::transform(LowerText.begin(), LowerText.end(), LowerText.begin(),
                        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    return LowerText.find(FilterToken) != std::string::npos;
                };

                auto MatchesFilterLiteral = [&](const char* Text) -> bool
                {
                    return MatchesFilter(Text ? std::string(Text) : std::string());
                };

                auto DoesEmitterMatchFilter = [&](UParticleEmitter* Emitter, const std::string& LabelText) -> bool
                {
                    if (MatchesFilter(LabelText))
                    {
                        return true;
                    }

                    if (FilterToken.empty() || !Emitter)
                    {
                        return FilterToken.empty();
                    }

                    for (UParticleLODLevel* LODLevel : Emitter->LODLevels)
                    {
                        if (!LODLevel)
                        {
                            continue;
                        }

                        if (LODLevel->RequiredModule && MatchesFilterLiteral("Required"))
                        {
                            return true;
                        }

                        if (LODLevel->SpawnModule && MatchesFilterLiteral("Spawn"))
                        {
                            return true;
                        }

                        for (UParticleModule* Module : LODLevel->Modules)
                        {
                            if (!Module)
                            {
                                continue;
                            }

                            if (MatchesFilterLiteral(Module->GetClass()->Name))
                            {
                                return true;
                            }
                        }
                    }

                    return false;
                };

                TArray<int32> VisibleEmitters;
                for (int32 i = 0; i < EmitterCount; ++i)
                {
                    UParticleEmitter* Emitter = ActiveState->CurrentParticleSystem->Emitters[i];
                    if (!Emitter)
                    {
                        continue;
                    }

                    char BaseLabel[64];
                    sprintf_s(BaseLabel, "Emitter %d", i);
                    std::string LabelText = BaseLabel;

                    if (!DoesEmitterMatchFilter(Emitter, LabelText))
                    {
                        continue;
                    }

                    VisibleEmitters.Add(i);
                }

                if (VisibleEmitters.Num() == 0)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, FilterToken.empty() ? ImVec4(0.6f, 0.6f, 0.6f, 1.0f) : ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
                    ImGui::TextWrapped(FilterToken.empty() ?
                        "No emitters in the system. Use '+ Add Emitter' to create one." :
                        "No emitters match the current filter.");
                    ImGui::PopStyleColor();
                }
                else
                {
                    const float columnWidth = 220.0f;
                    for (int32 visibleIdx = 0; visibleIdx < VisibleEmitters.Num(); ++visibleIdx)
                    {
                        const int32 emitterIndex = VisibleEmitters[visibleIdx];
                        UParticleEmitter* Emitter = ActiveState->CurrentParticleSystem->Emitters[emitterIndex];
                        if (!Emitter)
                        {
                            continue;
                        }

                        if (visibleIdx > 0)
                        {
                            ImGui::SameLine(0.0f, 8.0f);
                        }

                        ImGui::PushID(emitterIndex);
                        ImGui::BeginChild(
                            "EmitterColumn",
                            ImVec2(columnWidth, 0),
                            ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);
                        {
                            const bool bSelected = (ActiveState->SelectedEmitterIndex == emitterIndex);
                            char HeaderLabel[64];
                            sprintf_s(HeaderLabel, "Emitter %d", emitterIndex);
                            if (ImGui::Selectable(HeaderLabel, bSelected, ImGuiSelectableFlags_AllowDoubleClick))
                            {
                                ActiveState->SelectedEmitterIndex = emitterIndex;
                                SelectDefaultModule(ActiveState, Emitter);
                            }
                            ImGui::Dummy(ImVec2(0.0f, 6.0f));

                            if (ImGui::BeginPopupContextItem("EmitterColumnContext"))
                            {
                                if (ImGui::MenuItem("Add Sprite Emitter"))
                                {
                                    CreateNewEmitter(ActiveState, EEmitterType::Sprite);
                                }
                                if (ImGui::MenuItem("Delete Emitter"))
                                {
                                    ActiveState->SelectedEmitterIndex = emitterIndex;
                                    DeleteSelectedEmitter(ActiveState);
                                }
                                ImGui::EndPopup();
                            }

                            UParticleLODLevel* LODLevel = GetActiveLODLevel(ActiveState, Emitter);
                            if (LODLevel)
                            {
                                ImGui::Dummy(ImVec2(0.0f, 4.0f));
                                DrawLODModuleStack(ActiveState, Emitter, LODLevel, emitterIndex);
                            }
                            else
                            {
                                ImGui::TextDisabled("No LOD data available.");
                            }
                        }
                        ImGui::EndChild();
                        ImGui::PopID();
                    }
                }
            }
        }
    }
    ImGui::EndChild();
}

void FParticleEditorEmitterSection::CreateNewEmitter(ParticleEditorState* State, EEmitterType EmitterType)
{
    if (!State || !State->CurrentParticleSystem)
        return;

    // 새 Emitter 생성
    UParticleEmitter* NewEmitter = NewObject<UParticleEmitter>();
    if (!NewEmitter)
        return;

    // LOD Level 생성
    UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
    if (!LODLevel)
    {
        DeleteObject(NewEmitter);
        return;
    }

    LODLevel->Level = 0;
    LODLevel->bEnabled = true;
    NewEmitter->LODLevels.Add(LODLevel);

    // Required Module 생성 (필수)
    UParticleModuleRequired* RequiredModule = NewObject<UParticleModuleRequired>();    

    RequiredModule->EmitterDuration = 2.0f;
    RequiredModule->EmitterLoops = 0;
    RequiredModule->bUseLocalSpace = false;

    UShader* DefaultShader = nullptr;
    if (EmitterType != EEmitterType::Beam)
    {
        // 기본 Material 생성 (텍스처 없는 흰색 파티클)
        DefaultShader = UResourceManager::GetInstance().Load<UShader>("Shaders/UI/Billboard.hlsl");        

        UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
        LocationModule->SpawnLocation.Operation = EDistributionMode::DOP_Constant;
        LocationModule->SpawnLocation.Constant = FVector(0.0f, 0.0f, 0.0f);

        UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
        VelocityModule->StartVelocity.Operation = EDistributionMode::DOP_Constant;
        VelocityModule->StartVelocity.Constant = FVector(0.0f, 0.0f, 10.0f);

        UParticleModuleRotation* RotationModule = NewObject<UParticleModuleRotation>();
        RotationModule->StartRotation.Operation = EDistributionMode::DOP_Constant;
        RotationModule->StartRotation.Constant = 0.0f;

        LODLevel->Modules.Add(LocationModule);
        LODLevel->Modules.Add(VelocityModule);
        LODLevel->Modules.Add(RotationModule);
        LODLevel->SpawnModules.Add(LocationModule);
        LODLevel->SpawnModules.Add(VelocityModule);
        LODLevel->SpawnModules.Add(RotationModule);
    }
    else if (EmitterType == EEmitterType::Beam)
    {
        // todo 빔 전용 모듈 생기면 추가
        DefaultShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Particles/TrailParticle.hlsl");
    }
    UMaterial* DefaultMaterial = NewObject<UMaterial>();
    if (DefaultShader)
    {
        DefaultMaterial->SetShader(DefaultShader);
    }
    RequiredModule->Material = DefaultMaterial;

    // Spawn Module 생성 (필수)
    UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
    SpawnModule->SpawnRate.Operation = EDistributionMode::DOP_Constant;
    SpawnModule->SpawnRate.Constant = 10.0f;

    // 기본 모듈들 생성
    UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
    ColorModule->StartColor.Operation = EDistributionMode::DOP_Constant;
    ColorModule->StartColor.Constant = FVector(1.0f, 1.0f, 1.0f);

    UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
    LifetimeModule->LifeTime.Operation = EDistributionMode::DOP_Constant;
    LifetimeModule->LifeTime.Constant = 2.0f;

    UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
    SizeModule->StartSize.Operation = EDistributionMode::DOP_Constant;
    SizeModule->StartSize.Constant = FVector(1.0f, 1.0f, 1.0f);

    // LOD Level에 모듈 할당
    LODLevel->RequiredModule = RequiredModule;
    LODLevel->SpawnModule = SpawnModule;
    LODLevel->TypeDataModule = nullptr; // Sprite는 TypeData 없음

    if (EmitterType == EEmitterType::Mesh)
    {
        UParticleModuleTypeDataMesh* MeshModule = NewObject<UParticleModuleTypeDataMesh>();
        MeshModule->StaticMesh = UResourceManager::GetInstance().Load<UStaticMesh>(GDataDir + "/Model/smokegrenade.obj");
        LODLevel->TypeDataModule = MeshModule;
    }
    else if (EmitterType == EEmitterType::Beam)
    {
        UParticleModuleTypeDataBeam* BeamModule = NewObject<UParticleModuleTypeDataBeam>();
        LODLevel->TypeDataModule = BeamModule;
    }

    LODLevel->Modules.Add(ColorModule);
    LODLevel->Modules.Add(LifetimeModule);
    LODLevel->Modules.Add(SizeModule);
    

    LODLevel->SpawnModules.Add(ColorModule);
    LODLevel->SpawnModules.Add(LifetimeModule);
    LODLevel->SpawnModules.Add(SizeModule);

    // Emitter 정보 캐싱
    NewEmitter->CacheEmitterModuleInfo();

    // 파티클 시스템에 추가
    State->CurrentParticleSystem->Emitters.Add(NewEmitter);

    // 프리뷰 파티클 시스템 업데이트 (EmitterInstance 재생성)
    State->UpdatePreviewParticleSystem();

    // 새로 생성된 Emitter 선택
    State->SelectedEmitterIndex = State->CurrentParticleSystem->Emitters.Num() - 1;
    SelectDefaultModule(State, NewEmitter);

    UE_LOG("New emitter created. Total emitters: %d", State->CurrentParticleSystem->Emitters.Num());
}

void FParticleEditorEmitterSection::DeleteSelectedEmitter(ParticleEditorState* State)
{
    if (!State || !State->CurrentParticleSystem)
        return;

    if (State->SelectedEmitterIndex < 0 || State->SelectedEmitterIndex >= State->CurrentParticleSystem->Emitters.Num())
        return;

    // Emitter 삭제
    UParticleEmitter* EmitterToDelete = State->CurrentParticleSystem->Emitters[State->SelectedEmitterIndex];
    if (EmitterToDelete)
    {
        DeleteObject(EmitterToDelete);
    }

    State->CurrentParticleSystem->Emitters.RemoveAt(State->SelectedEmitterIndex);

    // 프리뷰 파티클 시스템 업데이트 (EmitterInstance 재생성)
    State->UpdatePreviewParticleSystem();

    // 선택 인덱스 조정
    if (State->CurrentParticleSystem->Emitters.Num() > 0)
    {
        State->SelectedEmitterIndex = FMath::Min(State->SelectedEmitterIndex, State->CurrentParticleSystem->Emitters.Num() - 1);
        UParticleEmitter* NewSelection = State->CurrentParticleSystem->Emitters[State->SelectedEmitterIndex];
        SelectDefaultModule(State, NewSelection);
    }
    else
    {
        State->SelectedEmitterIndex = -1;
        State->SelectedModuleSelection = EParticleDetailSelection::None;
        State->SelectedModuleIndex = -1;
    }

    UE_LOG("Emitter deleted. Remaining emitters: %d", State->CurrentParticleSystem->Emitters.Num());
}
