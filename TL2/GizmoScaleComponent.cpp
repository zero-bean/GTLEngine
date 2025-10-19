#include "pch.h"
#include "GizmoScaleComponent.h"

UGizmoScaleComponent::UGizmoScaleComponent()
{
    SetStaticMesh("Data/Gizmo/ScaleHandle.obj");
    SetMaterial("Primitive.hlsl");
}

UGizmoScaleComponent::~UGizmoScaleComponent()
{
}
