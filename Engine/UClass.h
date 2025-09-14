#pragma once
#include "UEngineStatics.h"
#include "FDynamicBitset.h"
#include "json.hpp"
#include "UObject.h"
#include <memory>

/*
    UClass:
    - 런타임 타입 정보(RTTI)와 팩토리 역할
    - ClassName/SuperClassTypeName은 FName으로 저장(빠른 비교/조회)
    - NameToId로 FName → TypeId 매핑 유지
    - DisplayNameToId는 UI용 표시 문자열(FString) 인덱싱
    - TypeBitset은 상속 계층(자기/조상 타입) 포함 비트셋
*/
class UClass : public UObject
{
    DECLARE_UCLASS(UClass, UObject)
private:
    static inline TArray<TUniquePtr<UClass>> ClassList;
    // 타입명(FName) → TypeId
    static inline TMap<FName, uint32> NameToId;
    // 표시 이름(FString) → TypeId (UI/검색용)
    static inline TMap<FString, uint32> DisplayNameToId;
    static inline uint32 RegisteredCount = 0;

    // 메타데이터(임의 키/값, 주로 표시 이름 등)
    TMap<FName, FString> Metadata;

    // 타입 고유 ID 및 상속 비트셋
    uint32 TypeId;
    FDynamicBitset TypeBitset;

    // 타입 이름 및 부모 타입 이름(FName)
    FName ClassName = FName::GetNone();
    FName SuperClassTypeName = FName::GetNone();

    // 부모 타입(UClass*)
    UClass* SuperClass = nullptr;

    // 인스턴스 생성 함수 포인터(무인자 기본 생성)
    TFunction<UObject* ()> CreateFunction;

    // 타입 비트셋 처리 여부
    bool bProcessed = false;

public:
    // 타입 등록: 타입명/생성 함수/부모 타입명(FName) 등록 후 UClass* 반환
    static UClass* RegisterToFactory(const FName& TypeName,
        const TFunction<UObject* ()>& CreateFunction, const FName& SuperClassTypeName);

    // 상속 비트셋(자기/조상 타입 비트) 해석/구축
    static void ResolveTypeBitsets();
    void ResolveTypeBitset(UClass* ClassPtr);

    // TypeId로 UClass 조회
    static UClass* GetClass(uint32 TypeId) {
        return (TypeId < ClassList.size()) ? ClassList[TypeId].get() : nullptr;
    }

    // 타입명(FName/FString)으로 UClass 조회
    static UClass* FindClass(const FName& Name) {
        auto It = NameToId.find(Name);
        return (It != NameToId.end()) ? GetClass(It->second) : nullptr;
    }
    static UClass* FindClass(const FString& Name) {
        return FindClass(FName(Name));
    }

    // 표시 이름(FString) → 검색, 실패 시 타입명(FName)으로 폴백
    static UClass* FindClassWithDisplayName(const FString& Name)
    {
        auto It = DisplayNameToId.find(Name);
        if (It != DisplayNameToId.end())
            return GetClass(It->second);

        return FindClass(FName(Name));
    }

    // 등록된 전체 타입 리스트
    static const TArray<TUniquePtr<UClass>>& GetClassList()
    {
        return ClassList;
    }

    // BaseClass의 자식/자기 자신 여부 검사 (비트셋 기반)
    bool IsChildOrSelfOf(UClass* BaseClass) const {
        return BaseClass && TypeBitset.Test(BaseClass->TypeId);
    }

    // 타입명(FName)
    const FName& GetUClassName() const { return ClassName; }

    // 표시용 이름(FString). 없으면 ClassName을 문자열로 반환
    const FString& GetDisplayName() const
    {
        static const FName DisplayNameKey("DisplayName");
        auto It = Metadata.find(DisplayNameKey);
        if (It != Metadata.end())
        {
            return It->second;
        }
        static thread_local FString Tmp;
        Tmp = ClassName.ToString();
        return Tmp;
    }

    // 메타데이터 설정(표시 이름 등록 시 DisplayNameToId 동기화)
    void SetMeta(const FName& Key, const FString& Value)
    {
        Metadata[Key] = Value;

        static const FName DisplayNameKey("DisplayName");
        if (Key == DisplayNameKey)
        {
            DisplayNameToId[Value] = TypeId;
        }
    }

    // 메타데이터 조회(없으면 빈 문자열)
    const FString& GetMeta(const FName& Key) const 
    {
        static FString Empty;
        auto It = Metadata.find(Key);
        return (It != Metadata.end()) ? It->second : Empty;
    }

    // 팩토리 생성:
    // - 등록된 CreateFunction으로 인스턴스 생성
    // - 생성 완료 후 이 클래스 정보로 고유 이름 부여
    UObject* CreateDefaultObject() const
    {
        if (!CreateFunction)
        {
            return nullptr;
        }

        UObject* Object = CreateFunction();
        if (Object)
        {
            Object->AssignDefaultNameFromClass(this);
        }

        return Object;
    }
};