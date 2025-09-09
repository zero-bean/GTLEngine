#include "pch.h"
#include "Editor/Public/ObjectPicker.h"
#include "Mesh/Public/SceneComponent.h"
#include "Manager/Input/Public/InputManager.h"
#include "Editor/Public/Camera.h"
#include "ImGui/imgui.h"
#include "Mesh/Public/Actor.h"
#include "Level/Public/Level.h"

AActor* UObjectPicker::PickActor(ULevel* Level, HWND WindowHandle, UCamera& Camera)
{
	//Level로부터 Actor순회하면서 picked objects중에서 가장 가까운 object return
	AActor* ShortestActor = nullptr;
	float ShortestDistance = D3D11_FLOAT32_MAX;
	float PrimitiveDistance = D3D11_FLOAT32_MAX;
	const UInputManager& Input = UInputManager::GetInstance();

	RECT ClientRect;
	GetClientRect(WindowHandle, &ClientRect);
	int32 ViewportWidth = ClientRect.right - ClientRect.left;
	int32 ViewportHeight = ClientRect.bottom - ClientRect.top;


	if (!ImGui::GetIO().WantCaptureMouse && Input.IsKeyPressed(EKeyInput::MouseLeft))
	{
		FVector MousePosition = Input.GetMousePosition();

		FRay WorldRay = ConvertToWorldRay(Camera, static_cast<int>(MousePosition.X), static_cast<int>(MousePosition.Y),
		                                  ViewportWidth,ViewportHeight);

		for (AActor* Actor : Level->GetLevelActors())
		{
			for (auto& ActorComponent : Actor->GetOwnedComponents())
			{
				UPrimitiveComponent* Primitive = dynamic_cast<UPrimitiveComponent*>(ActorComponent);
				if (Primitive)
				{
					FMatrix ModelMat = Primitive->GetWorldTransformMatrix();
					FRay ModelRay = GetModelRay(WorldRay, Primitive); //Actor로부터 Primitive를 얻고 Ray를 모델 좌표계로 변환함

					if (IsRayPrimitiveCollided(ModelRay, Primitive, ModelMat, &PrimitiveDistance, Camera))
					//Ray와 Primitive가 충돌했다면 거리 테스트 후 가까운 Actor Picking
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

		Level->SetSelectedActor(ShortestActor);
	}


	return ShortestActor;
}

FRay UObjectPicker::ConvertToWorldRay(UCamera& Camera, int PixelX, int PixelY, int ViewportW, int ViewportH)
{
	/* *
	 * @brief 반환할 타입의 객체 선언
	 */
	FRay Ray = {};

	const FViewProjConstants& ViewProjMatrix = Camera.GetFViewProjConstantsInverse();

	/* *
	 * @brief 마우스 클릭한 Screen 좌표를 NDC 좌표로 변환합니다.
	 * NDC: (-1, -1, 0) ~ (1, 1, 1), Window: (0, 0) ~ (Width, Height)
	 */
	const float NdcX = (PixelX / (float)ViewportW) * 2.0f - 1.0f;
	const float NdcY = 1.0f - (PixelY / (float)ViewportH) * 2.0f; // 윈도우 좌표계 Y 반전

	/* *
	 * @brief NDC 좌표 정보를 행렬로 변환합니다.
	 */
	const FVector4 NdcNear(NdcX, NdcY, 0.0f, 1.0f);
	const FVector4 NdcFar(NdcX, NdcY, 1.0f, 1.0f);

	/* *
	 * @brief Projection 행렬을 View 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 ViewNear = Camera.MultiplyPointWithMatrix(NdcNear, ViewProjMatrix.Projection);
	const FVector4 ViewFar = Camera.MultiplyPointWithMatrix(NdcFar, ViewProjMatrix.Projection);

	/* *
	 * @brief View 행렬을 World 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 WorldNear = Camera.MultiplyPointWithMatrix(ViewNear, ViewProjMatrix.View);
	const FVector4 WorldFar = Camera.MultiplyPointWithMatrix(ViewFar, ViewProjMatrix.View);

	/* *
	 * @brief 카메라의 월드 좌표를 추출합니다.
	 * Row-major 기준, 마지막 행 벡터는 위치 정보를 가지고 있음
	 */
	const FVector4 CameraPosition(
		ViewProjMatrix.View.Data[3][0],
		ViewProjMatrix.View.Data[3][1],
		ViewProjMatrix.View.Data[3][2],
		ViewProjMatrix.View.Data[3][3]);

	if (Camera.GetCameraType() == ECameraType::ECT_Perspective)
	{
		FVector4 DirectionVector = WorldFar - CameraPosition;
		DirectionVector.Normalize();

		Ray.Origin = CameraPosition;
		Ray.Direction = DirectionVector;
	}
	else if (Camera.GetCameraType() == ECameraType::ECT_Orthographic)
	{
		FVector4 DirectionVector = WorldFar - WorldNear;
		DirectionVector.Normalize();

		Ray.Origin = WorldNear;
		Ray.Direction = DirectionVector;
	}

	return Ray;
}


FRay UObjectPicker::GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive)
{
	FMatrix ModelInverse = Primitive->GetWorldTransformMatrixInverse();

	FRay ModelRay;
	ModelRay.Origin = Ray.Origin * ModelInverse;
	ModelRay.Direction = Ray.Direction * ModelInverse;
	ModelRay.Direction.Normalize();
	return ModelRay;
}

//개별 primitive와 ray 충돌 검사
bool UObjectPicker::IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance, const UCamera& Camera)
{
	//FRay ModelRay = GetModelRay(Ray, Primitive);

	const TArray<FVertex>* Vertices = Primitive->GetVerticesData();

	float Distance = D3D11_FLOAT32_MAX; //Distance 초기화
	bool bIsHit = false;
	for (int32 a = 0; a < Vertices->size(); a = a + 3) //삼각형 단위로 Vertex 위치정보 읽음
	{
		const FVector& Vertex1 = (*Vertices)[a].Position;
		const FVector& Vertex2 = (*Vertices)[a + 1].Position;
		const FVector& Vertex3 = (*Vertices)[a + 2].Position;

		if (IsRayTriangleCollided(ModelRay, Vertex1, Vertex2, Vertex3, ModelMatrix, Camera, &Distance)) //Ray와 삼각형이 충돌하면 거리 비교 후 최단거리 갱신
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

bool UObjectPicker::IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
                           const FMatrix& ModelMatrix, const UCamera& Camera, float* Distance)
{
	FVector CameraForward = Camera.GetForward(); //카메라 정보 필요
	float NearZ = Camera.GetNearZ();
	float FarZ = Camera.GetFarZ();
	FMatrix ModelTransform; //Primitive로부터 얻어내야함.(카메라가 처리하는게 나을듯)


	//삼각형 내의 점은 E1*V + E2*U + Vertex1.Position으로 표현 가능( 0<= U + V <=1,  Y>=0, V>=0 )
	//Ray.Direction * T + Ray.Origin = E1*V + E2*U + Vertex1.Position을 만족하는 T U V값을 구해야 함.
	//[E1 E2 RayDirection][V U T] = [RayOrigin-Vertex1.Position]에서 cramer's rule을 이용해서 T U V값을 구하고
	//U V값이 저 위의 조건을 만족하고 T값이 카메라의 near값 이상이어야 함.
	FVector RayDirection{Ray.Direction.X, Ray.Direction.Y, Ray.Direction.Z};
	FVector RayOrigin{Ray.Origin.X, Ray.Origin.Y, Ray.Origin.Z};
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


	float V = Result.Dot(CrossE2Ray) / Determinant; //cramer's rule로 해를 구했음. 이게 0미만 1초과면 충돌하지 않음.

	if (V < 0 || V > 1)
	{
		return false;
	}

	float U = RayDirection.Dot(CrossE1Result) / Determinant;
	if (U < 0 || U + V > 1)
	{
		return false;
	}


	float T = E2.Dot(CrossE1Result) / Determinant;

	FVector HitPoint = RayOrigin + RayDirection * T; //모델 좌표계에서의 충돌점
	FVector4 HitPoint4{HitPoint.X, HitPoint.Y, HitPoint.Z, 1};
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
