#pragma once


enum class EDistributionMode
{
	// DOP: Distribution operator
	DOP_Constant,
	DOP_Uniform,
	// TODO: CurveDist
	DOP_Curve
};

inline float Hash(float Seed)
{
	// 시드를 예측 불가능하게 뒤섞음
	float Value = sin(Seed * 12.9898f) * 43758.5453f;

	// 0~1로 정규화(소수점 아래만 씀)
	return Value - std::floor(Value);
}

struct FRawDistributionFloat
{
public:

	EDistributionMode Operation = EDistributionMode::DOP_Constant;

	float Constant = 0.0f;
	float Min = 0.0f;
	float Max = 0.0f;

	//TODO: UCurveFloat* Curve = nullptr;

	FRawDistributionFloat() {};
	FRawDistributionFloat(EDistributionMode InOperation, float InConstant, float InMin, float InMax)
		:Operation(InOperation)
		, Constant(InConstant)
		, Min(InMin)
		, Max(InMax)
	{
	}


	// Rand는 초기화할때만 씀(이후의 update는 크기를 유지해야함)
	// Time도 Curve에서만 씀.
	float GetValue(float Time, float RandomStream = 0.5f)
	{
		switch (Operation)
		{
		case EDistributionMode::DOP_Constant:
			return Constant;
			break;
		case EDistributionMode::DOP_Uniform:
			return FMath::Lerp<float>(Min, Max, RandomStream);
			break;
		case EDistributionMode::DOP_Curve:
		default:
			break;
		}
	}
};

struct FRawDistributionVector
{
public:
	
	EDistributionMode Operation = EDistributionMode::DOP_Constant;

	FVector Constant = FVector::Zero();
	FVector Min = FVector::Zero();
	FVector Max = FVector::Zero();

	FRawDistributionVector(EDistributionMode InOperation, const FVector& InConstant, const FVector& InMin, const FVector& InMax)
		:Operation(InOperation)
		, Constant(InConstant)
		, Min(InMin)
		, Max(InMax)
	{
	}

	FVector GetValue(float Time, float RandomStream = 0.5f)
	{
		switch (Operation)
		{
		case EDistributionMode::DOP_Constant:
			return Constant;
		case EDistributionMode::DOP_Uniform:
			{
				float RandomY = Hash(RandomStream);
				float RandomZ = Hash(RandomY);
				return FVector(
					FMath::Lerp<float>(Min.X, Max.X, RandomStream),
					FMath::Lerp<float>(Min.Y, Max.Y, RandomY),
					FMath::Lerp<float>(Min.Z, Max.Z, RandomZ)
				);
			}
		case EDistributionMode::DOP_Curve:
		default:
			break;
		}
	}
};