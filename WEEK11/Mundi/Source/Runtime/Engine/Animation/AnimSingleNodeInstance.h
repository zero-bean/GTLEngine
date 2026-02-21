#pragma once
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "UAnimSingleNodeInstance.generated.h"

class UAnimSingleNodeInstance : public UAnimInstance
{
	GENERATED_REFLECTION_BODY()
public:
	UAnimSingleNodeInstance() = default;
	virtual ~UAnimSingleNodeInstance() = default;
	void Tick(float DeltaSeconds) override;
	void SetAnimSequence(UAnimSequence* InAnimSequence, const bool InLoop = false);
	void SetSpeed(const float InSpeed)
	{
		Speed = InSpeed;
	}
protected:
	void NativeUpdateAnimation(float DeltaSeconds) override;
	UAnimSequence* AnimSequence = nullptr;
private:
	bool bLoop = false;
	float Speed = 0.0f;
};