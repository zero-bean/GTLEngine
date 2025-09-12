#pragma once

extern TArray<FVertexPosColor> cube_vertices;
extern TArray<uint32> cube_indices;

extern TArray<FVertexPosColor> plane_vertices;
extern TArray<uint32> plane_indices;

extern TArray<FVertexPosColor> sphere_vertices;
extern TArray<uint32> sphere_indices;

////////////////////////////////////////////////////////////////////////////////////////
// 기즈모 (Vertex 있음, Index 없음), 필요하다면 알아서 수정할 것 - PYB -
extern const TArray<FVertexPosColor> gizmo_arrow_vertices;
extern const TArray<FVertexPosColor> gizmo_scale_handle_vertices;

class GridGenerator
{
public:
	static TArray<FVertexPosColor> CreateGridVertices(float gridSize, int32 gridCount);
	static TArray<FVertexPosColor> CreateRotationHandleVertices();
};
////////////////////////////////////////////////////////////////////////////////////////
