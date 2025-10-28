#pragma once

#include "Frustum.h"  // GetFrustumCornersWorldSpace 함수 사용

// Shadow rendering에 필요한 View/Projection 행렬 집합
// 주의: 이 헤더를 사용하기 전에 pch.h가 include되어야 함 (FMatrix, FVector 정의 필요)
struct FShadowViewProjection
{
	FMatrix View;
	FMatrix Projection;
	FMatrix ViewProjection;

	// SpotLight용 Shadow VP 행렬 생성
	// @param Position - 라이트 월드 위치
	// @param Direction - 라이트 방향
	// @param FOV - 전체 원뿔 각도 (OuterConeAngle * 2)
	// @param AttenuationRadius - 라이트 감쇠 반경 (Far plane으로 사용)
	// @param NearPlane - Near clipping plane (기본값 0.1f)
	static FShadowViewProjection CreateForSpotLight(
		const FVector& Position,
		const FVector& Direction,
		float FOV,
		float AttenuationRadius,
		float NearPlane = 0.1f)
	{
		FShadowViewProjection Result;

		// View 행렬 계산
		FVector LightUp = FVector(0, 1, 0);
		// Direction과 Up 벡터가 평행하면 다른 Up 벡터 사용
		if (FMath::Abs(FVector::Dot(Direction, LightUp)) > 0.99f)
		{
			LightUp = FVector(1, 0, 0);
		}

		Result.View = FMatrix::LookAtLH(Position, Position + Direction, LightUp);

		// Projection 행렬 계산 (정사각형 shadow map 가정)
		float AspectRatio = 1.0f;
		Result.Projection = FMatrix::PerspectiveFovLH(
			DegreesToRadians(FOV),
			AspectRatio,
			NearPlane,
			AttenuationRadius);

		// ViewProjection 행렬 계산
		Result.ViewProjection = Result.View * Result.Projection;

		return Result;
	}

	// DirectionalLight용 Shadow VP 행렬 생성 (View Frustum 기반)
	// @param Direction - 라이트 방향 (정규화되지 않아도 됨)
	// @param CameraView - 카메라의 View 행렬
	// @param CameraProjection - 카메라의 Projection 행렬
	// @param ShadowExtension - 그림자 범위 확장 비율 (기본값 0.2 = 20% 확장)
	static FShadowViewProjection CreateForDirectionalLight(
		const FVector& Direction,
		const FMatrix& CameraView,
		const FMatrix& CameraProjection,
		float ShadowExtension = 0.2f)
	{
		FShadowViewProjection Result;

		// === 1. Camera Frustum의 8개 코너를 월드 공간으로 변환 ===
		TArray<FVector> FrustumCorners;

		// ViewProjection의 역행렬 계산: (V * P)^-1 = P^-1 * V^-1
		// Projection이 Perspective인지 Orthographic인지 알 수 없으므로
		// 간단한 방법: ViewProjection을 재계산하고 역행렬 구하기
		FMatrix InvView = CameraView.InverseAffine();
		FMatrix InvProj = CameraProjection.InversePerspectiveProjection(); // 일단 Perspective 가정
		FMatrix InvViewProj = InvProj * InvView;

		GetFrustumCornersWorldSpace(InvViewProj, FrustumCorners);

		// === 2. Light View 행렬 생성 ===
		FVector LightDir = Direction.GetNormalized();

		// Up vector 선택 (LightDir와 평행하지 않은 벡터)
		FVector Up = (FMath::Abs(LightDir.Y) < 0.99f)
			? FVector(0, 1, 0)
			: FVector(1, 0, 0);

		// Frustum 중심점 계산 (8개 코너의 평균)
		FVector FrustumCenter = FVector::Zero();
		for (int i = 0; i < 8; ++i)
		{
			FrustumCenter += FrustumCorners[i];
		}
		FrustumCenter /= 8.0f;

		// Light Position: Frustum 중심에서 라이트 방향 반대쪽
		// 충분히 멀리 배치하여 카메라 프러스텀 뒤쪽의 그림자 캐스터도 포함
		FVector LightPos = FrustumCenter - LightDir;

		Result.View = FMatrix::LookAtLH(LightPos, LightPos + LightDir, Up);

		// === 3. Frustum 코너를 Light Space로 변환하여 AABB 계산 ===
		float MinX = FLT_MAX, MaxX = -FLT_MAX;
		float MinY = FLT_MAX, MaxY = -FLT_MAX;
		float MinZ = FLT_MAX, MaxZ = -FLT_MAX;

		for (int i = 0; i < 8; ++i)
		{
			// World Space 코너를 Light View Space로 변환
			FVector LightSpaceCorner = FrustumCorners[i] * Result.View;

			MinX = FMath::Min(MinX, LightSpaceCorner.X);
			MaxX = FMath::Max(MaxX, LightSpaceCorner.X);
			MinY = FMath::Min(MinY, LightSpaceCorner.Y);
			MaxY = FMath::Max(MaxY, LightSpaceCorner.Y);
			MinZ = FMath::Min(MinZ, LightSpaceCorner.Z);
			MaxZ = FMath::Max(MaxZ, LightSpaceCorner.Z);
		}

		// === 4. AABB 확장 및 Orthographic Projection 생성 ===
		float Width = MaxX - MinX;
		float Height = MaxY - MinY;
		float Depth = MaxZ - MinZ;

		// XY 평면 확장 (기본 20%)
		float ExtendX = Width * ShadowExtension;
		float ExtendY = Height * ShadowExtension;
		MinX -= ExtendX;
		MaxX += ExtendX;
		MinY -= ExtendY;
		MaxY += ExtendY;
		Width = MaxX - MinX;
		Height = MaxY - MinY;

		// Near Plane을 뒤로 확장하여 카메라 뒤의 오브젝트 포함
		float NearExtension = Depth * 2.0f;
		MinZ -= NearExtension;
		Depth = MaxZ - MinZ;

		// 중심을 원점으로 맞춘 Orthographic Projection
		Result.Projection = FMatrix::OrthoLH(Width, Height, 0.0f, Depth);

		// AABB 중심을 원점으로 이동시키기 위한 오프셋 적용
		float CenterX = (MaxX + MinX) * 0.5f;
		float CenterY = (MaxY + MinY) * 0.5f;

		// View 행렬에 오프셋 추가 (Translation 조정)
		Result.View.M[3][0] -= CenterX;
		Result.View.M[3][1] -= CenterY;
		Result.View.M[3][2] -= MinZ;  // Near plane을 0으로 맞춤

		// === 5. ViewProjection 계산 ===
		Result.ViewProjection = Result.View * Result.Projection;

		return Result;
	}

	// DirectionalLight용 Shadow VP 행렬 생성 (CSM용 - Frustum 코너를 직접 받음)
	// @param Direction - 라이트 방향 (정규화되지 않아도 됨)
	// @param FrustumCorners - Frustum의 8개 코너 (World Space)
	// @param ShadowExtension - 그림자 범위 확장 비율 (기본값 0.2 = 20% 확장)
	static FShadowViewProjection CreateForDirectionalLight_FromCorners(
		const FVector& Direction,
		const TArray<FVector>& FrustumCorners,
		float ShadowExtension = 0.2f)
	{
		FShadowViewProjection Result;

		// === Light View 행렬 생성 ===
		FVector LightDir = Direction.GetNormalized();

		// Up vector 선택 (LightDir와 평행하지 않은 벡터)
		FVector Up = (FMath::Abs(LightDir.Y) < 0.99f)
			? FVector(0, 1, 0)
			: FVector(1, 0, 0);

		// Frustum 중심점 계산 (8개 코너의 평균)
		FVector FrustumCenter = FVector::Zero();
		for (int i = 0; i < 8; ++i)
		{
			FrustumCenter += FrustumCorners[i];
		}
		FrustumCenter /= 8.0f;

		FVector LightPos = FrustumCenter - LightDir;

		Result.View = FMatrix::LookAtLH(LightPos, LightPos + LightDir, Up);

		// === Frustum 코너를 Light Space로 변환하여 AABB 계산 ===
		float MinX = FLT_MAX, MaxX = -FLT_MAX;
		float MinY = FLT_MAX, MaxY = -FLT_MAX;
		float MinZ = FLT_MAX, MaxZ = -FLT_MAX;

		for (int i = 0; i < 8; ++i)
		{
			// World Space 코너를 Light View Space로 변환
			FVector LightSpaceCorner = FrustumCorners[i] * Result.View;

			MinX = FMath::Min(MinX, LightSpaceCorner.X);
			MaxX = FMath::Max(MaxX, LightSpaceCorner.X);
			MinY = FMath::Min(MinY, LightSpaceCorner.Y);
			MaxY = FMath::Max(MaxY, LightSpaceCorner.Y);
			MinZ = FMath::Min(MinZ, LightSpaceCorner.Z);
			MaxZ = FMath::Max(MaxZ, LightSpaceCorner.Z);
		}

		// 해결책 2: AABB를 확장하여 프러스텀 밖의 그림자 캐스터 포함
		float Width = MaxX - MinX;
		float Height = MaxY - MinY;
		float Depth = MaxZ - MinZ;

		// XY 평면 확장 (기본 20%)
		float ExtendX = Width * ShadowExtension;
		float ExtendY = Height * ShadowExtension;
		MinX -= ExtendX;
		MaxX += ExtendX;
		MinY -= ExtendY;
		MaxY += ExtendY;
		Width = MaxX - MinX;
		Height = MaxY - MinY;

		// 해결책 3: Near Plane을 뒤로 확장하여 카메라 뒤의 오브젝트 포함
		float NearExtension = Depth * 2.0f; // Depth의 2배만큼 뒤로 확장
		MinZ -= NearExtension;
		Depth = MaxZ - MinZ;

		// === Orthographic Projection 생성 (확장된 AABB 범위 기반) ===
		// 중심을 원점으로 맞춘 Orthographic Projection
		Result.Projection = FMatrix::OrthoLH(Width, Height, 0.0f, Depth);

		// AABB 중심을 원점으로 이동시키기 위한 오프셋 적용
		float CenterX = (MaxX + MinX) * 0.5f;
		float CenterY = (MaxY + MinY) * 0.5f;

		// View 행렬에 오프셋 추가 (Translation 조정)
		Result.View.M[3][0] -= CenterX;
		Result.View.M[3][1] -= CenterY;
		Result.View.M[3][2] -= MinZ;  // Near plane을 0으로 맞춤

		// === ViewProjection 계산 ===
		Result.ViewProjection = Result.View * Result.Projection;

		return Result;
	}

	// PointLight용 Cube Map Shadow VP 행렬 6개 생성
	// @param Position - 라이트 월드 위치
	// @param AttenuationRadius - 라이트 감쇠 반경 (Far plane으로 사용)
	// @param NearPlane - Near clipping plane (기본값 0.1f)
	// @return 6개의 VP 행렬 배열
	static TArray<FShadowViewProjection> CreateForPointLightCube(
		const FVector& Position,
		float AttenuationRadius,
		float NearPlane = 0.01f)
	{
		TArray<FShadowViewProjection> Results;
		Results.Reserve(6);

		// Cube Map의 6개 면에 대한 방향 및 Up 벡터
		// DirectX Cube Map Face 순서: +X, -X, +Y, -Y, +Z, -Z
		struct FCubeFace
		{
			FVector Direction;
			FVector Up;
		};

		// Cube Map Face 정의
		FCubeFace CubeFaces[6] =
		{
			{ FVector( 1,  0,  0), FVector(0,  1,  0) },  // Slice 0: +X (Right)
			{ FVector(-1,  0,  0), FVector(0,  1,  0) },  // Slice 1: -X (Left)
			{ FVector( 0,  1,  0), FVector(0,  0,  -1) }, // Slice 2: +Y (Up)
			{ FVector( 0, -1,  0), FVector(0,  0,  1) }, // Slice 3: -Y (Down)
			{ FVector( 0,  0,  1), FVector(0,  1,  0) },  // Slice 4: +Z (Forward)                       
			{ FVector( 0,  0, -1), FVector(0,  1,  0) }   // Slice 5: -Z (Back)
		};

		// 각 면에 대한 VP 행렬 생성
		for (int i = 0; i < 6; i++)
		{
			FShadowViewProjection VP;

			// View 행렬 생성
			VP.View = FMatrix::LookAtLH(
				Position,
				Position + CubeFaces[i].Direction,
				CubeFaces[i].Up);

			// Projection 행렬 생성 (90도 FOV, 정사각형 aspect ratio)
			VP.Projection = FMatrix::PerspectiveFovLH(
				DegreesToRadians(90.0f),  // 90도 FOV
				1.0f,                      // 정사각형 aspect ratio
				NearPlane,
				AttenuationRadius);

			// ViewProjection 계산
			VP.ViewProjection = VP.View * VP.Projection;

			Results.Add(VP);
		}

		return Results;
	}

};
