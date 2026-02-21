#pragma once

#include <type_traits>

#include "Global/CoreTypes.h"
#include "Global/Vector.h"

struct FArchive
{
	virtual ~FArchive() = default;

	/** @brief 아카이브가 데이터를 불러올 경우 true를 반환한다. */
	virtual bool IsLoading() const = 0;
	virtual void Serialize(void* V, size_t Length) = 0;

	virtual FArchive& operator<<(FName& Name)
	{
		return *this;
	}

	virtual FArchive& operator<<(UObject*& Object)
	{
		return *this;
	}

	/** @brief POD(Plain Old Data) 타입을 아카이브로 직렬화한다. */
	template<typename T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
	FArchive& operator<<(T& Value)
	{
		Serialize(&Value, sizeof(T));
		return *this;
	}

	/** @brief TArray<T> 타입을 아카이브로 직렬화한다. */
	template<typename T>
	FArchive& operator<<(TArray<T>& Value)
	{
		size_t Length = Value.size();
		*this << Length;

		if (IsLoading())
		{
			Value.resize(Length);
		}

		for (T& Element : Value)
		{
			*this << Element;
		}

		return *this;
	}

	/** @brief FString 타입을 아카이브로 직렬화한다. */
	FArchive& operator<<(FString& Value)
	{
		size_t Length = Value.size();
		*this << Length;

		if (!IsLoading())
		{
			Serialize(Value.data(), Length * sizeof(FString::value_type));
		}
		else
		{
			Value.resize(Length);
			Serialize(Value.data(), Length * sizeof(FString::value_type));
		}

		return *this;
	}
};
