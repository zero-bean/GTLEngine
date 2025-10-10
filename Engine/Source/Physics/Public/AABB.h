#pragma once
#include "Physics/Public/BoundingVolume.h"
#include "Global/Vector.h"
#include <float.h>
#include <math.h>

#if defined(_MSC_VER)
  #include <intrin.h>
#else
  #include <immintrin.h>
#endif

// Optional: engine-wide FORCEINLINE macro
#ifndef FORCEINLINE
  #if defined(_MSC_VER)
	#define FORCEINLINE __forceinline
  #else
	#define FORCEINLINE inline __attribute__((always_inline))
  #endif
#endif

struct FAABB : public IBoundingVolume
{
	FVector Min;
	FVector Max;

	FAABB() : Min(0.f, 0.f, 0.f), Max(0.f, 0.f, 0.f) {}
	FAABB(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}

	bool IsIntersected(const FAABB& Other) const;

	bool RaycastHit(const FRay& Ray, float* OutDistance) const override;
	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::AABB; }
	bool Intersects(const IBoundingVolume& Other) const override;
	FAABB Union(const FAABB& A, const FAABB& B) const;
};

inline bool FAABB::Intersects(const IBoundingVolume& Other) const
{
	// 상대방의 타입을 확인합니다.
	switch (Other.GetType())
	{
	case EBoundingVolumeType::AABB:
	{
		// 상대방이 AABB라면, 기존의 AABB-AABB 검사를 수행합니다.
		const FAABB& OtherAABB = static_cast<const FAABB&>(Other);
		return this->IsIntersected(OtherAABB);
	}
	default:
		return false;
	}
}

inline FAABB FAABB::Union(const FAABB& A, const FAABB& B) const
{
	__m128 aMin = _mm_setr_ps(A.Min.X, A.Min.Y, A.Min.Z, 0.0f);
	__m128 bMin = _mm_setr_ps(B.Min.X, B.Min.Y, B.Min.Z, 0.0f);
	__m128 aMax = _mm_setr_ps(A.Max.X, A.Max.Y, A.Max.Z, 0.0f);
	__m128 bMax = _mm_setr_ps(B.Max.X, B.Max.Y, B.Max.Z, 0.0f);

	__m128 outMin = _mm_min_ps(aMin, bMin);
	__m128 outMax = _mm_max_ps(aMax, bMax);

	alignas(16) float tmpMin[4], tmpMax[4];
	_mm_store_ps(tmpMin, outMin);
	_mm_store_ps(tmpMax, outMax);

	return FAABB(FVector(tmpMin[0], tmpMin[1], tmpMin[2]),
				 FVector(tmpMax[0], tmpMax[1], tmpMax[2]));
}

inline bool FAABB::IsIntersected(const FAABB& Other) const
{
	return (Min.X <= Other.Max.X && Max.X >= Other.Min.X) &&
		(Min.Y <= Other.Max.Y && Max.Y >= Other.Min.Y) &&
		(Min.Z <= Other.Max.Z && Max.Z >= Other.Min.Z);
}

FORCEINLINE bool FAABB::RaycastHit(const FRay& Ray, float* OutDistance) const
{
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64)
	// Load xyz, ignore w
	__m128 o = _mm_set_ps(0.0f, Ray.Origin.Z, Ray.Origin.Y, Ray.Origin.X);
	__m128 d = _mm_set_ps(0.0f, Ray.Direction.Z, Ray.Direction.Y, Ray.Direction.X);
	__m128 bmin = _mm_set_ps(0.0f, Max.Z /*placeholder*/, Min.Y /*placeholder*/, Min.X /*placeholder*/); // we'll set properly below
	__m128 bmax = _mm_set_ps(0.0f, Max.Z, Max.Y, Max.X);

	// correct bmin packing (typo guard)
	bmin = _mm_set_ps(0.0f, Min.Z, Min.Y, Min.X);

	// Parallel mask: |d| < eps
	const __m128 eps = _mm_set1_ps(1e-8f);
	__m128 absd = _mm_andnot_ps(_mm_set1_ps(-0.0f), d); // clear sign bit
	__m128 parallelMask = _mm_cmplt_ps(absd, eps);

	// For parallel axes, the origin must lie within [min,max]
	__m128 o_ge_min = _mm_cmpge_ps(o, bmin);
	__m128 o_le_max = _mm_cmple_ps(o, bmax);
	__m128 insideSlab = _mm_and_ps(o_ge_min, o_le_max);
	__m128 parallelFail = _mm_and_ps(parallelMask, _mm_xor_ps(insideSlab, _mm_set1_ps(-0.0f))); // invert insideSlab -> outside
	// If any lane is parallel AND outside → miss
	if (_mm_movemask_ps(parallelFail) & 0x7) return false;

	// invD (safe because parallel lanes passed the test above)
	__m128 invD = _mm_div_ps(_mm_set1_ps(1.0f), d);

	// t1 = (min - o) * invD, t2 = (max - o) * invD
	__m128 t1 = _mm_mul_ps(_mm_sub_ps(bmin, o), invD);
	__m128 t2 = _mm_mul_ps(_mm_sub_ps(bmax, o), invD);

	// per-axis min/max (swap by picking)
	__m128 tmin_vec = _mm_min_ps(t1, t2);
	__m128 tmax_vec = _mm_max_ps(t1, t2);

	// Reduce: tMin = max(x,y,z), tMax = min(x,y,z)
	// (ignore w lane)
	__m128 shuffleY = _mm_shuffle_ps(tmin_vec, tmin_vec, _MM_SHUFFLE(3,3,3,1)); // y in lane 0
	__m128 shuffleZ = _mm_shuffle_ps(tmin_vec, tmin_vec, _MM_SHUFFLE(3,3,3,2)); // z in lane 0
	__m128 tmin_xy = _mm_max_ps(tmin_vec, shuffleY);
	__m128 tMinV   = _mm_max_ps(tmin_xy, shuffleZ);

	shuffleY       = _mm_shuffle_ps(tmax_vec, tmax_vec, _MM_SHUFFLE(3,3,3,1));
	shuffleZ       = _mm_shuffle_ps(tmax_vec, tmax_vec, _MM_SHUFFLE(3,3,3,2));
	__m128 tmax_xy = _mm_min_ps(tmax_vec, shuffleY);
	__m128 tMaxV   = _mm_min_ps(tmax_xy, shuffleZ);

	float tMin = _mm_cvtss_f32(tMinV);
	float tMax = _mm_cvtss_f32(tMaxV);

	if (tMin > tMax) return false;

	// If the near hit is behind the ray origin, use far
	if (tMin < 0.0f) tMin = tMax;
	if (tMin < 0.0f) return false;

	if (OutDistance) *OutDistance = tMin;
	return true;

#else
    // Scalar fallback: fast+robust merged version
    float tMin = -FLT_MAX;
    float tMax =  FLT_MAX;

    const FVector4& O = Ray.Origin;
    const FVector4& D = Ray.Direction;

    for (int i = 0; i < 3; ++i)
    {
        const float o = O[i];
        const float d = D[i];
        const float mn = Min[i];
        const float mx = Max[i];

        if (fabsf(d) < 1e-8f)
        {
            if (o < mn || o > mx) return false;
        }
        else
        {
            const float invD = 1.0f / d;
            float t1 = (mn - o) * invD;
            float t2 = (mx - o) * invD;
            if (t1 > t2) std::swap(t1, t2);
            tMin = (t1 > tMin) ? t1 : tMin;
            tMax = (t2 < tMax) ? t2 : tMax;
            if (tMin > tMax) return false;
        }
    }

    if (tMin < 0.0f) tMin = tMax;
    if (tMin < 0.0f) return false;

    if (OutDistance) *OutDistance = tMin;
    return true;
#endif
}
