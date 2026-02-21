#include "pch.h"

FString UObject::GetName()
{
    return ObjectName.ToString();
}

FString UObject::GetComparisonName()
{
    return FString();
}

UObject* UObject::GetOuter() const
{
	return nullptr;
}

UWorld* UObject::GetWorld() const
{
	if (UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}

	return nullptr;
}

// 얕은 복사가 필요하면 얕은 복사만 하고, 깊은 복사가 필요하면 깊은 복사까지 처리한 객체 반환 
UObject* UObject::Duplicate()
{
	// 1.복사 생성자를 이용해 '얕은 복사'를 수행 (포인터 없는 클래스면 이게 곧 깊은 복사)
	UObject* DuplicatedObject = new UObject(*this);

	// 2. 서브 오브젝트들의 깊은 복사를 위임
	DuplicatedObject->DuplicateSubObjects();

	// 3. 복사된 객체 반환
	return DuplicatedObject;
}

// 자신이 가진 멤버에 대해 Duplicate할 거면 override 필요
void UObject::DuplicateSubObjects()
{
	// override 시 부모의 Duplicate 먼저 호출하도록 설정

	// 1. 서브 오브젝트에 대해 Duplicate 호출 (위임하는 역할)
	//    이를 통해 서브 오브젝트 클래스로 가서 Duplicate가 수행될 거고
	//    - 서브 오브젝트에서 별도의 DuplicatedSubObjects를 override하지 않으면 
	//      Duplicate의 2번 과정이 pass되어 자신만 복사를 수행하겠다는 의미
	//	  - 서브 오브젝트에서 별도의 DuplicatedSubObjects를 override하면
	//      본인이 가진 서브 오브젝트에 대해 또 Duplicate 하겠다는 의미 -> 재귀적
	// - 
}
