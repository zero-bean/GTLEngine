#include "pch.h"
#include "FootStepNotify.h"

IMPLEMENT_CLASS(UFootStepNotify)

void UFootStepNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEvent& Event)
{
    UE_LOG("UFootStepNotify::Notify called. Name=%s, Time=%.3f, Duration=%.3f",
        Event.NotifyName.ToString().c_str(), Event.TriggerTime, Event.Duration);
}
