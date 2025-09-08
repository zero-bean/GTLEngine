#include "pch.h"
#include "Render/AxisLine/Public/Axis.h"
#include "Render/AxisLine/Public/AxisLine.h"

AAxis::AAxis()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	LineX = CreateDefaultSubobject<UAxisLineComponent>("LineX");
	LineX->SetRelativeScale3D({1.f, 1.f, 50000.f});
	LineX->SetRelativeRotation({0.f, 89.99f, 0.f});
	LineX->SetColor({1,0,0,1});
	LineX->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	LineY = CreateDefaultSubobject<UAxisLineComponent>("LineY");
	LineY->SetRelativeScale3D({1.f, 1.f, 50000.f});
	LineY->SetRelativeRotation({-89.99f, 0.f, 0.f});
	LineY->SetColor({0,1,0,1});
	LineY->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	LineZ = CreateDefaultSubobject<UAxisLineComponent>("LineZ");
	LineZ->SetRelativeScale3D({1.f, 1.f, 50000.f});
	LineZ->SetRelativeRotation({0.f, 0.f, 0.f});
	LineZ->SetColor({0,0,1,1});
	LineZ->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}
