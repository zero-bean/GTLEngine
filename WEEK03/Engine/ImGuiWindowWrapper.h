#pragma once
#include "stdafx.h"
#include "UObject.h"
#include "UEngineStatics.h"

class ImGuiWindowWrapper : public UObject
{
private:
    FString WindowName;
    ImVec2 FixedPos;
    ImVec2 FixedSize;

public:
    ImGuiWindowWrapper(FString windowName, ImVec2 fixedPos = ImVec2(0, 0), ImVec2 fixedSize = ImVec2(400, 300))
        : WindowName(windowName), FixedPos(fixedPos), FixedSize(fixedSize) {}

    void Render()
    {
        // Set fixed position and size before Begin
        ImGui::SetNextWindowPos(FixedPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(FixedSize, ImGuiCond_Always);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        if (ImGui::Begin(WindowName.c_str(), nullptr, windowFlags))
        {
            RenderContent();
        }
        ImGui::End();
    }

protected:
    virtual void RenderContent() = 0;  // 파생 클래스에서 구현
};