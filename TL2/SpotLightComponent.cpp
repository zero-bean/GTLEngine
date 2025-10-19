#include "pch.h"
#include "SpotLightComponent.h"
#include "ImGui/imgui.h"

USpotLightComponent::USpotLightComponent() : Direction(1.0, 0.0f, 0.0f, 0.0f), InnerConeAngle(10.0), OuterConeAngle(30.0), AttFactor( 0, 0, 1), InAntOutSmooth(1)
{ 

}

USpotLightComponent::~USpotLightComponent()
{
}

UObject* USpotLightComponent::Duplicate()
{
	return nullptr;
}

void USpotLightComponent::DuplicateSubObjects()
{
}

void USpotLightComponent::RenderDetails()
{
	ImGui::Separator();
	ImGui::Text("SpotLight Component Settings");

	// 🔸 색상 설정 (RGB Color Picker)
	float color[3] = { GetColor().R, GetColor().G, GetColor().B};
	if (ImGui::ColorEdit3("Color", color))
	{
		SetColor(FLinearColor(color[0], color[1], color[2], 1.0f));
	}

	ImGui::Spacing();

	// 🔸 반지름
	float radius = GetRadius();
	if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.0f, 20.0f))
	{
		SetRadius(radius);
	}

	// 🔸 밝기 (Intensity)
	float intensity = GetIntensity();
	if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
	{
		SetIntensity(intensity);
	}

	// 🔸 InnerConeAngle
	float InnerRadius = GetInnerConeAngle();
	if (ImGui::DragFloat("InnerConeAngle", &InnerRadius, 1.0f, 0.0f, 90.0f))
	{
		SetInnerConeAngle(InnerRadius);
	}

	// 🔸 OuterConeAngle
	float OuterRadius = GetOuterConeAngle();
	if (ImGui::DragFloat("OuterConeAngle", &OuterRadius, 1.0f, 0.0f, 90.0f))
	{
		SetOuterConeAngle(OuterRadius);
	}

	// 🔸 감쇠 정도 (FallOff)
	float falloff = GetRadiusFallOff();
	if (ImGui::DragFloat("FallOff", &falloff, 0.05f, 0.1f, 10.0f))
	{
		SetRadiusFallOff(falloff);
	}
	FVector attFactor = GetAttFactor();
	if (ImGui::DragFloat3("Attenuation Factor", &attFactor.X, 1.0f, 0.0f, 10.0f))
	{
		SetAttFactor(attFactor);
	}

	// 🔸 inner 원과 outter 원과 smooth하게 섞임
	float smooth = GetInAndOutSmooth();
	if (ImGui::DragFloat("In&Out Smooth Factor", &smooth, 1.0f, 1.0f, 10.0f))
	{
		SetInAndOutSmooth(smooth);
	}

	// Circle vertex count for end-cap visualization
	int segs = GetCircleSegments();
	if (ImGui::DragInt("Circle Segments", &segs, 1.0f, 3, 512))
	{
		segs = FMath::Clamp(segs, 3, 512);
		SetCircleSegments(segs);
	}
	ImGui::Spacing();

	// 🔸 시각적 미리보기용 Sphere 표시 (선택된 경우)
	ImGui::Text("Preview:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(color[0], color[1], color[2], 1.0f), "● PointLight Active");
}
