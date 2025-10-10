#pragma once
#include "Core/Public/Object.h"
#include "Global/Types.h"
#include "Global/Matrix.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/BatchLines.h"
#include "Editor/Public/ObjectPicker.h"
#include "Physics/Public/AABB.h"

class UStaticMesh;
struct FBVHNode
{
	FAABB Bounds;
	int LeftChild = -1;
	int RightChild = -1;
	int Start = 0;   // leaf start index
	int Count = 0;   // leaf count
	bool bIsLeaf = false;

	uint32 FrustumMask = 0;
};

struct TriBVHNode {
	FAABB Bounds;
	int LeftChild;    // -1 if leaf
	int RightChild;   // -1 if leaf
	int Start;        // index into triangle array
	int Count;        // number of triangles in leaf
	bool bIsLeaf;
};

struct FBVHPrimitive
{
	FVector Center;
	FAABB Bounds;
	TObjectPtr<UPrimitiveComponent> Primitive;
	FMatrix WorldToModel;
	EPrimitiveType PrimitiveType = EPrimitiveType::Cube;
	UStaticMesh* StaticMesh = nullptr;
};

class FFrustumCull;

class UBVHierarchy : UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UBVHierarchy, UObject)

public:
	void Initialize();

	void Build(const TArray<FBVHPrimitive>& InPrimitives, int MaxLeafSize = 5);
	// void QueryFrustum(const Frustum& frustum, TArray<int>& outVisible) const;
	bool Raycast(const FRay& InRay, UPrimitiveComponent*& HitComponent, float& HitT) const;
	void Refit();
	bool IsDebugDrawEnabled() const { return bDebugDrawEnabled; }
	void ConvertComponentsToBVHPrimitives(const TArray<TObjectPtr<UPrimitiveComponent>>& InComponents, TArray<FBVHPrimitive>& OutPrimitives);
	[[nodiscard]] const TArray<FBVHNode>& GetNodes() const { return Nodes; }
	void FrustumCull(FFrustumCull& InFrustum, TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents);

	TArray<FAABB>& GetBoxes() { return Boxes; }

private:
	int BuildRecursive(int Start, int Count, int MaxLeafSize);
	FAABB RefitRecursive(int NodeIndex);
	// void QueryRecursive(int nodeIdx, const Frustum& frustum, TArray<int>& outVisible) const;
	void RaycastIterative(const FRay& InRay, float& OutClosestHit, int& OutHitObject) const;
	void RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const;
	void CollectNodeBounds(TArray<FAABB>& OutBounds) const;
	void TraverseForCulling(uint32 NodeIndex, FFrustumCull& InFrustum, uint32 InMask, TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents);
	void AddAllPrimitives(uint32 NodeIndex, TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents);

	UObjectPicker ObjectPicker;

	TArray<FBVHNode> Nodes;
	TArray<FBVHPrimitive> Primitives;
	int RootIndex = -1;
	bool bDebugDrawEnabled = true;

	TArray<FAABB> Boxes;
};

