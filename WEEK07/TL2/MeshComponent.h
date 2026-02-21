#pragma once
#include "PrimitiveComponent.h"
#include "BoundingVolume.h"

class UMeshComponent : public UPrimitiveComponent
{
public:
    DECLARE_ABSTRACT_CLASS(UMeshComponent, UPrimitiveComponent)
    UMeshComponent();

protected:
    ~UMeshComponent() override;


};