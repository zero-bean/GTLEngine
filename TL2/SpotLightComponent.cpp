#include "pch.h"
#include "SpotLightComponent.h"
#include "ImGui/imgui.h"
#include "Renderer.h"
#include "World.h"
#include "CameraActor.h"

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

void USpotLightComponent::DrawDebugLines(URenderer* Renderer)
{
	if (!this->IsRender() || !Renderer)
	{
		return;
	}

	if (!bEnableDebugLine)
	{
		return;
	}

	const FVector SpotPos = GetWorldLocation();
	FVector dir = GetWorldRotation().RotateVector(FVector(0, 0, 1)).GetSafeNormal();
	const float range = GetRadius();

	if (range <= KINDA_SMALL_NUMBER || dir.SizeSquared() < KINDA_SMALL_NUMBER)
		return;

	const float angleDeg = FMath::Clamp(GetOuterConeAngle(), 0.0f, 89.0f);
	const float angleRad = DegreeToRadian(angleDeg);
	const float circleRadius = std::tan(angleRad) * range;
	const FVector center = SpotPos + dir * range;

	// Build orthonormal basis (u,v) for circle plane (perpendicular to dir)
	FVector arbitraryUp = fabsf(dir.Z) > 0.99f ? FVector(1, 0, 0) : FVector(0, 0, 1);
	FVector u = FVector::Cross(arbitraryUp, dir).GetSafeNormal();
	FVector v = FVector::Cross(dir, u).GetSafeNormal();

	const int segments = FMath::Clamp(GetCircleSegments(), 3, 512);
	const float step = TWO_PI / static_cast<float>(segments);
	const FVector4 color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for spotlight

	FVector prevPoint;
	for (int i = 0; i <= segments; ++i)
	{
		float t = step * i;
		FVector p = center + u * (cosf(t) * circleRadius) + v * (sinf(t) * circleRadius);

		// Draw circle edge
		if (i > 0)
		{
			Renderer->AddLine(prevPoint, p, color);
		}
		prevPoint = p;

		// Draw spotlight-to-circle vertex line
		Renderer->AddLine(SpotPos, p, color);
	}
}

void USpotLightComponent::RenderDetails()
{
	ImGui::Separator();
	ImGui::Text("SpotLight Component Settings");

	// 🔸 색상 설정 (RGB Color Picker)
	float color[3] = { GetTintColor().R, GetTintColor().G, GetTintColor().B};
	if (ImGui::ColorEdit3("Color", color))
	{
		SetTintColor(FLinearColor(color[0], color[1], color[2], 1.0f));
	}

	ImGui::Spacing();
	
	float Temperature = GetColorTemperature();
	if (ImGui::DragFloat("Temperature", &Temperature, 11.029f, 1000.0f, 15000.0f))
	{
		SetColorTemperature(Temperature);
	}

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

	bool bEnableDebugLine = IsEnabledDebugLine();
	if (ImGui::Checkbox("Debug Line", &bEnableDebugLine))
	{
		SetDebugLineEnable(bEnableDebugLine);
	}
	
	ImGui::Spacing();

	// 🔸 시각적 미리보기용 Sphere 표시 (선택된 경우)
	ImGui::Text("Preview:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(color[0], color[1], color[2], 1.0f), "● PointLight Active");
}
