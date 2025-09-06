#include "pch.h"
#include "Level/Public/Level.h"

ULevel::ULevel() = default;

ULevel::ULevel(const wstring& InName)
	: Name(InName)
{
}

ULevel::~ULevel()
{
	for (auto Object : LevelObjects)
	{
		SafeDelete(Object);
	}
}

void ULevel::Init()
{
}

void ULevel::Update()
{
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
}
