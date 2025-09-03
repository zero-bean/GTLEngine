#include "Ball.h"
#include "Sphere.h"
#include "Renderer.h"

Ball::Ball()
{

}
Ball::~Ball()
{
    Release();
}
void Ball::Initialize(Renderer& renderer)
{
    // ball color 랜덤 생성
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    BallColor = static_cast<eBallColor>(std::rand() % 3);


    std::wstring textureFileName; 
    switch (BallColor)
    {
    case eBallColor::Red:
        textureFileName = L"assets/sprite.png";
        break;
    case eBallColor::Green:
        textureFileName = L"assets/spriteG.png";
        break;
    case eBallColor::Blue:
        textureFileName = L"assets/spriteB.png";
        break;
    }

    // 텍스처 로드
    TextureSet textureSet = renderer.LoadTextureSet(textureFileName.c_str());
    SetTextureSet(textureSet);

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

    BallState = eBallState::Idle;
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
    BindTexture(renderer.GetDeviceContext(), 0);
    renderer.Render(ConstantBuffer, VertexBuffer, NumVertices);
}

void Ball::Release()
{
}
void Ball::Collidable()
{
}