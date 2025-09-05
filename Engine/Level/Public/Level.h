#pragma once
#include "Core/Public/Object.h"

class ULevel :
	public UObject
{
public:
	ULevel();
	ULevel(const wstring& InName);
	~ULevel() override;

	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Cleanup() = 0;

	const wstring& GetName() const { return Name; }

private:
	wstring Name;
	TArray<UObject*> LevelObjects;
};
