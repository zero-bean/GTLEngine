#include "stdafx.h"
#include "UClass.h"
#include "UPrimitiveComponent.h"
#include "UCubeComp.h"

IMPLEMENT_UCLASS(UCubeComp, UPrimitiveComponent)
UCLASS_META(UCubeComp, DisplayName, "Cube")
UCLASS_META(UCubeComp, MeshName, "Cube")
UCLASS_META(UCubeComp, BoundsType, "Box")
UCLASS_META(UCubeComp, MaterialName, "Default")