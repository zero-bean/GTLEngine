#pragma once
#include "JsonSerializer.h"

namespace json { class JSON; }
using JSON = json::JSON;

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

	void Serialize(const bool bInIsLoading, JSON& InOutHandle)
	{
		if (bInIsLoading)
		{
			// Load
			Operation = static_cast<EDistributionMode>(InOutHandle["operation"].ToInt());
			Constant = static_cast<float>(InOutHandle["constant"].ToFloat());
			Min = static_cast<float>(InOutHandle["min"].ToFloat());
			Max = static_cast<float>(InOutHandle["max"].ToFloat());
		}
		else
		{
			// Save
			InOutHandle["operation"] = static_cast<int>(Operation);
			InOutHandle["constant"] = Constant;
			InOutHandle["min"] = Min;
			InOutHandle["max"] = Max;
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

	void Serialize(const bool bInIsLoading, JSON& InOutHandle)
	{
		if (bInIsLoading)
		{
			// Load
			Operation = static_cast<EDistributionMode>(InOutHandle["operation"].ToInt());

			auto& constArr = InOutHandle["constant"];
			Constant = FVector(
				static_cast<float>(constArr[0].ToFloat()),
				static_cast<float>(constArr[1].ToFloat()),
				static_cast<float>(constArr[2].ToFloat())
			);

			auto& minArr = InOutHandle["min"];
			Min = FVector(
				static_cast<float>(minArr[0].ToFloat()),
				static_cast<float>(minArr[1].ToFloat()),
				static_cast<float>(minArr[2].ToFloat())
			);

			auto& maxArr = InOutHandle["max"];
			Max = FVector(
				static_cast<float>(maxArr[0].ToFloat()),
				static_cast<float>(maxArr[1].ToFloat()),
				static_cast<float>(maxArr[2].ToFloat())
			);
		}
		else
		{
			// Save
			InOutHandle["operation"] = static_cast<int>(Operation);

			JSON constArr = JSON::Make(JSON::Class::Array);
			constArr.append(Constant.X);
			constArr.append(Constant.Y);
			constArr.append(Constant.Z);
			InOutHandle["constant"] = constArr;

			JSON minArr = JSON::Make(JSON::Class::Array);
			minArr.append(Min.X);
			minArr.append(Min.Y);
			minArr.append(Min.Z);
			InOutHandle["min"] = minArr;

			JSON maxArr = JSON::Make(JSON::Class::Array);
			maxArr.append(Max.X);
			maxArr.append(Max.Y);
			maxArr.append(Max.Z);
			InOutHandle["max"] = maxArr;
		}
	}
};