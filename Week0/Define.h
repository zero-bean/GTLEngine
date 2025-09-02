#pragma once


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

};

struct FVertexSimple
{
    float x, y, z;
    float r, g, b, a;
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