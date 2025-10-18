#pragma once
#include "Widget.h"
#include "../../Vector.h"
#include "../../Enums.h"

class UUIManager;
class UWorld;
class AActor;
class AGizmoActor;
class UPointLightComponent;
class UDecalComponent;
class URotationMovementComponent;
class UProjectileMovementComponent;
class USpotLightComponent;
class UDirectionalLightComponent;

class UTargetActorTransformWidget
	: public UWidget
{
public:
	DECLARE_CLASS(UTargetActorTransformWidget, UWidget)

	// Special Member Function
	UTargetActorTransformWidget();
	~UTargetActorTransformWidget() override;

	void Initialize() override;
	void Update() override;
	void DuplicateTarget(AActor* SelectedActor) const;
	void RenderWidget() override;
	void PostProcess() override;

	void UpdateTransformFromActor();
	void ApplyTransformToComponent(USceneComponent* SelectedComponent);

	void RenderComponentHierarchy(USceneComponent* SceneComponent);

	// 선택된 액터가 외부에서 삭제되었을 때 호출되어 내부 상태를 정리
	void OnSelectedActorCleared();

private:
	UUIManager* UIManager = nullptr;
	USelectionManager* SelectionManager = nullptr;
	//AActor* SelectedActor = nullptr;
	//USceneComponent* SelectedComponent = nullptr;	// 현재 선택된 SceneComponent를 저장할 포인터
	FString CachedActorName; // 액터 이름 캐시 (안전한 출력을 위해)

	// Transform UI 상태
	FVector EditLocation = {0.0f, 0.0f, 0.0f};
	FVector EditRotation = {0.0f, 0.0f, 0.0f};
	FVector EditScale = {1.0f, 1.0f, 1.0f};

	// 이전 UI 값 저장 (Quat↔Euler 변환으로 인한 값 튐 방지)
	FVector PrevEditRotation = {0.0f, 0.0f, 0.0f};

	// 마지막으로 읽어온 컴포넌트 (선택이 바뀔 때만 Quat에서 Euler 읽기)
	USceneComponent* LastReadComponent = nullptr;

	// UI 변경 플래그
	bool bScaleChanged = false;
	bool bRotationChanged = false;
	bool bPositionChanged = false;
	bool bUniformScale = false;
	
	// 헬퍼 메서드
	void ResetChangeFlags();

	// Render component details func.
	// TODO: define UComponentDetailsWidget class, add to ComponentWidgetRegistry (TMap)
	void RenderExponentialHeightFogComponentDetails(UExponentialHeightFogComponent* InComponent);

	/* *
	* @brief StaticMeshComponent의 제어를 담당 및 렌더하는 함수입니다.
	* @function - RenderStaticMeshSelector: 스태틱 메시 선택 UI를 렌더링합니다.
	* @function - RenderMaterialSlots: 머티리얼 슬롯 목록 UI를 렌더링합니다.
	* @function - RenderNormalMapSelector:머티리얼의 노멀 맵 선택 UI를 렌더링합니다.
	*/
	void RenderStaticMeshComponentDetails(UStaticMeshComponent* InComponent);
	void RenderStaticMeshSelector(UStaticMeshComponent* InComponent);
	void RenderMaterialSlots(UStaticMeshComponent* InComponent);
	void RenderNormalMapSelector(UStaticMeshComponent* InComponent, int32 MaterialSlotIndex);

	void RenderBillboardComponentDetails(UBillboardComponent* InComponent);
	void RenderTextRenderComponentDetails(UTextRenderComponent* InComponent);
	void RenderPointLightComponentDetails(UPointLightComponent* InComponent);
	void RenderDecalComponentDetails(UDecalComponent* InComponent);
	void RenderRotationMovementComponentDetails(URotationMovementComponent* InComponent);
	void RenderProjectileMovementComponentDetails(UProjectileMovementComponent* InComponent);
	void RenderFXAAComponentDetails(UFXAAComponent* InComponent);
	void RenderSpotLightComponentDetails(USpotLightComponent* InComponent);
	void RenderDirectionalLightComponentDetails(UDirectionalLightComponent* InComponent);
};
