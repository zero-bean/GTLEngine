#pragma once
#include "UClass.h"

// 생성 경로 단일화용 팩토리
struct FObjectFactory
{
    // 1) 런타임 타입(UClass*)로 생성: 인자 전달 불가
    static UObject* ConstructObject(UClass* ClassType)
    {
        if (!ClassType)
        {
            return nullptr;
        }

        // UClass가 보유한 createFunction으로 생성하고 내부에서 AssignDefaultNameFromClass(this) 수행
        return ClassType->CreateDefaultObject();
    }

    // 2) 컴파일 타임 타입으로 생성: 인자 전달 가능
    template<typename T, typename... Args>
    static T* ConstructObject(Args&&... args)
    {
        static_assert(std::is_base_of<UObject, T>::value, "T must derive from UObject");
        T* Object = new T(std::forward<Args>(args)...);

        // 생성 완료 후, 정확한 타입의 클래스 정보로 고유 이름 부여/등록
        Object->AssignDefaultNameFromClass(T::StaticClass());
        return Object;
    }
};