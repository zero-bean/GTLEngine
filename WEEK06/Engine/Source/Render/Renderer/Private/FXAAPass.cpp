#include "pch.h"
#include "Render/Renderer/Public/FXAAPass.h"
#include "Render/Renderer/Public/DeviceResources.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Editor/Public/Viewport.h"
#include "Editor/Public/ViewportClient.h"

struct FFXAAConstants
{
    FVector4 ViewportRect; // xy: offset (norm), zw: scale (norm)
};

UFXAAPass::UFXAAPass() = default;
UFXAAPass::~UFXAAPass() { Release(); }

void UFXAAPass::Initialize(UDeviceResources* InDeviceResources)
{
    DeviceResources = InDeviceResources;
    EnsureFixedStates();
    CreateResources();
}

void UFXAAPass::Release()
{
    ReleaseResources();
    if (RasterState) { RasterState->Release(); RasterState = nullptr; }
}

void UFXAAPass::OnResize()
{
    ReleaseResources();
    CreateResources();
}

void UFXAAPass::EnsureFixedStates()
{
    if (!DeviceResources) return;
    if (!RasterState)
    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.DepthClipEnable = TRUE;
        DeviceResources->GetDevice()->CreateRasterizerState(&rd, &RasterState);
    }
}

void UFXAAPass::CreateResources()
{
    if (!DeviceResources) return;

    const D3D11_VIEWPORT vp = DeviceResources->GetViewportInfo();
    const UINT texWidth = static_cast<UINT>(std::max(1.0f, vp.Width));
    const UINT texHeight = static_cast<UINT>(std::max(1.0f, vp.Height));

    // Scene color
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = texWidth;
    texDesc.Height = texHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    DeviceResources->GetDevice()->CreateTexture2D(&texDesc, nullptr, &FXAASceneTexture);

    if (FXAASceneTexture)
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = texDesc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        DeviceResources->GetDevice()->CreateRenderTargetView(FXAASceneTexture, &rtvDesc, &FXAASceneRTV);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        DeviceResources->GetDevice()->CreateShaderResourceView(FXAASceneTexture, &srvDesc, &FXAASceneSRV);
    }

    if (!FXAASampler)
    {
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.MaxAnisotropy = 1;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        DeviceResources->GetDevice()->CreateSamplerState(&sampDesc, &FXAASampler);
    }

    // Shaders
    ID3DBlob* vs = nullptr; ID3DBlob* ps = nullptr; ID3DBlob* err = nullptr;
    HRESULT hrVS = D3DCompileFromFile(L"Asset/Shader/FXAA.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vs, &err);
    if (FAILED(hrVS) && err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); err = nullptr; }
    HRESULT hrPS = D3DCompileFromFile(L"Asset/Shader/FXAA.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &ps, &err);
    if (FAILED(hrPS) && err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); err = nullptr; }
    if (vs) { DeviceResources->GetDevice()->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &FXAAVertexShader); vs->Release(); }
    if (ps) { DeviceResources->GetDevice()->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &FXAAPixelShader); ps->Release(); }

    if (!ConstantBufferFXAA)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(FFXAAConstants) + 0xf & 0xfffffff0;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        DeviceResources->GetDevice()->CreateBuffer(&desc, nullptr, &ConstantBufferFXAA);
    }
}

void UFXAAPass::ReleaseResources()
{
    if (FXAAPixelShader) { FXAAPixelShader->Release(); FXAAPixelShader = nullptr; }
    if (FXAAVertexShader) { FXAAVertexShader->Release(); FXAAVertexShader = nullptr; }
    if (FXAASampler) { FXAASampler->Release(); FXAASampler = nullptr; }
    if (FXAASceneSRV) { FXAASceneSRV->Release(); FXAASceneSRV = nullptr; }
    if (FXAASceneRTV) { FXAASceneRTV->Release(); FXAASceneRTV = nullptr; }
    if (FXAASceneTexture) { FXAASceneTexture->Release(); FXAASceneTexture = nullptr; }
    if (ConstantBufferFXAA) { ConstantBufferFXAA->Release(); ConstantBufferFXAA = nullptr; }
}

void UFXAAPass::UpdateFXAAParamsPS(float offsetX, float offsetY, float scaleX, float scaleY)
{
    if (!ConstantBufferFXAA) return;
    ID3D11DeviceContext* ctx = DeviceResources->GetDeviceContext();
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(ctx->Map(ConstantBufferFXAA, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        auto* data = reinterpret_cast<FFXAAConstants*>(mapped.pData);
        data->ViewportRect = FVector4(offsetX, offsetY, scaleX, scaleY);
        ctx->Unmap(ConstantBufferFXAA, 0);
    }
}

void UFXAAPass::Apply(UPipeline* InPipeline, FViewport* InViewportManager, ID3D11DepthStencilState* InDisabledDepthState, const FLOAT InClearColor[4])
{
    if (!DeviceResources || !InPipeline || !FXAASceneSRV || !FXAAVertexShader || !FXAAPixelShader) return;

    // Bind backbuffer
    ID3D11RenderTargetView* backRTV = DeviceResources->GetRenderTargetView();
    DeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &backRTV, nullptr);
    DeviceResources->GetDeviceContext()->ClearRenderTargetView(backRTV, InClearColor);

    // Set fixed pipeline state for full-screen triangle
    FPipelineInfo p = {
        nullptr,                 // InputLayout
        FXAAVertexShader,        // VS
        RasterState,             // RS (solid, cull none)
        InDisabledDepthState,    // no depth
        FXAAPixelShader,         // PS
        nullptr,                 // BlendState
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    };
    InPipeline->UpdatePipeline(p);
    InPipeline->SetTexture(0, false, FXAASceneSRV);
    InPipeline->SetSamplerState(0, false, FXAASampler);

    const D3D11_VIEWPORT full = DeviceResources->GetViewportInfo();
    const float invW = (full.Width > 0.0f) ? (1.0f / full.Width) : 1.0f;
    const float invH = (full.Height > 0.0f) ? (1.0f / full.Height) : 1.0f;

    if (InViewportManager)
    {
        auto& Clients = InViewportManager->GetViewports();
        for (auto& VC : Clients)
        {
            const D3D11_VIEWPORT& vp = VC.GetViewportInfo();
            if (vp.Width < 1.0f || vp.Height < 1.0f) continue;

            VC.Apply(DeviceResources->GetDeviceContext());

            UpdateFXAAParamsPS(vp.TopLeftX * invW, vp.TopLeftY * invH, vp.Width * invW, vp.Height * invH);
            InPipeline->SetConstantBuffer(0, false, ConstantBufferFXAA);
            InPipeline->Draw(3, 0);
        }
    }

    // Unbind SRV to avoid hazards
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    DeviceResources->GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV);
}

