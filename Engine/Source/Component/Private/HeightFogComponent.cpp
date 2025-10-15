#include "pch.h"
#include "Component/Public/HeightFogComponent.h"
#include "Utility/Public/JsonSerializer.h"

#include <algorithm>

IMPLEMENT_CLASS(UHeightFogComponent, UPrimitiveComponent)

UHeightFogComponent::UHeightFogComponent()
{
	ComponentType = EComponentType::Scene;
	Type = EPrimitiveType::HeightFog;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void UHeightFogComponent::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
}

void UHeightFogComponent::SetFogDensity(float InDensity)
{
	FogDensity = std::max(InDensity, 0.0f);
}

void UHeightFogComponent::SetFogHeightFalloff(float InFalloff)
{
	FogHeightFalloff = std::max(InFalloff, 0.0f);
}

void UHeightFogComponent::SetStartDistance(float InDistance)
{
	StartDistance = std::max(InDistance, 0.0f);
}

void UHeightFogComponent::SetFogCutoffDistance(float InDistance)
{
	FogCutoffDistance = std::max(InDistance, 0.0f);
}

void UHeightFogComponent::SetFogMaxOpacity(float InOpacity)
{
	FogMaxOpacity = std::clamp(InOpacity, 0.0f, 1.0f);
}

void UHeightFogComponent::SetFogHeight(float InHeight)
{
	FogHeight = InHeight;
}

void UHeightFogComponent::SetFogInscatteringColor(const FVector4& InColor)
{
	FogInscatteringColor.X = std::clamp(InColor.X, 0.0f, 255.0f);
	FogInscatteringColor.Y = std::clamp(InColor.Y, 0.0f, 255.0f);
	FogInscatteringColor.Z = std::clamp(InColor.Z, 0.0f, 255.0f);
	FogInscatteringColor.W = std::clamp(InColor.W, 0.0f, 1.0f);
}

FHeightFogConstants UHeightFogComponent::BuildFogConstants() const
{
	float FogColor[4] =
	{
		FogInscatteringColor.X,
		FogInscatteringColor.Y,
		FogInscatteringColor.Z,
		FogInscatteringColor.W
	};

	return FHeightFogConstants(
		FogDensity,
		FogHeightFalloff,
		StartDistance,
		FogCutoffDistance,
		FogMaxOpacity,
		FogHeight,
		FogColor
	);
}

UObject* UHeightFogComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UHeightFogComponent*>(Super::Duplicate(Parameters));

	DupObject->bEnabled = bEnabled;
	DupObject->FogDensity = FogDensity;
	DupObject->FogHeightFalloff = FogHeightFalloff;
	DupObject->StartDistance = StartDistance;
	DupObject->FogCutoffDistance = FogCutoffDistance;
	DupObject->FogMaxOpacity = FogMaxOpacity;
	DupObject->FogHeight = FogHeight;
	DupObject->FogInscatteringColor = FogInscatteringColor;

	return DupObject;
}

void UHeightFogComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "FogDensity", FogDensity, FogDensity, false);
		FJsonSerializer::ReadFloat(InOutHandle, "FogHeightFalloff", FogHeightFalloff, FogHeightFalloff, false);
		FJsonSerializer::ReadFloat(InOutHandle, "StartDistance", StartDistance, StartDistance, false);
		FJsonSerializer::ReadFloat(InOutHandle, "FogCutoffDistance", FogCutoffDistance, FogCutoffDistance, false);
		FJsonSerializer::ReadFloat(InOutHandle, "FogMaxOpacity", FogMaxOpacity, FogMaxOpacity, false);
		FJsonSerializer::ReadFloat(InOutHandle, "FogHeight", FogHeight, FogHeight, false);
		FJsonSerializer::ReadVector4(InOutHandle, "FogInscatteringColor", FogInscatteringColor, FogInscatteringColor, false);

		if (InOutHandle.hasKey("bEnabled"))
		{
			try
			{
				bEnabled = InOutHandle.at("bEnabled").ToBool();
			}
			catch (const std::exception&)
			{
				// Ignore malformed data and keep previous value
			}
		}
	}
	else
	{
		InOutHandle["FogDensity"] = FogDensity;
		InOutHandle["FogHeightFalloff"] = FogHeightFalloff;
		InOutHandle["StartDistance"] = StartDistance;
		InOutHandle["FogCutoffDistance"] = FogCutoffDistance;
		InOutHandle["FogMaxOpacity"] = FogMaxOpacity;
		InOutHandle["FogHeight"] = FogHeight;
		InOutHandle["FogInscatteringColor"] = FJsonSerializer::Vector4ToJson(FogInscatteringColor);
		InOutHandle["bEnabled"] = bEnabled;
	}
}
