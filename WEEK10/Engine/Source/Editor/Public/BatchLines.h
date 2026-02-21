#pragma once
#include "Global/Types.h"
#include "Global/CoreTypes.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/BoundingBoxLines.h"

struct FVertex;
class FOctree;
class UDecalSpotLightComponent;
class USkeletalMeshComponent;

class UBatchLines : UObject
{
	DECLARE_CLASS(UBatchLines, UObject)
public:
	UBatchLines();
	~UBatchLines();

	// 종류별 Vertices 업데이트
	void UpdateUGridVertices(const float newCellSize);
	void UpdateBoundingBoxVertices(const IBoundingVolume* NewBoundingVolume);
	void UpdateOctreeVertices(const FOctree* InOctree);
	// Decal SpotLight용 불법 증축
	void UpdateDecalSpotLightVertices(UDecalSpotLightComponent* SpotLightComponent);
	void UpdateConeVertices(const FVector& InCenter, float InGeneratingLineLength
		, float InOuterHalfAngleRad, float InInnerHalfAngleRad, FQuaternion InRotation);
	
	// Bone Pyramid 렌더링용 업데이트 (특정 본 인덱스에 대한 사각뿔만 렌더링)
	void UpdateBonePyramidVertices(USkeletalMeshComponent* SkeletalMeshComponent, int32 InBoneIndex);
	void UpdateAllBonePyramidVertices(USkeletalMeshComponent* SkeletalMeshComponent, int32 InBoneIndex);
	void ClearBonePyramids();
	
	// GPU VertexBuffer에 복사
	void UpdateVertexBuffer();

	float GetCellSize() const
	{
		return Grid.GetCellSize();
	}

	void DisableRenderBoundingBox()
	{
		UpdateBoundingBoxVertices(BoundingBoxLines.GetDisabledBoundingBox());
		bRenderSpotLight = false;
	}

	void ClearOctreeLines()
	{
		OctreeLines.Empty();
		bChangedVertices = true;
	}

	//void UpdateConstant(FBoundingBox boundingBoxInfo);

	//void Update();

	/**
	 * @brief 모든 BatchLines 렌더링 (Grid, AABB, Light Lines, Octree, Bone Pyramids)
	 * @note 내부에서 ShowFlags 체크 및 선택 상태에 따라 적절히 렌더링
	 */
	void Render();

	/**
	 * @brief HitProxy 패스에서 본 피라미드 렌더링
	 * @param SkeletalMeshComponent 스켈레탈 메시 컴포넌트
	 */
	void RenderBonePyramidsForHitProxy(USkeletalMeshComponent* SkeletalMeshComponent);

private:
	void RenderGridAndLightLines();  // Grid + Light Lines
	void RenderBoundingBox();        // AABB
	void RenderOctree();             // Octree
	void RenderBonePyramids();       // Bone Pyramids
	void SetIndices();

	void TraverseOctree(const FOctree* InNode);

	// Pyramid 생성 헬퍼 함수
	void CreatePyramid(const FVector& Base, const FVector& Tip, float BaseSize, const FVector4& Color, TArray<FVector>& OutVertices, TArray<uint32>& OutIndices, uint32 BaseVertexIndex);

	// HitProxy용 Solid Pyramid 생성 (삼각형 면으로 구성)
	void CreateSolidPyramid(const FVector& Base, const FVector& Tip, float BaseSize, TArray<FVector>& OutVertices, TArray<uint32>& OutIndices, uint32 BaseVertexIndex);

	/*void AddWorldGridVerticesAndConstData();
	void AddBoundingBoxVertices();*/

	bool bChangedVertices = false;

	TArray<FVector> Vertices; // 그리드 라인 정보 + (offset 후)디폴트 바운딩 박스 라인 정보(minx, miny가 0,0에 정의된 크기가 1인 cube)
	TArray<uint32> Indices; // 월드 그리드는 그냥 정점 순서, 바운딩 박스는 실제 인덱싱

	FEditorPrimitive Primitive;

	UGrid Grid;
	UBoundingBoxLines BoundingBoxLines;
	UBoundingBoxLines SpotLightLines;
	TArray<UBoundingBoxLines> OctreeLines;

	// Bone Pyramid 데이터
	TArray<FVector> BonePyramidVertices;
	TArray<uint32> BonePyramidIndices;
	bool bRenderBonePyramids = false;
	uint32 HighlightedIndexStart = 0;
	uint32 HighlightedIndexCount = 0;

	bool bRenderBox;
	bool bRenderSpotLight = false;
};

