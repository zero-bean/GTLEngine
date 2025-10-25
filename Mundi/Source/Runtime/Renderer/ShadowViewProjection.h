#pragma once

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

	// DirectionalLight용 Shadow VP 행렬 생성 (여기서 만들어 쓰세요 영빈씨)
	// @param Direction - 라이트 방향
	// @param SceneBounds - 씬의 바운딩 박스
	// @param ViewFrustum - 카메라 절두체
	// static FShadowViewProjection CreateForDirectionalLight(
	//     const FVector& Direction,
	//     const FBox& SceneBounds,
	//     const FFrustumPlanes& ViewFrustum);
};
