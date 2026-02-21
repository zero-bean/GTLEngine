#include "pch.h"
#include "Editor/Public/BatchLines.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Global/Octree.h"
#include "Component/Public/DecalSpotLightComponent.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Runtime/Engine/Public/SkeletalMesh.h"
#include "Runtime/Engine/Public/ReferenceSkeleton.h"
#include "Level/Public/Level.h"
#include "Physics/Public/OBB.h"
#include "Render/HitProxy/Public/HitProxy.h"

IMPLEMENT_CLASS(UBatchLines, UObject)

UBatchLines::UBatchLines() : Grid(), BoundingBoxLines()
{
	Vertices.Reserve(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());
	Vertices.SetNum(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());

	Grid.MergeVerticesAt(Vertices, 0);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());

	SetIndices();

	ID3D11VertexShader* VertexShader;
	ID3D11InputLayout* InputLayout;
	TArray<D3D11_INPUT_ELEMENT_DESC> Layout = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/BatchLineShader.hlsl", Layout, &VertexShader, &InputLayout);
	ID3D11PixelShader* PixelShader;
	FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/BatchLineShader.hlsl", &PixelShader);

	Primitive.VertexShader = VertexShader;
	Primitive.InputLayout = InputLayout;
	Primitive.PixelShader = PixelShader;
	Primitive.NumVertices = static_cast<uint32>(Vertices.Num());
	Primitive.NumIndices = static_cast<uint32>(Indices.Num());
	Primitive.VertexBuffer = FRenderResourceFactory::CreateVertexBuffer(Vertices.GetData(), Primitive.NumVertices * sizeof(FVector), true);
	Primitive.IndexBuffer = FRenderResourceFactory::CreateIndexBuffer(Indices.GetData(), Primitive.NumIndices * sizeof(uint32));
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
}

UBatchLines::~UBatchLines()
{
	SafeRelease(Primitive.InputLayout);
	SafeRelease(Primitive.VertexShader);
	SafeRelease(Primitive.PixelShader);
	SafeRelease(Primitive.VertexBuffer);
	SafeRelease(Primitive.IndexBuffer);
}

void UBatchLines::UpdateUGridVertices(const float newCellSize)
{
	if (newCellSize == Grid.GetCellSize()) { return; }
	Grid.UpdateVerticesBy(newCellSize);
	bChangedVertices = true;
}

void UBatchLines::UpdateBoundingBoxVertices(const IBoundingVolume* NewBoundingVolume)
{
	if (NewBoundingVolume && NewBoundingVolume->GetType() != EBoundingVolumeType::SpotLight)
	{
		bRenderSpotLight = false;
	}

	BoundingBoxLines.UpdateVertices(NewBoundingVolume);
	bChangedVertices = true;
}


void UBatchLines::UpdateOctreeVertices(const FOctree* InOctree)
{
	OctreeLines.Empty();
	if (InOctree)
	{
		TraverseOctree(InOctree);
	}
	bChangedVertices = true;
}

void UBatchLines::UpdateDecalSpotLightVertices(UDecalSpotLightComponent* SpotLightComponent)
{
	if (!SpotLightComponent)
	{
		bRenderSpotLight = false;
		return;
	}

	// GetBoundingBox updates the underlying volume, so we need non-const access.
	const FSpotLightOBB* SpotLightBounding = SpotLightComponent ? SpotLightComponent->GetSpotLightBoundingBox() : nullptr;
	if (!SpotLightBounding)
	{
		bRenderSpotLight = false;
		return;
	}

	SpotLightLines.UpdateVertices(SpotLightBounding);
	bRenderSpotLight = true;
	bChangedVertices = true;
}

void UBatchLines::UpdateConeVertices(const FVector& InCenter, float InGeneratingLineLength
	, float InOuterHalfAngleRad, float InInnerHalfAngleRad, FQuaternion InRotation)
{
	// SpotLight는 scale의 영향을 받지 않으므로 world transformation matrix 직접 계산
	FMatrix TranslationMat = FMatrix::TranslationMatrix(InCenter);
	FMatrix ScaleMat = FMatrix::ScaleMatrix(FVector(InGeneratingLineLength, InGeneratingLineLength, InGeneratingLineLength));
	FMatrix RotationMat = InRotation.ToRotationMatrix();

	if (InGeneratingLineLength <= 0.0f)
	{
		bRenderSpotLight = false;
		return;
	}

	constexpr uint32 NumSegments = 40;
	const float CosOuter = cosf(InOuterHalfAngleRad);
	const float SinOuter = sinf(InOuterHalfAngleRad);
	const float CosInner = InInnerHalfAngleRad > MATH_EPSILON ? cosf(InInnerHalfAngleRad) : 0.0f;
	const float SinInner = InInnerHalfAngleRad > MATH_EPSILON ? sinf(InInnerHalfAngleRad) : 0.0f;

	constexpr float BaseSegmentAngle = (2.0f * PI) / static_cast<float>(NumSegments);
	const float ArcSegmentAngle = 2.0f * InOuterHalfAngleRad / static_cast<float>(NumSegments);
	const bool bHasInnerCone = InInnerHalfAngleRad > MATH_EPSILON;

	TArray<FVector> LocalVertices;
	LocalVertices.Reserve(1 + NumSegments + (bHasInnerCone ? NumSegments : 0));

	// x, y 평면 위 호 버텍스
	for (uint32 Segment = 0; Segment <= NumSegments; ++Segment)
	{
		const float Angle = -InOuterHalfAngleRad + ArcSegmentAngle * static_cast<float>(Segment);

		LocalVertices.Emplace(cosf(Angle), sinf(Angle), 0.0f);
	}

	// z, x 평면 위 호 버텍스
	for (uint32 Segment = 0; Segment <= NumSegments; ++Segment)
	{
		const float Angle = -InOuterHalfAngleRad + ArcSegmentAngle * static_cast<float>(Segment);

		LocalVertices.Emplace(cosf(Angle), 0.0f, sinf(Angle));
	}

	LocalVertices.Emplace(0.0f, 0.0f, 0.0f); // Apex

	// 외곽 원 버텍스
	for (uint32 Segment = 0; Segment < NumSegments; ++Segment)
	{
		const float Angle = BaseSegmentAngle * static_cast<float>(Segment);
		const float CosValue = cosf(Angle);
		const float SinValue = sinf(Angle);

		LocalVertices.Emplace(CosOuter, SinOuter * CosValue, SinOuter * SinValue);
	}

	// 내곽 원 버텍스 (있을 경우)
	if (bHasInnerCone)
	{
		for (uint32 Segment = 0; Segment < NumSegments; ++Segment)
		{
			const float Angle = BaseSegmentAngle * static_cast<float>(Segment);
			const float CosValue = cosf(Angle);
			const float SinValue = sinf(Angle);

			LocalVertices.Emplace(CosInner, SinInner * CosValue, SinInner * SinValue);
		}
	}

	FMatrix WorldMatrix = ScaleMat;
	WorldMatrix *= RotationMat;
	WorldMatrix *= TranslationMat;

	TArray<FVector> WorldVertices(LocalVertices.Num());
	for (int32 Index = 0; Index < LocalVertices.Num(); ++Index)
	{
		WorldVertices[Index] = WorldMatrix.TransformPosition(LocalVertices[Index]);
	}

	SpotLightLines.UpdateSpotLightVertices(WorldVertices);
	bRenderSpotLight = true;
	bChangedVertices = true;
}

void UBatchLines::UpdateBonePyramidVertices(USkeletalMeshComponent* SkeletalMeshComponent, int32 InBoneIndex)
{
	BonePyramidVertices.Empty();
	BonePyramidIndices.Empty();

	if (!SkeletalMeshComponent)
	{
		bRenderBonePyramids = false;
		bChangedVertices = true;
		return;
	}

	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!SkeletalMesh)
	{
		bRenderBonePyramids = false;
		bChangedVertices = true;
		return;
	}

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	const int32 NumBones = RefSkeleton.GetRawBoneNum();

	if (NumBones == 0 || !RefSkeleton.IsValidRawIndex(InBoneIndex))
	{
		bRenderBonePyramids = false;
		bChangedVertices = true;
		return;
	}

	// ComponentSpaceTransforms이 정상적으로 갱신되어 있는지 확인
	if(SkeletalMeshComponent->IsSkinningDirty() == false)
	{
		SkeletalMeshComponent->RefreshBoneTransforms();
	}

	// 사각뿔 크기 (본 간 거리에 비례하여 조정)
	constexpr float BaseSizeRatio = 0.1f;

	uint32 CurrentVertexIndex = 0;

	// 1. 주황색 피라미드: 부모 본 -> InBoneIndex 본
	int32 ParentIndex = RefSkeleton.GetRawParentIndex(InBoneIndex);
	const TArray<FTransform>& ComponentSpaceTransforms = SkeletalMeshComponent->GetComponentSpaceTransforms();
	FVector WorldPosition = SkeletalMeshComponent->GetWorldLocation();
	if (ParentIndex != INDEX_NONE)
	{
		FVector ParentPos = SkeletalMeshComponent->GetWorldTransformMatrix().TransformPosition(ComponentSpaceTransforms[ParentIndex].Translation);
		FVector CurrentPos = SkeletalMeshComponent->GetWorldTransformMatrix().TransformPosition(ComponentSpaceTransforms[InBoneIndex].Translation);
		float Distance = (CurrentPos - ParentPos).Length();
		float BaseSize = Distance * BaseSizeRatio;

		HighlightedIndexStart = BonePyramidIndices.Num();
		HighlightedIndexCount = 16; // 피라미드 라인 수

		CreatePyramid(ParentPos, CurrentPos, BaseSize, FVector4(1.0f, 0.5f, 0.0f, 1.0f), // Orange
			BonePyramidVertices, BonePyramidIndices, CurrentVertexIndex);
		CurrentVertexIndex += 5; // 피라미드는 5개의 정점 (base 4 + tip 1)
	}
	else
	{
		HighlightedIndexStart = 0;
		HighlightedIndexCount = 0;
	}

	// 2. 초록색 피라미드: InBoneIndex 본 -> 각 자식 본
	for (int32 ChildIndex = 0; ChildIndex < NumBones; ++ChildIndex)
	{
		if (RefSkeleton.GetRawParentIndex(ChildIndex) == InBoneIndex)
		{
			FVector CurrentPos = ComponentSpaceTransforms[InBoneIndex].Translation + WorldPosition;
			FVector ChildPos = ComponentSpaceTransforms[ChildIndex].Translation + WorldPosition;
			float Distance = (ChildPos - CurrentPos).Length();
			float BaseSize = Distance * BaseSizeRatio;

			CreatePyramid(CurrentPos, ChildPos, BaseSize, FVector4(0.0f, 1.0f, 0.0f, 1.0f), // Green
				BonePyramidVertices, BonePyramidIndices, CurrentVertexIndex);
			CurrentVertexIndex += 5;
		}
	}

	bRenderBonePyramids = (BonePyramidVertices.Num() > 0);
	bChangedVertices = true;
}

void UBatchLines::UpdateAllBonePyramidVertices(USkeletalMeshComponent* SkeletalMeshComponent, int32 InBoneIndex)
{
	BonePyramidVertices.Empty();
	BonePyramidIndices.Empty();

	if (!SkeletalMeshComponent)
	{
		bRenderBonePyramids = false;
		bChangedVertices = true;
		return;
	}

	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!SkeletalMesh)
	{
		bRenderBonePyramids = false;
		bChangedVertices = true;
		return;
	}

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	const int32 NumBones = RefSkeleton.GetRawBoneNum();

	if (NumBones == 0)
	{
		bRenderBonePyramids = false;
		bChangedVertices = true;
		return;
	}

	// ComponentSpaceTransforms이 정상적으로 갱신되어 있는지 확인
	if (SkeletalMeshComponent->IsSkinningDirty() == false)
	{
		SkeletalMeshComponent->RefreshBoneTransforms();
	}

	const TArray<FTransform>& ComponentSpaceTransforms = SkeletalMeshComponent->GetComponentSpaceTransforms();
	const FMatrix& ComponentWorldMatrix = SkeletalMeshComponent->GetWorldTransformMatrix();

	// 사각뿔 크기 (본 간 거리에 비례하여 조정)
	constexpr float BaseSizeRatio = 0.1f;

	uint32 CurrentVertexIndex = 0;

	// 모든 본에 대해 피라미드 생성
	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		int32 ParentIndex = RefSkeleton.GetRawParentIndex(BoneIndex);

		// 루트 본(부모가 없는 본)은 건너뜀
		if (ParentIndex == INDEX_NONE)
		{
			HighlightedIndexStart = 0;
			HighlightedIndexCount = 0;
			continue;
		}

		// 부모 본 -> 현재 본으로 이어지는 피라미드 생성
		FVector ParentPos = ComponentWorldMatrix.TransformPosition(ComponentSpaceTransforms[ParentIndex].Translation);
		FVector CurrentPos = ComponentWorldMatrix.TransformPosition(ComponentSpaceTransforms[BoneIndex].Translation);

		float Distance = (CurrentPos - ParentPos).Length();

		// 너무 짧은 본은 건너뜀 (거리가 거의 0인 경우)
		if (Distance < MATH_EPSILON)
		{
			continue;
		}

		float BaseSize = Distance * BaseSizeRatio;

		if(BoneIndex == InBoneIndex)
		{
			HighlightedIndexStart = BonePyramidIndices.Num();
			HighlightedIndexCount = 16; // 피라미드 라인 수
		}

		// 모든 본을 흰색으로 렌더링 (또는 원하는 색상으로 변경 가능)
		CreatePyramid(ParentPos, CurrentPos, BaseSize, FVector4(1.0f, 1.0f, 1.0f, 1.0f), // White
			BonePyramidVertices, BonePyramidIndices, CurrentVertexIndex);
		CurrentVertexIndex += 5; // 피라미드는 5개의 정점 (base 4 + tip 1)
	}

	bRenderBonePyramids = (BonePyramidVertices.Num() > 0);
	bChangedVertices = true;
}

void UBatchLines::ClearBonePyramids()
{
	BonePyramidVertices.Empty();
	BonePyramidIndices.Empty();
	bRenderBonePyramids = false;
	bChangedVertices = true;
}

void UBatchLines::CreatePyramid(const FVector& Base, const FVector& Tip, float BaseSize, const FVector4& Color,
	TArray<FVector>& OutVertices, TArray<uint32>& OutIndices, uint32 BaseVertexIndex)
{
	// 방향 벡터 계산
	FVector Direction = (Tip - Base).GetNormalized();
	
	// Base 평면을 위한 직교 벡터 생성
	FVector Up = (fabsf(Direction.Z) < 0.999f) ? FVector(0, 0, 1) : FVector(1, 0, 0);
	FVector Right = Direction.Cross(Up).GetNormalized();
	FVector Forward = Right.Cross(Direction).GetNormalized();

	// 사각형 Base의 4개 코너 계산
	float HalfSize = BaseSize * 0.5f;
	FVector Corner0 = Base + Right * HalfSize + Forward * HalfSize;
	FVector Corner1 = Base - Right * HalfSize + Forward * HalfSize;
	FVector Corner2 = Base - Right * HalfSize - Forward * HalfSize;
	FVector Corner3 = Base + Right * HalfSize - Forward * HalfSize;

	// 정점 추가 (Base 4개 + Tip 1개)
	OutVertices.Add(Corner0);
	OutVertices.Add(Corner1);
	OutVertices.Add(Corner2);
	OutVertices.Add(Corner3);
	OutVertices.Add(Tip);

	// 인덱스 추가 (라인으로 렌더링)
	// Base 사각형 (4개 라인)
	OutIndices.Add(BaseVertexIndex + 0);
	OutIndices.Add(BaseVertexIndex + 1);
	
	OutIndices.Add(BaseVertexIndex + 1);
	OutIndices.Add(BaseVertexIndex + 2);
	
	OutIndices.Add(BaseVertexIndex + 2);
	OutIndices.Add(BaseVertexIndex + 3);
	
	OutIndices.Add(BaseVertexIndex + 3);
	OutIndices.Add(BaseVertexIndex + 0);

	// Tip으로 연결되는 라인 (4개)
	OutIndices.Add(BaseVertexIndex + 0);
	OutIndices.Add(BaseVertexIndex + 4);
	
	OutIndices.Add(BaseVertexIndex + 1);
	OutIndices.Add(BaseVertexIndex + 4);
	
	OutIndices.Add(BaseVertexIndex + 2);
	OutIndices.Add(BaseVertexIndex + 4);
	
	OutIndices.Add(BaseVertexIndex + 3);
	OutIndices.Add(BaseVertexIndex + 4);
}

void UBatchLines::CreateSolidPyramid(const FVector& Base, const FVector& Tip, float BaseSize,
	TArray<FVector>& OutVertices, TArray<uint32>& OutIndices, uint32 BaseVertexIndex)
{
	// 방향 벡터 계산
	FVector Direction = (Tip - Base).GetNormalized();

	// Base 평면을 위한 직교 벡터 생성
	FVector Up = (fabsf(Direction.Z) < 0.999f) ? FVector(0, 0, 1) : FVector(1, 0, 0);
	FVector Right = Direction.Cross(Up).GetNormalized();
	FVector Forward = Right.Cross(Direction).GetNormalized();

	// 사각형 Base의 4개 코너 계산
	float HalfSize = BaseSize * 0.5f;
	FVector Corner0 = Base + Right * HalfSize + Forward * HalfSize;
	FVector Corner1 = Base - Right * HalfSize + Forward * HalfSize;
	FVector Corner2 = Base - Right * HalfSize - Forward * HalfSize;
	FVector Corner3 = Base + Right * HalfSize - Forward * HalfSize;

	// 정점 추가 (Base 4개 + Tip 1개)
	OutVertices.Add(Corner0);
	OutVertices.Add(Corner1);
	OutVertices.Add(Corner2);
	OutVertices.Add(Corner3);
	OutVertices.Add(Tip);

	// 삼각형 인덱스 추가 (Solid 렌더링)

	// 바닥면 (2개의 삼각형)
	OutIndices.Add(BaseVertexIndex + 0);
	OutIndices.Add(BaseVertexIndex + 1);
	OutIndices.Add(BaseVertexIndex + 2);

	OutIndices.Add(BaseVertexIndex + 0);
	OutIndices.Add(BaseVertexIndex + 2);
	OutIndices.Add(BaseVertexIndex + 3);

	// 측면 4개 (각 면이 1개의 삼각형)
	// 면 1: Corner0 -> Corner1 -> Tip
	OutIndices.Add(BaseVertexIndex + 0);
	OutIndices.Add(BaseVertexIndex + 1);
	OutIndices.Add(BaseVertexIndex + 4);

	// 면 2: Corner1 -> Corner2 -> Tip
	OutIndices.Add(BaseVertexIndex + 1);
	OutIndices.Add(BaseVertexIndex + 2);
	OutIndices.Add(BaseVertexIndex + 4);

	// 면 3: Corner2 -> Corner3 -> Tip
	OutIndices.Add(BaseVertexIndex + 2);
	OutIndices.Add(BaseVertexIndex + 3);
	OutIndices.Add(BaseVertexIndex + 4);

	// 면 4: Corner3 -> Corner0 -> Tip
	OutIndices.Add(BaseVertexIndex + 3);
	OutIndices.Add(BaseVertexIndex + 0);
	OutIndices.Add(BaseVertexIndex + 4);
}

void UBatchLines::TraverseOctree(const FOctree* InNode)
{
	if (!InNode) { return; }

	UBoundingBoxLines BoxLines;
	BoxLines.UpdateVertices(&InNode->GetBoundingBox());
	OctreeLines.Add(BoxLines);

	if (!InNode->IsLeafNode())
	{
		for (const auto& Child : InNode->GetChildren())
		{
			TraverseOctree(Child);
		}
	}
}

void UBatchLines::UpdateVertexBuffer()
{
	if (bChangedVertices)
	{
		uint32 NumGridVertices = Grid.GetNumVertices();
		uint32 NumBoxVertices = BoundingBoxLines.GetNumVertices();
		uint32 NumSpotLightVertices = bRenderSpotLight ? SpotLightLines.GetNumVertices() : 0;
		uint32 NumOctreeVertices = 0;
		for (const auto& Line : OctreeLines)
		{
			NumOctreeVertices += Line.GetNumVertices();
		}
		uint32 NumBonePyramidVertices = bRenderBonePyramids ? static_cast<uint32>(BonePyramidVertices.Num()) : 0;

		Vertices.SetNum(NumGridVertices + NumBoxVertices + NumSpotLightVertices + NumOctreeVertices + NumBonePyramidVertices);

		Grid.MergeVerticesAt(Vertices, 0);
		BoundingBoxLines.MergeVerticesAt(Vertices, NumGridVertices);

		uint32 CurrentOffset = NumGridVertices + NumBoxVertices;
		if (bRenderSpotLight)
		{
			SpotLightLines.MergeVerticesAt(Vertices, CurrentOffset);
			CurrentOffset += SpotLightLines.GetNumVertices();
		}

		for (auto& Line : OctreeLines)
		{
			Line.MergeVerticesAt(Vertices, CurrentOffset);
			CurrentOffset += Line.GetNumVertices();
		}

		if (bRenderBonePyramids)
		{
			for (int32 i = 0; i < BonePyramidVertices.Num(); ++i)
			{
				Vertices[CurrentOffset + i] = BonePyramidVertices[i];
			}
		}

		SetIndices();

		Primitive.NumVertices = static_cast<uint32>(Vertices.Num());
		Primitive.NumIndices = static_cast<uint32>(Indices.Num());

		SafeRelease(Primitive.VertexBuffer);
		SafeRelease(Primitive.IndexBuffer);

		Primitive.VertexBuffer = FRenderResourceFactory::CreateVertexBuffer(Vertices.GetData(), Primitive.NumVertices * sizeof(FVector), true);
		Primitive.IndexBuffer = FRenderResourceFactory::CreateIndexBuffer(Indices.GetData(), Primitive.NumIndices * sizeof(uint32));
	}
	bChangedVertices = false;
}

void UBatchLines::Render()
{
	// Grid + Light Lines
	RenderGridAndLightLines();

	// AABB
	RenderBoundingBox();

	// Octree
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (EditorWorld && EditorWorld->GetLevel())
	{
		uint64 ShowFlags = EditorWorld->GetLevel()->GetShowFlags();
		if (ShowFlags & EEngineShowFlags::SF_Octree)
		{
			RenderOctree();
		}
	}

	// Bone Pyramids
	RenderBonePyramids();
}

void UBatchLines::RenderGridAndLightLines()
{
	// Grid + Light Lines 렌더링 (항상 표시)
	URenderer& Renderer = URenderer::GetInstance();

	const uint32 NumGridVertices = Grid.GetNumVertices();
	const uint32 NumGridIndices = NumGridVertices; // Grid는 인덱스가 순차적

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	uint32 NumSpotLightIndices = 0;
	if (bRenderSpotLight)
	{
		const EBoundingVolumeType SpotLightType = SpotLightLines.GetCurrentType();
		NumSpotLightIndices = SpotLightLines.GetNumIndices(SpotLightType);
	}

	// Grid 렌더링
	if (NumGridIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			0,
			NumGridIndices
		);
	}

	// SpotLight 렌더링
	const uint32 SpotLightStartIndex = NumGridIndices + NumBoundingIndices;
	if (NumSpotLightIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			SpotLightStartIndex,
			NumSpotLightIndices
		);
	}
}

void UBatchLines::RenderBoundingBox()
{
	// AABB 렌더링 (선택 시에만 데이터 있음)
	URenderer& Renderer = URenderer::GetInstance();

	const uint32 NumGridVertices = Grid.GetNumVertices();
	const uint32 NumGridIndices = NumGridVertices;

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	if (NumBoundingIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			NumGridIndices,
			NumBoundingIndices
		);
	}
}

void UBatchLines::RenderOctree()
{
	// Octree 렌더링 (SF_Octree 플래그로 제어)
	URenderer& Renderer = URenderer::GetInstance();

	const uint32 NumGridVertices = Grid.GetNumVertices();
	const uint32 NumGridIndices = NumGridVertices;

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	uint32 NumSpotLightIndices = 0;
	if (bRenderSpotLight)
	{
		const EBoundingVolumeType SpotLightType = SpotLightLines.GetCurrentType();
		NumSpotLightIndices = SpotLightLines.GetNumIndices(SpotLightType);
	}

	uint32 NumOctreeIndices = 0;
	for (auto& OctreeLine : OctreeLines)
	{
		const EBoundingVolumeType OctreeType = OctreeLine.GetCurrentType();
		NumOctreeIndices += OctreeLine.GetNumIndices(OctreeType);
	}

	const uint32 OctreeStartIndex = NumGridIndices + NumBoundingIndices + NumSpotLightIndices;
	if (NumOctreeIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			OctreeStartIndex,
			NumOctreeIndices
		);
	}
}

void UBatchLines::RenderBonePyramids()
{
	if (!bRenderBonePyramids || BonePyramidIndices.Num() == 0)
	{
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();

	const uint32 NumGridVertices = Grid.GetNumVertices();
	const uint32 NumGridIndices = NumGridVertices;

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	uint32 NumSpotLightIndices = 0;
	if (bRenderSpotLight)
	{
		const EBoundingVolumeType SpotLightType = SpotLightLines.GetCurrentType();
		NumSpotLightIndices = SpotLightLines.GetNumIndices(SpotLightType);
	}

	uint32 NumOctreeIndices = 0;
	for (auto& OctreeLine : OctreeLines)
	{
		const EBoundingVolumeType OctreeType = OctreeLine.GetCurrentType();
		NumOctreeIndices += OctreeLine.GetNumIndices(OctreeType);
	}

	const uint32 BonePyramidStartIndex = NumGridIndices + NumBoundingIndices + NumSpotLightIndices + NumOctreeIndices;
	const uint32 NumBonePyramidIndices = static_cast<uint32>(BonePyramidIndices.Num());

	if (NumBonePyramidIndices > 0)
	{
		// Depth Test를 끄기 위해 DisabledDepthStencilState로 설정
		Primitive.bShouldAlwaysVisible = true;

		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			BonePyramidStartIndex,
			NumBonePyramidIndices
		);

		// 특정 본에 해당하는 피라미드 강조 렌더링
		if(HighlightedIndexCount > 0 && HighlightedIndexStart >= 0)
		{
			FVector4 OriginalColor = Primitive.Color;
			Primitive.Color = FVector4(1.0f, 0.5f, 0.0f, 1.0f); // Highlighted Color (Orange)
			Renderer.RenderEditorPrimitiveIndexed(
				Primitive,
				Primitive.RenderState,
				sizeof(FVector),
				sizeof(uint32),
				BonePyramidStartIndex + HighlightedIndexStart,
				HighlightedIndexCount
			);
			Primitive.Color = OriginalColor;
		}

		// 이전 DepthStencilState 복원
		Primitive.bShouldAlwaysVisible = false;
	}
		
}

void UBatchLines::SetIndices()
{
	Indices.Empty();

	const uint32 NumGridVertices = Grid.GetNumVertices();

	for (uint32 Index = 0; Index < NumGridVertices; ++Index)
	{
		Indices.Add(Index);
	}

	uint32 BaseVertexOffset = NumGridVertices;

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	int32* BoundingLineIdx = BoundingBoxLines.GetIndices(BoundingType);
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	if (BoundingLineIdx)
	{
		for (uint32 Idx = 0; Idx < NumBoundingIndices; ++Idx)
		{
			Indices.Add(BaseVertexOffset + BoundingLineIdx[Idx]);
		}
	}

	BaseVertexOffset += BoundingBoxLines.GetNumVertices();

	if (bRenderSpotLight)
	{
		const EBoundingVolumeType SpotLightType = SpotLightLines.GetCurrentType();
		int32* SpotLineIdx = SpotLightLines.GetIndices(SpotLightType);
		const uint32 NumSpotLightIndices = SpotLightLines.GetNumIndices(SpotLightType);

		if (SpotLineIdx)
		{
			for (uint32 Idx = 0; Idx < NumSpotLightIndices; ++Idx)
			{
				Indices.Add(BaseVertexOffset + SpotLineIdx[Idx]);
			}
		}

		BaseVertexOffset += SpotLightLines.GetNumVertices();
	}

	for (auto& OctreeLine : OctreeLines)
	{
		const EBoundingVolumeType OctreeType = OctreeLine.GetCurrentType();
		int32* OctreeLineIdx = OctreeLine.GetIndices(OctreeType);
		const uint32 NumOctreeIndices = OctreeLine.GetNumIndices(OctreeType);

		if (!OctreeLineIdx)
		{
			continue;
		}

		for (uint32 Idx = 0; Idx < NumOctreeIndices; ++Idx)
		{
			Indices.Add(BaseVertexOffset + OctreeLineIdx[Idx]);
		}

		BaseVertexOffset += OctreeLine.GetNumVertices();
	}

	// Bone Pyramid Indices 추가
	if (bRenderBonePyramids)
	{
		for (uint32 Idx = 0; Idx < BonePyramidIndices.Num(); ++Idx)
		{
			Indices.Add(BaseVertexOffset + BonePyramidIndices[Idx]);
		}
	}
}

void UBatchLines::RenderBonePyramidsForHitProxy(USkeletalMeshComponent* SkeletalMeshComponent)
{
	if (!SkeletalMeshComponent)
	{
		return;
	}

	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!SkeletalMesh)
	{
		return;
	}

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	const int32 NumBones = RefSkeleton.GetRawBoneNum();

	if (NumBones == 0)
	{
		return;
	}

	// ComponentSpaceTransforms이 정상적으로 갱신되어 있는지 확인
	if (SkeletalMeshComponent->IsSkinningDirty() == false)
	{
		SkeletalMeshComponent->RefreshBoneTransforms();
	}

	const TArray<FTransform>& ComponentSpaceTransforms = SkeletalMeshComponent->GetComponentSpaceTransforms();
	const FMatrix& ComponentWorldMatrix = SkeletalMeshComponent->GetWorldTransformMatrix();

	// 사각뿔 크기 (본 간 거리에 비례하여 조정)
	constexpr float BaseSizeRatio = 0.1f;

	URenderer& Renderer = URenderer::GetInstance();
	FHitProxyManager& HitProxyManager = FHitProxyManager::GetInstance();

	// 본 정보를 저장할 구조체
	struct FBoneRenderInfo
	{
		int32 BoneIndex;
		int32 ParentIndex;
		FVector ParentPos;
		FVector CurrentPos;
		float Distance;
	};

	// 모든 본 정보를 먼저 수집
	TArray<FBoneRenderInfo> BoneInfos;
	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		int32 ParentIndex = RefSkeleton.GetRawParentIndex(BoneIndex);

		// 루트 본(부모가 없는 본)은 건너뜀
		if (ParentIndex == INDEX_NONE)
		{
			continue;
		}

		// 부모 본 -> 현재 본으로 이어지는 피라미드 생성
		FVector ParentPos = ComponentWorldMatrix.TransformPosition(ComponentSpaceTransforms[ParentIndex].Translation);
		FVector CurrentPos = ComponentWorldMatrix.TransformPosition(ComponentSpaceTransforms[BoneIndex].Translation);

		float Distance = (CurrentPos - ParentPos).Length();

		// 너무 짧은 본은 건너뜀 (거리가 거의 0인 경우)
		if (Distance < MATH_EPSILON)
		{
			continue;
		}

		// 본 정보 저장
		FBoneRenderInfo Info;
		Info.BoneIndex = BoneIndex;
		Info.ParentIndex = ParentIndex;
		Info.ParentPos = ParentPos;
		Info.CurrentPos = CurrentPos;
		Info.Distance = Distance;
		BoneInfos.Add(Info);
	}

	// 본 크기 순으로 정렬 (큰 본부터 작은 본 순으로) - 작은 본이 나중에 렌더링됨
	BoneInfos.Sort([](const FBoneRenderInfo& A, const FBoneRenderInfo& B) {
		return A.Distance > B.Distance;
	});

	// 정렬된 순서대로 본 렌더링
	for (const FBoneRenderInfo& BoneInfo : BoneInfos)
	{
		float BaseSize = BoneInfo.Distance * BaseSizeRatio;

		// HBone HitProxy 생성 및 ID 할당
		HBone* BoneProxy = new HBone(BoneInfo.BoneIndex, SkeletalMeshComponent, InvalidHitProxyId);
		FHitProxyId BoneHitProxyId = HitProxyManager.AllocateHitProxyId(BoneProxy);

		// 피라미드 지오메트리 생성 (Solid)
		TArray<FVector> PyramidVertices;
		TArray<uint32> PyramidIndices;
		CreateSolidPyramid(BoneInfo.ParentPos, BoneInfo.CurrentPos, BaseSize,
			PyramidVertices, PyramidIndices, 0);

		// 버퍼 생성 및 렌더링
		if (PyramidVertices.Num() > 0 && PyramidIndices.Num() > 0)
		{
			ID3D11Device* Device = Renderer.GetDevice();
			ID3D11Buffer* VertexBuffer = nullptr;
			ID3D11Buffer* IndexBuffer = nullptr;

			// Vertex Buffer 생성
			D3D11_BUFFER_DESC vbDesc = {};
			vbDesc.ByteWidth = static_cast<UINT>(PyramidVertices.Num() * sizeof(FVector));
			vbDesc.Usage = D3D11_USAGE_DEFAULT;
			vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vbDesc.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA vbData = {};
			vbData.pSysMem = PyramidVertices.GetData();

			if (SUCCEEDED(Device->CreateBuffer(&vbDesc, &vbData, &VertexBuffer)))
			{
				// Index Buffer 생성
				D3D11_BUFFER_DESC ibDesc = {};
				ibDesc.ByteWidth = static_cast<UINT>(PyramidIndices.Num() * sizeof(uint32));
				ibDesc.Usage = D3D11_USAGE_DEFAULT;
				ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				ibDesc.CPUAccessFlags = 0;

				D3D11_SUBRESOURCE_DATA ibData = {};
				ibData.pSysMem = PyramidIndices.GetData();

				if (SUCCEEDED(Device->CreateBuffer(&ibDesc, &ibData, &IndexBuffer)))
				{
					// EditorPrimitive 설정
					FEditorPrimitive TempPrimitive;
					TempPrimitive.VertexBuffer = VertexBuffer;
					TempPrimitive.IndexBuffer = IndexBuffer;
					TempPrimitive.NumVertices = static_cast<uint32>(PyramidVertices.Num());
					TempPrimitive.NumIndices = static_cast<uint32>(PyramidIndices.Num());
					TempPrimitive.Location = FVector(0.0f, 0.0f, 0.0f);
					TempPrimitive.Rotation = FQuaternion::Identity();
					TempPrimitive.Scale = FVector(1.0f, 1.0f, 1.0f);
					TempPrimitive.Color = BoneHitProxyId.GetColor();
					TempPrimitive.bShouldAlwaysVisible = true;
					TempPrimitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

					// BatchLines의 셰이더 사용
					TempPrimitive.VertexShader = Primitive.VertexShader;
					TempPrimitive.PixelShader = Primitive.PixelShader;
					TempPrimitive.InputLayout = Primitive.InputLayout;

					// RenderState 설정
					FRenderState RenderState;
					RenderState.FillMode = EFillMode::Solid;
					RenderState.CullMode = ECullMode::Back;

					// 렌더링
					Renderer.RenderEditorPrimitiveIndexed(
						TempPrimitive,
						RenderState,
						sizeof(FVector),
						sizeof(uint32),
						0,
						TempPrimitive.NumIndices
					);

					// 버퍼 해제
					IndexBuffer->Release();
				}
				VertexBuffer->Release();
			}
		}
	}
}

