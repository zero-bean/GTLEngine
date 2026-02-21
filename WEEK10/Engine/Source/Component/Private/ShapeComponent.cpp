#include "pch.h"
#include "Component/Public/ShapeComponent.h"

IMPLEMENT_ABSTRACT_CLASS(UShapeComponent, UPrimitiveComponent)

UShapeComponent::UShapeComponent()
{
	bCanEverTick = false;  // ShapeComponent doesn't need tick (overlaps managed centrally)
	// ShapeComponent는 충돌 감지용이므로 에디터에서 선택 불가
	bCanPick = false;

	// ShapeComponent는 게임플레이 충돌 검사용이므로 overlap 이벤트 활성화
	bGenerateOverlapEvents = true;

	// Mobility는 기본값(Movable) 사용
}
