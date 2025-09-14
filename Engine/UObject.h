#pragma once
#include "UEngineStatics.h"
#include "Array.h"
#include "ISerializable.h"
#include "UNamePool.h"
#include <type_traits>

// Forward declaration (순환 참조 방지)
class UClass;

typedef int int32;
typedef unsigned int uint32;

/*
    루트 클래스 매크로:
    - StaticClass(): 타입 메타(UClass) 정적 접근자
    - GetClass(): 런타임 타입 정보 접근(가상)
    - s_StaticClass: RegisterToFactory 결과를 저장
    - CreateInstance(): 팩토리용 기본 생성 함수 포인터
*/
#define DECLARE_ROOT_UCLASS(ClassName) \
public: \
    virtual UClass* GetClass() const; \
    static UClass* StaticClass(); \
private: \
    static UClass* s_StaticClass; \
    static UObject* CreateInstance();

#define IMPLEMENT_ROOT_UCLASS(ClassName) \
UObject* ClassName::CreateInstance() { return new ClassName(); } \
UClass* ClassName::s_StaticClass = UClass::RegisterToFactory( \
    FName(#ClassName), &ClassName::CreateInstance, FName::GetNone()); \
UClass* ClassName::StaticClass() { return s_StaticClass; } \
UClass* ClassName::GetClass() const { return StaticClass(); }

/*
    파생 클래스 매크로:
    - using Super = ParentClass; 상위 타입 별칭 제공(언리얼 스타일)
    - RegisterToFactory로 타입 이름, 생성 함수, 부모 타입 등록 → 상속 트리와 팩토리 구성
*/
#define DECLARE_UCLASS(ClassName, ParentClass) \
public: \
    using Super = ParentClass; \
    UClass* GetClass() const override; \
    static UClass* StaticClass(); \
private: \
    static UClass* s_StaticClass; \
    static UObject* CreateInstance();

#define IMPLEMENT_UCLASS(ClassName, ParentClass) \
UObject* ClassName::CreateInstance() { return new ClassName(); } \
UClass* ClassName::s_StaticClass = UClass::RegisterToFactory( \
    FName(#ClassName), &ClassName::CreateInstance, FName(#ParentClass)); \
UClass* ClassName::StaticClass() { return s_StaticClass; } \
UClass* ClassName::GetClass() const { return StaticClass(); }

/*
    클래스 메타데이터 등록 매크로:
    - 정적 객체 생성자 타이밍에 ClassName::StaticClass()->SetMeta 호출
    - 소스 한 줄로 런타임 전 메타 자동 등록
*/
#define UCLASS_META(ClassName, Key, Value) \
struct _MetaRegister_##ClassName##_##Key { \
    _MetaRegister_##ClassName##_##Key() { ClassName::StaticClass()->SetMeta(#Key, Value); } \
} _MetaRegisterInstance_##ClassName##_##Key;

/*
    UObject:
    - 모든 엔진 객체의 루트
    - GUObjectArray에 라이브 객체 포인터를 보관(빈 슬롯은 nullptr로 유지, FreeIndices로 재사용)
    - GAllObjectNames에 라이브 객체의 고유 FName을 보관(ComparisonIndex 기반 유일성)
*/
class UObject : public ISerializable
{
    DECLARE_ROOT_UCLASS(UObject)
private:
    // 전역 테이블 인덱스 재사용 관리
    static inline TArray<uint32> FreeIndices;
    static inline uint32 NextFreshIndex = 0;

    // 이름 생성용 접미사 카운터: Base(FName) → 다음 붙일 숫자
    static inline TMap<FName, uint32> NameSuffixCounters;

    // 라이브 오브젝트의 고유 이름 집합(ComparisonIndex 기준 비교)
    static inline TSet<FName> GAllObjectNames;

    // Base(FName)로부터 유니크 이름 생성 (첫 번째는 Base, 이후 Base_1, Base_2 ...)
    static FName GenerateUniqueName(const FName& Base);

    // 전역 이름 집합 조작
    static bool IsNameInUse(const FName& Name);
    static void RegisterName(const FName& Name);
    static void UnregisterName(const FName& Name);

public:
    // 전역 객체 테이블(슬롯 기반, nullptr 허용)
    static inline TArray<UObject*> GUObjectArray;

    // 전역 고유 ID(메모리/수명과 무관)
    uint32 UUID;

    // GUObjectArray 내 인덱스(슬롯 인덱스)
    uint32 InternalIndex;

    // 객체 고유 이름(FNamePool 관리)
    FName Name;

    // 생성 시 GUObjectArray에 O(1) 등록 (이때 타입 정보 사용 금지)
    UObject()
        : Name("None")
    {
        UUID = UEngineStatics::GenUUID();

        if (!FreeIndices.empty())
        {
            InternalIndex = FreeIndices.back();
            FreeIndices.pop_back();
            GUObjectArray[InternalIndex] = this;
        }
        else
        {
            InternalIndex = NextFreshIndex++;
            GUObjectArray.push_back(this);
        }
        // 주의: 생성자에서는 가상 호출(예: GetClass) 사용 금지
    }

    // 소멸 시 이름 해제 + 슬롯 반환
    virtual ~UObject()
    {
        if (Name.ToString() != "None")
        {
            UnregisterName(Name);
        }

        if (InternalIndex < GUObjectArray.size() && GUObjectArray[InternalIndex] == this)
        {
            GUObjectArray[InternalIndex] = nullptr;
            FreeIndices.push_back(InternalIndex);
        }
    }

    // 에디터/인스펙터 노출 여부(필요 시 파생에서 true)
    virtual bool CountOnInspector() { return false; }

    // 런타임 타입 검사/캐스팅 (UClass의 비트셋 사용)
    template<typename T>
    bool IsA() const {
        return GetClass()->IsChildOrSelfOf(T::StaticClass());
    }

    template<typename T>
    T* Cast() {
        return IsA<T>() ? static_cast<T*>(this) : nullptr;
    }

    template<typename T>
    const T* Cast() const {
        return IsA<T>() ? static_cast<const T*>(this) : nullptr;
    }

    // 메모리 추적용 new/delete 오버로드
    void* operator new(size_t Size)
    {
        UEngineStatics::AddAllocation(Size);
        return ::operator new(Size);
    }

    void operator delete(void* Ptr, size_t Size)
    {
        UEngineStatics::RemoveAllocation(Size);
        ::operator delete(Ptr);
    }

    // placement new/delete (추적 제외)
    void* operator new(size_t /*Size*/, void* Ptr)
    {
        return Ptr;
    }

    void operator delete(void* /*Ptr*/, void* /*Place*/)
    {
    }

    // 직렬화 인터페이스(파생에서 확장)
    json::JSON Serialize() const override
    {
        return json::JSON();
    }

    bool Deserialize(const json::JSON& Data) override
    {
        return true;
    }

    void SetUUID(uint32 InUuid)
    {
        UUID = InUuid;
    }

    // 완전 생성 이후 클래스 정보 기반 기본 이름 부여
    void AssignDefaultNameFromClass(const UClass* Cls);

    // 문자열 베이스로 이름 생성/등록(특수 경로 용도)
    void AssignNameFromString(const FString& Base);

    // 재사용 큐 리셋(정책 초기화 용도)
    static void ClearFreeIndices()
    {
        FreeIndices.clear();
    }

    // 디버그/에디터: 현재 라이브 오브젝트 이름 스냅샷 수집
    static void CollectLiveObjectNames(TArray<FString>& OutNames);
};

// 편의 생성기:
// - 직접 new 대신 사용 시, 생성 완료 후 정확한 클래스 기반으로 고유 이름 자동 부여
template<typename T, typename... Args>
inline T* NewObject(Args&&... ArgsForward)
{
    static_assert(std::is_base_of<UObject, T>::value, "T must derive from UObject");
    T* Object = new T(std::forward<Args>(ArgsForward)...);
    Object->AssignDefaultNameFromClass(T::StaticClass());
    return Object;
}