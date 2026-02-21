#pragma once
#include "Core/Public/Object.h"

#include "Editor/Public/Camera.h"

class AAxis;
class AGizmo;
class AGrid;
class AActor;
class UPrimitiveComponent;

class ULevel :
	public UObject
{
public:
	ULevel();
	ULevel(const FString& InName);
	~ULevel() override;

	virtual void Init();
	virtual void Update();
	virtual void Render();
	virtual void Cleanup();

	TArray<AActor*> GetLevelActors() const { return LevelActors; }
	TArray<UPrimitiveComponent*> GetLevelPrimitiveComponents() const { return LevelPrimitiveComponents; }
	//Deprecated : EditorPrimitive는 에디터에서 처리
	//TArray<AActor*> GetEditorActors() const { return EditorActors; }
	//TArray<UPrimitiveComponent*> GetEditorPrimitiveComponents() const { return EditorPrimitiveComponents; }

	void AddLevelPrimitiveComponent(AActor* Actor);
	//void AddEditorPrimitiveComponent(AActor* Actor);

	template<typename T, typename... Args>
	T* SpawnActor(const FString& InName = "");
	// template<typename T, typename... Args>
	// T* SpawnEditorActor(Args&&... args);

	// Actor 삭제
	bool DestroyActor(AActor* InActor);
	void MarkActorForDeletion(AActor* InActor); // 지연 삭제를 위한 마킹

	void SetSelectedActor(AActor* InActor);
	AActor* GetSelectedActor() const { return SelectedActor; }
	AGizmo* GetGizmo() const { return Gizmo; }

	void SetCamera(UCamera* InCamera) { CameraPtr = InCamera; }
	UCamera* GetCamera() const { return CameraPtr; }

private:
	TArray<AActor*> LevelActors;
	TArray<UPrimitiveComponent*> LevelPrimitiveComponents;

	//Deprecated : EditorPrimitive는 에디터에서 처리
	//TArray<AActor*> EditorActors;
	//TArray<UPrimitiveComponent*> EditorPrimitiveComponents;

	// 지연 삭제를 위한 리스트
	TArray<AActor*> ActorsToDelete;

	AActor* SelectedActor = nullptr;
	AGizmo* Gizmo = nullptr;
	AAxis* Axis = nullptr;
	AGrid* Grid = nullptr;
	//////////////////////////////////////////////////////////////////////////
	// TODO(PYB): Editor 제작되면 해당 클래스에 존재하는 카메라 관련 코드 제거
	//////////////////////////////////////////////////////////////////////////
	UCamera* CameraPtr = nullptr;

	// 지연 삭제 처리 함수
	void ProcessPendingDeletions();
};

template <typename T, typename ... Args>
T* ULevel::SpawnActor(const FString& InName)
{
	T* NewActor = new T();

	///////////////////////////////////////////
	NewActor->AddMemoryUsage(sizeof(T));
	///생성자에서 자신의 메모리 설정하게 수정 필요///
	NewActor->SetOuter(this);
	//Outer 설정 시 Outer의 메모리 카운트에 자신의 메모리 합산 작업 수행

	LevelActors.push_back(NewActor);
	NewActor->BeginPlay();

	return NewActor;
}

//Deprecated : EditorPrimitive는 에디터에서 처리
//template <typename T, typename ... Args>
//T* ULevel::SpawnEditorActor(Args&&... InArgs)
//{
//	T* NewActor = new T(std::forward<Args>(InArgs)...);
//
//	EditorActors.push_back(NewActor);
//	NewActor->BeginPlay();
//
//	return NewActor;
//}
