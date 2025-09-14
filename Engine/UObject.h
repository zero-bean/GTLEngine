#pragma once
#include "UEngineStatics.h"
#include "TArray.h"
#include "ISerializable.h"
#include "UNamePool.h"
#include <type_traits>

// Forward declaration으로 순환 종속성 해결
class UClass;

typedef int int32;
typedef unsigned int uint32;
/*
    루트 클래스용
    StaticClass() 정적 접근자, GetClass() 가상 함수
    s_StaticClass 정적 포인터에 UClass::RegisterToFactory(...) 결과를 담아둠
    UClass는 “클래스명 문자열 → 생성자 함수” 매핑을 갖게 되어, 문자열로도 인스턴스 생성 가능(팩토리 패턴).
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
    파생 클래스용
    using Super = ParentClass;로 상위 타입 별칭을 제공해 언리얼 스타일 사용성 부여.
    UClass::RegisterToFactory(#ClassName, &CreateInstance, #ParentClass) 호출로
    - 클래스명,
    - 생성 함수,
    - 부모 클래스명
    을 UClass에 등록 → 상속 트리 구축/검증에 이용 가능.
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
    정적 객체의 생성자를 이용해 프로그램 시작 시점에 ClassName::StaticClass()->SetMeta(#Key, Value) 호출
    즉, 소스에 한 줄 넣어두면 런타임 전에 자동으로 메타가 박힙니다.
    Ex) UCLASS_META(UMeshComponent, MeshName, "Gizmo_Mesh")
*/
#define UCLASS_META(ClassName, Key, Value) \
struct _MetaRegister_##ClassName##_##Key { \
    _MetaRegister_##ClassName##_##Key() { ClassName::StaticClass()->SetMeta(#Key, Value); } \
} _MetaRegisterInstance_##ClassName##_##Key;

class UObject : public ISerializable
{
    DECLARE_ROOT_UCLASS(UObject)
private:
    static inline TArray<uint32> FreeIndices;     // 비어있는 슬롯 스택
    static inline uint32 NextFreshIndex = 0;

    // 이름 생성용: "기본 이름(FName) -> 다음 붙일 숫자"
    static inline TMap<FName, uint32> NameSuffixCounters;

    // 라이브 오브젝트 이름 집합 (FName 비교는 ComparisonIndex 기반)
    static inline TSet<FName> GAllObjectNames;

    // 유니크 이름을 생성(Base_0부터). Base는 클래스명(FName).
    static FName GenerateUniqueName(const FName& base);

    // 전역 이름 집합 등록/해제
    static bool IsNameInUse(const FName& name);
    static void RegisterName(const FName& name);
    static void UnregisterName(const FName& name);

public:
    static inline TArray<UObject*> GUObjectArray; // 전역 객체 테이블
    uint32 UUID;                                  // 엔진 전역 고유 ID(UEngineStatics::GenUUID())
    uint32 InternalIndex;                         // GUObjectArray 내 인덱스
    FName Name;                                   // 이름 (UNamePool 관리)

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
        // 주의: 여기서 GetClass() 사용 금지(베이스 생성자 단계에서 가상 디스패치 불안정)
    }

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

    virtual bool CountOnInspector() { return false; }

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

    void* operator new(size_t size)
    {
        UEngineStatics::AddAllocation(size);
        return ::operator new(size);
    }

    void operator delete(void* ptr, size_t size)
    {
        UEngineStatics::RemoveAllocation(size);
        ::operator delete(ptr);
    }

    void* operator new(size_t size, void* ptr)
    {
        return ptr;  // placement new는 메모리 추적 안함
    }

    void operator delete(void* ptr, void* place)
    {
        // placement delete는 아무것도 안함
    }

    json::JSON Serialize() const override
    {
        return json::JSON();
    }

    bool Deserialize(const json::JSON& data) override
    {
        return true;
    }

    void SetUUID(uint32 uuid)
    {
        UUID = uuid;
    }

    // 클래스 정보로 기본 이름을 부여(완전 생성된 뒤 호출)
    void AssignDefaultNameFromClass(const UClass* cls);
    // 편의 함수: 가상 GetClass() 사용(완전 생성 이후에만 호출)
    void AssignDefaultName();

    // 임의 문자열(base)로 고유 이름 생성/등록
    void AssignNameFromString(const FString& base);

    static void ClearFreeIndices()
    {
        FreeIndices.clear();
    }

    static void CollectLiveObjectNames(TArray<FString>& OutNames);
};

// 편의 생성기: 직접 new 대신 사용하면 자동 네이밍
template<typename T, typename... Args>
inline T* NewObject(Args&&... args)
{
    static_assert(std::is_base_of<UObject, T>::value, "T must derive from UObject");
    T* obj = new T(std::forward<Args>(args)...);
    obj->AssignDefaultNameFromClass(T::StaticClass());
    return obj;
}