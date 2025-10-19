#include "pch.h"
#include "DirectionalLightComponent.h"
#include "SceneLoader.h"
#include "ImGui/imgui.h"

UDirectionalLightComponent::UDirectionalLightComponent()
{
    bCanEverTick = true;    
}

UDirectionalLightComponent::~UDirectionalLightComponent()
{
}

void UDirectionalLightComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // Call parent class serialization for transforms
    ULightComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        Intensity = InOut.DirectionalLightProperty.Intensity;
        FinalColor = InOut.DirectionalLightProperty.Color;
        bEnableSpecular = InOut.DirectionalLightProperty.bEnableSpecular;
    }
    else
    {
        InOut.DirectionalLightProperty.Intensity = Intensity;
        InOut.DirectionalLightProperty.Color = FinalColor;
        InOut.DirectionalLightProperty.bEnableSpecular = bEnableSpecular;
    }
}

void UDirectionalLightComponent::TickComponent(float DeltaSeconds)
{
    // Directional light는 보통 고정되어 있지만, 필요시 동적 변화 구현 가능
    // 예: 해의 이동에 따른 방향 변화 등
}

void UDirectionalLightComponent::SetSpecularEnable(bool bEnable)
{
    bEnableSpecular = bEnable ? 1 : 0;
}

UObject* UDirectionalLightComponent::Duplicate()
{
    UDirectionalLightComponent* DuplicatedComponent = NewObject<UDirectionalLightComponent>();
    CopyCommonProperties(DuplicatedComponent);
    DuplicatedComponent->Intensity = this->Intensity;
    DuplicatedComponent->FinalColor = this->FinalColor;    
    DuplicatedComponent->bEnableSpecular = this->bEnableSpecular;
    DuplicatedComponent->DuplicateSubObjects();
    return DuplicatedComponent;
}

void UDirectionalLightComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}

void UDirectionalLightComponent::RenderDetails()
{
	ImGui::Separator();
	ImGui::Text("PointLight Component Settings");

	// 🔸 색상 설정 (RGB Color Picker)
	float color[3] = { GetColor().R, GetColor().G, GetColor().B};
	if (ImGui::ColorEdit3("Color", color))
	{
		SetColor(FLinearColor(color[0], color[1], color[2], 1.0f));
	}

	ImGui::Spacing();

	// 🔸 밝기 (Intensity)
	float intensity = GetIntensity();
	if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
	{
		SetIntensity(intensity);
	}

	bool bEnableSpecular = IsEnabledSpecular() && true;
	if (ImGui::Checkbox("Specular Enable", &bEnableSpecular))
	{
		SetSpecularEnable(bEnableSpecular);
	}

	ImGui::Spacing();

	// 🔸 시각적 미리보기용 Sphere 표시 (선택된 경우)
	ImGui::Text("Preview:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(color[0], color[1], color[2], 1.0f), "● DirectionalLight Active");
}
