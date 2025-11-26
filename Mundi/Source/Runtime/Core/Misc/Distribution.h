#pragma once
#include "JsonSerializer.h"
#include <vector>
#include <algorithm>

namespace json { class JSON; }
using JSON = json::JSON;

enum class EDistributionMode
{
	// DOP: Distribution operator
	DOP_Constant,
	DOP_Uniform,
	DOP_Curve
};

// 커브의 키프레임 (시간-값 쌍)
struct FCurveKey
{
	float Time = 0.0f;   // 0.0 ~ 1.0 (파티클 생애주기)
	float Value = 0.0f;  // 해당 시간의 값

	// Cubic Bezier 제어점 (Auto 모드에서 자동 계산됨)
	// 상대 좌표: 키프레임 위치 기준 오프셋
	float OutTangentWeight = 0.333f;  // 나가는 제어점 거리 (0~1, 기본 1/3)
	float InTangentWeight = 0.333f;   // 들어오는 제어점 거리 (0~1, 기본 1/3)

	FCurveKey() = default;
	FCurveKey(float InTime, float InValue) : Time(InTime), Value(InValue) {}

	bool operator<(const FCurveKey& Other) const { return Time < Other.Time; }
};

// 커브 전체 (Cubic Bezier 보간)
struct FCurve
{
	std::vector<FCurveKey> Keys;
	bool bUseCubicInterpolation = true; // true: Cubic Bezier, false: Linear

	FCurve()
	{
		// 기본 키프레임: 시작(0.0, 1.0), 끝(1.0, 1.0)
		Keys.push_back(FCurveKey(0.0f, 1.0f));
		Keys.push_back(FCurveKey(1.0f, 1.0f));
	}

	// Auto 탄젠트 계산 (Catmull-Rom 스타일)
	void CalculateAutoTangents()
	{
		// 키가 2개 이하면 탄젠트 계산 불필요
		if (Keys.size() <= 2) return;

		for (size_t i = 0; i < Keys.size(); ++i)
		{
			// 첫 키: 다음 키로만 계산
			if (i == 0)
			{
				// 탄젠트는 다음 키 방향
				// (사실 OutTangentWeight만 사용하므로 큰 의미 없음)
			}
			// 마지막 키: 이전 키로만 계산
			else if (i == Keys.size() - 1)
			{
				// 탄젠트는 이전 키 방향
			}
			// 중간 키: 이전과 다음 키의 평균 기울기
			else
			{
				// 이전-다음 키 사이의 기울기를 자동 계산
				// (현재는 Weight만 사용하므로 생략)
			}
		}
	}

	// 키프레임 추가 (자동 정렬)
	void AddKey(float Time, float Value)
	{
		Keys.push_back(FCurveKey(Time, Value));
		std::sort(Keys.begin(), Keys.end());
	}

	// 키프레임 삭제
	void RemoveKey(int32 Index)
	{
		if (Index >= 0 && Index < (int32)Keys.size())
		{
			Keys.erase(Keys.begin() + Index);
		}
	}

	// 키프레임 수정 (시간 변경 시 재정렬)
	void UpdateKey(int32 Index, float Time, float Value)
	{
		if (Index >= 0 && Index < (int32)Keys.size())
		{
			Keys[Index].Time = Time;
			Keys[Index].Value = Value;
			std::sort(Keys.begin(), Keys.end());
		}
	}

	// 시간에 따른 값 계산 (Cubic Bezier 또는 Linear)
	float Evaluate(float Time) const
	{
		if (Keys.empty()) return 0.0f;
		if (Keys.size() == 1) return Keys[0].Value;

		// Time을 0~1로 클램프
		Time = FMath::Clamp(Time, 0.0f, 1.0f);

		// Time보다 큰 첫 번째 키 찾기
		for (size_t i = 1; i < Keys.size(); ++i)
		{
			if (Time <= Keys[i].Time)
			{
				const FCurveKey& Key0 = Keys[i - 1];
				const FCurveKey& Key1 = Keys[i];

				float Alpha = (Time - Key0.Time) / (Key1.Time - Key0.Time);

				if (bUseCubicInterpolation)
				{
					// Cubic Bezier 보간
					return EvaluateCubicBezier(Key0, Key1, Alpha);
				}
				else
				{
					// Linear 보간
					return FMath::Lerp(Key0.Value, Key1.Value, Alpha);
				}
			}
		}

		// Time이 마지막 키보다 크면 마지막 값 반환
		return Keys.back().Value;
	}

private:
	// Cubic Bezier 보간 함수 (Hermite Spline 방식)
	float EvaluateCubicBezier(const FCurveKey& Key0, const FCurveKey& Key1, float t) const
	{
		// Hermite Spline (Cubic) 방식
		// 간단한 Auto 탄젠트: 0 기울기 (수평)

		float P0 = Key0.Value;
		float P1 = Key1.Value;

		// Auto 모드: 탄젠트를 0으로 (부드러운 S자 곡선)
		// 나중에 인접 키 기반으로 개선 가능
		float m0 = 0.0f;  // 나가는 탄젠트 (수평)
		float m1 = 0.0f;  // 들어오는 탄젠트 (수평)

		// Hermite 기저 함수
		float t2 = t * t;
		float t3 = t2 * t;

		float h00 =  2*t3 - 3*t2 + 1;   // P0 가중치
		float h10 =    t3 - 2*t2 + t;   // m0 가중치
		float h01 = -2*t3 + 3*t2;       // P1 가중치
		float h11 =    t3 -   t2;       // m1 가중치

		return h00 * P0 + h10 * m0 + h01 * P1 + h11 * m1;
	}

public:

	// 직렬화
	void Serialize(const bool bInIsLoading, JSON& InOutHandle)
	{
		if (bInIsLoading)
		{
			Keys.clear();
			auto& keysArray = InOutHandle["keys"];
			for (auto& keyJson : keysArray.ArrayRange())
			{
				float time = static_cast<float>(keyJson["time"].ToFloat());
				float value = static_cast<float>(keyJson["value"].ToFloat());
				Keys.push_back(FCurveKey(time, value));
			}
		}
		else
		{
			JSON keysArray = JSON::Make(JSON::Class::Array);
			for (const auto& key : Keys)
			{
				JSON keyJson;
				keyJson["time"] = key.Time;
				keyJson["value"] = key.Value;
				keysArray.append(keyJson);
			}
			InOutHandle["keys"] = keysArray;
		}
	}
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

	FCurve Curve;

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
			return Curve.Evaluate(Time);
			break;
		default:
			return Constant;
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

			if (InOutHandle.hasKey("curve"))
			{
				Curve.Serialize(bInIsLoading, InOutHandle["curve"]);
			}
		}
		else
		{
			// Save
			InOutHandle["operation"] = static_cast<int>(Operation);
			InOutHandle["constant"] = Constant;
			InOutHandle["min"] = Min;
			InOutHandle["max"] = Max;

			JSON curveJson;
			Curve.Serialize(bInIsLoading, curveJson);
			InOutHandle["curve"] = curveJson;
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

	// 각 축(X, Y, Z)에 대한 개별 커브
	FCurve CurveX;
	FCurve CurveY;
	FCurve CurveZ;

	FRawDistributionVector() {};
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
			return FVector(
				CurveX.Evaluate(Time),
				CurveY.Evaluate(Time),
				CurveZ.Evaluate(Time)
			);
		default:
			return Constant;
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

			if (InOutHandle.hasKey("curveX"))
			{
				CurveX.Serialize(bInIsLoading, InOutHandle["curveX"]);
			}
			if (InOutHandle.hasKey("curveY"))
			{
				CurveY.Serialize(bInIsLoading, InOutHandle["curveY"]);
			}
			if (InOutHandle.hasKey("curveZ"))
			{
				CurveZ.Serialize(bInIsLoading, InOutHandle["curveZ"]);
			}
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

			JSON curveXJson;
			CurveX.Serialize(bInIsLoading, curveXJson);
			InOutHandle["curveX"] = curveXJson;

			JSON curveYJson;
			CurveY.Serialize(bInIsLoading, curveYJson);
			InOutHandle["curveY"] = curveYJson;

			JSON curveZJson;
			CurveZ.Serialize(bInIsLoading, curveZJson);
			InOutHandle["curveZ"] = curveZJson;
		}
	}
};