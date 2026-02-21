#include "pch.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Global/OverlapInfo.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"
#include "Physics/Public/OBB.h"
#include "Physics/Public/BoundingSphere.h"
#include "Physics/Public/Capsule.h"
#include "Physics/Public/Bounds.h"
#include "Physics/Public/CollisionHelper.h"
#include "Utility/Public/JsonSerializer.h"
#include "Actor/Public/Actor.h"
#include "Level/Public/Level.h"
#include "Level/Public/World.h"
#include "Global/Octree.h"
#include <unordered_set>

IMPLEMENT_ABSTRACT_CLASS(UPrimitiveComponent, USceneComponent)

UPrimitiveComponent::UPrimitiveComponent()
{
	bCanEverTick = true;
}

void UPrimitiveComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    // Note: Overlap updates are now managed centrally by Level::UpdateAllOverlaps()
    // No per-component tick logic needed
}
void UPrimitiveComponent::OnSelected()
{
	SetColor({ 1.f, 0.8f, 0.2f, 0.4f });
}

void UPrimitiveComponent::OnDeselected()
{
	SetColor({ 0.f, 0.f, 0.f, 0.f });
}

void USceneComponent::SetUniformScale(bool bIsUniform)
{
	bIsUniformScale = bIsUniform;
}

bool USceneComponent::IsUniformScale() const
{
	return bIsUniformScale;
}

const TArray<FNormalVertex>* UPrimitiveComponent::GetVerticesData() const
{
	return Vertices;
}

const TArray<uint32>* UPrimitiveComponent::GetIndicesData() const
{
	return Indices;
}

ID3D11Buffer* UPrimitiveComponent::GetVertexBuffer() const
{
	return VertexBuffer;
}

ID3D11Buffer* UPrimitiveComponent::GetIndexBuffer() const
{
	return IndexBuffer;
}

uint32 UPrimitiveComponent::GetNumVertices() const
{
	return NumVertices;
}

uint32 UPrimitiveComponent::GetNumIndices() const
{
	return NumIndices;
}

void UPrimitiveComponent::SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
	Topology = InTopology;
}

D3D11_PRIMITIVE_TOPOLOGY UPrimitiveComponent::GetTopology() const
{
	return Topology;
}

// === Collision System (SOLID Separation) ===

FBounds UPrimitiveComponent::CalcBounds() const
{
	// Default: Use GetWorldAABB (subclasses should override for efficiency)
	FVector WorldMin, WorldMax;
	const_cast<UPrimitiveComponent*>(this)->GetWorldAABB(WorldMin, WorldMax);
	return FBounds(WorldMin, WorldMax);
}

const IBoundingVolume* UPrimitiveComponent::GetCollisionShape() const
{
	// Update collision shape to world space before returning
	if (BoundingBox)
	{
		BoundingBox->Update(GetWorldTransformMatrix());
	}
	return BoundingBox;
}

const IBoundingVolume* UPrimitiveComponent::GetBoundingBox()
{
	// Legacy method - kept for backward compatibility with Octree, culling, etc.
	if (BoundingBox)
	{
		BoundingBox->Update(GetWorldTransformMatrix());
	}
	return BoundingBox;
}

void UPrimitiveComponent::GetWorldAABB(FVector& OutMin, FVector& OutMax)
{
	// Keep existing implementation for Octree and other systems
	if (!BoundingBox)
	{
		OutMin = FVector(); OutMax = FVector();
		return;
	}

	if (bIsAABBCacheDirty)
	{
		if (BoundingBox->GetType() == EBoundingVolumeType::AABB)
		{
			const FAABB* LocalAABB = static_cast<const FAABB*>(BoundingBox);
			FVector LocalCorners[8] =
			{
				FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Min.Z),
				FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Min.Z),
				FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Max.Z),
				FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Max.Z)
			};

			const FMatrix& WorldTransform = GetWorldTransformMatrix();
			FVector WorldMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
			FVector WorldMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			for (int32 Idx = 0; Idx < 8; Idx++)
			{
				FVector4 WorldCorner = FVector4(LocalCorners[Idx].X, LocalCorners[Idx].Y, LocalCorners[Idx].Z, 1.0f) * WorldTransform;
				WorldMin.X = min(WorldMin.X, WorldCorner.X);
				WorldMin.Y = min(WorldMin.Y, WorldCorner.Y);
				WorldMin.Z = min(WorldMin.Z, WorldCorner.Z);
				WorldMax.X = max(WorldMax.X, WorldCorner.X);
				WorldMax.Y = max(WorldMax.Y, WorldCorner.Y);
				WorldMax.Z = max(WorldMax.Z, WorldCorner.Z);
			}

			CachedWorldMin = WorldMin;
			CachedWorldMax = WorldMax;
		}
		else if (BoundingBox->GetType() == EBoundingVolumeType::OBB ||
			BoundingBox->GetType() == EBoundingVolumeType::SpotLight)
		{
			const FOBB* OBB = static_cast<const FOBB*>(BoundingBox);
			FAABB AABB = OBB->ToWorldAABB();

			CachedWorldMin = AABB.Min;
			CachedWorldMax = AABB.Max;
		}
		else
		{
			// For other types (Sphere, Capsule, etc), use CalcBounds()
			// These are primarily shape components used for collision, not spatial partitioning
			FBounds Bounds = CalcBounds();
			CachedWorldMin = Bounds.Min;
			CachedWorldMax = Bounds.Max;
		}

		bIsAABBCacheDirty = false;
	}

	OutMin = CachedWorldMin;
	OutMax = CachedWorldMax;
}

void UPrimitiveComponent::MarkAsDirty()
{
	bIsAABBCacheDirty = true;
	bNeedsOverlapUpdate = true;  // 이동했으므로 overlap 재검사 필요 (Unreal-style)
	Super::MarkAsDirty();

	// Update octree position immediately (required for rendering/culling/picking)
	AActor* Owner = GetOwner();
	if (Owner && Owner->GetOuter())
	{
		ULevel* Level = Cast<ULevel>(Owner->GetOuter());
		if (Level)
		{
			Level->UpdatePrimitiveInOctree(this);
		}
	}

	// Note: Overlap updates are now managed centrally by Level::UpdateAllOverlaps()
	// called once per frame in World::Tick()
}


UObject* UPrimitiveComponent::Duplicate()
{
	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Super::Duplicate());

	PrimitiveComponent->Color = Color;
	PrimitiveComponent->Topology = Topology;
	PrimitiveComponent->RenderState = RenderState;
	PrimitiveComponent->bVisible = bVisible;
	PrimitiveComponent->bCanPick = bCanPick;
	PrimitiveComponent->bGenerateOverlapEvents = bGenerateOverlapEvents;
	PrimitiveComponent->Mobility = Mobility;  // Mobility 복제
	PrimitiveComponent->bNeedsOverlapUpdate = true;  // 복제된 컴포넌트는 overlap 체크 필요
	PrimitiveComponent->bReceivesDecals = bReceivesDecals;

	PrimitiveComponent->Vertices = Vertices;
	PrimitiveComponent->Indices = Indices;
	PrimitiveComponent->VertexBuffer = VertexBuffer;
	PrimitiveComponent->IndexBuffer = IndexBuffer;
	PrimitiveComponent->NumVertices = NumVertices;
	PrimitiveComponent->NumIndices = NumIndices;

	if (!bOwnsBoundingBox)
	{
		PrimitiveComponent->BoundingBox = BoundingBox;
	}

	return PrimitiveComponent;
}

void UPrimitiveComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);

}
void UPrimitiveComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FString VisibleString;
		FJsonSerializer::ReadString(InOutHandle, "bVisible", VisibleString, "true");
		SetVisibility(VisibleString == "true");

		FString GenerateOverlapString;
		FJsonSerializer::ReadString(InOutHandle, "bGenerateOverlapEvents", GenerateOverlapString, "false");
		SetGenerateOverlapEvents(GenerateOverlapString == "true");

		// Mobility 로드
		FString MobilityString;
		FJsonSerializer::ReadString(InOutHandle, "Mobility", MobilityString, "Movable");
		if (MobilityString == "Static")
			Mobility = EComponentMobility::Static;
		else
			Mobility = EComponentMobility::Movable;
	}
	else
	{
		InOutHandle["bVisible"] = bVisible ? "true" : "false";
		InOutHandle["bGenerateOverlapEvents"] = bGenerateOverlapEvents ? "true" : "false";
		InOutHandle["Mobility"] = (Mobility == EComponentMobility::Static) ? "Static" : "Movable";
	}

}

// === Overlap Query Implementation ===

bool UPrimitiveComponent::IsOverlappingComponent(const UPrimitiveComponent* OtherComp) const
{
	if (!OtherComp)
		return false;

	for (const FOverlapInfo& Info : OverlappingComponents)
	{
		if (Info.OverlapComponent.Get() == OtherComp)
			return true;
	}
	return false;
}

bool UPrimitiveComponent::IsOverlappingActor(const AActor* OtherActor) const
{
	if (!OtherActor)
		return false;

	for (const FOverlapInfo& Info : OverlappingComponents)
	{
		if (Info.IsValid() && Info.GetActor() == OtherActor)
			return true;
	}
	return false;
}

// === Overlap State Management API ===
// Note: These are public APIs for Level::UpdateAllOverlaps() to manage overlap state

void UPrimitiveComponent::AddOverlapInfo(UPrimitiveComponent* OtherComp)
{
	if (!OtherComp)
		return;

	OverlappingComponents.Add(FOverlapInfo(OtherComp));
}

void UPrimitiveComponent::RemoveOverlapInfo(UPrimitiveComponent* OtherComp)
{
	if (!OtherComp)
		return;

	OverlappingComponents.RemoveAll([OtherComp](const FOverlapInfo& Info) {
		return Info.OverlapComponent.Get() == OtherComp;
	});
}

// === Event Notification API ===
// Note: Overlap detection is now managed centrally by Level::UpdateAllOverlaps()

void UPrimitiveComponent::NotifyComponentBeginOverlap(UPrimitiveComponent* OtherComp, const FHitResult& SweepResult)
{
	if (!OtherComp)
		return;

	AActor* MyOwner = GetOwner();
	AActor* OtherOwner = OtherComp->GetOwner();

	// Broadcast component-level event (single-direction)
	// Note: Level::UpdateAllOverlaps() calls this for both components
	UE_LOG("BeginOverlap: [%s]%s overlaps with [%s]%s",
		MyOwner ? MyOwner->GetName().ToString().c_str() : "None",
		GetName().ToString().c_str(),
		OtherOwner ? OtherOwner->GetName().ToString().c_str() : "None",
		OtherComp->GetName().ToString().c_str());
	OnComponentBeginOverlap.Broadcast(this, OtherOwner, OtherComp, SweepResult);

	// Broadcast actor-level event (single-direction)
	if (MyOwner)
	{
		MyOwner->OnActorBeginOverlap.Broadcast(MyOwner, OtherOwner);
	}
}

void UPrimitiveComponent::NotifyComponentEndOverlap(UPrimitiveComponent* OtherComp)
{
	if (!OtherComp)
		return;

	AActor* MyOwner = GetOwner();
	AActor* OtherOwner = OtherComp->GetOwner();

	// Broadcast component-level event (single-direction)
	// Note: Level::UpdateAllOverlaps() calls this for both components
	UE_LOG("EndOverlap: [%s]%s stopped overlapping with [%s]%s",
		MyOwner ? MyOwner->GetName().ToString().c_str() : "None",
		GetName().ToString().c_str(),
		OtherOwner ? OtherOwner->GetName().ToString().c_str() : "None",
		OtherComp->GetName().ToString().c_str());
	OnComponentEndOverlap.Broadcast(this, OtherOwner, OtherComp);

	// Broadcast actor-level event (single-direction)
	if (MyOwner)
	{
		MyOwner->OnActorEndOverlap.Broadcast(MyOwner, OtherOwner);
	}
}
