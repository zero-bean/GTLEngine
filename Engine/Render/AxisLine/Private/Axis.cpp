#include "pch.h"
#include "Render/AxisLine/Public/Axis.h"
#include "Render/AxisLine/Public/AxisLine.h"

AAxis::AAxis()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	LineX = CreateDefaultSubobject<UAxisLineComponent>("LineX");
	LineX->LoadMeshResource(EPrimitiveType::LineR);
	LineX->SetRelativeScale3D({10000.f, 0.f, 0.f});
	LineX->SetColor({1,0,0,1});
	LineX->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	LineY = CreateDefaultSubobject<UAxisLineComponent>("LineY");
	LineY->LoadMeshResource(EPrimitiveType::LineG);
	LineY->SetRelativeScale3D({0.f, 10000.f, 0.f});
	LineY->SetColor({0,1,0,1});
	LineY->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	LineZ = CreateDefaultSubobject<UAxisLineComponent>("LineZ");
	LineZ->LoadMeshResource(EPrimitiveType::LineB);
	LineZ->SetRelativeScale3D({0.f, 0.f, 10000.f});
	LineZ->SetColor({0,0,1,1});
	LineZ->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}
