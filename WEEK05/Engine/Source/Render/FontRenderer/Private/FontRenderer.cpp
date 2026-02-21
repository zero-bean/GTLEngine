#include "pch.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Asset/Public/AssetManager.h"

UFontRenderer::UFontRenderer() = default;

UFontRenderer::~UFontRenderer()
{
    Release();
}

/// @brief 폰트 렌더러 초기화
/// 셰이더, 텍스처 로드, 동적 정점 버퍼 및 상수 버퍼 생성 등을 수행
bool UFontRenderer::Initialize()
{
    URenderer& Renderer = URenderer::GetInstance();

    // 셰이더 생성
    TArray<D3D11_INPUT_ELEMENT_DESC> layoutDesc = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FFontVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FFontVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, offsetof(FFontVertex, CharIndex), D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    Renderer.CreateVertexShaderAndInputLayout(L"Asset/Shader/ShaderFont.hlsl", layoutDesc, &FontVertexShader, &FontInputLayout);
    if (!FontVertexShader || !FontInputLayout)
    {
        UE_LOG_ERROR("FontRenderer: 버텍스 셰이더 또는 입력 레이아웃 생성 실패");
        return false;
    }

    Renderer.CreatePixelShader(L"Asset/Shader/ShaderFont.hlsl", &FontPixelShader);
    if (!FontPixelShader)
    {
        UE_LOG_ERROR("FontRenderer: 픽셀 셰이더 생성 실패");
        return false;
    }

    // 폰트 텍스처 로드
    if (!LoadFontTexture())
    {
        UE_LOG_ERROR("FontRenderer: 폰트 텍스처 로드 실패");
        return false;
    }

    // 샘플러 스테이트 생성
    FontSampler = Renderer.CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
    if (!FontSampler)
    {
        UE_LOG_ERROR("FontRenderer: 샘플러 스테이트 생성 실패");
        return false;
    }

    // 동적 정점 버퍼 생성
    if (!CreateDynamicVertexBuffer())
    {
        UE_LOG_ERROR("FontRenderer: 동적 정점 버퍼 생성 실패");
        return false;
    }

    // 폰트 데이터용 상수 버퍼 생성
    FontDataConstantBuffer = Renderer.CreateConstantBuffer<FFontConstantBuffer>();
    if (!FontDataConstantBuffer)
    {
        UE_LOG_ERROR("FontRenderer: 폰트 데이터 상수 버퍼 생성 실패");
        return false;
    }

    UE_LOG_SUCCESS("FontRenderer: 초기화 완료");
    return true;
}

/// @brief 리소스 해제
void UFontRenderer::Release()
{
    SafeRelease(DynamicVertexBuffer);
    SafeRelease(FontDataConstantBuffer);
    SafeRelease(FontSampler);
    SafeRelease(FontInputLayout);
    SafeRelease(FontPixelShader);
    SafeRelease(FontVertexShader);
    // FontAtlasTexture는 AssetManager가 관리하므로 여기서 해제하지 않음
}

/// @brief 임의의 텍스트 렌더링
void UFontRenderer::RenderText(const char* Text, const FMatrix& WorldMatrix, const FViewProjConstants& ViewProjectionConstants,
                               float CenterY, float StartZ, float CharWidth, float CharHeight, bool bEnableDepthTest)
{
    if (!Text || strlen(Text) == 0) { return; }

    URenderer& Renderer = URenderer::GetInstance();
    ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();
    UPipeline* Pipeline = Renderer.GetPipeline();

    size_t TextLength = strlen(Text);
    uint32 VertexCount = static_cast<uint32>(TextLength * 6);

    if (VertexCount > MAX_FONT_VERTICES)
    {
        UE_LOG_WARNING("FontRenderer: 텍스트가 너무 길어 렌더링할 수 없습니다. (정점 %u개 > 최대 %u개)", VertexCount, MAX_FONT_VERTICES);
        return;
    }

    // 1. 정점 데이터 생성 및 동적 버퍼 업데이트
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(DeviceContext->Map(DynamicVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        FFontVertex* Vertices = static_cast<FFontVertex*>(mappedResource.pData);
        float currentY = CenterY - (TextLength * CharWidth) / 2.0f;

        for (size_t i = 0; i < TextLength; ++i)
        {
            char ch = Text[i];
            uint32 asciiCode = static_cast<uint32>(ch);

            FVector2 uv_topLeft(0.0f, 0.0f);
            FVector2 uv_topRight(1.0f, 0.0f);
            FVector2 uv_bottomLeft(0.0f, 1.0f);
            FVector2 uv_bottomRight(1.0f, 1.0f);

            float y = currentY + i * CharWidth;
            float z = StartZ;

            FVector p0(0.0f, y, z + CharHeight);
            FVector p1(0.0f, y + CharWidth, z + CharHeight);
            FVector p2(0.0f, y, z);
            FVector p3(0.0f, y + CharWidth, z);

            // Triangle 1
            Vertices[i * 6 + 0] = { p0, uv_topLeft, asciiCode };
            Vertices[i * 6 + 1] = { p1, uv_topRight, asciiCode };
            Vertices[i * 6 + 2] = { p2, uv_bottomLeft, asciiCode };

            // Triangle 2
            Vertices[i * 6 + 3] = { p1, uv_topRight, asciiCode };
            Vertices[i * 6 + 4] = { p3, uv_bottomRight, asciiCode };
            Vertices[i * 6 + 5] = { p2, uv_bottomLeft, asciiCode };
        }
        DeviceContext->Unmap(DynamicVertexBuffer, 0);
    }

    // 2. 파이프라인 상태 설정
    FPipelineInfo PipelineInfo = {};
    PipelineInfo.InputLayout = FontInputLayout;
    PipelineInfo.VertexShader = FontVertexShader;
    PipelineInfo.PixelShader = FontPixelShader;
    PipelineInfo.RasterizerState = Renderer.GetRasterizerState({ ECullMode::None, EFillMode::Solid }); // 폰트는 보통 양면 렌더링
    PipelineInfo.BlendState = Renderer.GetAlphaBlendState();
    PipelineInfo.DepthStencilState = bEnableDepthTest ? Renderer.GetDefaultDepthStencilState() : Renderer.GetDisabledDepthStencilState();
    Pipeline->UpdatePipeline(PipelineInfo);

    // 3. 상수 버퍼 업데이트
    Renderer.UpdateConstantBuffer(Renderer.GetConstantBufferModels(), WorldMatrix, 0, true);
    Renderer.UpdateConstantBuffer(Renderer.GetConstantBufferViewProj(), ViewProjectionConstants, 1, true);
    Renderer.UpdateConstantBuffer(FontDataConstantBuffer, ConstantBufferData, 2, true);

    // 4. 리소스 바인딩
    Pipeline->SetVertexBuffer(DynamicVertexBuffer, sizeof(FFontVertex));
    Pipeline->SetTexture(0, false, FontAtlasTexture);
    Pipeline->SetSamplerState(0, false, FontSampler);

    // 5. 드로우 콜
    Pipeline->Draw(VertexCount, 0);
}

/// @brief 폰트 텍스처 로드
bool UFontRenderer::LoadFontTexture()
{
    UAssetManager& ResourceManager = UAssetManager::GetInstance();
    auto TextureComPtr = ResourceManager.LoadTexture("Asset/Texture/DejaVu Sans Mono.png");
    FontAtlasTexture = TextureComPtr.Get();

    if (!FontAtlasTexture)
    {
        UE_LOG_ERROR("FontRenderer: 폰트 텍스처 로드 실패");
        return false;
    }
    UE_LOG_SUCCESS("FontRenderer: 폰트 텍스처 로드 성공");
    return true;
}

/// @brief 텍스트 렌더링을 위한 동적 정점 버퍼 생성
bool UFontRenderer::CreateDynamicVertexBuffer()
{
    URenderer& Renderer = URenderer::GetInstance();
    ID3D11Device* Device = Renderer.GetDevice();

    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = sizeof(FFontVertex) * MAX_FONT_VERTICES;
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Device->CreateBuffer(&BufferDesc, nullptr, &DynamicVertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("FontRenderer: 동적 정점 버퍼 생성 실패 (HRESULT: 0x%08lX)", hr);
        return false;
    }

    UE_LOG_SUCCESS("FontRenderer: 동적 정점 버퍼 생성 완료 (최대 정점 %d개)", MAX_FONT_VERTICES);
    return true;
}