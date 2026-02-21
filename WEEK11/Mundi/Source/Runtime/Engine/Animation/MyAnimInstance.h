#pragma once
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "UMyAnimInstance.generated.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"

class UMyAnimInstance : public UAnimInstance
{
	GENERATED_REFLECTION_BODY()

public:
	UMyAnimInstance() = default;
	virtual ~UMyAnimInstance() = default;
	void SetBlendAnimation(UAnimSequence* SequenceA, UAnimSequence* SequenceB);
	void SetBlendAlpha(const float InBlendAlpha) { BlendAlpha = InBlendAlpha; }

public:

	UPROPERTY(EditAnywhere, Category = "MyAnimInstance")
	UAnimSequence* AnimA = nullptr;

	UPROPERTY(EditAnywhere, Category = "MyAnimInstance")
	UAnimSequence* AnimB = nullptr;

	UPROPERTY(EditAnywhere, Category = "MyAnimInstance", Range = "0.0, 1.0")
	float BlendAlpha = 0.0f;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

};