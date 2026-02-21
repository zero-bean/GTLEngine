#pragma once
#include "stdafx.h"
#include "UEngineStatics.h"
#include "Vector.h"

struct FVector4
{
    float X;
    float Y;
    float Z;
    float W;
    FVector4(float _x = 0, float _y = 0, float _z = 0, float _w = 1) : X(_x), Y(_y), Z(_z), W(_w) {}

    float& operator[](int32 index)
    {
        switch (index)
        {
        case 0: return X;
        case 1: return Y;
        case 2: return Z;
        case 3: return W;
        default: throw std::out_of_range("FVector4 index out of range");
        }
    }

    // 동차좌표 → 3D (안전한 동차화)
    void Homogenize(float eps = 1e-6f)
    {
        if (fabsf(W) < eps) {
            // W≈0이면 투영 불능. 쓰임새에 따라 0으로 두거나 1로 보정.
            // 여기선 안전 보정: 변화 없이 W만 1로 둠.
            W = 1.0f;
            return;
        }
        X /= W; Y /= W; Z /= W; W = 1.0f;
    }
    FVector ToVec3Homogenized(float eps = 1e-6f) const {
        if (fabsf(W) < eps) return FVector(X, Y, Z); // 최선 보정
        return FVector(X / W, Y / W, Z / W);
    }

    // 내적 (4차원)
    float Dot(const FVector4& Other) const
    {
        return X * Other.X + Y * Other.Y + Z * Other.Z + W * Other.W;
    }

    // 길이 (4차원)
    float Length() const
    {
        return sqrtf(X * X + Y * Y + Z * Z + W * W);
    }

    // 길이 (XYZ만)
    float Length3() const
    {
        return sqrtf(X * X + Y * Y + Z * Z);
    }

    // 정규화 (4차원)
    FVector4 Normalized() const
    {
        float len = Length();
        if (len == 0.f) return FVector4();
        return FVector4(X / len, Y / len, Z / len, W / len);
    }

    // XYZ만 정규화
    FVector4 Normalized3() const
    {
        float len = Length3();
        if (len == 0.f) return FVector4();
        return FVector4(X / len, Y / len, Z / len, W);
    }

    // 연산자 오버로딩
    FVector4 operator+(const FVector4& Other) const
    {
        return FVector4(X + Other.X, Y + Other.Y, Z + Other.Z, W + Other.W);
    }

    FVector4 operator-(const FVector4& Other) const
    {
        return FVector4(X - Other.X, Y - Other.Y, Z - Other.Z, W - Other.W);
    }

    FVector4 operator*(float Scalar) const
    {
        return FVector4(X * Scalar, Y * Scalar, Z * Scalar, W * Scalar);
    }

    FVector4 operator/(float Scalar) const
    {
        return FVector4(X / Scalar, Y / Scalar, Z / Scalar, W / Scalar);
    }
};