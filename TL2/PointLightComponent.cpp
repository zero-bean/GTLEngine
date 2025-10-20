#include"pch.h"
#include "PointLightComponent.h"
#include "Renderer.h"
#include "World.h"
#include"SceneLoader.h"
#include "ImGui/imgui.h"


UPointLightComponent::UPointLightComponent()
{
    // 초기 라이트 데이터 세팅
    FVector WorldPos = GetWorldLocation();
    //PointLightBuffer.Position = FVector4(WorldPos, PointData.Radius);
    //PointLightBuffer.Color = FVector4(PointData.Color.R, PointData.Color.G, PointData.Color.B, PointData.Intensity);
    //PointLightBuffer.FallOff = PointData.RadiusFallOff;

    bCanEverTick = true;
}

UPointLightComponent::~UPointLightComponent()
{
}

void UPointLightComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // Call parent class serialization for transforms
    ULightComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        Radius = InOut.PointLightProperty.Radius;
        RadiusFallOff = InOut.PointLightProperty.RadiusFallOff;
    }
    else
    {
        InOut.PointLightProperty.Radius = Radius;
        InOut.PointLightProperty.RadiusFallOff = RadiusFallOff;
    }
}


void UPointLightComponent::TickComponent(float DeltaSeconds)
{
    static int Time;
    Time += DeltaSeconds;
   // Radius = 300.0f + sinf(Time) * 100.0f;

    //// 🔹 GPU 업로드용 버퍼 갱신
    //FVector WorldPos = GetWorldLocation();
    //PointLightBuffer.Position = FVector4(WorldPos, PointData.Radius);
    //PointLightBuffer.Color = FVector4(PointData.Color.R, PointData.Color.G, PointData.Color.B, PointData.Intensity);
    //PointLightBuffer.FallOff = PointData.RadiusFallOff;
}

//const FAABB UPointLightComponent::GetWorldAABB() const
//{
//    // PointLight의 Radius를 기반으로 AABB 생성
//    FVector WorldLocation = GetWorldLocation();
//    FVector Extent(PointData.Radius, PointData.Radius, PointData.Radius);
//
//    return FAABB(WorldLocation - Extent, WorldLocation + Extent);
//}

UObject* UPointLightComponent::Duplicate()
{
    UPointLightComponent* DuplicatedComponent = NewObject<UPointLightComponent>();
    CopyCommonProperties(DuplicatedComponent);
     DuplicatedComponent->Radius =this->Radius; // 복제 (단, UObject 포인터 복사는 주의)
    DuplicatedComponent->RadiusFallOff = this->RadiusFallOff; // 복제 (단, UObject 포인터 복사는 주의)
    DuplicatedComponent->DuplicateSubObjects();
    return DuplicatedComponent;
}

void UPointLightComponent::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();
}

void UPointLightComponent::RenderDetails()
{
	ImGui::Separator();
	ImGui::Text("PointLight Component Settings");

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

	// 🔸 밝기 (Intensity)
	float intensity = GetIntensity();
	if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
	{
		SetIntensity(intensity);
	}

	// 🔸 반경 (Radius)
	float radius = GetRadius();
	if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 1000.0f))
	{
		SetRadius(radius);
	}

	// 🔸 감쇠 정도 (FallOff)
	float falloff = GetRadiusFallOff();
	if (ImGui::DragFloat("FallOff", &falloff, 0.05f, 0.1f, 10.0f))
	{
		SetRadiusFallOff(falloff);
	}

	ImGui::Spacing();

	// 🔸 시각적 미리보기용 Sphere 표시 (선택된 경우)
	ImGui::Text("Preview:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(color[0], color[1], color[2], 1.0f), "● PointLight Active");
}

void UPointLightComponent::DrawDebugLines(URenderer* Renderer)
{
	if (!this->IsRender() || !Renderer)
	{
		return;
	}

	if (KINDA_SMALL_NUMBER > Radius)
	{
		return;
	}

	// 중심 = 현재 배치된 월드 포지션
	FVector Center = GetWorldLocation();
	uint32 Segments = 24;

	float Step = TWO_PI / Segments;

	FVector4 Color(1.0f, 1.0f, 0.5f,1.0f);

	FVector PrevPointXY = Center + FVector(0.0f, Radius, 0.0f);
	FVector PrevPointXZ = Center + FVector(Radius, 0.0f, 0.0f);
	FVector PrevPointYZ = Center + FVector(0.0f, Radius, 0.0f);
	for (uint32 i = 0; i< Segments; i++)
	{		
		float Angle = Step * (i + 1);
		FVector NextPointXY = Center + FVector(Radius * sinf(Angle), Radius * cosf(Angle), 0.0f);
		FVector NextPointXZ = Center + FVector(Radius * cosf(Angle), 0.0f, Radius * sinf(Angle));
		FVector NextPointYZ = Center + FVector(0.0f, Radius * cosf(Angle), Radius * sinf(Angle));
		Renderer->AddLine(PrevPointXY, NextPointXY, Color);
		Renderer->AddLine(PrevPointXZ, NextPointXZ, Color);
		Renderer->AddLine(PrevPointYZ, NextPointYZ, Color);
		PrevPointXY = NextPointXY;
		PrevPointXZ = NextPointXZ;
		PrevPointYZ = NextPointYZ;
	}	
}



