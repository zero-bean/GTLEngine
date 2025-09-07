#include "pch.h"
//#include "ObjectPicking.h"
//#include "Mesh/Public/SceneComponent.h"
//#include "Manager/Input/Public/InputManager.h"
//#include "Camera/Public/Camera.h"
//#include "Render/Public/Renderer.h"
//
//FRay ConvertToWorldRay(int PixelX, int PixelY, int ViewportW, int ViewportH, const FViewProjConstants& ViewProjConstantsInverse);
//bool IsRayPrimitiveCollided(const FRay& Ray, UPrimitiveComponent* Primitive, float* ShortestDistance);
//FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
//bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, const FMatrix& ModelMatrix, float* Distance);
//
//camera* mycamera;
//aactor* pickactor(ulevel* level)
//{
//	//level로부터 actor순회하면서 picked objects중에서 가장 가까운 object return
//	aactor* shortestactor = nullptr;
//	float shortestdistance = d3d11_float32_max;
//	float primitivedistance = d3d11_float32_max;
//	const uinputmanager& input = uinputmanager::getinstance();
//	if (input.iskeydown(ekeyinput::mouseleft))
//	{
//		fvector mouseposition = input.getmouseposition();
//		fray worldray = converttoworldray(mouseposition.x, mouseposition.y, render::init_screen_width, render::init_screen_height, mycamera->getfviewprojconstantinverse());
//		for (uobject* object : level->getlevelobjects())
//		{
//			aactor* actor = dynamic_cast<aactor*>(object);
//			if (actor)
//			{
//				for (uactorcomponent* actorcomponent : actor->ownedcomponents)
//				{
//					uprimitivecomponent* primitive = dynamic_cast<uprimitivecomponent*>(actorcomponent);
//					if (primitive)
//					{
//						fray modelray = getmodelray(worldray, primitive);		//actor로부터 primitive를 얻고 ray를 모델 좌표계로 변환함
//						
//						if (israyprimitivecollided(modelray, primitive, &primitivedistance)) //ray와 primitive가 충돌했다면 거리 테스트 후 가까운 actor picking
//						{
//							if (primitivedistance < shortestdistance)
//							{
//								shortestactor = actor;
//								shortestdistance = primitivedistance;
//							}
//						}
//					}
//				}
//			}
//		}
//
//		level->setselectedactor(shortestactor);
//	}
//	
//
//	return shortestactor;
//}
//
//
//fray converttoworldray(int pixelx, int pixely, int viewportw, int viewporth, const fviewprojconstants& viewprojconstantsinverse)
//{
//	//screen to ndc
//	float ndcx = (pixelx / viewportw) * 2 - 1;
//	float ndcy = 1 - (pixely / viewporth) * 2; //window는 좌측 위가 0,0이지만 ndc는 좌측 아래가 0,0(y축의 부호가 반대)
//
//	//ndc에서의 ray
//	fvector4 ndcfarpointray{ ndcx, ndcy, 1.0f, 1.0f };	//far plane과 만나는 점
//
//	fvector4 viewfarpointray = ndcfarpointray * viewprojconstantsinverse.projection; // viewing frustum의 far plane과 ray가 만나는 점
//
//	fvector4 cameraposition{ viewprojconstantsinverse.view.data[0][3], viewprojconstantsinverse.view.data[1][3], viewprojconstantsinverse.view.data[2][3], viewprojconstantsinverse.view.data[3][3] };
//	fvector4 direction = (viewfarpointray * viewprojconstantsinverse.view - cameraposition);
//	direction.normalize();
//	fray ray{ cameraposition, direction};
//
//	return ray;
//}
//
//
//
//fray getmodelray(const fray& ray, uprimitivecomponent* primitive)
//{
//	fvector rotation = primitive->getrelativerotation();
//	fvector scale = primitive->getrelativescale3d();
//	fvector location = primitive->getrelativelocation();
//	const float roll = fvector::getdegreetoradian(rotation.x);
//	const float pitch = fvector::getdegreetoradian(rotation.y);
//	const float yaw = fvector::getdegreetoradian(rotation.z);
//
//	scale.x = 1 / scale.x;
//	scale.y = 1 / scale.y;
//	scale.z = 1 / scale.z;
//	location.x *= -1;
//	location.y *= -1;
//	location.z *= -1;
//	fmatrix c = fmatrix::translationmatrix(fvector(0, 0, 0));
//	fmatrix s = fmatrix::scalematrix(scale);
//	fmatrix r = fmatrix::rotationmatrix({ -roll,-pitch, -yaw });
//	fmatrix t = fmatrix::translationmatrix(location);
//
//	fmatrix modelinverse = c * s * r * t;
//
//	fray modelray;
//	modelray.origin = ray.origin * modelinverse;
//	modelray.direction = ray.direction * modelinverse;
//
//	return modelray;
//}
//
////개별 primitive와 ray 충돌 검사
//bool israyprimitivecollided(const fray& ray, uprimitivecomponent* primitive, float* shortestdistance) 
//{
//	fray modelray = getmodelray(ray, primitive);
//
//	const tarray<fvertex>* vertices = primitive->getverticesdata();
//
//	bool bishit = false;
//	for (int a = 0;a < vertices->size();a = a + 3)	//삼각형 단위로 vertex 위치정보 읽음
//	{
//		const fvector& vertex1 = (*vertices)[a].position;
//		const fvector& vertex2 = (*vertices)[a+1].position;
//		const fvector& vertex3 = (*vertices)[a+2].position;
//
//		float distance=d3d11_float32_max;		//distance 초기화
//		
//		if (israytrianglecollided(modelray, vertex1, vertex2, vertex3, fmatrix::getmodelmatrix(primitive),  &distance))	//ray와 삼각형이 충돌하면 거리 비교 후 최단거리 갱신
//		{
//			bishit = true;
//			if (distance < *shortestdistance)
//			{
//				*shortestdistance = distance;
//			}
//		}
//	}
//
//	return bishit;
//
//}
//
//bool israytrianglecollided(const fray& ray, const fvector& vertex1, const fvector& vertex2, const fvector& vertex3, const fmatrix& modelmatrix, float* distance)
//{
//
//	fvector4 cameraforward = mycamera->getforwardvector();	//카메라 정보 필요
//	float nearz=mycamera->getnearz();
//	float farz=mycamera->getfarz();
//	fmatrix modeltransform; //primitive로부터 얻어내야함.(카메라가 처리하는게 나을듯)
//
//
//	//삼각형 내의 점은 e1*v + e2*u + vertex1.position으로 표현 가능( 0<= u + v <=1,  y>=0, v>=0 )
//	//ray.direction * t + ray.origin = e1*v + e2*u + vertex1.position을 만족하는 t u v값을 구해야 함.
//	//[e1 e2 raydirection][v u t] = [rayorigin-vertex1.position]에서 cramer's rule을 이용해서 t u v값을 구하고
//	//u v값이 저 위의 조건을 만족하고 t값이 카메라의 near값 이상이어야 함.
//	fvector raydirection{ ray.direction.x, ray.direction.y,ray.direction.z };
//	fvector rayorigin{ ray.origin.x, ray.origin.y , ray.origin.z };
//	fvector e1 = vertex2 - vertex1;
//	fvector e2 = vertex3 - vertex1;
//	fvector result = (rayorigin - vertex1); //[e1 e2 -raydirection]x = [rayorigin - vertex1.position] 의 result임.
//
//	
//	fvector crosse2ray = e2.cross(raydirection);
//	fvector crosse1result = e1.cross(result);
//
//	float determinant = e1.dot(crosse2ray);
//
//	float noinverse = 0.0001f; //0.0001이하면 determinant가 0이라고 판단=>역행렬 존재 x
//	if (determinant <= noinverse || -determinant <= noinverse)
//	{
//		return false;
//	}
//
//	
//
//	float v = result.dot(crosse2ray)/determinant;	//cramer's rule로 해를 구했음. 이게 0미만 1초과면 충돌하지 않음.
//
//	if (v < 0 || v>1)
//	{
//		return false;
//	}
//
//	float u = raydirection.dot(crosse1result) / determinant;
//	if (u < 0 || u + v > 1)
//	{
//		return false;
//	}
//
//
//	float t = e2.dot(crosse1result) / determinant;
//
//	fvector hitpoint = rayorigin + raydirection*t;	//모델 좌표계에서의 충돌점
//	fvector4 hitpoint4{ hitpoint.x, hitpoint.y, hitpoint.z, 1 };
//	//이제 이것을 월드 좌표계로 변환해서 view frustum안에 들어가는지 판단할 것임.(near, far plane만 테스트하면 됨)
//
//	fvector4 worldhitpoint4 = hitpoint4 * modeltransform;
//
//	fvector4 distancevector = worldhitpoint4 - ray.origin;
//	
//	if (distancevector.dot3(cameraforward) < nearz || distancevector.dot3(cameraforward) > farz)	//worldhitpoint가 view frustum 안에 들어가는지 확인
//	{
//		return false;
//	}
//	
//	*distance = distancevector.length();
//	return true;
//}
