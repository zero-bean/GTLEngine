#include "Button.h"
#include "Renderer.h"
#include "ScreenUtil.h"


Button::Button(const wchar_t * newImagePath, const std::pair<float,float> newSize)
{
    imagePath = newImagePath;
    size = newSize;
}

Button::~Button()
{
    Release();
}

void Button::Initialize(Renderer& renderer)
{
    // 텍스처 로드
      TextureSet textureSet = renderer.LoadTextureSet(imagePath);
    SetTextureSet(textureSet);

    FVertexSimple ButtonVertices[] = {
        // 첫 번째 삼각형 (좌상, 우상, 좌하)
        { -size.first,  size.second, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f },  // 좌상
        {  size.first,  size.second, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },  // 우상
        { -size.first, -size.second, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },  // 좌하

        // 두 번째 삼각형 (좌하, 우상, 우하)
        { -size.first, -size.second, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },  // 좌하
        {  size.first,  size.second, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },  // 우상
        {  size.first, -size.second, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f }   // 우하
    };

    //버텍스 버퍼 셋
    NumVertices = sizeof(ButtonVertices) / sizeof(FVertexSimple);
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = sizeof(ButtonVertices);
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { ButtonVertices };
    renderer.GetDevice()->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &VertexBuffer);

    //상수 버퍼 셋
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = (sizeof(FConstants) + 0xf) & 0xfffffff0; // 16바이트 정렬
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    renderer.GetDevice()->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
    WorldPosition = { 0.0f, 0.0f, 0.0f };
    Scale = 1.0f;
}

void Button::Update(Renderer& renderer)
{
    renderer.UpdateConstant(ConstantBuffer, WorldPosition, Scale, 0);
}

void Button::Render(Renderer& renderer)
{
    BindTexture(renderer.GetDeviceContext(), 0);
    renderer.Render(ConstantBuffer, VertexBuffer, NumVertices);
}

void Button::Release()
{
}

void Button::Collidable()
{
}

bool Button::IsOverlapWithMouse(int mouseX, int mouseY)
{
    std::pair<float, float> mouseNdcPos = ScreenUtil::ScreenPosToNDC({ mouseX, mouseY });
    float aspectRatio = ScreenUtil::GetAspectRatio();
    if (WorldPosition.x - size.first/aspectRatio >= mouseNdcPos.first|| mouseNdcPos.first >= WorldPosition.x + size.first/aspectRatio) return false;
    if (WorldPosition.y -size.second >= mouseNdcPos.second|| mouseNdcPos.second >= WorldPosition.y + size.second) return false;
     return true;
}

void Button::OnClick()
{
    if (callback)
    {
        callback();
    }
}
