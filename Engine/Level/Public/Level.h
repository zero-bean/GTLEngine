#pragma once
#include "Core/Public/Object.h"

class AGizmo;
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
	TArray<AActor*> GetLevelActors() const { return LevelActors; }

	template<typename T, typename... Args>
	T* SpawnActor(Args&&... args);

	void AddActor(AActor* Actor) { LevelActors.push_back(Actor); }

	void SetSelectedActor(AActor* InActor) { SelectedActor = InActor; }
	AActor* GetSelectedActor() const { return SelectedActor; }
	AGizmo* GetGizmo() const { return Gizmo; };

private:
	wstring Name;
	TArray<AActor*> LevelActors;
	AActor* SelectedActor = nullptr;
	AGizmo* Gizmo = nullptr;
};

template <typename T, typename ... Args>
T* ULevel::SpawnActor(Args&&... args)
{
	T* NewActor = new T(std::forward<Args>(args)...);

	LevelActors.push_back(NewActor);

	NewActor->BeginPlay();

	return NewActor;
}
