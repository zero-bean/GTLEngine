#pragma once
#include "Core/Public/Object.h"

class AActor;

class ULevel :
	public UObject
{
public:
	ULevel();
	ULevel(const wstring& InName);
	~ULevel() override;

	virtual void Init();
	virtual void Update();
	virtual void Render();
	virtual void Cleanup();

	const wstring& GetName() const { return Name; }
	TArray<UObject*> GetLevelObjects() const { return LevelObjects; }

	void AddObject(UObject* Object) { LevelObjects.push_back(Object); }

	void SetSelectedActor(AActor* InActor) { SelectedActor = InActor; }
	AActor* GetSelectedActor() const { return SelectedActor; }
private:
	wstring Name;
	TArray<UObject*> LevelObjects;
	AActor* SelectedActor = nullptr;
};
