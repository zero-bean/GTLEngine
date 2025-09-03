#pragma once

#define ROWS 12
#define COLS 9


struct FVector3
{
    FVector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
    float x, y, z;
};

struct FVector4
{
    FVector4(float _r = 0, float _g = 0, float _b = 0, float _a = 0) : r(_r), g(_g), b(_b), a(_a) {}
    float r, g, b, a;
};

struct FConstants
{
    FVector4 Color;
    FVector3 WorldPosition;
    float Scale;
    DirectX::XMFLOAT4X4 rotation;
};

struct FVertexSimple
{
    float x, y, z;
    float r, g, b, a;
    float u, v;
};

// 공 상태
enum class eBallState
{
    Idle = 0,
    Fallen = 1,
    Die = 2
};

// 공 타입
enum class eBallType
{
    Static = 0,
    Dynamic = 1
};


namespace CalculateUtil
{
    inline const FVector3 operator* (const FVector3& lhs, float rhs) { return FVector3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs); }
    inline const FVector3 operator/ (const FVector3& lhs, float rhs) { return FVector3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs); }
    inline const FVector3 operator+ (const FVector3& lhs, const FVector3& rhs) { return FVector3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); }
    inline const FVector3 operator- (const FVector3& lhs, const FVector3& rhs) { return FVector3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }

    inline const float Length(const FVector3& lhs) { return sqrtf(lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z); }
    inline const float Dot(const FVector3& lhs, const FVector3& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }
}


namespace RandomUtil
{
    inline float CreateRandomFloat(const float fMin, const float fMax) { return fMin + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (fMax - fMin))); }
}
