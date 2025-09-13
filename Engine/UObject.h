#pragma once
#include "UEngineStatics.h"
#include "TArray.h"
#include "ISerializable.h"
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
    #ClassName, &ClassName::CreateInstance, ""); \
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
    #ClassName, &ClassName::CreateInstance, #ParentClass); \
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
public:
    static inline TArray<UObject*> GUObjectArray; // 전역 객체 테이블
    uint32 UUID;                                  // 엔진 전역 고유 ID(UEngineStatics::GenUUID())
    uint32 InternalIndex;                         // GUObjectArray 내 인덱스

    UObject()
    {
        UUID = UEngineStatics::GenUUID();

        // 방법 1: 빠른 추가 - 재사용 인덱스 우선 사용
        if (!FreeIndices.empty())
        {
            // 삭제된 슬롯 재사용 (O(1))
            InternalIndex = FreeIndices.back();
            FreeIndices.pop_back();
            GUObjectArray[InternalIndex] = this;
        }
        else
        {
            // 없으면 새 슬롯 할당 (O(1) amortized)
            InternalIndex = NextFreshIndex++;
            GUObjectArray.push_back(this);
        }
    }

    virtual ~UObject()
    {
        // 방법 1: 빠른 삭제 - nullptr 마킹 + 인덱스 재사용 큐에 추가
        if (InternalIndex < GUObjectArray.size() && GUObjectArray[InternalIndex] == this)
        {
            GUObjectArray[InternalIndex] = nullptr;  // O(1)
            FreeIndices.push_back(InternalIndex);    // O(1)
        }
    }
    // 에디터/인스펙터 노출 여부 플래그
    virtual bool CountOnInspector() {
        return false;
    }
    // 런타임 타입 검사/캐스팅
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
    // Override new/delete for tracking
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

    // 배치 new/delete도 오버라이드 (필요한 경우)
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
    // 재사용 큐만 비움(특정 상황에서 인덱스 정책 리셋 시)
    static void ClearFreeIndices()
    {
        FreeIndices.clear();
    }
};