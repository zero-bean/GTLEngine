#include "pch.h"
#include "GizmoArrowComponent.h"

UGizmoArrowComponent::UGizmoArrowComponent()
{
    SetStaticMesh("Data/Gizmo/Arrow.obj");
    SetMaterial("Primitive.hlsl");
}

UGizmoArrowComponent::~UGizmoArrowComponent()
{

}
