#pragma once
#include "Core/Public/Object.h"

class AAxis;
class AGizmo;
class AActor;
class UPrimitiveComponent;

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
	TArray<UPrimitiveComponent*> GetLevelPrimitiveComponents() const { return LevelPrimitiveComponents; }

	TArray<AActor*> GetEditorActors() const { return EditorActors; }
	TArray<UPrimitiveComponent*> GetEditorPrimitiveComponents() const { return EditorPrimitiveComponents; }

	void AddLevelPrimitiveComponent(AActor* Actor);
	void AddEditorPrimitiveComponent(AActor* Actor);

	template<typename T, typename... Args>
	T* SpawnActor(Args&&... args);
	template<typename T, typename... Args>
	T* SpawnEditorActor(Args&&... args);

	void SetSelectedActor(AActor* InActor) { SelectedActor = InActor; }
	AActor* GetSelectedActor() const { return SelectedActor; }
	AGizmo* GetGizmo() const { return Gizmo; };

private:
	wstring Name;
	TArray<AActor*> LevelActors;
	TArray<UPrimitiveComponent*> LevelPrimitiveComponents;

	TArray<AActor*> EditorActors;
	TArray<UPrimitiveComponent*> EditorPrimitiveComponents;

	AActor* SelectedActor = nullptr;
	AGizmo* Gizmo = nullptr;
	AAxis* Axis = nullptr;
};

template <typename T, typename ... Args>
T* ULevel::SpawnActor(Args&&... args)
{
	T* NewActor = new T(std::forward<Args>(args)...);

	LevelActors.push_back(NewActor);
	NewActor->BeginPlay();

	return NewActor;
}

template <typename T, typename ... Args>
T* ULevel::SpawnEditorActor(Args&&... args)
{
	T* NewActor = new T(std::forward<Args>(args)...);

	EditorActors.push_back(NewActor);
	NewActor->BeginPlay();

	return NewActor;
}
