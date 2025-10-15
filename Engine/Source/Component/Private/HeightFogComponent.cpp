#include "pch.h"
#include "Component/Public/HeightFogComponent.h"

IMPLEMENT_CLASS(UHeightFogComponent, UPrimitiveComponent)

UHeightFogComponent::UHeightFogComponent()
{
	Type = EPrimitiveType::HeightFog;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

