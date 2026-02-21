#pragma once
#include "Component/Public/SceneComponent.h"
#include "Physics/Public/BoundingVolume.h"
#include "Global/OverlapInfo.h"
#include "Core/Public/Delegate.h"
#include "Physics/Public/HitResult.h"

// Component-level overlap event signatures
DECLARE_DELEGATE(FComponentBeginOverlapSignature,
	UPrimitiveComponent*, /* OverlappedComponent */
	AActor*,              /* OtherActor */
	UPrimitiveComponent*, /* OtherComp */
	const FHitResult&     /* OverlapInfo */
);

DECLARE_DELEGATE(FComponentEndOverlapSignature,
	UPrimitiveComponent*, /* OverlappedComponent */
	AActor*,              /* OtherActor */
	UPrimitiveComponent*  /* OtherComp */
);

UCLASS()
class UPrimitiveComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UPrimitiveComponent, USceneComponent)


public:
	UPrimitiveComponent();

	void TickComponent(float DeltaTime) override;
	virtual void OnSelected() override;
	virtual void OnDeselected() override;

	const TArray<FNormalVertex>* GetVerticesData() const;
	const TArray<uint32>* GetIndicesData() const;
	ID3D11Buffer* GetVertexBuffer() const;
	ID3D11Buffer* GetIndexBuffer() const;
	uint32 GetNumVertices() const;
	uint32 GetNumIndices() const;

	const FRenderState& GetRenderState() const { return RenderState; }

	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology);
	D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;
	//void Render(const URenderer& Renderer) const override;

	bool IsVisible() const { return bVisible; }
	void SetVisibility(bool bVisibility) { bVisible = bVisibility; }

	bool CanPick() const { return bCanPick; }
	void SetCanPick(bool bInCanPick) { bCanPick = bInCanPick; }

	// === Overlap Events Control ===
	bool GetGenerateOverlapEvents() const { return bGenerateOverlapEvents; }
	void SetGenerateOverlapEvents(bool bGenerate) { bGenerateOverlapEvents = bGenerate; }

	// === Mobility Control ===
	EComponentMobility GetMobility() const { return Mobility; }
	void SetMobility(EComponentMobility InMobility) { Mobility = InMobility; }

	// === Overlap Update Control ===
	bool GetNeedsOverlapUpdate() const { return bNeedsOverlapUpdate; }
	void SetNeedsOverlapUpdate(bool bNeedsUpdate) { bNeedsOverlapUpdate = bNeedsUpdate; }


	FVector4 GetColor() const { return Color; }
	void SetColor(const FVector4& InColor) { Color = InColor; }

	// === Collision System (SOLID Separation) ===

	// Broad phase: Calculate world-space AABB for spatial queries (Octree, culling)
	virtual struct FBounds CalcBounds() const;

	// Narrow phase: Get collision shape for precise overlap testing
	// Note: Returns existing BoundingBox pointer (do not delete)
	virtual const IBoundingVolume* GetCollisionShape() const;

	// Legacy methods (kept for backward compatibility)
	virtual const IBoundingVolume* GetBoundingBox();
	void GetWorldAABB(FVector& OutMin, FVector& OutMax);

	// === Collision Event Delegates ===
	// Public so users can bind to these events
	FComponentBeginOverlapSignature OnComponentBeginOverlap;
	FComponentEndOverlapSignature OnComponentEndOverlap;

	// === Overlap Query API ===
	const TArray<FOverlapInfo>& GetOverlapInfos() const { return OverlappingComponents; }
	bool IsOverlappingComponent(const UPrimitiveComponent* OtherComp) const;
	bool IsOverlappingActor(const AActor* OtherActor) const;

	// === Overlap State Management API (for Level::UpdateAllOverlaps) ===
	void AddOverlapInfo(UPrimitiveComponent* OtherComp);
	void RemoveOverlapInfo(UPrimitiveComponent* OtherComp);

	// === Event Notification API (for Level::UpdateAllOverlaps) ===
	void NotifyComponentBeginOverlap(UPrimitiveComponent* OtherComp, const struct FHitResult& SweepResult);
	void NotifyComponentEndOverlap(UPrimitiveComponent* OtherComp);

	virtual void MarkAsDirty() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	// 데칼에 덮일 수 있는가
	bool bReceivesDecals = true;

	// 다른 곳에서 사용할 인덱스
	mutable int32 CachedAABBIndex = -1;
	mutable uint32 CachedFrame = 0;

protected:
	const TArray<FNormalVertex>* Vertices = nullptr;
	const TArray<uint32>* Indices = nullptr;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	uint32 NumVertices = 0;
	uint32 NumIndices = 0;

	FVector4 Color = FVector4{ 0.f,0.f,0.f,0.f };

	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FRenderState RenderState = {};

	bool bVisible = true;
	bool bCanPick = true;

	// Overlap event generation control (Unreal-style)
	// If false, this component will be skipped in Level::UpdateAllOverlaps()
	// DEFAULT: false (opt-in for overlap events, more efficient)
	bool bGenerateOverlapEvents = false;

	// Component mobility (Unreal-style)
	// Static: Don't move during gameplay (overlap with Static skipped, only with Movable)
	// Movable: Can move during gameplay (overlap checked every frame)
	// DEFAULT: Movable (for backward compatibility)
	EComponentMobility Mobility = EComponentMobility::Movable;

	mutable IBoundingVolume* BoundingBox = nullptr;
	bool bOwnsBoundingBox = false;
	
	mutable FVector CachedWorldMin;
	mutable FVector CachedWorldMax;
	mutable bool bIsAABBCacheDirty = true;

	// Overlap update tracking (성능 최적화)
	// MarkAsDirty() 호출 시 true로 설정, UpdateAllOverlaps() 검사 후 false로 초기화
	// Movable이어도 실제로 이동하지 않으면 overlap 재검사 불필요
	bool bNeedsOverlapUpdate = true;

	// Overlap tracking
	TArray<FOverlapInfo> OverlappingComponents;

public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;
};
