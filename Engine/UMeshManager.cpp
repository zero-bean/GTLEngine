#include "stdafx.h"
#include "UMeshManager.h"
#include "UClass.h"
// 임시
namespace {
	static void BuildWireCubeUnit(
		TArray<FVertexPosColor>& outVerts,
		TArray<uint32>& outIdx)
	{
		auto V = [](float x, float y, float z)->FVertexPosColor {
			FVertexPosColor v{};
			v.x = x; v.y = y; v.z = z;
			v.r = 1; v.g = 1; v.b = 0; v.a = 1; // ★ 전부 노랑
			return v;
			};

		// [-1,+1]^3
		outVerts = {
			V(-1,-1,-1), //0
			V(1,-1,-1), //1
			V(1, 1,-1), //2
			V(-1, 1,-1), //3
			V(-1,-1, 1), //4
			V(1,-1, 1), //5
			V(1, 1, 1), //6
			V(-1, 1, 1)  //7
		};

		// 12 edges → 24 indices (LINELIST)
		outIdx = {
			// bottom (-Y)
			0,1, 1,2, 2,3, 3,0,
			// top (+Y)
			4,5, 5,6, 6,7, 7,4,
			// vertical
			0,4, 1,5, 2,6, 3,7
		};
	}
	// [-1,+1]^3, 전부 노랑, 인덱스 없이 24정점으로 12개 선분
	static void BuildWireCubeUnit_NoIndex(TArray<FVertexPosColor>& outVerts)
	{
		auto V = [](float x, float y, float z)->FVertexPosColor {
			FVertexPosColor v{};
			v.x = x; v.y = y; v.z = z;
			v.r = 1; v.g = 1; v.b = 0; v.a = 1;
			return v;
			};

		const FVector p[8] = {
			{-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
			{-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
		};

		auto addEdge = [&](int a, int b) {
			outVerts.push_back(V(p[a].X, p[a].Y, p[a].Z));
			outVerts.push_back(V(p[b].X, p[b].Y, p[b].Z));
			};

		outVerts.clear(); outVerts.reserve(24);
		// bottom
		addEdge(0, 1); addEdge(1, 2); addEdge(2, 3); addEdge(3, 0);
		// top
		addEdge(4, 5); addEdge(5, 6); addEdge(6, 7); addEdge(7, 4);
		// vertical
		addEdge(0, 4); addEdge(1, 5); addEdge(2, 6); addEdge(3, 7);
	}
}

IMPLEMENT_UCLASS(UMeshManager, UEngineSubsystem)
UMesh* UMeshManager::CreateMeshInternal(const TArray<FVertexPosColor>& vertices,
	D3D_PRIMITIVE_TOPOLOGY primitiveType)
{
	// vector의 데이터 포인터와 크기를 ConvertVertexData에 전달
	auto convertedVertices = FVertexPosColor4::ConvertVertexData(vertices.data(), static_cast<int32>(vertices.size()));
	UMesh* mesh = new UMesh(convertedVertices, primitiveType);
	return mesh;
}

UMesh* UMeshManager::CreateMeshInternal(const TArray<FVertexPosColor>& vertices, const TArray<uint32>& indices, D3D_PRIMITIVE_TOPOLOGY primitiveType)
{
	// vector의 데이터 포인터와 크기를 ConvertVertexData에 전달
	TArray<FVertexPosColor4> convertedVertices = FVertexPosColor4::ConvertVertexData(vertices.data(), static_cast<int32>(vertices.size()));
	UMesh* mesh = new UMesh(convertedVertices, indices, primitiveType);
	return mesh;
}

UMesh* UMeshManager::CreateMeshInternal(const TArray<FVertexPosTexCoord>& vertices, const TArray<uint32>& indices,
	D3D_PRIMITIVE_TOPOLOGY primitiveTypeda)
{
	UMesh* mesh = new UMesh(vertices, indices, primitiveType);
	return mesh;
}

// 생성자
UMeshManager::UMeshManager()
{
	meshes["Sphere"] = CreateMeshInternal(sphere_vertices, sphere_indices);
	meshes["Plane"] = CreateMeshInternal(plane_vertices, plane_indices);
	meshes["Cube"] = CreateMeshInternal(cube_vertices, cube_indices);
	meshes["Quad"] = CreateMeshInternal(quad_vertices, quad_indices);

	meshes["GizmoGrid"] = CreateMeshInternal(GridGenerator::CreateGridVertices(1, 1000), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	meshes["GizmoAxis"] = CreateMeshInternal(GridGenerator::CreateAxisVertices(1, 1000), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	meshes["GizmoArrow"] = CreateMeshInternal(gizmo_arrow_vertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	meshes["GizmoRotationHandle"] = CreateMeshInternal(GridGenerator::CreateRotationHandleVertices(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	meshes["GizmoScaleHandle"] = CreateMeshInternal(gizmo_scale_handle_vertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 인덱스 있는 오버로드 사용!
	{
		TArray<FVertexPosColor> verts; TArray<uint32> idx;
		BuildWireCubeUnit(verts, idx);
		meshes["UnitCube_Wire"] = CreateMeshInternal(
			verts, idx, D3D11_PRIMITIVE_TOPOLOGY_LINELIST
		); 
	}
}

// 소멸자 (메모리 해제)
UMeshManager::~UMeshManager()
{
	for (auto& pair : meshes)
	{
		delete pair.second;
	}
	meshes.clear();
}

bool UMeshManager::Initialize(URenderer* renderer)
{
	if (!renderer) return false;

	try
	{
		for (const auto& var : meshes)
		{
			var.second->Init(renderer->GetDevice());
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "UMeshManager::Initialize failed: " << e.what() << std::endl;
		return false;
	}
	catch (...)
	{
		std::cerr << "UMeshManager::Initialize failed: unknown exception" << std::endl;
		return false;
	}

	return true;
}

UMesh* UMeshManager::RetrieveMesh(FString meshName)
{
	auto itr = meshes.find(meshName);
	if (itr == meshes.end()) return nullptr;
	return itr->second;
}
