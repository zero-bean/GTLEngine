#include "Ball.h"
#include "Sphere.h"
Ball::Ball()
{

}
Ball::~Ball()
{
    Release();
}
void Ball::Initialize(const Renderer& renderer)
{
    //버텍스 버퍼 셋
    NumVertices = sizeof(SphereVertices) / sizeof(FVertexSimple);
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = sizeof(SphereVertices);
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { SphereVertices };
    renderer.GetDevice()->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &VertexBuffer);

    //상수 버퍼 셋
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = (sizeof(FConstants) + 0xf) & 0xfffffff0; // 16바이트 정렬
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    renderer.GetDevice()->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);

    // 변수 설정
    WorldPosition = { 0.0f, -0.9f, 0.0f };
    Scale = 0.11f;
    //Radius = 40.0f * (2.0f / 720.0f);
    BallType = eBallType::Dynamic;
}
void Ball::Update(Renderer& renderer)
{
    //if (BallType == eBallType::Static)
    //{
    //    return;
    //}
    // update
    WorldPosition = CalculateUtil::operator+(WorldPosition, Velocity);
    renderer.UpdateConstant(ConstantBuffer, WorldPosition, Scale, RotationDeg);
    //lateupdate
}
void Ball::Render(Renderer& renderer)
{
    renderer.Render(ConstantBuffer, VertexBuffer, NumVertices);
}

void Ball::Release()
{
}
void Ball::Collidable()
{
}