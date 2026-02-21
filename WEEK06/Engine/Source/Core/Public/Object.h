#pragma once
#include "Class.h"
#include "Name.h"
#include "ObjectPtr.h"

namespace json { class JSON; }
using JSON = json::JSON;

struct FObjectDuplicationParameters;

/** StaticDuplicateObject()와 관련 함수에서 사용되는 Enum */
namespace EDuplicateMode
{
	enum Type
	{
		/** 복제에 구체적인 정보가 없는경우 */
		Normal,
		/** 월드 복제의 일부로 오브젝트가 복제되는 경우 */
		World,
		/** PIE(Play In Editor)를 위해 오브젝트가 복사되는 경우 */
		PIE
	};
}

UCLASS()
class UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UObject, UObject)

public:
	// 생성자 및 소멸자
	UObject();
	explicit UObject(const FName& InName);
	virtual ~UObject();

	// 2. 가상 함수 (인터페이스)
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle);

	/** @brief UObject 계층을 타고 재귀적으로 UObject에서 상속 받는 클래스를 복제한다. */
	virtual UObject* Duplicate(FObjectDuplicationParameters Parameters);

	/** @deprecated 아무런 작업을 수행하지 않는다. 발제 내용과의 호환을 위해 남겨둠. */
	[[deprecated]] virtual void DuplicateSubObjects(FObjectDuplicationParameters Parameters);

	virtual void PreDuplicate(FObjectDuplicationParameters& DupParams) {}

	virtual void PostDuplicate(bool bDuplicateForPIE) {}
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode)
	{
		PostDuplicate(DuplicateMode == EDuplicateMode::PIE);
	}

	// 3. Public 멤버 함수
	bool IsA(TObjectPtr<UClass> InClass) const;
	void AddMemoryUsage(uint64 InBytes, uint32 InCount);
	void RemoveMemoryUsage(uint64 InBytes, uint32 InCount);

	// Getter & Setter
	const FName& GetName() const { return Name; }
	UObject* GetOuter() const { return Outer.Get(); }
	uint64 GetAllocatedBytes() const { return AllocatedBytes; }
	uint32 GetAllocatedCount() const { return AllocatedCounts; }
	uint32 GetUUID() const { return UUID; }

	void SetName(const FName& InName) { Name = InName; }
	void SetOuter(UObject* InObject);
	void SetDisplayName(const FString& InName) const { Name.SetDisplayName(InName); }

protected:
	/**
	 * @brief Duplicate 과정에서 복제 오브젝트의 이름 생성용 함수
	 */
	std::string GenerateDuplicatedObjectName(const FName& OriginalName, const uint32& NewUUID);

private:
	void PropagateMemoryChange(uint64 InBytesDelta, uint32 InCountDelta);

private:
	uint32 UUID;
	uint32 InternalIndex;
	FName Name;
	TObjectPtr<UObject> Outer;
	uint64 AllocatedBytes = 0;
	uint32 AllocatedCounts = 0;
};


/**
 * @brief 안전한 타입 캐스팅 함수 (원시 포인터용)
 * UClass::IsChildOf를 사용한 런타임 타입 체크
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 원시 포인터
 * @return 캐스팅 성공시 T*, 실패시 nullptr
 */
template <typename T>
T* Cast(UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return nullptr;
	}

	// 런타임 타입 체크: InObject가 T 타입인가?
	if (InObject->IsA(T::StaticClass()))
	{
		return static_cast<T*>(InObject);
	}

	return nullptr;
}

/**
 * @brief 안전한 타입 캐스팅 함수 (const 원시 포인터용)
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 const 원시 포인터
 * @return 캐스팅 성공시 const T*, 실패시 nullptr
 */
template <typename T>
const T* Cast(const UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return nullptr;
	}

	// 런타임 타입 체크: InObject가 T 타입인가?
	if (InObject->IsA(T::StaticClass()))
	{
		return static_cast<const T*>(InObject);
	}

	return nullptr;
}

/**
 * @brief 안전한 타입 캐스팅 함수 (TObjectPtr용)
 * @tparam T 캐스팅할 대상 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 캐스팅할 TObjectPtr
 * @return 캐스팅 성공시 TObjectPtr<T>, 실패시 nullptr을 담은 TObjectPtr<T>
 */
template <typename T, typename U>
TObjectPtr<T> Cast(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "Cast<T>: U는 UObject를 상속받아야 합니다");

	U* RawPtr = InObjectPtr.Get();
	T* CastedPtr = Cast<T>(RawPtr);

	return TObjectPtr<T>(CastedPtr);
}

/**
 * @brief 빠른 캐스팅 함수 (런타임 체크 없음, 디버그에서만 체크)
 * 성능이 중요한 상황에서 사용, 타입 안전성을 개발자가 보장해야 함
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 원시 포인터
 * @return T* (런타임 체크 없이 static_cast)
 */
template <typename T>
T* CastChecked(UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "CastChecked<T>: T는 UObject를 상속받아야 합니다");

#ifdef _DEBUG
	// 디버그 빌드에서는 안전성 체크
	if (InObject && !InObject->IsA(T::StaticClass()))
	{
		// 로그나 assert 추가 가능
		return nullptr;
	}
#endif

	return static_cast<T*>(InObject);
}

/**
 * @brief 빠른 캐스팅 함수 (TObjectPtr용)
 * @tparam T 캐스팅할 대상 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 캐스팅할 TObjectPtr
 * @return TObjectPtr<T>
 */
template <typename T, typename U>
TObjectPtr<T> CastChecked(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "CastChecked<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "CastChecked<T>: U는 UObject를 상속받아야 합니다");

	U* RawPtr = InObjectPtr.Get();
	T* CastedPtr = CastChecked<T>(RawPtr);

	return TObjectPtr<T>(CastedPtr);
}

/**
 * @brief 타입 체크 함수 (캐스팅하지 않고 체크만)
 * @tparam T 체크할 타입
 * @param InObject 체크할 객체
 * @return InObject가 T 타입이면 true
 */
template <typename T>
bool IsA(const UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "IsA<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return false;
	}

	return InObject->IsA(T::StaticClass());
}

/**
 * @brief 타입 체크 함수 (TObjectPtr용)
 * @tparam T 체크할 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 체크할 TObjectPtr
 * @return InObjectPtr가 T 타입이면 true
 */
template <typename T, typename U>
bool IsA(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "IsA<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "IsA<T>: U는 UObject를 상속받아야 합니다");

	return IsA<T>(InObjectPtr.Get());
}

/**
 * @brief 유효성과 타입을 동시에 체크하는 함수
 * @tparam T 체크할 타입
 * @param InObject 체크할 객체
 * @return 객체가 유효하고 T 타입이면 true
 */
template <typename T>
bool IsValid(const UObject* InObject)
{
	return InObject && IsA<T>(InObject);
}

/**
 * @brief 유효성과 타입을 동시에 체크하는 함수 (TObjectPtr용)
 * @tparam T 체크할 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 체크할 TObjectPtr
 * @return 객체가 유효하고 T 타입이면 true
 */
template <typename T, typename U>
bool IsValid(const TObjectPtr<U>& InObjectPtr)
{
	return InObjectPtr && IsA<T>(InObjectPtr);
}

TArray<TObjectPtr<UObject>>& GetUObjectArray();

/*-----------------------------------------------------------------------------
	UObject 헬퍼 함수(UObjectGlobals.h 참고)
-----------------------------------------------------------------------------*/


struct FObjectDuplicationParameters
{
	/** 복사될 오브젝트 */
	UObject*	SourceObject;

	/** 복사될 오브젝트의 소유자로 사용될 오브젝트 */
	UObject*	DestOuter;

	/** SourceObject의 복제에 사용될 이름 */
	FName		DestName;

	/** 복제될 클래스의 타입 */
	UClass*		DestClass;

	EDuplicateMode::Type DuplicateMode;

	/**
	 * StaticDuplicateObject에서 사용하는 복제 매핑 테이블을 미리 채워넣기 위한 용도.
	 * 특정 오브젝트를 복제하지 않고, 이미 존재하는 다른 오브젝트를 그 복제본으로 사용하고 싶을 때 활용한다.
	 *
	 * 이 맵에 들어간 오브젝트들은 실제로 복제되지 않는다.
	 * Key는 원본 오브젝트, Value는 복제본으로 사용될 오브젝트이다.
	 */
	TMap<UObject*, UObject*>& DuplicationSeed;

	/**
	 * null이 아니라면, StaticDuplicateObject 호출 시 생성된 모든 복제 오브젝트들이 이 맵에 기록된다.
	 *
	 * Key는 원본 오브젝트, Value는 새로 생성된 복제 오브젝트이다.
	 * DuplicationSeed에 의해 미리 매핑된 오브젝트들은 여기에는 들어가지 않는다.
	 */
	TMap<UObject*, UObject*>& CreatedObjects;

	/** 생성자 */
	FObjectDuplicationParameters(UObject* InSourceObject, UObject* InDestOuter, TMap<UObject*, UObject*>& InDuplicationSeed, TMap<UObject*, UObject*>& InCreatedObjects)
		: SourceObject(InSourceObject), DestOuter(InDestOuter), DuplicationSeed(InDuplicationSeed), CreatedObjects(InCreatedObjects) {}
};

/**
 * @brief 오브젝트 복사를 위한 편의성 템플릿 
 *
 * @param SourceObject 복사되는 오브젝트
 * @param Outer 복사된 오브젝트의 소유자
 * @param Name 오브젝트를 위한 선택적 이름 
 *
 * @return 복사된 오브젝트, 혹은 실패할 경우 null 
 */
template<typename T>
T* DuplicateObject(T* const SourceObject, UObject* Outer, const FName Name = FName::GetNone())
{
	if (SourceObject != nullptr)
	{
		if (Outer == nullptr)
		{
			// [UE] TODO: Outer = (UObject*)GetTransientOuterForRename(T::StaticClass());
		}
		TMap<UObject*, UObject*> DuplicatedSeed;
		TMap<UObject*, UObject*> CreatedObjects;
		return static_cast<T*>(StaticDuplicateObject(SourceObject, Outer, Name, DuplicatedSeed, CreatedObjects));
	}
	return nullptr;
}

template<typename T>
T* DuplicateObject(const TObjectPtr<T> SourceObject, UObject* Outer, const FName Name = FName::GetNone())
{
	if (SourceObject != nullptr)
	{
		if (Outer == nullptr)
		{
			// [UE] TODO: Outer = (UObject*)GetTransientOuterForRename(T::StaticClass());
		}
		TMap<UObject*, UObject*> DuplicatedSeed;
		TMap<UObject*, UObject*> CreatedObjects;
		return static_cast<T*>(StaticDuplicateObject(SourceObject, Outer, Name, DuplicatedSeed, CreatedObjects));
	}
	return nullptr;
}

inline FObjectDuplicationParameters InitStaticDuplicateObjectParams(UObject const* SourceObject, UObject* DestOuter, const FName DestName,
	TMap<UObject*, UObject*>& DuplicationSeed, TMap<UObject*, UObject*>& CreatedObjects, EDuplicateMode::Type DuplicateMode = EDuplicateMode::Normal)
{
	FObjectDuplicationParameters Parameters(const_cast<UObject*>(SourceObject), DestOuter, DuplicationSeed, CreatedObjects);
	if (DestName != FName::GetNone())
	{
		Parameters.DestName = DestName;
	}

	Parameters.DestClass = SourceObject->GetClass();

	Parameters.DuplicateMode = DuplicateMode;

	return Parameters;
}

inline UObject* StaticDuplicateObjectEX(FObjectDuplicationParameters Parameters)
{
	Parameters.SourceObject->PreDuplicate(Parameters);

	UObject* DupRootObject = nullptr;
	if (auto It = Parameters.DuplicationSeed.find(Parameters.SourceObject); It != Parameters.DuplicationSeed.end())
	{
		DupRootObject = It->second;	
	}

	if (DupRootObject == nullptr)
	{
		DupRootObject = Parameters.SourceObject->Duplicate(Parameters);
	}

	return DupRootObject;
}

inline UObject* StaticDuplicateObject(UObject* SourceObject, UObject* DestOuter, const FName DestName, TMap<UObject*, UObject*>& DuplicatedSeed, TMap<UObject*, UObject*>& CreatedObjects)
{
	FObjectDuplicationParameters Parameters = InitStaticDuplicateObjectParams(SourceObject, DestOuter, DestName, DuplicatedSeed, CreatedObjects);
	return StaticDuplicateObjectEX(Parameters);
}
