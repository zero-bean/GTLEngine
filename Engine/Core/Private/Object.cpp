#include "pch.h"
#include "Core/Public/Object.h"
#include "Core/Public/EngineStatics.h"

UINT UEngineStatics::NextUUID = 0;
TArray<UObject*> GUObjectArray;

UObject::UObject()
{
	UUID = UEngineStatics::GenUUID();

	GUObjectArray.push_back(this);
	InternalIndex = static_cast<UINT>(GUObjectArray.size()) - 1;
}

UObject::~UObject()
{

}
