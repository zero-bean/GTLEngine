#include "stdafx.h"
#include "UClass.h"
#include "UObject.h"

IMPLEMENT_UCLASS(UClass, UObject)

// 타입 등록:
// - UClass 인스턴스 생성 후 TypeId/이름/부모/생성 함수 저장
// - NameToId에 FName → TypeId 매핑 추가
// - UClass 자신도 UObject이므로 "Class:<TypeName>" 이름을 부여(고유성 보장)
UClass* UClass::RegisterToFactory(const FName& TypeName, const TFunction<UObject* ()>& CreateFunction, const FName& SuperClassTypeName)
{
    TUniquePtr<UClass> ClassType = MakeUnique<UClass>();

    ClassType->TypeId = RegisteredCount++;
    ClassType->ClassName = TypeName;
    ClassType->SuperClassTypeName = SuperClassTypeName;
    ClassType->CreateFunction = CreateFunction;

    NameToId[TypeName] = ClassType->TypeId;

    // 메타 오브젝트(UClass 인스턴스) 이름 등록: "Class:<TypeName>"
    ClassType->AssignNameFromString(FString("Class:") + TypeName.ToString());

    UClass* RawPtr = ClassType.get();
    ClassList.push_back(std::move(ClassType));
    return RawPtr;
}

// 상속 비트셋 구성:
// 1) 부모 UClass 포인터 결합
// 2) 각 타입의 비트셋에 조상 비트와 자기 비트 설정
void UClass::ResolveTypeBitsets()
{
    for (const TUniquePtr<UClass>& ClassEntry : ClassList)
    {
        // 부모 이름이 있으면 NameToId로 연동
        if (!ClassEntry->SuperClassTypeName.ToString().empty()) {
            auto It = NameToId.find(ClassEntry->SuperClassTypeName);
            ClassEntry->SuperClass = (It != NameToId.end()) ? ClassList[It->second].get() : nullptr;
        }
    }
    for (const TUniquePtr<UClass>& ClassEntry : ClassList)
    {
        if (ClassEntry->bProcessed) continue;
        ClassEntry->ResolveTypeBitset(ClassEntry.get());
    }
}

// 상속 비트셋 단일 타입 처리(부모부터 처리되도록 스택 기반)
void UClass::ResolveTypeBitset(UClass* ClassPtr)
{
    TArray<UClass*> Stack;
    Stack.push_back(ClassPtr);

    while (!Stack.empty())
    {
        UClass* Cur = Stack.back();

        // 부모가 미처리면 먼저 부모를 처리
        while (Cur->SuperClass && !Cur->SuperClass->bProcessed)
        {
            Stack.push_back(Cur->SuperClass);
            Cur = Stack.back();
        }

        // 현재 노드 처리
        Cur->TypeBitset.Clear();
        if (Cur->SuperClass) Cur->TypeBitset |= Cur->SuperClass->TypeBitset;
        Cur->TypeBitset.Set(Cur->TypeId);  // 자기 비트 추가
        Cur->bProcessed = true;

        Stack.pop_back();
    }
}