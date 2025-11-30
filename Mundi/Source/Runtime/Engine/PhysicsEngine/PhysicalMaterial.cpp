#include "pch.h"
#include "PhysicalMaterial.h"

#include "PhysXSupport.h"

UPhysicalMaterial::UPhysicalMaterial()
{
    Friction                        = 0.7f;
    StaticFriction                  = 0.7f;
    Restitution                     = 0.3f;
    Density                         = 1000.0f;

    FrictionCombineMode             = EFrictionCombineMode::Average;
    RestitutionCombineMode          = EFrictionCombineMode::Average;

    bOverrideFrictionCombineMode    = false;
    bOverrideRestitutionCombineMode = false;

    SleepLinearVelocityThreshold    = -1.0f;
}

UPhysicalMaterial::~UPhysicalMaterial()
{
    if (PxMat)
    {
        PxMat->release();
        PxMat = nullptr;
    }
}

PxMaterial* UPhysicalMaterial::GetPxMaterial()
{
    if (PxMat)
    {
        return PxMat;
    }

    if (GPhysXSDK)
    {
        PxMat = GPhysXSDK->createMaterial(StaticFriction, Friction, Restitution);

        if (PxMat)
        {
            PxMat->userData = this;

            if (bOverrideRestitutionCombineMode)
            {
                PxMat->setFrictionCombineMode((PxCombineMode::Enum)FrictionCombineMode);
            }
            if (bOverrideRestitutionCombineMode)
            {
                PxMat->setRestitutionCombineMode((PxCombineMode::Enum)RestitutionCombineMode);
            }
        }
    }

    return PxMat;
}

void UPhysicalMaterial::UpdatePhysicsMaterial()
{
    if (PxMat)
    {
        PxMat->setStaticFriction(StaticFriction);
        PxMat->setDynamicFriction(Friction);
        PxMat->setRestitution(Restitution);

        if (bOverrideFrictionCombineMode)
        {
            PxMat->setFrictionCombineMode((PxCombineMode::Enum)FrictionCombineMode);
        }
        
        if (bOverrideRestitutionCombineMode)
        {
            PxMat->setRestitutionCombineMode((PxCombineMode::Enum)RestitutionCombineMode);
        }
    }
}
