#pragma once
#include "Widget.h"
#include "../../Vector.h"

class UUIManager;
class UWorld;

// FSpawnInfo는 더 이상 사용되지 않습니다 - Reflection 기반 시스템으로 대체됨

class UPrimitiveSpawnWidget
	:public UWidget
{
public:
	DECLARE_CLASS(UPrimitiveSpawnWidget, UWidget)

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void SpawnActor(UClass* ActorClass) const;

	// Special Member Function
	UPrimitiveSpawnWidget();
	~UPrimitiveSpawnWidget() override;

private:
	UUIManager* UIManager = nullptr;
	TArray<UClass*> SpawnableClasses; // 자동으로 채워질 스폰 가능 클래스 목록
	mutable int32 SelectedClassIndex = 0; // 선택된 클래스 인덱스

	// Spawn 설정
	int32 NumberOfSpawn = 1;
	float SpawnRangeMin = -5.0f;
	float SpawnRangeMax = 5.0f;
	
	// 추가 옵션
	bool bRandomRotation = true;
	bool bRandomScale = true;
	float MinScale = 0.5f;
	float MaxScale = 2.0f;

	// 메시 선택
	mutable int32 SelectedMeshIndex = -1;
	mutable TArray<FString> CachedMeshFilePaths;
	
	// 헬퍼 메서드
	UWorld* GetCurrentWorld() const;
	FVector GenerateRandomLocation() const;
	float GenerateRandomScale() const;
	FQuat GenerateRandomRotation() const;
	const char* GetPrimitiveTypeName(int32 TypeIndex) const;
};