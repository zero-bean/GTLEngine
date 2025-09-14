#pragma once
#include "UEngineStatics.h"
#include "FDynamicBitset.h"
#include "json.hpp"
#include "UObject.h"
#include <memory>

class UClass : public UObject
{
    DECLARE_UCLASS(UClass, UObject)
private:
    static inline TArray<TUniquePtr<UClass>> classList;
    // 클래스 이름 키를 FName으로 변경
    static inline TMap<FName, uint32> nameToId;
    static inline TMap<FString, uint32> displayNameToId;
    static inline uint32 registeredCount = 0;

    TMap<FString, FString> metadata;
    uint32 typeId;
    FDynamicBitset typeBitset;
    // 클래스명과 부모 클래스명도 FName으로 보관
	FName className = FName::GetNone();
	FName superClassTypeName = FName::GetNone();
    UClass* superClass = nullptr;
    TFunction<UObject* ()> createFunction;
    bool processed = false;

public:
    // FName 기반 등록
    static UClass* RegisterToFactory(const FName& typeName,
        const TFunction<UObject* ()>& createFunction, const FName& superClassTypeName);

    static void ResolveTypeBitsets();
    void ResolveTypeBitset(UClass* classPtr);

    static UClass* GetClass(uint32 typeId) {
        return (typeId < classList.size()) ? classList[typeId].get() : nullptr;
    }

    // FName 기반 조회
    static UClass* FindClass(const FName& name) {
        auto it = nameToId.find(name);
        return (it != nameToId.end()) ? GetClass(it->second) : nullptr;
    }
    // 호환용: 문자열로 들어오면 FName으로 변환하여 조회
    static UClass* FindClass(const FString& name) {
        return FindClass(FName(name));
    }

    static UClass* FindClassWithDisplayName(const FString& name)
    {
        // 1) DisplayName lookup (표시용 문자열은 FString 유지)
        auto it = displayNameToId.find(name);
        if (it != displayNameToId.end())
            return GetClass(it->second);

        // 2) className fallback (FName으로 변환)
        return FindClass(FName(name));
    }

    static const TArray<TUniquePtr<UClass>>& GetClassList()
    {
        return classList;
    }

    bool IsChildOrSelfOf(UClass* baseClass) const {
        return baseClass && typeBitset.Test(baseClass->typeId);
    }

    // FName 반환
    const FName& GetUClassName() const { return className; }

    const FString& GetDisplayName() const
    {
        auto itr = metadata.find("DisplayName");
        if (itr != metadata.end())
        {
            return itr->second;
        }
        // 필요 시 호출자가 ToString()을 통해 문자열로 사용
        static thread_local FString tmp;
        tmp = className.ToString();
        return tmp;
    }

    void SetMeta(const FString& key, const FString& value)
    {
        metadata[key] = value;

        if (key == "DisplayName")
        {
            displayNameToId[value] = typeId;  // typeId는 인스턴스 멤버
        }
    }

    const FString& GetMeta(const FString& key) const {
        static FString empty;
        auto it = metadata.find(key);
        return (it != metadata.end()) ? it->second : empty;
    }

    // 팩토리 경로로 생성 시 자동으로 이름 부여
    UObject* CreateDefaultObject() const
    {
        if (!createFunction)
        {
            return nullptr;
        }

        UObject* Object = createFunction();
        if (Object)
        {
            Object->AssignDefaultNameFromClass(this);
        }

        return Object;
    }
};