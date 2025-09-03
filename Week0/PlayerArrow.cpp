#include "PlayerArrow.h"
#include "Arrow.h"
#include "Renderer.h"

PlayerArrow::PlayerArrow()
{
}

PlayerArrow::~PlayerArrow()
{
    Release();
}


void PlayerArrow::Initialize(Renderer& renderer)
{
    // 텍스처 로드
    TextureSet textureSet = renderer.LoadTextureSet(L"assets/sprite.png");
    SetTextureSet(textureSet);

    //버텍스 버퍼 셋
    NumVertices = sizeof(ArrowVertices) / sizeof(FVertexSimple);

    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = sizeof(ArrowVertices);
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { ArrowVertices };

    renderer.GetDevice()->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &VertexBuffer);



    //상수 버퍼 셋
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = (sizeof(FConstants) + 0xf) & 0xfffffff0; // 16바이트 정렬
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    renderer.GetDevice()->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);



    WorldPosition = { 0.0f, -0.9f, 0.0f };
    Scale = 0.4f;
}

void PlayerArrow::Update(Renderer& renderer)
{
    renderer.UpdateConstant(ConstantBuffer, WorldPosition, Scale, RotationDeg);
}

void PlayerArrow::Render(Renderer& renderer)
{
    BindTexture(renderer.GetDeviceContext(), 0);
    renderer.Render(ConstantBuffer, VertexBuffer, NumVertices);
}

void PlayerArrow::Release()
{
}

void PlayerArrow::Collidable()
{

}
