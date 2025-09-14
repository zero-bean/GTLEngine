#include "stdafx.h"
#include "UClass.h"
#include "UObject.h"

IMPLEMENT_UCLASS(UClass, UObject)

UClass* UClass::RegisterToFactory(const FName& typeName, const TFunction<UObject* ()>& createFunction, const FName& superClassTypeName)
{
    TUniquePtr<UClass> classType = MakeUnique<UClass>();

    classType->typeId = registeredCount++;
    classType->className = typeName;
    classType->superClassTypeName = superClassTypeName;
    classType->createFunction = createFunction;

    nameToId[typeName] = classType->typeId;

    // 메타 오브젝트(UClass 인스턴스)도 고유 이름 등록
    // "Class:<TypeName>", 중복 시 _1, _2 ... 부여
    classType->AssignNameFromString(FString("Class:") + typeName.ToString());

    UClass* rawPtr = classType.get();
    classList.push_back(std::move(classType));
    return rawPtr;
}

void UClass::ResolveTypeBitsets()
{
    for (const TUniquePtr<UClass>& _class : classList)
    {
        // superClassTypeName이 비어있지 않으면 부모 결합
        if (!_class->superClassTypeName.ToString().empty()) {
            auto it = nameToId.find(_class->superClassTypeName);
            _class->superClass = (it != nameToId.end()) ? classList[it->second].get() : nullptr;
        }
    }
    for (const TUniquePtr<UClass>& _class : classList)
    {
        if (_class->processed) continue;
        _class->ResolveTypeBitset(_class.get());
    }
}

void UClass::ResolveTypeBitset(UClass* classPtr)
{
    TArray<UClass*> stack;
    stack.push_back(classPtr);

    while (!stack.empty())
    {
        UClass* cur = stack.back();

        // 부모가 아직 처리되지 않았다면 먼저 스택에 push
        while (cur->superClass && !cur->superClass->processed)
        {
            stack.push_back(cur->superClass);
            cur = stack.back();
        }

        // 현재 노드 처리
        cur->typeBitset.Clear();
        if (cur->superClass) cur->typeBitset |= cur->superClass->typeBitset;
        cur->typeBitset.Set(cur->typeId);  // 자신의 비트 추가
        cur->processed = true;

        stack.pop_back();
    }
}