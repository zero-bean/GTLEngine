#pragma once

#define BLUE_R 0.0f
#define BLUE_G 0.0f
#define BLUE_B 1.0f
#define BLUE_A 1.0f

FVertexSimple arrowVertices[] =
{
    // 몸통 (직사각형) - 시계 방향
    {  0.05f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 1.0f, 0.0f }, // 우상
    {  0.05f, -0.2f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 1.0f, 1.0f }, // 우하
    { -0.05f, -0.2f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 0.0f, 1.0f }, // 좌하
    { -0.05f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 0.0f, 0.0f }, // 좌상
    {  0.05f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 1.0f, 0.0f }, // 우상 (중복)
    { -0.05f, -0.2f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 0.0f, 1.0f }, // 좌하 (중복)

    // 화살표 머리 (삼각형) - 시계 방향
    {  0.0f,  0.6f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 0.5f, 0.0f }, // 상단 중심
    {  0.1f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 1.0f, 1.0f }, // 우하
    { -0.1f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A, 0.0f, 1.0f }  // 좌하
};