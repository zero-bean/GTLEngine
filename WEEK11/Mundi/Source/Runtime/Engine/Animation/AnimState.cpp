#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimState.h"

IMPLEMENT_CLASS(UAnimState)

UAnimState::UAnimState()
{
	UE_LOG("%d AnimState Create", UUID);
}

UAnimState::~UAnimState()
{
	UE_LOG("%d AnimState Delete", UUID);
}	
void UAnimState::SetBlnedValue(const float InBlendValue)
{
	BlendValue = InBlendValue;
	TArray<float> SequenceBlendKeys = AnimSequenceMap.GetKeys();
	if (SequenceBlendKeys.Num() > 0)
	{
		float min = SequenceBlendKeys[0];
		float max = SequenceBlendKeys[0];
		for (float BlendValue : SequenceBlendKeys)
		{
			min = BlendValue < min ? BlendValue : min;
			max = BlendValue > max ? BlendValue : max;
		}

		BlendValue = Clamp(BlendValue, min, max);
	}
}
void UAnimState::GetStatePose(UAnimInstance* AnimInstance, FPoseContext& Pose, float CurrentTime)
{
	int AnimSequenceCount = AnimSequenceMap.Num();
	if (AnimSequenceCount == 0)
	{
		return;
	}

	TArray<float> KeyArray = AnimSequenceMap.GetKeys();
	if (AnimSequenceCount == 1)
	{
		Pose.SetPose(AnimSequenceMap[KeyArray[0]], CurrentTime);
	}
	else
	{

		//blendvalue 를 사이에 둔 두 값을 꺼내야함 그런데 사이드 blendvalue보다 바깥일 경우 처리 필요
		int LeftIdx = 0;
		int RightIdx = 1;
		for (int i = 1; i < AnimSequenceCount; i++)
		{
			if (BlendValue < KeyArray[i])
			{
				LeftIdx = i - 1;
				RightIdx = i;
				break;
			}

			//가장 오른쪽 값보다 크거나 같은경우
			if (i == AnimSequenceCount - 1)
			{
				LeftIdx = i - 1;
				RightIdx = i;
			}
		}

		float LeftKey = KeyArray[LeftIdx];
		float RightKey = KeyArray[RightIdx];
		UAnimSequence* LeftSequence = AnimSequenceMap[LeftKey];
		UAnimSequence* RightSequence = AnimSequenceMap[RightKey];
		float CurBlendAlpha = (BlendValue - LeftKey) / (RightKey - LeftKey);
		FPoseContext LeftPose(AnimInstance);
		LeftPose.SetPose(LeftSequence, CurrentTime / TotalSequenceTime * LeftSequence->GetPlayLength());
		FPoseContext RightPose(AnimInstance);
		RightPose.SetPose(RightSequence, CurrentTime / TotalSequenceTime * RightSequence->GetPlayLength());
		FPoseContext::BlendTwoPoses(LeftPose, RightPose, CurBlendAlpha, Pose);
	}
}

TArray<UAnimSequence*> UAnimState::GetCurrentActiveSequence()
{
	TArray<UAnimSequence*> ActiveSequences;
	int AnimSequenceCount = AnimSequenceMap.Num();
	if (AnimSequenceCount == 0)
	{
		return ActiveSequences;
	}

	TArray<float> KeyArray = AnimSequenceMap.GetKeys();
	if (AnimSequenceCount == 1)
	{
		ActiveSequences.Push(AnimSequenceMap[KeyArray[0]]);
	}
	else
	{

		//blendvalue 를 사이에 둔 두 값을 꺼내야함 그런데 사이드 blendvalue보다 바깥일 경우 처리 필요
		int LeftIdx = 0;
		int RightIdx = 1;
		for (int i = 1; i < AnimSequenceCount; i++)
		{
			if (BlendValue < KeyArray[i])
			{
				LeftIdx = i - 1;
				RightIdx = i;
				break;
			}

			//가장 오른쪽 값보다 크거나 같은경우
			if (i == AnimSequenceCount - 1)
			{
				LeftIdx = i - 1;
				RightIdx = i;
			}
		}

		float LeftKey = KeyArray[LeftIdx];
		float RightKey = KeyArray[RightIdx];
		UAnimSequence* LeftSequence = AnimSequenceMap[LeftKey];
		UAnimSequence* RightSequence = AnimSequenceMap[RightKey];
		ActiveSequences.Push(LeftSequence);
		ActiveSequences.Push(RightSequence);
	}
	return ActiveSequences;
}
