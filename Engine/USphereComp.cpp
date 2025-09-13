#include "stdafx.h"
#include "UClass.h"
#include "USphereComp.h"
#include "UPrimitiveComponent.h"

IMPLEMENT_UCLASS(USphereComp, UPrimitiveComponent)
UCLASS_META(USphereComp, DisplayName, "Sphere")
UCLASS_META(USphereComp, MeshName, "Sphere")

// AABB 판단용 메타
UCLASS_META(USphereComp, BoundsType, "Sphere")
// sphere 버텍스의 정보에 따라서 반지름 길이 설정
UCLASS_META(USphereComp, BoundsRadius, "0.5")