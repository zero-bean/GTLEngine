#pragma once
#include "LightComponent.h"
#include "LightManager.h"

// 방향성 라이트 (태양광 같은 평행광)
class UDirectionalLightComponent : public ULightComponent
{
public:
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)
	GENERATED_REFLECTION_BODY()

	UDirectionalLightComponent();
	virtual ~UDirectionalLightComponent() override;

public:
	// 월드 회전을 반영한 라이트 방향 반환 (Transform의 Forward 벡터)
	FVector GetLightDirection() const;

	// Light Info
	FDirectionalLightInfo GetLightInfo() const;

	// Shadow ViewProjection (updated by ShadowManager each frame)
	void SetLightViewProjection(const FMatrix& InViewProjection) { CachedLightViewProjection = InViewProjection; }
	const FMatrix& GetLightViewProjection() const { return CachedLightViewProjection; }

	// CSM (Cascaded Shadow Maps) Data
	void SetCascadeViewProjection(int CascadeIndex, const FMatrix& VP)
	{
		if (CascadeIndex >= 0 && CascadeIndex < 4)
		{
			CascadeViewProjections[CascadeIndex] = VP;
		}
	}

	const FMatrix& GetCascadeViewProjection(int CascadeIndex) const
	{
		static FMatrix Identity = FMatrix::Identity();
		if (CascadeIndex >= 0 && CascadeIndex < 4)
		{
			return CascadeViewProjections[CascadeIndex];
		}
		return Identity;
	}

	void SetCascadeSplitDistance(int CascadeIndex, float Distance)
	{
		if (CascadeIndex >= 0 && CascadeIndex < 4)
		{
			CascadeSplitDistances[CascadeIndex] = Distance;
		}
	}

	float GetCascadeSplitDistance(int CascadeIndex) const
	{
		if (CascadeIndex >= 0 && CascadeIndex < 4)
		{
			return CascadeSplitDistances[CascadeIndex];
		}
		return 0.0f;
	}

	// Virtual Interface
	void OnRegister(UWorld* InWorld) override;
	void OnUnregister() override;
	virtual void UpdateLightData() override;
	void OnTransformUpdated() override;

	// Serialization & Duplication
	virtual void OnSerialized() override;
	virtual void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(UDirectionalLightComponent)

	// Update Gizmo to match light properties
	void UpdateDirectionGizmo();

protected:
	// Direction Gizmo (shows light direction)
	class UGizmoArrowComponent* DirectionGizmo = nullptr;

	// Cached LightViewProjection matrix (updated by ShadowManager)
	FMatrix CachedLightViewProjection = FMatrix::Identity();

	// CSM Data (4 cascades)
	FMatrix CascadeViewProjections[4] = {
		FMatrix::Identity(),
		FMatrix::Identity(),
		FMatrix::Identity(),
		FMatrix::Identity()
	};
	float CascadeSplitDistances[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
