#include "pch.h"
#include "GizmoRotateComponent.h"

UGizmoRotateComponent::UGizmoRotateComponent()
{
    SetStaticMesh("Data/Gizmo/RotationHandle.obj");
    SetMaterial("Primitive.hlsl");
}

UGizmoRotateComponent::~UGizmoRotateComponent()
{
}
