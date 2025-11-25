#include "pch.h"
#include "ParticleEditorSections.h"
#include "SParticleEditorWindow.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorState.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Grid/GridActor.h"
#include "ParticleSystem.h"

void FParticleEditorMenuBarSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Particle System"))
            {
                if (ActiveState)
                {
                    ActiveState->CurrentParticleSystem = UParticleSystem::GetTestParticleSystem();
                    ActiveState->LoadedParticleSystemPath = "Test Particle System";
                    ActiveState->SelectedEmitterIndex = -1;
                }
            }
            if (ImGui::MenuItem("Open..."))
            {
                // TODO: Open particle system
            }
            if (ImGui::MenuItem("Save"))
            {
                // TODO: Save particle system
            }
            if (ImGui::MenuItem("Save As..."))
            {
                // TODO: Save as particle system
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close"))
            {
                Context.Window.Close();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
            {
                // TODO: Undo
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y"))
            {
                // TODO: Redo
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Emitter"))
            {
                // TODO: Delete selected emitter
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emitter"))
        {
            if (ImGui::MenuItem("Add Sprite Emitter"))
            {
                // TODO: Add sprite emitter
            }
            if (ImGui::MenuItem("Add Mesh Emitter"))
            {
                // TODO: Add mesh emitter
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate Emitter"))
            {
                // TODO: Duplicate selected emitter
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ActiveState)
            {
                if (ImGui::MenuItem("Show Grid", nullptr, &ActiveState->bShowGrid))
                {
                    if (ActiveState->World && ActiveState->World->GetGridActor())
                    {
                        ActiveState->World->GetGridActor()->SetGridVisible(ActiveState->bShowGrid);
                    }
                }
                if (ImGui::MenuItem("Show Bounds", nullptr, &ActiveState->bShowBounds))
                {
                    // TODO: Toggle bounds visibility
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Reset Camera"))
                {
                    // TODO: Reset camera
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}
