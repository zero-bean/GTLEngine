#include "pch.h"
#include "ParticleEditorSections.h"
#include "SParticleEditorWindow.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorState.h"

void FParticleEditorMenuBarSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Particle System"))
            {
                // TODO: Create new particle system
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
                    // TODO: Toggle grid visibility
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

void FParticleEditorToolBarSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::BeginChild("ToolBar", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.40f, 0.55f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.50f, 0.70f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.35f, 0.50f, 1.0f));

        if (ImGui::Button("New System", ImVec2(100, 30)))
        {
            // TODO: Create new particle system
        }
        ImGui::SameLine();

        if (ImGui::Button("Add Emitter", ImVec2(100, 30)))
        {
            // TODO: Add emitter to system
        }
        ImGui::SameLine();

        if (ImGui::Button("Save", ImVec2(80, 30)))
        {
            // TODO: Save particle system
        }
        ImGui::SameLine();

        ImGui::Separator();
        ImGui::SameLine();

        if (ActiveState)
        {
            if (ImGui::Button(ActiveState->bIsPlaying ? "Stop" : "Play", ImVec2(80, 30)))
            {
                ActiveState->bIsPlaying = !ActiveState->bIsPlaying;
                // TODO: Toggle particle system playback
            }
            ImGui::SameLine();

            if (ImGui::Button("Reset", ImVec2(80, 30)))
            {
                ActiveState->CurrentTime = 0.0f;
                // TODO: Reset particle system
            }
        }

        ImGui::PopStyleColor(3);
    }
    ImGui::EndChild();
}

void FParticleEditorViewportSection::Draw(const FParticleEditorSectionContext& Context)
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Viewport");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::BeginChild("ViewportContent", viewportSize, false, ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::Text("Particle Preview Viewport");
        ImGui::Text("Size: %.0fx%.0f", viewportSize.x, viewportSize.y);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void FParticleEditorEmitterSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.50f, 0.35f, 0.8f));
    ImGui::Text("Emitters");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Particle System Emitters:");
    ImGui::Spacing();

    ImGui::BeginChild("EmitterList", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No emitters in the system.");
        ImGui::TextWrapped("Use 'Add Emitter' button to add emitters.");
        ImGui::PopStyleColor();

        // TODO: Display list of emitters once particle systems are wired up
        (void)ActiveState;
    }
    ImGui::EndChild();
}

void FParticleEditorDetailSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.50f, 0.35f, 0.25f, 0.8f));
    ImGui::Text("Details");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Emitter Properties:");
    ImGui::Spacing();

    ImGui::BeginChild("DetailContent", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Select an emitter to edit its properties.");
        ImGui::PopStyleColor();

        // TODO: Display emitter properties when selection is available
        (void)ActiveState;
    }
    ImGui::EndChild();
}

void FParticleEditorCurveSection::Draw(const FParticleEditorSectionContext& Context)
{
    (void)Context;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.25f, 0.50f, 0.8f));
    ImGui::Text("Curve Editor");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Property Curves:");
    ImGui::Spacing();

    ImGui::BeginChild("CurveEditorContent", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Curve editor for particle properties over time.");
        ImGui::TextWrapped("Select a property from Details panel to edit its curve.");
        ImGui::PopStyleColor();

        // TODO: Implement the actual curve editor overlay
    }
    ImGui::EndChild();
}
