#pragma once

class UCamera;
struct  FAABB;

// 평면의 비트 표현
enum class EFrustumPlane : uint32
{
	Left	= 1 << 0, // 0b000001
	Right	= 1 << 1, // 0b000010
	Bottom	= 1 << 2, // 0b000100
	Top		= 1 << 3, // 0b001000
	Near	= 1 << 4, // 0b010000
	Far 	= 1 << 5, // 0b100000

	// 모든 평면 검사용 마스크 값
	All = 0x3F		  // 0b111111
};

enum class EPlaneIndex : uint8
{
	Left,
	Right,
	Bottom,
	Top,
	Near,
	Far
};

enum class EFrustumTestResult : uint8
{
	Outside,
	Inside,
	CompletelyOutside,
	Intersect,
	CompletelyInside
};

struct FPlane
{
	// Ax + By + Cz + D = 0
	// {A, B, C}
	FVector NormalVector;
	// D
	float ConstantD;
	FPlane() : NormalVector(FVector::Zero()), ConstantD(0.0f) {}
	FPlane(const FVector& Normal, const float Distance) : NormalVector(Normal), ConstantD(Distance) {}

	void Normalize()
	{
		float Magnitude = NormalVector.Length();
		if (Magnitude > 1.0e-6f)
		{
			NormalVector *=  (1.0f / Magnitude);
			ConstantD /= Magnitude;
		}
	}
};

struct FFrustum
{
	FPlane FarPlane;
	FPlane NearPlane;
	FPlane LeftPlane;
	FPlane RightPlane;
	FPlane TopPlane;
	FPlane BottomPlane;
};

class FFrustumCull : public UObject
{
	DECLARE_CLASS(FFrustumCull, UObject)
	GENERATED_BODY()

public:
	FFrustumCull();
	~FFrustumCull();

	UObject* Duplicate(FObjectDuplicationParameters Parameters);

	void Update(UCamera* InCamera);
	EFrustumTestResult IsInFrustum(const FAABB& TargetAABB);
	const EFrustumTestResult TestAABBWithPlane(const FAABB& TargetAABB, const EPlaneIndex Index);

	FPlane& GetPlane(EPlaneIndex Index) { return Planes[static_cast<uint8>(Index)]; }

private:
	EFrustumTestResult CheckPlane(const FAABB& TargetAABB,  EPlaneIndex Index);

private:
	// 순서대로 left, right, bottom, top, near, far
	FPlane Planes[6];
};

template<typename E>
constexpr typename std::enable_if_t<std::is_enum_v<E>, E>
operator&(E lhs, E rhs) noexcept {
	// underlying_type_t를 이용해서 enum class의 기저 타입 추출  (uint32, char ...)
	using EnumBaseType = std::underlying_type_t<E>;

	// 열거형을 Basetype으로 캐스팅 후 연산
	// 다시 열거형으로 캐스팅
	return static_cast<E>(static_cast<EnumBaseType>(lhs) & static_cast<EnumBaseType>(rhs));
}

template<typename E>
constexpr typename std::enable_if_t<std::is_enum_v<E>, E>
operator|(E lhs, E rhs) noexcept {
	// underlying_type_t를 이용해서 enum class의 기저 타입 추출  (uint32, char ...)
	using EnumBaseType = std::underlying_type_t<E>;

	return static_cast<E>(static_cast<EnumBaseType>(lhs) | static_cast<EnumBaseType>(rhs));
}

template<typename E>
constexpr typename std::enable_if_t<std::is_enum_v<E>, E&>
operator&=(E& lhs, E rhs) noexcept {
	// underlying_type_t를 이용해서 enum class의 기저 타입 추출  (uint32, char ...)
	using EnumBaseType = std::underlying_type_t<E>;

	// 참조로 캐스팅
	auto& Value = reinterpret_cast<EnumBaseType&>(lhs);
	Value &= static_cast<EnumBaseType>(rhs);
	return lhs;
}

template<typename E>
constexpr typename std::enable_if_t<std::is_enum_v<E>, E&>
operator|=(E& lhs, E rhs) noexcept {
	// underlying_type_t를 이용해서 enum class의 기저 타입 추출  (uint32, char ...)
	using EnumBaseType = std::underlying_type_t<E>;

	// 참조로 캐스팅
	auto& Value = reinterpret_cast<EnumBaseType&>(lhs);
	Value |= static_cast<EnumBaseType>(rhs);
	return lhs;
}

// enum class를 basetype으로 캐스팅하는 함수
template<typename E>
constexpr std::underlying_type_t<E> ToBaseType(E InEnum) noexcept
{
	using BaseType = std::underlying_type_t<E>;
	return static_cast<BaseType>(InEnum);
}
