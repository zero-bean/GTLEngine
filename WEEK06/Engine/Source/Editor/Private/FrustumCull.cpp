#include "pch.h"
#include <immintrin.h>
#include "Editor/Public/FrustumCull.h"
#include "Physics/Public/AABB.h"
#include "Editor/Public/Camera.h"


IMPLEMENT_CLASS(FFrustumCull, UObject)

FFrustumCull::FFrustumCull()
{
}
FFrustumCull::~FFrustumCull()
{
}

UObject* FFrustumCull::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<FFrustumCull*>(Super::Duplicate(Parameters));

	for (size_t i = 0; i < 6; ++i)
	{
		DupObject->Planes[i] = Planes[i];
	}

	return DupObject;
}

void FFrustumCull::Update(UCamera* InCamera)
{
	FMatrix ViewMatrix = InCamera->GetFViewProjConstants().View;
	FMatrix ProjMatrix =  InCamera->GetFViewProjConstants().Projection;
	FMatrix ViewProjMatrix = ViewMatrix * ProjMatrix;

	// left
	Planes[0].NormalVector.X = ViewProjMatrix.Data[0][3] + ViewProjMatrix.Data[0][0];
	Planes[0].NormalVector.Y = ViewProjMatrix.Data[1][3] + ViewProjMatrix.Data[1][0];
	Planes[0].NormalVector.Z = ViewProjMatrix.Data[2][3] + ViewProjMatrix.Data[2][0];
	Planes[0].ConstantD = ViewProjMatrix.Data[3][3] + ViewProjMatrix.Data[3][0];

	// right
	Planes[1].NormalVector.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][0];
	Planes[1].NormalVector.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][0];
	Planes[1].NormalVector.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][0];
	Planes[1].ConstantD = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][0];

	// bottom
	Planes[2].NormalVector.X = ViewProjMatrix.Data[0][3] + ViewProjMatrix.Data[0][1];
	Planes[2].NormalVector.Y = ViewProjMatrix.Data[1][3] + ViewProjMatrix.Data[1][1];
	Planes[2].NormalVector.Z = ViewProjMatrix.Data[2][3] + ViewProjMatrix.Data[2][1];
	Planes[2].ConstantD = ViewProjMatrix.Data[3][3] + ViewProjMatrix.Data[3][1];

	// top
	Planes[3].NormalVector.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][1];
	Planes[3].NormalVector.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][1];
	Planes[3].NormalVector.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][1];
	Planes[3].ConstantD = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][1];

	// near
	Planes[4].NormalVector.X = ViewProjMatrix.Data[0][2];
	Planes[4].NormalVector.Y = ViewProjMatrix.Data[1][2];
	Planes[4].NormalVector.Z = ViewProjMatrix.Data[2][2];
	Planes[4].ConstantD = ViewProjMatrix.Data[3][2];

	// far
	Planes[5].NormalVector.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][2];
	Planes[5].NormalVector.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][2];
	Planes[5].NormalVector.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][2];
	Planes[5].ConstantD = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][2];

	for (auto& Plane : Planes)
	{
		Plane.Normalize();
	}
}

EFrustumTestResult FFrustumCull::IsInFrustum(const FAABB& TargetAABB)
{
	// Near/Far 평면을 먼저 검사 (더 높은 확률로 culling 가능)
	static const int PlaneOrder[6] = { 4, 5, 0, 1, 2, 3 }; // Near, Far, Left, Right, Bottom, Top

	for (int idx = 0; idx < 6; idx++)
	{
		int i = PlaneOrder[idx];
		const FPlane& Plane = Planes[i];

		// 평면의 법선 방향으로 가장 멀리 있는 꼭짓점
		// 이 꼭짓점이 양수면 frustum 내부에 있거나, 겹침 상태
		FVector PositiveVertex{};
		PositiveVertex.X = (Plane.NormalVector.X >= 0.0f) ? TargetAABB.Max.X : TargetAABB.Min.X;
		PositiveVertex.Y = (Plane.NormalVector.Y >= 0.0f) ? TargetAABB.Max.Y : TargetAABB.Min.Y;
		PositiveVertex.Z = (Plane.NormalVector.Z >= 0.0f) ? TargetAABB.Max.Z : TargetAABB.Min.Z;

		float Distance = (Plane.NormalVector.X * PositiveVertex.X)
						+ (Plane.NormalVector.Y * PositiveVertex.Y)
						+ (Plane.NormalVector.Z * PositiveVertex.Z)
						+ Plane.ConstantD;
		if (Distance < 0.0f)
		{
			// Invisible - early out
			return EFrustumTestResult::Outside;
		}
	}
	// Visible
	return EFrustumTestResult::Inside;
}

const EFrustumTestResult FFrustumCull::TestAABBWithPlane(const FAABB& TargetAABB, const EPlaneIndex Index)
{
	EFrustumTestResult Result = this->CheckPlane(TargetAABB, Index);
	return Result;
}

EFrustumTestResult FFrustumCull::CheckPlane(const FAABB& TargetAABB, const EPlaneIndex Index)
{
	const FPlane& Plane = Planes[static_cast<uint8>(Index)];

	FVector PositiveVertex{};
	PositiveVertex.X = (Plane.NormalVector.X >= 0.0f) ? TargetAABB.Max.X : TargetAABB.Min.X;
	PositiveVertex.Y = (Plane.NormalVector.Y >= 0.0f) ? TargetAABB.Max.Y : TargetAABB.Min.Y;
	PositiveVertex.Z = (Plane.NormalVector.Z >= 0.0f) ? TargetAABB.Max.Z : TargetAABB.Min.Z;

	float PositiveDistance = (Plane.NormalVector.X * PositiveVertex.X)
							+ (Plane.NormalVector.Y * PositiveVertex.Y)
							+ (Plane.NormalVector.Z * PositiveVertex.Z)
							+ Plane.ConstantD;

	// 평면 안쪽의 vertex의 거리가 음수이면 완전히 바깥
	if (PositiveDistance < 0.0f)
	{
		return EFrustumTestResult::CompletelyOutside;
	}

	FVector NegativeVertex{};
	NegativeVertex.X = (Plane.NormalVector.X >= 0.0f) ? TargetAABB.Min.X : TargetAABB.Max.X;
	NegativeVertex.Y = (Plane.NormalVector.Y >= 0.0f) ? TargetAABB.Min.Y : TargetAABB.Max.Y;
	NegativeVertex.Z = (Plane.NormalVector.Z >= 0.0f) ? TargetAABB.Min.Z : TargetAABB.Max.Z;

	float NegativeDistance = (Plane.NormalVector.X * NegativeVertex.X)
							+ (Plane.NormalVector.Y * NegativeVertex.Y)
							+ (Plane.NormalVector.Z * NegativeVertex.Z)
							+ Plane.ConstantD;

	// PositiveDistance가 완전히 바깥이 아닌 상태
	// 평면 바깥의 Vertex의 거리가 음수이면 교차
	if (NegativeDistance < 0.0f)
	{
		return EFrustumTestResult::Intersect;
	}

	// 완전히 바깥도 아니고 교차도 아니면 완전히 내부
	return EFrustumTestResult::CompletelyInside;
}


