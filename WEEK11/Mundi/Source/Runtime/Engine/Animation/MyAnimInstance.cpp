#include "pch.h"
#include "Source/Runtime/Engine/Animation/MyAnimInstance.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

void UMyAnimInstance::SetBlendAnimation(UAnimSequence* SequenceA, UAnimSequence* SequenceB)
{
	AnimA = SequenceA;
	AnimB = SequenceB;
}

void UMyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if (AnimA && AnimB)
	{
		float ASequenceTime = AnimA->GetSequenceLength();
		float BSequenceTime = AnimB->GetSequenceLength();
		bool AMax = ASequenceTime > BSequenceTime;
		float MaxSequenceTime = fmax(ASequenceTime, BSequenceTime);

		if (AnimA->IsLooping() || AnimB->IsLooping())
		{
			CurrentTime = ClampTimeLooped(CurrentTime, CurrentTime - PrevTime, MaxSequenceTime);
		}
		else
		{
			CurrentTime = Clamp(CurrentTime, 0.0f, MaxSequenceTime);
			if (CurrentTime != MaxSequenceTime)
			{
				bPlay = false;
			}
		}

		float CurrentTimeA = AMax ? CurrentTime : CurrentTime / BSequenceTime * ASequenceTime;
		float CurrentTimeB = AMax ? CurrentTime / ASequenceTime * BSequenceTime : CurrentTime;

		FPoseContext PoseA(this);
		FPoseContext PoseB(this);
		PoseA.SetPose(AnimA, CurrentTimeA);
		PoseB.SetPose(AnimB, CurrentTimeB);
		FPoseContext::BlendTwoPoses(PoseA, PoseB, BlendAlpha, PoseA);
		OwnerComponent->SetPose(PoseA);


	}
}