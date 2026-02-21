#include "pch.h"
#include "Component/Public/LightComponent.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>
#include <filesystem>

IMPLEMENT_CLASS(ULightComponent, UPrimitiveComponent)
ULightComponent::ULightComponent()
{}

void ULightComponent::UpdateLightColor(FVector4 InColor)
{
	LightColor = InColor;
}

UObject* ULightComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<ULightComponent*>(Super::Duplicate(Parameters));
	DupObject->Brightness = Brightness;
	DupObject->LightColor = LightColor;

	return DupObject;
}

void ULightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// --- 불러오기 ---
	if (bInIsLoading)
	{
		FJsonSerializer::ReadVector4(InOutHandle, "Brightness", Brightness, FVector4::ZeroVector());
		FJsonSerializer::ReadVector4(InOutHandle, "LightColor", LightColor, FVector4::ZeroVector());

		UpdateBrightness();
		UpdateLightColor(LightColor);
	}
	// --- 저장하기 ---
	else
	{
		InOutHandle["Brightness"] = FJsonSerializer::Vector4ToJson(Brightness);
		InOutHandle["LightColor"] = FJsonSerializer::Vector4ToJson(LightColor);
	}
}
