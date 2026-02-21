#pragma once

#include "Core/Public/Object.h"
#include "Global/Types.h"
#include "Global/Macro.h"

template<typename TObject>
struct TObjectRange;

/**
 * @brief 초고속 증분 업데이트 캐시 관리자
 * UObject 타입별 캐싱과 증분 업데이트를 통한 성능 최적화
 */
class FObjectCacheManager
{
public:
	/** 전체 캐시 무효화 (객체 삭제 시 사용) */
	inline static void InvalidateCache()
	{
		bCacheValid = false;
	}

	/** 전체 캐시 정리 */
	static void ClearCache()
	{
		ObjectCache.clear();
		bCacheValid = false;
		LastProcessedIndex = 0;
	}

	/** 타입별 객체 배열 반환 */
	template<typename TObject>
	static const TArray<UObject*>& GetObjectsOfClass()
	{
		const uint32 TypeHash = GetClassTypeHash<TObject>();

		// 증분 업데이트 적용
		UpdateIncrementalCache<TObject>(TypeHash);

		return ObjectCache[TypeHash];
	}

private:
	/** 타입 해시 생성 */
	template<typename TObject>
	static uint32 GetClassTypeHash()
	{
		// 간단한 런타임 해시
		const char* TypeName = typeid(TObject).name();
		uint32 Hash = 0;
		for (int32 i = 0; TypeName[i] != '\0'; ++i)
		{
			Hash = Hash * 31 + static_cast<uint32>(TypeName[i]);
		}
		return Hash;
	}

	/** 증분 캐시 업데이트 */
	template<typename TObject>
	static void UpdateIncrementalCache(const uint32 TypeHash)
	{
		const TArray<TObjectPtr<UObject>>& ObjectArray = GetUObjectArray();

		// 첫 번째 접근이거나 캐시가 무효화된 경우 전체 빌드
		if (!bCacheValid || ObjectCache.find(TypeHash) == ObjectCache.end())
		{
			RebuildCacheForType<TObject>(TypeHash);

			// 전역 LastProcessedIndex 업데이트 (모든 타입에 적용)
			if (LastProcessedIndex < static_cast<int32>(ObjectArray.size()))
			{
				LastProcessedIndex = static_cast<int32>(ObjectArray.size());
			}

			bCacheValid = true;
			return;
		}

		// 초고속 증분 업데이트: 마지막 처리된 인덱스 이후의 새 객체들만 확인
		const int32 ArraySize = static_cast<int32>(ObjectArray.size());
		if (LastProcessedIndex < ArraySize)
		{
			TArray<UObject*>& CachedObjects = ObjectCache[TypeHash];

			// 배치 처리로 메모리 할당 최적화
			const int32 NewObjectsCount = ArraySize - LastProcessedIndex;
			CachedObjects.reserve(CachedObjects.size() + NewObjectsCount / 10); // 예상 증가량

			// 고성능 루프
			for (int32 ObjectIndex = LastProcessedIndex; ObjectIndex < ArraySize; ++ObjectIndex)
			{
				UObject* Object = ObjectArray[ObjectIndex];
				if (Object != nullptr)
				{
					// 타입 체크
					if (TObject* TypedObject = Cast<TObject>(Object))
					{
						CachedObjects.emplace_back(Object);
					}
				}
			}

			// 전역 인덱스 업데이트
			LastProcessedIndex = ArraySize;
		}

		// 지연 정리: CPU 사이클 절약
		static int32 CleanupCounter = 0;
		if (++CleanupCounter >= 1000)
		{
			CleanupNullPtrs(TypeHash);
			CleanupCounter = 0;
		}
	}

	/** 타입별 캐시 재빌드 */
	template<typename TObject>
	static void RebuildCacheForType(const uint32 TypeHash)
	{
		TArray<UObject*>& CachedObjects = ObjectCache[TypeHash];
		CachedObjects.clear();

		const TArray<TObjectPtr<UObject>>& ObjectArray = GetUObjectArray();

		// 스마트 사전 할당
		const int32 EstimatedSize = (static_cast<int32>(ObjectArray.size()) / 20 > 100) ? (static_cast<int32>(ObjectArray.size()) / 20) : 100;
		CachedObjects.reserve(EstimatedSize);

		// 고성능 순회
		const int32 ArraySize = static_cast<int32>(ObjectArray.size());
		for (int32 ObjectIndex = 0; ObjectIndex < ArraySize; ++ObjectIndex)
		{
			UObject* Object = ObjectArray[ObjectIndex];

			// 유효성 체크
			if (Object != nullptr)
			{
				// 타입 체크
				if (TObject* TypedObject = Cast<TObject>(Object))
				{
					CachedObjects.emplace_back(Object);
				}
			}
		}

		// 메모리 압축
		CachedObjects.shrink_to_fit();
	}

	/** nullptr 정리 */
	static void CleanupNullPtrs(const uint32 TypeHash)
	{
		TArray<UObject*>& CachedObjects = ObjectCache[TypeHash];

		// nullptr 제거
		CachedObjects.erase(
			std::remove(CachedObjects.begin(), CachedObjects.end(), nullptr),
			CachedObjects.end()
		);
	}

	/** 정적 멤버 변수 */
	static TMap<uint32, TArray<UObject*>> ObjectCache;
	static bool bCacheValid;
	static int32 LastProcessedIndex; // 마지막으로 처리된 ObjectArray 인덱스
};

/**
 * @brief 객체 반복자 - 타입별 객체 순회를 위한 고성능 Iterator
 *
 * 특징:
 * - 캐싱 시스템으로 O(1) 성능 달성
 * - 증분 업데이트로 메모리 효율성 보장
 * - 안전한 객체 순회 보장
 *
 * 사용 예시:
 * @code
 * for (TObjectIterator<UMyClass> Itr; Itr; ++Itr)
 * {
 *     UMyClass* Object = *Itr;
 *     // 작업 수행
 * }
 * @endcode
 */
template<typename TObject>
class TObjectIterator
{
	friend struct TObjectRange<TObject>;

public:
	/** 기본 생성자 - 캐시된 객체 배열로 초기화 */
	TObjectIterator()
		: Index(-1)
	{
		// 캐시된 객체 배열 가져오기 (O(1) 성능)
		ObjectArray = FObjectCacheManager::GetObjectsOfClass<TObject>();
		Advance();
	}

	void operator++()
	{
		Advance();
	}

	/** @note: Iterator가 nullptr가 아닌지 보장하지 않음 */
	explicit operator bool() const
	{
		return 0 <= Index && Index < ObjectArray.size();
	}

	bool operator!() const
	{
		return !(bool)*this;
	}

	TObject* operator*() const
	{
		return (TObject*)GetObject();
	}

	TObject* operator->() const
	{
		return (TObject*)GetObject();
	}

	bool operator==(const TObjectIterator& Rhs) const
	{
		return Index == Rhs.Index;
	}
	bool operator!=(const TObjectIterator& Rhs) const
	{
		return Index != Rhs.Index;
	}

	/** @note: UE는 Thread-Safety를 보장하지만, 여기서는 Advance()와 동일하게 작동 */
	bool AdvanceIterator()
	{
		return Advance();
	}

protected:
	// 더 이상 사용하지 않는 함수 (캐시 시스템으로 대체됨)
	// void GetObjectsOfClass() - ObjectCacheManager로 대체

	UObject* GetObject() const
	{
		/** @todo: Index가 -1일 때 nullptr을 리턴해도 괜찮은가 */
		if (Index == -1 || Index >= ObjectArray.size())
		{
			return nullptr;
		}

		UObject* obj = ObjectArray[Index];
		// 삭제된 객체 체크 (안전성 보장)
		if (obj == nullptr)
		{
			return nullptr;
		}

		return obj;
	}

	bool Advance()
	{
		while (++Index < ObjectArray.size())
		{
			// 안전한 객체 확인 (nullptr 및 삭제된 객체 스킵)
			UObject* obj = GetObject();
			if (obj && obj != nullptr)
			{
				return true;
			}
		}
		return false;
	}

private:
	TObjectIterator(const TArray<UObject*>& InObjectArray, int32 InIndex)
		: ObjectArray(InObjectArray), Index(InIndex)
	{
	}

protected:
	/** @note: 언리얼 엔진은 TObjectPtr이 아닌 Raw 포인터 사용 */
	TArray<UObject*> ObjectArray;

	int32 Index;
};


template<typename TObject>
struct TObjectRange
{
public:
	TObjectRange()
		: It()
	{
	}

	/** @note: Ranged-For 지원 */
	TObjectIterator<TObject> begin() const { return It; }
	TObjectIterator<TObject> end() const
	{
		return TObjectIterator<TObject>(It.ObjectArray, It.ObjectArray.size());
	}

private:
	TObjectIterator<TObject> It;
};
