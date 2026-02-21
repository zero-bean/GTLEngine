#pragma once
#include "Widget.h"

class UPrimitiveSpawnWidget
	: public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void SpawnActors() const;

	// Special Member Function
	UPrimitiveSpawnWidget();
	~UPrimitiveSpawnWidget() override;

private:
	EPrimitiveType SelectedPrimitiveType = EPrimitiveType::Sphere;
	int32 NumberOfSpawn = 1;
	FVector SpawnRangeMin;
	FVector SpawnRangeMax;
	bool bUseUniformSpawnRange;
};
