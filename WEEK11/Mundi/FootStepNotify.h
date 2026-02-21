#pragma once

#include "Source/Runtime/Engine/Animation/Notifies/AnimNotify.h"

class UFootStepNotify : public UAnimNotify
{
    DECLARE_CLASS(UFootStepNotify, UAnimNotify)
public:
    UFootStepNotify() = default;
    virtual ~UFootStepNotify() override = default;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEvent& Event) override;
};
