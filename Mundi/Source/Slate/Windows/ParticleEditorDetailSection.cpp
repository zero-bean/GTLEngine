#include "pch.h"
#include "ParticleEditorSections.h"
#include "ParticleEditorState.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleSpawn.h"
#include "ParticleModuleColor.h"
#include "ParticleModuleSize.h"
#include "ParticleModuleLifetime.h"
#include "ParticleModuleLocation.h"
#include "ParticleModuleVelocity.h"
#include "ParticleModuleRotation.h"
#include "ParticleSystemComponent.h"
#include "ParticleModuleTypeDataMesh.h"
#include "ResourceManager.h"
#include "Material.h"
#include "Texture.h"
#include "Shader.h"
#include "PathUtils.h"
#include "PlatformProcess.h"
#include "Widgets/PropertyRenderer.h"
#include <cstring>
#include <filesystem>

namespace
{
    constexpr ImGuiTableFlags PropertyTableFlags =
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV;

    FString MakeDataRelativePath(const std::filesystem::path& AbsolutePath)
    {
        try
        {
            const fs::path CanonicalPath = fs::weakly_canonical(AbsolutePath);
            const fs::path DataRoot = fs::weakly_canonical(fs::absolute(fs::path(UTF8ToWide(GDataDir))));

            std::error_code ec;
            fs::path RelativePath = fs::relative(CanonicalPath, DataRoot, ec);
            if (!ec && !RelativePath.empty())
            {
                return NormalizePath(GDataDir + "/" + WideToUTF8(RelativePath.generic_wstring()));
            }

            return NormalizePath(WideToUTF8(CanonicalPath.generic_wstring()));
        }
        catch (const std::exception&)
        {
            return NormalizePath(WideToUTF8(AbsolutePath.generic_wstring()));
        }
    }

    template <typename ValueCallable>
    void DrawPropertyRow(const char* Label, ValueCallable&& ValueDrawer)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(Label);
        ImGui::TableNextColumn();
        ValueDrawer();
    }

    void EnsureValidModuleSelection(ParticleEditorState* State, UParticleLODLevel* LODLevel)
    {
        if (!State)
        {
            return;
        }

        bool bNeedsFallback = false;
        switch (State->SelectedModuleSelection)
        {
        case EParticleDetailSelection::Required:
            bNeedsFallback = !LODLevel || !LODLevel->RequiredModule;
            break;
        case EParticleDetailSelection::Spawn:
            bNeedsFallback = !LODLevel || !LODLevel->SpawnModule;
            break;
        case EParticleDetailSelection::Module:
            bNeedsFallback = !LODLevel || State->SelectedModuleIndex < 0 || State->SelectedModuleIndex >= LODLevel->Modules.Num();
            break;
        default:
            bNeedsFallback = true;
            break;
        }

        if (!bNeedsFallback)
        {
            return;
        }

        if (LODLevel && LODLevel->TypeDataModule)
        {
            if (Cast<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule))
            {
                State->SelectedModuleSelection = EParticleDetailSelection::MeshType;
                State->SelectedModuleIndex = -1;
                return;
            }
        }
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
}

void FParticleEditorDetailSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.50f, 0.35f, 0.25f, 0.8f));
    ImGui::Text("Details");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::BeginChild("DetailContent", ImVec2(0, 0), true);

    // Early exit: No particle system
    if (!ActiveState || !ActiveState->HasParticleSystem())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No particle system loaded.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }

    // Early exit: No emitter selected
    UParticleEmitter* SelectedEmitter = ActiveState->GetSelectedEmitter();
    if (!SelectedEmitter)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Select an emitter to edit its properties.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }

    // Emitter 헤더 + Add Module 버튼
    ImGui::Text("Emitter %d Properties", ActiveState->SelectedEmitterIndex);
    ImGui::SameLine();

    const char* AddButtonText = "+ Add Module";
    ImVec2 LabelSize = ImGui::CalcTextSize(AddButtonText);
    float ButtonWidth = LabelSize.x + 16.0f;
    float AvailableWidth = ImGui::GetContentRegionAvail().x;
    if (AvailableWidth > ButtonWidth)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + AvailableWidth - ButtonWidth);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 2.0f));

    float TextHeight = ImGui::GetTextLineHeight();
    if (ImGui::Button(AddButtonText, ImVec2(ButtonWidth, TextHeight + 4.0f)))
    {
        ImGui::OpenPopup("AddModulePopup");
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    // Get active LOD level (기본 LOD 0만 사용)
    if (SelectedEmitter->LODLevels.Num() == 0 || !SelectedEmitter->LODLevels[0])
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No LOD levels available for this emitter.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }

    const int32 LodCount = SelectedEmitter->LODLevels.Num();
    int32 ActiveLODIndex = FMath::Clamp(ActiveState ? ActiveState->ActiveLODLevel : 0, 0, LodCount - 1);
    if (ActiveState && ActiveState->ActiveLODLevel != ActiveLODIndex)
    {
        ActiveState->ActiveLODLevel = ActiveLODIndex;
    }

    ImGui::PushID("DetailLODSelector");
    char CurrentLODLabel[32];
    sprintf_s(CurrentLODLabel, "LOD %d", ActiveLODIndex);
    ImGui::TextUnformatted("Active LOD");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##DetailActiveLOD", CurrentLODLabel))
    {
        for (int32 lodIndex = 0; lodIndex < LodCount; ++lodIndex)
        {
            char Option[32];
            sprintf_s(Option, "LOD %d", lodIndex);
            const bool bSelected = (lodIndex == ActiveLODIndex);
            if (ImGui::Selectable(Option, bSelected))
            {
                ActiveLODIndex = lodIndex;
                if (ActiveState)
                {
                    ActiveState->ActiveLODLevel = lodIndex;
                }
            }
            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    UParticleLODLevel* LODLevel = SelectedEmitter->LODLevels[ActiveLODIndex];
    if (!LODLevel)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Selected LOD data is missing.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }
    EnsureValidModuleSelection(ActiveState, LODLevel);

    if (!LODLevel->SpawnModule)
    {
        if (ImGui::Button("Add Spawn Module"))
        {
            AddSpawnModule(ActiveState, LODLevel);
        }
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
    }

    auto DrawModuleCard = [&](const char* ContainerId, const char* HeaderLabel, bool bClosable, const char* CloseButtonId, auto&& Body) -> bool
    {
        bool bDeleteRequested = false;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.35f, 0.35f, 0.6f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

        ImGui::BeginChild(
            ContainerId,
            ImVec2(0, 0),
            ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::Indent(8.0f);

        const char* DisplayLabel = HeaderLabel;
        std::string LabelStorage;
        if (const char* HashPos = strstr(HeaderLabel, "##"))
        {
            LabelStorage.assign(HeaderLabel, HashPos - HeaderLabel);
            DisplayLabel = LabelStorage.c_str();
        }

        ImVec2 labelPos = ImGui::GetCursorPos();
        ImGui::TextUnformatted(DisplayLabel);

        if (bClosable && CloseButtonId)
        {
            ImGui::SameLine();
            float closeSize = 22.0f;
            float available = ImGui::GetContentRegionAvail().x;
            ImVec2 restorePos = ImGui::GetCursorPos();
            float buttonX = restorePos.x + available - closeSize;
            float buttonY = labelPos.y - 2.0f;
            ImGui::SetCursorPos(ImVec2(buttonX, buttonY));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.05f, 0.05f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));

            if (ImGui::Button("X", ImVec2(closeSize, closeSize)))
            {
                bDeleteRequested = true;
            }

            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(4);
            ImGui::SetCursorPos(restorePos);
        }

        ImGui::Dummy(ImVec2(0.0f, 12.0f));
        if (!bDeleteRequested)
        {
            Body();
        }

        ImGui::Unindent(8.0f);
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        return bDeleteRequested;
    };

    if (ImGui::BeginPopup("AddModulePopup"))
    {
        if (!LODLevel->SpawnModule && ImGui::MenuItem("Spawn Module"))
        {
            AddSpawnModule(ActiveState, LODLevel);
        }

        if (!LODLevel->SpawnModule)
        {
            ImGui::Separator();
        }

        ImGui::Text("Initialization Modules");
        ImGui::Separator();
        if (ImGui::MenuItem("Initial Color"))
        {
            AddModule(ActiveState, LODLevel, "UParticleModuleColor");
        }
        if (ImGui::MenuItem("Initial Size"))
        {
            AddModule(ActiveState, LODLevel, "UParticleModuleSize");
        }
        if (ImGui::MenuItem("Initial Lifetime"))
        {
            AddModule(ActiveState, LODLevel, "UParticleModuleLifetime");
        }
        if (ImGui::MenuItem("Initial Location"))
        {
            AddModule(ActiveState, LODLevel, "UParticleModuleLocation");
        }
        if (ImGui::MenuItem("Initial Velocity"))
        {
            AddModule(ActiveState, LODLevel, "UParticleModuleVelocity");
        }
        if (ImGui::MenuItem("Initial Rotation"))
        {
            AddModule(ActiveState, LODLevel, "UParticleModuleRotation");
        }
        if (ImGui::MenuItem("Type Data Mesh"))
        {
            AddModule(ActiveState, LODLevel, "UParticleModuleTypeDataMesh");
        }
        ImGui::EndPopup();
    }

    const EParticleDetailSelection Selection = ActiveState ? ActiveState->SelectedModuleSelection : EParticleDetailSelection::None;
    const int32 SelectedModuleIndex = ActiveState ? ActiveState->SelectedModuleIndex : -1;
    bool bDeleteSpawn = false;
    bool bDeleteType = false;
    int32 ModuleToDelete = -1;

    auto ShowSelectionHint = [&](const char* Message)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::TextWrapped("%s", Message);
        ImGui::PopStyleColor();
    };

    switch (Selection)
    {
    case EParticleDetailSelection::MeshType:
        if (LODLevel->TypeDataModule)
        {
            UParticleModuleTypeDataMesh* MeshModule = static_cast<UParticleModuleTypeDataMesh*>(LODLevel->TypeDataModule);
            bDeleteType = DrawModuleCard("TypeDataMeshContainer", "Type Data Mesh Module", true, "X##DeleteTypeDataMesh", [&]()
            {
                DrawTypeDataMeshModuleProperties(MeshModule, Context);
            });
        }
        break;
    case EParticleDetailSelection::Required:
        if (LODLevel->RequiredModule)
        {
            DrawModuleCard("RequiredModuleContainer", "Required Module", false, nullptr, [&]()
            {
                DrawRequiredModuleProperties(LODLevel->RequiredModule, Context);
            });
        }
        else
        {
            ShowSelectionHint("Required module is missing on this LOD.");
        }
        break;

    case EParticleDetailSelection::Spawn:
        if (LODLevel->SpawnModule)
        {
            bDeleteSpawn = DrawModuleCard("SpawnModuleContainer", "Spawn Module", true, "X##DeleteSpawn", [&]()
            {
                DrawSpawnModuleProperties(LODLevel->SpawnModule);
            });
        }
        else
        {
            ShowSelectionHint("No spawn module exists. Use 'Add Spawn Module' to create one.");
        }
        break;

    case EParticleDetailSelection::Module:
        if (SelectedModuleIndex >= 0 && SelectedModuleIndex < LODLevel->Modules.Num())
        {
            UParticleModule* Module = LODLevel->Modules[SelectedModuleIndex];
            if (Module)
            {
                ImGui::PushID(SelectedModuleIndex);
                char ContainerId[64];
                sprintf_s(ContainerId, "ModuleContainer##%d", SelectedModuleIndex);
                const char* ClassName = Module->GetClass()->Name;
                char HeaderLabel[128];
                sprintf_s(HeaderLabel, "%s##ModuleSelected", ClassName);

                if (DrawModuleCard(ContainerId, HeaderLabel, true, "X##DeleteModule", [&]()
                    {
                        DrawModuleProperties(Module, SelectedModuleIndex);
                    }))
                {
                    ModuleToDelete = SelectedModuleIndex;
                }
                ImGui::PopID();
            }
            else
            {
                ShowSelectionHint("Selected module no longer exists.");
            }
        }
        else
        {
            ShowSelectionHint("Select a module from the emitter stack to edit it.");
        }
        break;

    default:
        ShowSelectionHint("Select a module chip in the emitter section to view its properties.");
        break;
    }

    if (bDeleteType)
    {
        DeleteTypeModule(ActiveState, LODLevel);
    }
    if (bDeleteSpawn)
    {
        DeleteSpawnModule(ActiveState, LODLevel);
    }

    if (ModuleToDelete >= 0)
    {
        DeleteModule(ActiveState, LODLevel, ModuleToDelete);
        if (ActiveState)
        {
            ActiveState->SelectedModuleSelection = EParticleDetailSelection::None;
            ActiveState->SelectedModuleIndex = -1;
        }
    }

    ImGui::EndChild();
}

void FParticleEditorDetailSection::DrawDistributionFloat(const char* Label, FRawDistributionFloat& Distribution, float Min, float Max)
{
    const char* DistModes[] = { "Constant", "Uniform", "Curve" };
    char ComboLabel[128];
    sprintf_s(ComboLabel, "%s Mode", Label);

    char ComboId[128];
    sprintf_s(ComboId, "##%sMode", Label);

    DrawPropertyRow(ComboLabel, [&]()
    {
        int32 CurrentMode = static_cast<int32>(Distribution.Operation);
        if (ImGui::Combo(ComboId, &CurrentMode, DistModes, IM_ARRAYSIZE(DistModes)))
        {
            Distribution.Operation = static_cast<EDistributionMode>(CurrentMode);
        }
    });

    if (Distribution.Operation == EDistributionMode::DOP_Constant)
    {
        char ConstantId[128];
        sprintf_s(ConstantId, "##%sConstant", Label);
        DrawPropertyRow(Label, [&]() { ImGui::DragFloat(ConstantId, &Distribution.Constant, 0.1f, Min, Max); });
    }
    else if (Distribution.Operation == EDistributionMode::DOP_Uniform)
    {
        char MinLabel[128], MaxLabel[128], MinId[128], MaxId[128];
        sprintf_s(MinLabel, "Min %s", Label);
        sprintf_s(MaxLabel, "Max %s", Label);
        sprintf_s(MinId, "##%sMin", Label);
        sprintf_s(MaxId, "##%sMax", Label);

        DrawPropertyRow(MinLabel, [&]() { ImGui::DragFloat(MinId, &Distribution.Min, 0.1f, Min, Max); });
        DrawPropertyRow(MaxLabel, [&]() { ImGui::DragFloat(MaxId, &Distribution.Max, 0.1f, Min, Max); });
    }
    else if (Distribution.Operation == EDistributionMode::DOP_Curve)
    {
        DrawPropertyRow(Label, [&]()
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
            ImGui::TextWrapped("Curve Editor에서 편집 가능");
            ImGui::PopStyleColor();
        });
    }
}

void FParticleEditorDetailSection::DrawDistributionVector(const char* Label, FRawDistributionVector& Distribution, float Min, float Max, bool bIsColor)
{
    const char* DistModes[] = { "Constant", "Uniform", "Curve" };
    char ComboLabel[128];
    sprintf_s(ComboLabel, "%s Mode", Label);

    char ComboId[128];
    sprintf_s(ComboId, "##%sMode", Label);

    DrawPropertyRow(ComboLabel, [&]()
    {
        int32 CurrentMode = static_cast<int32>(Distribution.Operation);
        if (ImGui::Combo(ComboId, &CurrentMode, DistModes, IM_ARRAYSIZE(DistModes)))
        {
            Distribution.Operation = static_cast<EDistributionMode>(CurrentMode);
        }
    });

    if (Distribution.Operation == EDistributionMode::DOP_Constant)
    {
        char ConstantId[128];
        sprintf_s(ConstantId, "##%sConstant", Label);
        DrawPropertyRow(Label, [&]()
        {
            if (bIsColor)
            {
                ImGui::ColorEdit3(ConstantId, &Distribution.Constant.X);
            }
            else
            {
                ImGui::DragFloat3(ConstantId, &Distribution.Constant.X, 0.1f, Min, Max);
            }
        });
    }
    else if (Distribution.Operation == EDistributionMode::DOP_Uniform)
    {
        char MinLabel[128], MaxLabel[128], MinId[128], MaxId[128];
        sprintf_s(MinLabel, "Min %s", Label);
        sprintf_s(MaxLabel, "Max %s", Label);
        sprintf_s(MinId, "##%sMin", Label);
        sprintf_s(MaxId, "##%sMax", Label);

        DrawPropertyRow(MinLabel, [&]()
        {
            if (bIsColor)
            {
                ImGui::ColorEdit3(MinId, &Distribution.Min.X);
            }
            else
            {
                ImGui::DragFloat3(MinId, &Distribution.Min.X, 0.1f, Min, Max);
            }
        });

        DrawPropertyRow(MaxLabel, [&]()
        {
            if (bIsColor)
            {
                ImGui::ColorEdit3(MaxId, &Distribution.Max.X);
            }
            else
            {
                ImGui::DragFloat3(MaxId, &Distribution.Max.X, 0.1f, Min, Max);
            }
        });
    }
    else if (Distribution.Operation == EDistributionMode::DOP_Curve)
    {
        DrawPropertyRow(Label, [&]()
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
            ImGui::TextWrapped("Curve Editor에서 편집 가능 (X, Y, Z 각각)");
            ImGui::PopStyleColor();
        });
    }
}

void FParticleEditorDetailSection::AddSpawnModule(ParticleEditorState* State, UParticleLODLevel* LODLevel)
{
    if (!State || !LODLevel || LODLevel->SpawnModule)
    {
        return;
    }

    UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
    SpawnModule->SpawnRate.Operation = EDistributionMode::DOP_Constant;
    SpawnModule->SpawnRate.Constant = 10.0f;

    LODLevel->SpawnModule = SpawnModule;

    if (State->GetSelectedEmitter())
    {
        State->GetSelectedEmitter()->CacheEmitterModuleInfo();
    }

    State->SelectedModuleSelection = EParticleDetailSelection::Spawn;
    State->SelectedModuleIndex = -1;

    UE_LOG("Spawn module added");
}

void FParticleEditorDetailSection::DeleteTypeModule(ParticleEditorState* State, UParticleLODLevel* LODLevel)
{
    if (!State || !LODLevel || !LODLevel->TypeDataModule)
    {
        return;
    }

    DeleteObject(LODLevel->TypeDataModule);
    LODLevel->TypeDataModule = nullptr;

    if (State->GetSelectedEmitter())
    {
        State->GetSelectedEmitter()->CacheEmitterModuleInfo();
        State->PreviewComponent->EndPlay();
        State->PreviewComponent->InitParticles();
        State->PreviewComponent->BeginPlay();
    }
    State->SelectedModuleSelection = EParticleDetailSelection::None;
    State->SelectedModuleIndex = -1;

    UE_LOG("Type Data module deleted");
}

void FParticleEditorDetailSection::DeleteSpawnModule(ParticleEditorState* State, UParticleLODLevel* LODLevel)
{
    if (!State || !LODLevel || !LODLevel->SpawnModule)
    {
        return;
    }

    DeleteObject(LODLevel->SpawnModule);
    LODLevel->SpawnModule = nullptr;

    if (State->GetSelectedEmitter())
    {
        State->GetSelectedEmitter()->CacheEmitterModuleInfo();
    }

    State->SelectedModuleSelection = EParticleDetailSelection::None;
    State->SelectedModuleIndex = -1;

    UE_LOG("Spawn module deleted");
}

void FParticleEditorDetailSection::AddModule(ParticleEditorState* State, UParticleLODLevel* LODLevel, const char* ModuleClassName)
{
    if (!State || !LODLevel || !ModuleClassName)
    {
        return;
    }

    UParticleModule* NewModule = nullptr;

    auto HasModuleOfType = [&](const char* ClassName) -> bool
    {
        for (UParticleModule* Existing : LODLevel->Modules)
        {
            if (Existing && Existing->GetClass()->Name && strcmp(Existing->GetClass()->Name, ClassName) == 0)
            {
                return true;
            }
        }
        return false;
    };

    // 모듈 타입에 따른 기본값 설정
    if (strcmp(ModuleClassName, "UParticleModuleColor") == 0)
    {
        UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
        ColorModule->StartColor.Operation = EDistributionMode::DOP_Constant;
        ColorModule->StartColor.Constant = FVector(1.0f, 1.0f, 1.0f);
        NewModule = ColorModule;
    }
    else if (strcmp(ModuleClassName, "UParticleModuleSize") == 0)
    {
        UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
        SizeModule->StartSize.Operation = EDistributionMode::DOP_Constant;
        SizeModule->StartSize.Constant = FVector(1.0f, 1.0f, 1.0f);
        NewModule = SizeModule;
    }
    else if (strcmp(ModuleClassName, "UParticleModuleLifetime") == 0)
    {
        UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
        LifetimeModule->LifeTime.Operation = EDistributionMode::DOP_Constant;
        LifetimeModule->LifeTime.Constant = 2.0f;
        NewModule = LifetimeModule;
    }
    else if (strcmp(ModuleClassName, "UParticleModuleLocation") == 0)
    {
        UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
        LocationModule->SpawnLocation.Operation = EDistributionMode::DOP_Constant;
        LocationModule->SpawnLocation.Constant = FVector(0.0f, 0.0f, 0.0f);
        NewModule = LocationModule;
    }
    else if (strcmp(ModuleClassName, "UParticleModuleVelocity") == 0)
    {
        UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
        VelocityModule->StartVelocity.Operation = EDistributionMode::DOP_Constant;
        VelocityModule->StartVelocity.Constant = FVector(0.0f, 0.0f, 10.0f);
        NewModule = VelocityModule;
    }
    else if (strcmp(ModuleClassName, "UParticleModuleRotation") == 0)
    {
        UParticleModuleRotation* RotationModule = NewObject<UParticleModuleRotation>();
        RotationModule->StartRotation.Operation = EDistributionMode::DOP_Constant;
        RotationModule->StartRotation.Constant = 0.0f;
        NewModule = RotationModule;
    }
    else if (strcmp(ModuleClassName, "UParticleModuleTypeDataMesh") == 0)
    {
        UParticleModuleTypeDataMesh* MeshModule = NewObject<UParticleModuleTypeDataMesh>();
        MeshModule->StaticMesh = UResourceManager::GetInstance().Load<UStaticMesh>(GDataDir + "/Model/smokegrenade.obj");
        NewModule = MeshModule;
        LODLevel->TypeDataModule = MeshModule;
    }

    if (!NewModule)
    {
        return;
    }

    if (HasModuleOfType(ModuleClassName))
    {
        UE_LOG("Module %s already exists; skipping duplicate add.", ModuleClassName);
        DeleteObject(NewModule);
        return;
    }

    if (!Cast<UParticleModuleTypeDataBase>(NewModule))
    {
        LODLevel->Modules.Add(NewModule);
        LODLevel->SpawnModules.Add(NewModule);
        State->SelectedModuleSelection = EParticleDetailSelection::Module;
        State->SelectedModuleIndex = LODLevel->Modules.Num() - 1;
    }
    else
    {
        State->SelectedModuleSelection = EParticleDetailSelection::MeshType;
        State->SelectedModuleIndex = 1002;
    }

    if (State->GetSelectedEmitter())
    {
        State->GetSelectedEmitter()->CacheEmitterModuleInfo();
        State->PreviewComponent->EndPlay();
        State->PreviewComponent->InitParticles();
        State->PreviewComponent->BeginPlay();
    }

    

    UE_LOG("Module added: %s. Total modules: %d", ModuleClassName, LODLevel->Modules.Num());
}

void FParticleEditorDetailSection::DeleteModule(ParticleEditorState* State, UParticleLODLevel* LODLevel, int32 ModuleIndex)
{
    if (!State || !LODLevel || ModuleIndex < 0 || ModuleIndex >= LODLevel->Modules.Num())
    {
        return;
    }

    UParticleModule* ModuleToDelete = LODLevel->Modules[ModuleIndex];
    if (!ModuleToDelete)
    {
        return;
    }

    LODLevel->SpawnModules.Remove(ModuleToDelete);
    LODLevel->Modules.RemoveAt(ModuleIndex);

    DeleteObject(ModuleToDelete);

    if (State->GetSelectedEmitter())
    {
        State->GetSelectedEmitter()->CacheEmitterModuleInfo();
    }

    State->SelectedModuleSelection = EParticleDetailSelection::None;
    State->SelectedModuleIndex = -1;

    UE_LOG("Module deleted. Remaining modules: %d", LODLevel->Modules.Num());
}

void FParticleEditorDetailSection::DrawTypeDataMeshModuleProperties(UParticleModuleTypeDataMesh* Module, const FParticleEditorSectionContext& Context)
{
    if (!Module)
    {
        ImGui::TextDisabled("TypeDataMeshModule Missing");
        return;
    }

    ImGui::PushID("TypeDataMeshModuleProps");
    if (ImGui::BeginTable("TypeDataMeshModulePropsTable", 2, PropertyTableFlags))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        DrawPropertyRow("Mesh", [&]()
            {
                FString CurrentPath = "";
                if (Module->StaticMesh)
                {
                    CurrentPath = Module->StaticMesh->GetFilePath();
                }
                if (UPropertyRenderer::CachedStaticMeshPaths.empty())
                {
                    ImGui::Text("<No Meshes>");
                    return;
                }
                int SelectedIdx = -1;
                for (int i = 0; i < static_cast<int>(UPropertyRenderer::CachedStaticMeshPaths.size()); ++i)
                {
                    if (UPropertyRenderer::CachedStaticMeshPaths[i] == CurrentPath)
                    {
                        SelectedIdx = i;
                        break;
                    }
                }
                TArray<const char*> ItemsPtr;
                ItemsPtr.reserve(UPropertyRenderer::CachedStaticMeshItems.size());
                for (const FString& item : UPropertyRenderer::CachedStaticMeshItems)
                {
                    ItemsPtr.push_back(item.c_str());
                }

                //ImGui::SetNextItemWidth(240);
                if (ImGui::Combo("##MeshCombInParticleEditor", &SelectedIdx, ItemsPtr.data(), static_cast<int>(ItemsPtr.size())))
                {
                    if (SelectedIdx >= 0 && SelectedIdx < static_cast<int>(UPropertyRenderer::CachedStaticMeshPaths.size()))
                    {
                        Module->StaticMesh = UResourceManager::GetInstance().Load<UStaticMesh>(UPropertyRenderer::CachedStaticMeshPaths[SelectedIdx]);
                    }
                    return;
                }
            });
        ImGui::EndTable();
    }
    ImGui::PopID();
}

void FParticleEditorDetailSection::DrawRequiredModuleProperties(UParticleModuleRequired* Module, const FParticleEditorSectionContext& Context)
{
    if (!Module)
    {
        ImGui::TextDisabled("Required module missing.");
        return;
    }

    ImGui::PushID("RequiredModuleProps");
    if (ImGui::BeginTable("RequiredPropsTable", 2, PropertyTableFlags))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        DrawPropertyRow("Enabled", [&]() { ImGui::Checkbox("##RequiredEnabled", &Module->bEnabled); });

        // Material/Texture 선택
        DrawPropertyRow("Material", [&]()
        {
            // 현재 Material 경로에서 파일명만 추출
            FString CurrentMaterialPath = "";
            FString CurrentMaterialFileName = "None";

            if (Module->Material)
            {
                CurrentMaterialPath = Module->Material->GetFilePath();
                if (!CurrentMaterialPath.empty())
                {
                    // 파일명만 추출 (경로 제거)
                    std::filesystem::path filePath(CurrentMaterialPath);
                    CurrentMaterialFileName = filePath.filename().string();
                }
                else
                {
                    // 동적으로 생성된 Material의 경우 텍스처 파일명 표시
                    const FMaterialInfo& MatInfo = Module->Material->GetMaterialInfo();
                    if (!MatInfo.DiffuseTextureFileName.empty())
                    {
                        std::filesystem::path texPath(MatInfo.DiffuseTextureFileName);
                        CurrentMaterialFileName = texPath.filename().string();
                        CurrentMaterialPath = MatInfo.DiffuseTextureFileName;
                    }
                }
            }

            // 버튼 크기 계산
            float availableWidth = ImGui::GetContentRegionAvail().x;
            float buttonWidth = 60.0f;
            float textWidth = availableWidth - buttonWidth - 8.0f;

            // Material 파일명 표시 (읽기 전용, 비활성화)
            ImGui::PushItemWidth(textWidth);
            ImGui::BeginDisabled(true);
            char buffer[512];
            strncpy_s(buffer, sizeof(buffer), CurrentMaterialFileName.c_str(), _TRUNCATE);
            ImGui::InputText("##MaterialPath", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();

            // 호버 시 전체 경로를 툴팁으로 표시
            if (ImGui::IsItemHovered() && Module->Material && !CurrentMaterialPath.empty())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(CurrentMaterialPath.c_str());
                ImGui::EndTooltip();
            }

            ImGui::PopItemWidth();

            // Browse 버튼 - 배경색을 더 진하게
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            if (ImGui::Button("Browse##MaterialBrowse", ImVec2(buttonWidth, 0)))
            {
                // 스프라이트 타입: 텍스처 선택 후 Billboard Material 생성
                const FWideString BaseDir = L"Data/Textures";
                const FWideString Extension = L".png;.jpg;.dds;.tga";
                const FWideString Description = L"Texture Files";
                std::filesystem::path SelectedFile = FPlatformProcess::OpenLoadFileDialog(BaseDir, Extension, Description);

                if (!SelectedFile.empty())
                {
                    try
                    {
                        FString TexturePath = MakeDataRelativePath(SelectedFile);

                        // 텍스처 로드
                        UTexture* Texture = UResourceManager::GetInstance().Load<UTexture>(TexturePath);
                        if (!Texture)
                        {
                            UE_LOG("Failed to load texture: %s", TexturePath.c_str());
                            return;
                        }

                        // Billboard Material 생성 (스프라이트용)
                        UMaterial* NewMaterial = NewObject<UMaterial>();
                        if (!NewMaterial)
                        {
                            UE_LOG("Failed to create material");
                            return;
                        }

                        // Billboard 셰이더 로드
                        UShader* BillboardShader = UResourceManager::GetInstance().Load<UShader>("Shaders/UI/Billboard.hlsl");
                        if (!BillboardShader)
                        {
                            DeleteObject(NewMaterial);
                            UE_LOG("Failed to load Billboard shader");
                            return;
                        }

                        NewMaterial->SetShader(BillboardShader);

                        // Material 정보 설정
                        FMaterialInfo MatInfo;
                        MatInfo.MaterialName = "ParticleSpriteMaterial";
                        MatInfo.DiffuseTextureFileName = TexturePath;
                        NewMaterial->SetMaterialInfo(MatInfo);
                        NewMaterial->ResolveTextures();

                        Module->Material = NewMaterial;
                        UE_LOG("Texture loaded and material created: %s", TexturePath.c_str());

                        // 프리뷰 업데이트 - Material이 변경되었으므로 파티클 재초기화
                        if (Context.ActiveState && Context.ActiveState->PreviewComponent)
                        {
                            Context.ActiveState->PreviewComponent->EndPlay();
                            Context.ActiveState->PreviewComponent->InitParticles();
                            Context.ActiveState->PreviewComponent->BeginPlay();
                        }
                    }
                    catch (const std::exception& e)
                    {
                        UE_LOG("Exception while loading texture: %s", e.what());
                    }
                }
            }
            ImGui::PopStyleColor(3);  // Browse 버튼 색상 복원
        });

        DrawPropertyRow("Emitter Duration", [&]() { ImGui::DragFloat("##EmitterDuration", &Module->EmitterDuration, 0.1f, 0.0f, 100.0f); });
        DrawPropertyRow("Emitter Delay", [&]() { ImGui::DragFloat("##EmitterDelay", &Module->EmitterDelay, 0.1f, 0.0f, 100.0f); });
        DrawPropertyRow("Emitter Loops", [&]() { ImGui::DragInt("##EmitterLoops", &Module->EmitterLoops, 1, 0, 100); });
        DrawPropertyRow("Use Local Space", [&]() { ImGui::Checkbox("##UseLocalSpace", &Module->bUseLocalSpace); });

        ImGui::EndTable();
    }
    ImGui::PopID();
}

void FParticleEditorDetailSection::DrawSpawnModuleProperties(UParticleModuleSpawn* Module)
{
    if (!Module)
    {
        ImGui::TextDisabled("Spawn module missing.");
        return;
    }

    ImGui::PushID("SpawnModuleProps");
    if (ImGui::BeginTable("SpawnPropsTable", 2, PropertyTableFlags))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        DrawPropertyRow("Enabled", [&]() { ImGui::Checkbox("##SpawnEnabled", &Module->bEnabled); });
        DrawDistributionFloat("Spawn Rate", Module->SpawnRate, 0.0f, 1000.0f);

        ImGui::EndTable();
    }
    ImGui::PopID();
}

void FParticleEditorDetailSection::DrawModuleProperties(UParticleModule* Module, int32 ModuleIndex)
{
    if (!Module)
    {
        ImGui::TextDisabled("Module missing.");
        return;
    }

    char TableId[32];
    sprintf_s(TableId, "ModulePropsTable##%d", ModuleIndex);
    if (ImGui::BeginTable(TableId, 2, PropertyTableFlags))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::PushID(ModuleIndex);

        char EnabledId[64];
        sprintf_s(EnabledId, "##Enabled%d", ModuleIndex);
        DrawPropertyRow("Enabled", [&]() { ImGui::Checkbox(EnabledId, &Module->bEnabled); });

        if (UParticleModuleLifetime* LifetimeMod = dynamic_cast<UParticleModuleLifetime*>(Module))
        {
            DrawDistributionFloat("Lifetime", LifetimeMod->LifeTime, 0.0f, 100.0f);
        }
        else if (UParticleModuleSize* SizeMod = dynamic_cast<UParticleModuleSize*>(Module))
        {
            DrawDistributionVector("Start Size", SizeMod->StartSize, 0.0f, 100.0f, false);
        }
        else if (UParticleModuleColor* ColorMod = dynamic_cast<UParticleModuleColor*>(Module))
        {
            DrawDistributionVector("Start Color", ColorMod->StartColor, 0.0f, 1.0f, true);
        }
        else if (UParticleModuleLocation* LocMod = dynamic_cast<UParticleModuleLocation*>(Module))
        {
            DrawDistributionVector("Spawn Location", LocMod->SpawnLocation, -1000.0f, 1000.0f, false);
        }
        else if (UParticleModuleVelocity* VelMod = dynamic_cast<UParticleModuleVelocity*>(Module))
        {
            DrawDistributionVector("Start Velocity", VelMod->StartVelocity, -1000.0f, 1000.0f, false);
        }
        else if (UParticleModuleRotation* RotMod = dynamic_cast<UParticleModuleRotation*>(Module))
        {
            DrawDistributionVector("Start Rotation (Degrees)", RotMod->StartRotation, -360.0f, 360.0f);
        }

        ImGui::PopID();
        ImGui::EndTable();
    }
}

