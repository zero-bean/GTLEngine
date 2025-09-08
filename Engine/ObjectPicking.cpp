#include "pch.h"
#include "ObjectPicking.h"
#include "Mesh/Public/SceneComponent.h"
#include "Manager/Input/Public/InputManager.h"
#include "Camera/Public/Camera.h"
#include "Render/Public/Renderer.h"
#include "Mesh/Public/Actor.h"
#include "Level/Public/Level.h"

FRay ConvertToWorldRay(int PixelX, int PixelY, int ViewportW, int ViewportH, const FViewProjConstants& ViewProjConstantsInverse);
bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, float* ShortestDistance);
FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, const FMatrix& ModelMatrix, float* Distance);

extern Camera* MyCamera;
AActor* PickActor(ULevel* Level)
{
	//Level로부터 Actor순회하면서 picked objects중에서 가장 가까운 object return
	AActor* ShortestActor = nullptr;
	float ShortestDistance = D3D11_FLOAT32_MAX;
	float PrimitiveDistance = D3D11_FLOAT32_MAX;
	const UInputManager& Input = UInputManager::GetInstance();
	if (Input.IsKeyDown(EKeyInput::MouseLeft))
	{
		FVector MousePosition = Input.GetMousePosition();
		FRay WorldRay = ConvertToWorldRay(MousePosition.X, MousePosition.Y, Render::INIT_SCREEN_WIDTH, Render::INIT_SCREEN_HEIGHT, MyCamera->GetFViewProjConstantsInverse());

		for (UObject* Object : Level->GetLevelObjects())
		{
			AActor* Actor = dynamic_cast<AActor*>(Object);
			if (Actor)
			{
				for (auto& ActorComponent : Actor->OwnedComponents)
				{
					UPrimitiveComponent* Primitive = dynamic_cast<UPrimitiveComponent*>(ActorComponent);
					if (Primitive)
					{
						FRay ModelRay = GetModelRay(WorldRay, Primitive);		//Actor로부터 Primitive를 얻고 Ray를 모델 좌표계로 변환함

						if (IsRayPrimitiveCollided(ModelRay, Primitive, &PrimitiveDistance)) //Ray와 Primitive가 충돌했다면 거리 테스트 후 가까운 Actor Picking
						{
							if (PrimitiveDistance < ShortestDistance)
							{
								ShortestActor = Actor;
								ShortestDistance = PrimitiveDistance;
							}
						}
					}
				}
			}
		}

		Level->SetSelectedActor(ShortestActor);
	}


	return ShortestActor;
}


FRay ConvertToWorldRay(int PixelX, int PixelY, int ViewportW, int ViewportH, const FViewProjConstants& ViewProjConstantsInverse)
{
	//Screen to NDC
	float NDCX = (PixelX / (float)ViewportW) * 2 - 1;
	float NDCY = 1 - (PixelY / (float)ViewportH) * 2; //window는 좌측 위가 0,0이지만 NDC는 좌측 아래가 0,0(y축의 부호가 반대)

	//NDC에서의 RAY
	FVector4 NDCFarPointRay{ NDCX, NDCY, 1.0f, 1.0f };	//far plane과 만나는 점
	

	FVector4 ViewFarPointRay = NDCFarPointRay * ViewProjConstantsInverse.Projection; // viewing frustum의 far plane과 Ray가 만나는 점
	

	ViewFarPointRay *= 1/ViewFarPointRay.W;

	FVector4 CameraPosition{ ViewProjConstantsInverse.View.Data[3][0], ViewProjConstantsInverse.View.Data[3][1], ViewProjConstantsInverse.View.Data[3][2], ViewProjConstantsInverse.View.Data[3][3] };
	FVector4 Direction = (ViewFarPointRay * ViewProjConstantsInverse.View - CameraPosition);
	Direction.Normalize();
	FRay Ray{ CameraPosition, Direction };

	return Ray;
}



FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive)
{
	FMatrix ModelInverse = FMatrix::GetModelMatrixInverse(Primitive->GetRelativeLocation(), FVector::GetDegreeToRadian(Primitive->GetRelativeRotation()),Primitive->GetRelativeScale3D());

	FRay ModelRay;
	ModelRay.Origin = Ray.Origin * ModelInverse;
	ModelRay.Direction = Ray.Direction * ModelInverse;
	ModelRay.Direction.Normalize();
	return ModelRay;
}

//개별 primitive와 ray 충돌 검사
bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, float* ShortestDistance)
{
	//FRay ModelRay = GetModelRay(Ray, Primitive);

	const TArray<FVertex>* Vertices = Primitive->GetVerticesData();
	

	float Distance = D3D11_FLOAT32_MAX;		//Distance 초기화
	bool bIsHit = false;
	for (int a = 0;a < Vertices->size();a = a + 3)	//삼각형 단위로 Vertex 위치정보 읽음
	{
		const FVector& Vertex1 = (*Vertices)[a].Position;
		const FVector& Vertex2 = (*Vertices)[a + 1].Position;
		const FVector& Vertex3 = (*Vertices)[a + 2].Position;

		

		if (IsRayTriangleCollided(ModelRay, Vertex1, Vertex2, Vertex3, FMatrix::GetModelMatrix(Primitive->GetRelativeLocation(),
			FVector::GetDegreeToRadian(Primitive->GetRelativeRotation()), Primitive->GetRelativeScale3D()), &Distance))	//Ray와 삼각형이 충돌하면 거리 비교 후 최단거리 갱신
		{
			bIsHit = true;
			if (Distance < *ShortestDistance)
			{
				*ShortestDistance = Distance;
			}
		}
	}

	return bIsHit;

}

bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, const FMatrix& ModelMatrix, float* Distance)
{

	FVector CameraForward = MyCamera->GetForward();	//카메라 정보 필요
	float NearZ = MyCamera->GetNearZ();
	float FarZ = MyCamera->GetFarZ();
	FMatrix ModelTransform; //Primitive로부터 얻어내야함.(카메라가 처리하는게 나을듯)


	//삼각형 내의 점은 E1*V + E2*U + Vertex1.Position으로 표현 가능( 0<= U + V <=1,  Y>=0, V>=0 )
	//Ray.Direction * T + Ray.Origin = E1*V + E2*U + Vertex1.Position을 만족하는 T U V값을 구해야 함.
	//[E1 E2 RayDirection][V U T] = [RayOrigin-Vertex1.Position]에서 cramer's rule을 이용해서 T U V값을 구하고
	//U V값이 저 위의 조건을 만족하고 T값이 카메라의 near값 이상이어야 함.
	FVector RayDirection{ Ray.Direction.X, Ray.Direction.Y,Ray.Direction.Z };
	FVector RayOrigin{ Ray.Origin.X, Ray.Origin.Y , Ray.Origin.Z };
	FVector E1 = Vertex2 - Vertex1;
	FVector E2 = Vertex3 - Vertex1;
	FVector Result = (RayOrigin - Vertex1); //[E1 E2 -RayDirection]x = [RayOrigin - Vertex1.Position] 의 result임.


	FVector CrossE2Ray = E2.Cross(RayDirection);
	FVector CrossE1Result = E1.Cross(Result);

	float Determinant = E1.Dot(CrossE2Ray);

	float NoInverse = 0.0001f; //0.0001이하면 determinant가 0이라고 판단=>역행렬 존재 X
	if (abs(Determinant) <= NoInverse)
	{
		return false;
	}



	float V = Result.Dot(CrossE2Ray) / Determinant;	//cramer's rule로 해를 구했음. 이게 0미만 1초과면 충돌하지 않음.

	if (V < 0 || V>1)
	{
		return false;
	}

	float U = RayDirection.Dot(CrossE1Result) / Determinant;
	if (U < 0 || U + V > 1)
	{
		return false;
	}


	float T = E2.Dot(CrossE1Result) / Determinant;

	FVector HitPoint = RayOrigin + RayDirection * T;	//모델 좌표계에서의 충돌점
	FVector4 HitPoint4{ HitPoint.X, HitPoint.Y, HitPoint.Z, 1 };
	//이제 이것을 월드 좌표계로 변환해서 view Frustum안에 들어가는지 판단할 것임.(near, far plane만 테스트하면 됨)

	FVector4 HitPointWorld = HitPoint4 * ModelMatrix;
	FVector4 RayOriginWorld = Ray.Origin * ModelMatrix;

	FVector4 DistanceVec = HitPointWorld - RayOriginWorld;
	if (DistanceVec.Dot3(CameraForward) >= NearZ && DistanceVec.Dot3(CameraForward) <= FarZ)
	{
		*Distance = DistanceVec.Length();
		return true;
	}
	return false;
}
