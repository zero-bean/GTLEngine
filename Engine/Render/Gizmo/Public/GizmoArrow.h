#pragma once

class GizmoArrow
{
public:

private:
	const TArray<FVertex>* Vertices = nullptr;
	ID3D11Buffer* Vertexbuffer = nullptr;
	UINT NumVertices = 0;
};

