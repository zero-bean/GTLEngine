//#pragma once
//#include "ShapeComponent.h"
//
//struct FOrientedBound;
//struct FBound;
//struct FBox
//{
//    FVector Min;
//    FVector Max;
//
//    FBox() : Min(FVector()), Max(FVector()) {}
//    FBox(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}
//};
//
//class UOBoundingBoxComponent :
//    public UShapeComponent
//{
//    DECLARE_CLASS(UOBoundingBoxComponent,UShapeComponent)
//public:
//    UOBoundingBoxComponent();
//    ~UOBoundingBoxComponent() override;
//    // 주어진 로컬 버텍스들로부터 Min/Max 계산
//    void SetFromVertices(const std::vector<FVector>& Verts);
//
//    // 로컬 공간에서의 Extent (절반 크기)
//    FVector GetExtent() const;
//
//    // 로컬 기준 8개 꼭짓점 반환
//    std::vector<FVector> GetLocalCorners() const;
//
//    TArray<FVector> GetWorldCorners() const;
//
//    FBox GetWorldOBBFromAttachParent() const;
//
//	void Render(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj) override;
//
//    void CreateLineData(const std::vector<FVector>& corners, OUT TArray<FVector>& Start, OUT TArray<FVector>& End, OUT TArray<FVector4>& Color);
//
//    // OBB 관련 함수들
//    FOrientedBound GetWorldOrientedBound() const;
//    bool RayIntersectsOBB(const FVector& Origin, const FVector& Direction, float& Distance) const;
//
//    void SetLineColor(FVector4 InLineColor) { LineColor = InLineColor; }
//
//    bool IntersectWithAABB(const FBound& AABB) const;
//    // Duplicate
//    UObject* Duplicate() override;
//
//private:
// /*   void CreateLineData(
//        const FVector& Min, const FVector& Max,
//        OUT TArray<FVector>& Start,
//        OUT TArray<FVector>& End,
//        OUT TArray<FVector4>& Color);*/
//
//   // CreateOBBLineData(worldCorners, OBBStart, OBBEnd, OBBColor);
//
//    FVector LocalMin;
//    FVector LocalMax;
//    FVector4 LineColor;
//};
//
