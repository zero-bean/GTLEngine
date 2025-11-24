#include "pch.h"
#include "ParticleRenderer.h"
#include "ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "ParticleLODLevel.h"
#include "ParticleEmitter.h"
#include "SceneView.h"

FParticleRenderer::FParticleRenderer()
{
    Shader = NewObject<UShader>();
}

FParticleRenderer::FParticleRenderer(D3D11RHI* InRHI)
    : RHI(InRHI)
{
    Shader = NewObject<UShader>();
}

FParticleRenderer::~FParticleRenderer()
{
    Shutdown();
}

void FParticleRenderer::Initialize()
{
    QuadMesh = UResourceManager::GetInstance().Get<UQuad>("BillboardQuad");
    VertexBuffer = QuadMesh->GetVertexBuffer();
    IndexBuffer = QuadMesh->GetIndexBuffer();

    constexpr uint32 InstanceBytes = sizeof(FSpriteParticleInstance) * MAX_PARTICLES;
    RHI->CreateVertexBufferWithSize(RHI->GetDevice(), &InstanceBuffer, InstanceBytes, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    if (!Shader)
    {
        Shader = NewObject<UShader>();
    }
    Shader->Load("Shaders/Particles/SpriteParticle.hlsl", RHI->GetDevice());
    CurrentVariant = Shader->GetOrCompileShaderVariant();
    if (!CurrentVariant || !CurrentVariant->VSBlob)
    {
        UE_LOG("[FParticleRenderer/Initialize]Get shadervariant fail");
        return;
    }

    // D3D11_INPUT_ELEMENT_DESC LayoutDesc[] = {
    //     // Slot 0 : Quad
    //     {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    //     {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    //     // Slot 1 : Instance
    //     { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    //     { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    //     { "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    //     { "TEXCOORD", 3, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1}
    // };
    //
    // RHIDevice->GetDevice()->CreateInputLayout(LayoutDesc,
    //     6,
    //     CurrentVariant->VSBlob->GetBufferPointer(),
    //     CurrentVariant->VSBlob->GetBufferSize(),
    //     &InputLayout);
}

void FParticleRenderer::Shutdown()
{
    RELEASE_RESOURCE(SAFE_RELEASE)
}

void FParticleRenderer::RenderParticles(const FSceneView* View, const TArray<UParticleSystemComponent*>& ParticleComponents)
{
    if (!CurrentVariant)
    {
        return;
    }

    if (!InstanceBuffer)
    {
        return;
    }
    RHI->OMSetRenderTargets(ERTVMode::SceneColorTargetWithId);
    // 깊이 테스트 On, 쓰기 off
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqualReadOnly);
    RHI->OMSetBlendState(true);
    RHI->RSSetState(ERasterizerMode::Solid_NoCull);


    ID3D11DeviceContext* DeviceContext = RHI->GetDeviceContext();
    // PS 리소스 초기화
    ID3D11ShaderResourceView* NullSRVs[] = { nullptr };
    DeviceContext->PSSetShaderResources(0, 1, NullSRVs);

    ID3D11SamplerState* DefaultSampler = RHI->GetSamplerState(RHI_Sampler_Index::Default);
    DeviceContext->PSSetSamplers(0, 1, &DefaultSampler);

    DeviceContext->VSSetShader(CurrentVariant->VertexShader, nullptr, 0);
    DeviceContext->PSSetShader(CurrentVariant->PixelShader, nullptr, 0);

    FMatrix InveresProjMatrix = {};
    if (View->ProjectionMode == ECameraProjectionMode::Perspective)
    {
        InveresProjMatrix = View->ProjectionMatrix.InversePerspectiveProjection();
    }
    else
    {
        InveresProjMatrix = View->ProjectionMatrix.InverseOrthographicProjection();
    }
    // 상수버퍼 갱신
    RHI->SetAndUpdateConstantBuffer(ViewProjBufferType(View->ViewMatrix, View->ProjectionMatrix, View->ViewMatrix.InverseAffine(), InveresProjMatrix));
    ID3D11Buffer* VertexBuffers[] = { VertexBuffer};
    UINT QuadStride = sizeof(FParticleQuad);
    UINT InstanceStride = sizeof(FSpriteParticleInstance);
    UINT Offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &QuadStride, &Offset);
    DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    DeviceContext->IASetInputLayout(CurrentVariant->InputLayout);
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    for (UParticleSystemComponent* PSC : ParticleComponents)
    {
        for (FParticleEmitterInstance* Emitter : PSC->EmitterInstances)
        {
            // SpriteEmitter
            if (!Emitter->CurrentLODLevel->TypeDataModule)
            {
                FParticleSpriteEmitterInstance* SpriteEmitter = static_cast<FParticleSpriteEmitterInstance*>(Emitter);
                ParticleInstanceData.Empty();
                SpriteEmitter->GetParticleInstanceData(ParticleInstanceData);
                // 동적 정점 버퍼 갱신
                RHI->VertexBufferUpdate(InstanceBuffer, ParticleInstanceData);

                DeviceContext->IASetVertexBuffers(1, 1, &InstanceBuffer, &InstanceStride, &Offset);

                ID3D11ShaderResourceView* DiffuseTextureSRV = nullptr;
                
                UMaterialInterface* Material = Emitter->CurrentMaterial;
                // 아래 if문은 테스트용임. 원래 머티리얼이 그냥 텍스처를 가지고 있어야 함.
                if (Emitter->InstanceSRV)
                {
                    DiffuseTextureSRV = Emitter->InstanceSRV;
                }
                // 2순위: 머티리얼 텍스처 (스태틱 메시)
                else if (Material)
                {
                    const FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();
                    if (!MaterialInfo.DiffuseTextureFileName.empty())
                    {
                        if (UTexture* TextureData = Material->GetTexture(EMaterialTextureSlot::Diffuse))
                        {
                            DiffuseTextureSRV = TextureData->GetShaderResourceView();
                        }
                    }
                }

                DeviceContext->PSSetShaderResources(0, 1, &DiffuseTextureSRV);
                DeviceContext->DrawIndexedInstanced(6, (UINT)ParticleInstanceData.Num(), 0, 0, 0);
            }
        }
    }
    RHI->RSSetState(ERasterizerMode::Solid);
    RHI->OMSetBlendState(false);
    // 상태 원복, default 값은 없음. 렌더패스에서 RenderShadowMaps이 가장 먼저 호출되는데 이때 LessEqual로 설정됨
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);
}

//void FParticleRenderer::InitializeQuad()
//{
//    FMeshData Mesh = {};
//    Mesh.Vertices.SetNum(4);
//    Mesh.Indices.SetNum(6);
//    Mesh.UV.SetNum(4);
//
//    Mesh.Vertices[0] = { -0.5f, -0.5f, 0.0f }; // left-bottom
//    Mesh.Vertices[1] = { -0.5f,  0.5f, 0.0f }; // left-top
//    Mesh.Vertices[2] = { 0.5f,  0.5f, 0.0f }; // right-top
//    Mesh.Vertices[3] = { 0.5f, -0.5f, 0.0f }; // right-bottom
//
//    Mesh.UV[0] = { 0.0f, 1.0f };
//    Mesh.UV[1] = { 0.0f, 0.0f };
//    Mesh.UV[2] = { 1.0f, 0.0f };
//    Mesh.UV[3] = { 1.0f, 1.0f };
//
//    Mesh.Indices[0] = 0;
//    Mesh.Indices[1] = 2;
//    Mesh.Indices[2] = 1;
//    Mesh.Indices[3] = 0;
//    Mesh.Indices[4] = 3;
//    Mesh.Indices[5] = 2;
//
//    if (!QuadMesh)
//    {
//        QuadMesh = NewObject<UQuad>();
//    }
//    QuadMesh->LoadParticleQuad(&Mesh, RHIDevice->GetDevice());
//}


