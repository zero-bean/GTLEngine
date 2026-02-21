#include "pch.h"

#include "Core/Public/ObjectIterator.h"

// FObjectCacheManager 정적 변수 정의 (UE 스타일)
TMap<uint32, TArray<UObject*>> FObjectCacheManager::ObjectCache;
bool FObjectCacheManager::bCacheValid = false;
int32 FObjectCacheManager::LastProcessedIndex = 0;

