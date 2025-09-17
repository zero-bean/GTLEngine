#include "stdafx.h"
#include "UClass.h"
#include "UPrimitiveComponent.h"
#include "UPlaneComp.h"

IMPLEMENT_UCLASS(UPlaneComp, UPrimitiveComponent)
UCLASS_META(UPlaneComp, DisplayName, "Plane")
UCLASS_META(UPlaneComp, MeshName, "Plane")
UCLASS_META(UPlaneComp, MaterialName, "Font");
UCLASS_META(UPlaneComp, BoundsType, "Box")   // 평면은 박스로 처리(두께 0)