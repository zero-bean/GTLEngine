#include "pch.h"

#include "Core/Public/ScopeCycleCounter.h"

#ifdef MULTI_THREADING
#include "cpp-thread-pool/thread_pool.h"
#endif

#include "Component/Public/PrimitiveComponent.h"
#include "Core/Public/AppWindow.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/ObjectPicker.h"
#include "Global/Quaternion.h"
#include "ImGui/imgui.h"
#include "Level/Public/Level.h"
#include "Manager/Input/Public/InputManager.h"
#include "Physics/Public/AABB.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Physics/Public/RayIntersection.h"
#include "Core/Public/BVHierarchy.h"

FRay UObjectPicker::GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive) const
{
	FMatrix ModelInverse = Primitive->GetWorldTransformMatrixInverse();

	FRay ModelRay;
	ModelRay.Origin = Ray.Origin * ModelInverse;
	ModelRay.Direction = Ray.Direction * ModelInverse;
	ModelRay.Direction.Normalize();
	return ModelRay;
}

UPrimitiveComponent* UObjectPicker::PickPrimitive(const FRay& WorldRay, const TArray<TObjectPtr<UPrimitiveComponent>>& Candidates, float* OutDistance)
{
	(void)Candidates;
	UPrimitiveComponent* ShortestPrimitive = nullptr;
	float PrimitiveDistance = D3D11_FLOAT32_MAX;

	UBVHierarchy::GetInstance().Raycast(WorldRay, ShortestPrimitive, PrimitiveDistance);
	*OutDistance = PrimitiveDistance;

	return ShortestPrimitive;
}

void UObjectPicker::PickGizmo(UCamera* InActiveCamera, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint)
{
	//Forward, Right, Up순으로 테스트할거임.
	//원기둥 위의 한 점 P, 축 위의 임의의 점 A에(기즈모 포지션) 대해, AP벡터와 축 벡터 V와 피타고라스 정리를 적용해서 점 P의 축부터의 거리 r을 구할 수 있음.
	//r이 원기둥의 반지름과 같다고 방정식을 세운 후 근의공식을 적용해서 충돌여부 파악하고 distance를 구할 수 있음.

	//FVector4 PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
	//dot(PointOnCylinder - GizmoLocation)*Dot(PointOnCylinder - GizmoLocation) - Dot(PointOnCylinder - GizmoLocation, GizmoAxis)^2 = r^2 = radiusOfGizmo
	//이 t에 대한 방정식을 풀어서 근의공식 적용하면 됨.

	FVector GizmoLocation = Gizmo.GetGizmoLocation();
	FVector GizmoAxises[3] = { FVector{1, 0, 0}, FVector{0, 1, 0}, FVector{0, 0, 1} };

	if (Gizmo.GetGizmoMode() == EGizmoMode::Scale || !Gizmo.IsWorldMode())
	{
		FVector Rad = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
		FMatrix R = FMatrix::RotationMatrix(Rad);
		//FQuaternion q = FQuaternion::FromEuler(Rad);

		for (int i = 0; i < 3; i++)
		{
			//GizmoAxises[a] = FQuaternion::RotateVector(q, GizmoAxises[a]); // 쿼터니언으로 축 회전
			//GizmoAxises[a].Normalize();
			const FVector4 a4(GizmoAxises[i].X, GizmoAxises[i].Y, GizmoAxises[i].Z, 0.0f);
			FVector4 rotated4 = a4 * R;
			FVector V(rotated4.X, rotated4.Y, rotated4.Z);
			V.Normalize();
			GizmoAxises[i] = V;
		}
	}

	FVector WorldRayOrigin{ WorldRay.Origin.X,WorldRay.Origin.Y ,WorldRay.Origin.Z };
	FVector WorldRayDirection(WorldRay.Direction.X, WorldRay.Direction.Y, WorldRay.Direction.Z);
	WorldRayDirection.Normalize();

	switch (Gizmo.GetGizmoMode())
	{
	case EGizmoMode::Translate:
	case EGizmoMode::Scale:
	{
		FVector GizmoDistanceVector = WorldRayOrigin - GizmoLocation;
		bool bIsCollide = false;

		float GizmoRadius = Gizmo.GetTranslateRadius();
		float GizmoHeight = Gizmo.GetTranslateHeight();
		float A, B, C; //Ax^2 + Bx + C의 ABC
		float X; //해
		float Det; //판별식
		//0 = forward 1 = Right 2 = UP

		for (int a = 0; a < 3; a++)
		{
			FVector GizmoAxis = GizmoAxises[a];
			float dDotA = WorldRay.Direction.Dot3(GizmoAxis);
			A = 1 - (dDotA * dDotA);
			B = WorldRay.Direction.Dot3(GizmoDistanceVector) - WorldRay.Direction.Dot3(GizmoAxis) * GizmoDistanceVector.
				Dot(GizmoAxis); //B가 2의 배수이므로 미리 약분
			float distDotA = GizmoDistanceVector.Dot(GizmoAxis);
			C = (GizmoDistanceVector.Dot(GizmoDistanceVector) -
				distDotA * distDotA) - GizmoRadius * GizmoRadius;

			Det = B * B - A * C;
			if (Det >= 0) //판별식 0이상 => 근 존재. 높이테스트만 통과하면 충돌
			{
				X = (-B + sqrtf(Det)) / A;
				FVector PointOnCylinder = WorldRayOrigin + WorldRayDirection * X;
				float Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0) //충돌
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;

				}
				X = (-B - sqrtf(Det)) / A;
				PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
				Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0)
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;
				}
				if (bIsCollide)
				{
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	case EGizmoMode::Rotate:
	{
		for (int a = 0; a < 3; a++)
		{
			if (DoesRayIntersectPlane(WorldRay, GizmoLocation, GizmoAxises[a], CollisionPoint))
			{
				FVector RadiusVector = CollisionPoint - GizmoLocation;
				if (Gizmo.IsInRadius(RadiusVector.Length()))
				{
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	default: break;
	}

	Gizmo.SetGizmoDirection(EGizmoDirection::None);
}

bool UObjectPicker::DoesRayIntersectPrimitive(
    UCamera* InActiveCamera,
    const FRay& InWorldRay,
    UPrimitiveComponent* InPrimitive,
    const FMatrix& InModelMatrix,
    float* OutShortestDistance)
{
    // --- Broad phase: world-space AABB vs world ray ---
    FVector AabbMin, AabbMax;
    InPrimitive->GetWorldAABB(AabbMin, AabbMax);
    const FAABB worldAABB(AabbMin, AabbMax);

    float AabbDist;
    if (!worldAABB.RaycastHit(InWorldRay, &AabbDist))
        return false;

    // --- Narrow phase: only if AABB hit ---
    return DoesRayIntersectPrimitive_MollerTrumbore(InWorldRay, InPrimitive, OutShortestDistance);
}

//개별 primitive와 ray 충돌 검사
bool UObjectPicker::DoesRayIntersectPrimitive_MollerTrumbore(const FRay& InWorldRay,
	UPrimitiveComponent* InPrimitive, float* OutShortestDistance) const
{
	if (!InPrimitive || !OutShortestDistance)
	{
		return false;
	}

	const TArray<FNormalVertex>* Vertices = InPrimitive->GetVerticesData();
	const TArray<uint32>* Indices = InPrimitive->GetIndicesData();

	if (!Vertices || Vertices->empty())
	{
		return false;
	}

	FRay ModelRay = GetModelRay(InWorldRay, InPrimitive);

	float LocalShortest = FLT_MAX;
	if (OutShortestDistance && *OutShortestDistance > 0.0f)
	{
		LocalShortest = *OutShortestDistance;
	}
	bool bIsHit = false;

	bool bBVHHit = false;
	const bool bBVHExecuted = RaycastMeshTrianglesBVH(ModelRay, InPrimitive, LocalShortest, bBVHHit);
	if (bBVHExecuted && bBVHHit)
	{
		bIsHit = true;
	}

	const bool bNeedsFallback = !bBVHExecuted || !bBVHHit;
	if (bNeedsFallback)
	{
		const bool bHasIndices = Indices && !Indices->empty();
		const size_t TriangleCount = bHasIndices ? (Indices->size() / 3) : (Vertices->size() / 3);

		for (size_t TriIndex = 0; TriIndex < TriangleCount; ++TriIndex)
		{
			FVector V0;
			FVector V1;
			FVector V2;

			if (bHasIndices)
			{
				const size_t Base = TriIndex * 3;
				const uint32 I0 = (*Indices)[Base + 0];
				const uint32 I1 = (*Indices)[Base + 1];
				const uint32 I2 = (*Indices)[Base + 2];
				V0 = (*Vertices)[I0].Position;
				V1 = (*Vertices)[I1].Position;
				V2 = (*Vertices)[I2].Position;
			}
			else
			{
				const size_t Base = TriIndex * 3;
				V0 = (*Vertices)[Base + 0].Position;
				V1 = (*Vertices)[Base + 1].Position;
				V2 = (*Vertices)[Base + 2].Position;
			}

			float Distance = 0.0f;
			if (RayTriangleIntersectModel(ModelRay, V0, V1, V2, Distance) && Distance < LocalShortest)
			{
				LocalShortest = Distance;
				bIsHit = true;
			}
		}
	}

	if (bIsHit)
	{
		*OutShortestDistance = LocalShortest;
	}

	return bIsHit;
}

bool UObjectPicker::RaycastMeshTrianglesBVH(const FRay& ModelRay, UPrimitiveComponent* InPrimitive, float& InOutDistance, bool& OutHit) const
{
	OutHit = false;
	if (!InPrimitive || InPrimitive->GetPrimitiveType() != EPrimitiveType::StaticMesh)
	{
		return false;
	}

	auto* StaticMeshComponent = static_cast<UStaticMeshComponent*>(InPrimitive);
	if (!StaticMeshComponent)
	{
		return false;
	}

	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	if (!StaticMesh)
	{
		return false;
	}

	OutHit = StaticMesh->RaycastTriangleBVH(ModelRay, InOutDistance);
	return true;
}

bool UObjectPicker::DoesRayIntersectTriangle(const FRay& InRay, UPrimitiveComponent* InPrimitive,
	const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, float* OutDistance) const
{
	FRay ModelRay = GetModelRay(InRay, InPrimitive);

	float Distance = 0.0f;
	if (!RayTriangleIntersectModel(ModelRay, Vertex1, Vertex2, Vertex3, Distance))
	{
		return false;
	}

	if (OutDistance)
	{
		*OutDistance = Distance;
	}

	return true;
}

bool UObjectPicker::DoesRayIntersectPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane)
{
	FVector WorldRayOrigin{ WorldRay.Origin.X, WorldRay.Origin.Y ,WorldRay.Origin.Z };

	if (abs(WorldRay.Direction.Dot3(Normal)) < 0.01f)
		return false;

	float Distance = (PlanePoint - WorldRayOrigin).Dot(Normal) / WorldRay.Direction.Dot3(Normal);

	if (Distance < 0)
		return false;
	PointOnPlane = WorldRay.Origin + WorldRay.Direction * Distance;


	return true;
}


