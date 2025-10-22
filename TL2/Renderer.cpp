#include "pch.h"

#include "BillboardComponent.h"
#include "TextRenderComponent.h"
#include "Shader.h"
#include "StaticMesh.h"
#include "TextQuad.h"
#include "StaticMeshComponent.h"
#include "RenderingStats.h"
#include "UI/StatsOverlayD2D.h"
#include "SMultiViewportWindow.h"
#include "SelectionManager.h"
#include "DecalComponent.h"
#include "DecalSpotLightComponent.h"
#include "GridActor.h"
#include "FViewport.h"
#include "CameraActor.h"
#include "Frustum.h"
#include "BoundingVolume.h"
#include "GizmoActor.h"
#include "PointLightComponent.h"
#include "ExponentialHeightFogComponent.h"
#include "FXAAComponent.h"
#include "CameraComponent.h"
#include "AmbientLightComponent.h"
#include "SpotLightComponent.h"
#include "DirectionalLightComponent.h"
#include "LightCullingManager.h"
#include "GizmoArrowComponent.h"
#include "AmbientActor.h"

URenderer::URenderer(URHIDevice* InDevice) : RHIDevice(InDevice)
{
    InitializeLineBatch();

    // Initialize Light Culling Manager
    LightCullingManager = new ULightCullingManager();
    // Will be initialized with proper screen dimensions in RenderScene
}

URenderer::~URenderer()
{
    if (LineBatchData)
    {
        delete LineBatchData;
    }

    if (LightCullingManager)
    {
        delete LightCullingManager;
        LightCullingManager = nullptr;
    }
}

void URenderer::Update(float DeltaSeconds)
{
    
}

void URenderer::BeginFrame()
{
    // 렌더링 통계 수집 시작
    URenderingStatsCollector::GetInstance().BeginFrame();
    
    // 상태 추적 리셋
    ResetRenderStateTracking();
    
    // 백버퍼/깊이버퍼를 클리어
    RHIDevice->ClearBackBuffer();  // 배경색
    RHIDevice->ClearDepthBuffer(1.0f, 0);                 // 깊이값 초기화
    //RHIDevice->CreateBlendState();
    RHIDevice->IASetPrimitiveTopology();
    // RS
    RHIDevice->RSSetViewport();

    //OM
    RHIDevice->OMSetBlendState(false);
   RHIDevice->OMSetRenderTargets(ERenderTargetType::Frame | ERenderTargetType::ID);
}

void URenderer::PrepareShader(UShader* InShader)
{
    UShader* ShaderToUse = OverrideShader ? OverrideShader : InShader;

    // 셰이더 변경 추적
    if (LastShader != ShaderToUse)
    {
        URenderingStatsCollector::GetInstance().IncrementShaderChanges();
        LastShader = ShaderToUse;
    }
    
    // Ensure uber-shader variant matches current selection
    ShaderToUse->SetActiveMode(CurrentShadingModel);
    RHIDevice->GetDeviceContext()->VSSetShader(ShaderToUse->GetVertexShader(), nullptr, 0);
    RHIDevice->GetDeviceContext()->PSSetShader(ShaderToUse->GetPixelShader(), nullptr, 0);
    RHIDevice->GetDeviceContext()->IASetInputLayout(ShaderToUse->GetInputLayout());
}

void URenderer::OMSetBlendState(bool bIsChecked)
{
    if (bIsChecked == true)
    {
        RHIDevice->OMSetBlendState(true);
    }
    else
    {
        RHIDevice->OMSetBlendState(false);
    }
}

void URenderer::RSSetState(EViewModeIndex ViewModeIndex)
{
    RHIDevice->RSSetState(ViewModeIndex);
}

void URenderer::RSSetFrontCullState()
{
    RHIDevice->RSSetFrontCullState();
}

void URenderer::RSSetNoCullState()
{
    RHIDevice->RSSetNoCullState();
}

void URenderer::RSSetDefaultState()
{
    RHIDevice->RSSetDefaultState();
}

void URenderer::RenderFrame(UWorld* World)
{
    BeginFrame();
    UUIManager::GetInstance().Render();

    // 컴포넌트는 OnRegister 시점에 자동으로 레벨에 등록됨
    // InitializeActorsForPlay에서 RegisterComponent 호출

    RenderViewPorts(World);

    UUIManager::GetInstance().EndFrame();
    EndFrame();
}

void URenderer::DrawIndexedPrimitiveComponent(UStaticMesh* InMesh, D3D11_PRIMITIVE_TOPOLOGY InTopology, const TArray<FMaterialSlot>& InComponentMaterialSlots)
{
    URenderingStatsCollector& StatsCollector = URenderingStatsCollector::GetInstance();
    
    // 디버그: StaticMesh 렌더링 통계
    
    UINT stride = 0;
    switch (InMesh->GetVertexType())
    {
    case EVertexLayoutType::PositionColor:
        stride = sizeof(FVertexSimple);
        break;
    case EVertexLayoutType::PositionColorTexturNormal:
        stride = sizeof(FVertexDynamic);
        break;
    case EVertexLayoutType::PositionBillBoard:
        stride = sizeof(FBillboardVertexInfo_GPU);
        break;
    case EVertexLayoutType::PositionColorTexturNormalTangent:
        stride = sizeof(FVertexDynamic);
        break;
    case EVertexLayoutType::PositionUV:
        stride = sizeof(FVertexUV);
        break;
    default:
        // Handle unknown or unsupported vertex types
        assert(false && "Unknown vertex type!");
        return; // or log an error
    }
    UINT offset = 0;

    ID3D11Buffer* VertexBuffer = InMesh->GetVertexBuffer();
    ID3D11Buffer* IndexBuffer = InMesh->GetIndexBuffer();
    uint32 VertexCount = InMesh->GetVertexCount();
    uint32 IndexCount = InMesh->GetIndexCount();

    RHIDevice->GetDeviceContext()->IASetVertexBuffers(
        0, 1, &VertexBuffer, &stride, &offset
    );

    RHIDevice->GetDeviceContext()->IASetIndexBuffer(
        IndexBuffer, DXGI_FORMAT_R32_UINT, 0
    );

    RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(InTopology);
    RHIDevice->PSSetDefaultSampler(0);

    if (InMesh->HasMaterial())
    {
        const TArray<FGroupInfo>& MeshGroupInfos = InMesh->GetMeshGroupInfo();
        const uint32 NumMeshGroupInfos = static_cast<uint32>(MeshGroupInfos.size());

        for (uint32 i = 0; i < NumMeshGroupInfos; ++i)
        {
            const FMaterialSlot& CurrentSlot = InComponentMaterialSlots[i]; //  현재 슬롯 정보를 가져옵니다.
            UMaterial* const Material = UResourceManager::GetInstance().Get<UMaterial>(CurrentSlot.MaterialName);

            if (Material == nullptr)
            {
                continue;
            }

            UTexture* NormalTexture = nullptr; // 최종적으로 바인딩될 노멀 텍스처
            const FObjMaterialInfo& MaterialInfo = Material->GetMaterialInfo();
            bool bHasTexture = !(MaterialInfo.DiffuseTextureFileName == FName::None());

            // 오버라이드가 켜져있으면, 컴포넌트의 오버라이드 텍스처를 사용합니다.
            if (CurrentSlot.bOverrideNormalTexture)
            {
                NormalTexture = CurrentSlot.NormalTextureOverride;
            }
            // 오버라이드가 꺼져있으면, 원본 머티리얼의 텍스처를 사용합니다.
            else
            {
                /* lazy: defer loading actual normal texture until binding */
                NormalTexture = nullptr;
            }

            bool bHasNormalTexture = CurrentSlot.bOverrideNormalTexture ? (CurrentSlot.NormalTextureOverride != nullptr) : !(MaterialInfo.NormalTextureFileName == FName::None()); // 머티리얼의 노말맵 보유 여부

            // Ensure the active shader variant matches per-section normal map availability
            {
                UShader* ActiveShader = Material->GetShader();
                if (!ActiveShader)
                {
                    // Fall back to the currently bound shader (set by component earlier), if any
                    ActiveShader = LastShader;
                }
                if (ActiveShader)
                {
                    ActiveShader->SetActiveNormalMode(bHasNormalTexture ? ENormalMapMode::HasNormalMap : ENormalMapMode::NoNormalMap);
                    PrepareShader(ActiveShader);
                }
            }

            if (LastMaterial != Material)
            {
                StatsCollector.IncrementMaterialChanges();
                LastMaterial = const_cast<UMaterial*>(Material);
            }

            if (bHasTexture)
            {
                FTextureData* TextureData = UResourceManager::GetInstance().CreateOrGetTextureData(MaterialInfo.DiffuseTextureFileName);

                UTexture* CurrentTexture = reinterpret_cast<UTexture*>(TextureData);
                if (LastTexture != CurrentTexture)
                {
                    StatsCollector.IncrementTextureChanges();
                    LastTexture = CurrentTexture; 
                }

                RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &(TextureData->TextureSRV));
            }

            // 노멀 맵이 존재한다면, 최종적으로 결정된 NormalTexture의 SRV를 바인딩합니다.
            if (bHasNormalTexture)
            {
                if (NormalTexture == nullptr) { if (CurrentSlot.bOverrideNormalTexture) NormalTexture = CurrentSlot.NormalTextureOverride; else NormalTexture = Material->GetNormalTexture(); } ID3D11ShaderResourceView* NormalSRV = NormalTexture ? NormalTexture->GetShaderResourceView() : nullptr;
                RHIDevice->GetDeviceContext()->PSSetShaderResources(1, 1, &NormalSRV);
            }
            // 텍스처가 없는 경우, 슬롯을 명시적으로 비웁니다.
            else
            {
                ID3D11ShaderResourceView* NullSRV = nullptr;
                RHIDevice->GetDeviceContext()->PSSetShaderResources(1, 1, &NullSRV);
            }

            RHIDevice->UpdateSetCBuffer({ FMaterialInPs(MaterialInfo), (uint32)true, (uint32)bHasTexture, (uint32)bHasNormalTexture });
            RHIDevice->GetDeviceContext()->DrawIndexed(MeshGroupInfos[i].IndexCount, MeshGroupInfos[i].StartIndex, 0);
            StatsCollector.IncrementDrawCalls();
        }
    }
    else
    {
        FObjMaterialInfo ObjMaterialInfo;
        RHIDevice->UpdateSetCBuffer({ FMaterialInPs(ObjMaterialInfo), (uint32)false, (uint32)false, (uint32)false }); 
        RHIDevice->GetDeviceContext()->DrawIndexed(IndexCount, 0, 0);
        StatsCollector.IncrementDrawCalls();
    }
}

void URenderer::DrawIndexedPrimitiveComponent(UTextRenderComponent* Comp, D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
    URenderingStatsCollector& StatsCollector = URenderingStatsCollector::GetInstance();
    
    // 디버그: TextRenderComponent 렌더링 통계
    
    UINT Stride = sizeof(FBillboardVertexInfo_GPU);
    ID3D11Buffer* VertexBuff = Comp->GetStaticMesh()->GetVertexBuffer();
    ID3D11Buffer* IndexBuff = Comp->GetStaticMesh()->GetIndexBuffer();

    // 매테리얼 변경 추적
    UMaterial* CompMaterial = Comp->GetMaterial();
    if (LastMaterial != CompMaterial)
    {
        StatsCollector.IncrementMaterialChanges();
        LastMaterial = CompMaterial;
    }
    
    UShader* CompShader = CompMaterial->GetShader();
    // 셰이더 변경 추적
    if (LastShader != CompShader)
    {
        StatsCollector.IncrementShaderChanges();
        LastShader = CompShader;
    }
    
    RHIDevice->GetDeviceContext()->IASetInputLayout(CompShader->GetInputLayout());

    
    UINT offset = 0;
    RHIDevice->GetDeviceContext()->IASetVertexBuffers(
        0, 1, &VertexBuff, &Stride, &offset
    );
    RHIDevice->GetDeviceContext()->IASetIndexBuffer(
        IndexBuff, DXGI_FORMAT_R32_UINT, 0
    );

    // 텍스처 변경 추적 (텍스처 비교)
    UTexture* CompTexture = CompMaterial->GetTexture();
    if (LastTexture != CompTexture)
    {
        StatsCollector.IncrementTextureChanges();
        LastTexture = CompTexture;
    }
    
    ID3D11ShaderResourceView* TextureSRV = CompTexture->GetShaderResourceView();
    RHIDevice->PSSetDefaultSampler(0);
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &TextureSRV);
    RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(InTopology);
    RHIDevice->GetDeviceContext()->DrawIndexed(Comp->GetStaticMesh()->GetIndexCount(), 0, 0);
    StatsCollector.IncrementDrawCalls();
}


void URenderer::DrawIndexedPrimitiveComponent(UBillboardComponent* Comp, D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
    URenderingStatsCollector& StatsCollector = URenderingStatsCollector::GetInstance();
    
    // 디버그: TextRenderComponent 렌더링 통계
    
    UINT Stride = sizeof(FBillboardVertexInfo_GPU);
    ID3D11Buffer* VertexBuff = Comp->GetStaticMesh()->GetVertexBuffer();
    ID3D11Buffer* IndexBuff = Comp->GetStaticMesh()->GetIndexBuffer();

    // 매테리얼 변경 추적
    UMaterial* CompMaterial = Comp->GetMaterial();
    if (LastMaterial != CompMaterial)
    {
        StatsCollector.IncrementMaterialChanges();
        LastMaterial = CompMaterial;
    }
    
    UShader* CompShader = CompMaterial->GetShader();
    // 셰이더 변경 추적
    if (LastShader != CompShader)
    {
        StatsCollector.IncrementShaderChanges();
        LastShader = CompShader;
    }
    
    RHIDevice->GetDeviceContext()->IASetInputLayout(CompShader->GetInputLayout());

    
    UINT offset = 0;
    RHIDevice->GetDeviceContext()->IASetVertexBuffers(
        0, 1, &VertexBuff, &Stride, &offset
    );
    RHIDevice->GetDeviceContext()->IASetIndexBuffer(
        IndexBuff, DXGI_FORMAT_R32_UINT, 0
    );

    // 텍스처 변경 추적 (텍스처 비교)
    UTexture* CompTexture = CompMaterial->GetTexture();
    if (LastTexture != CompTexture)
    {
        StatsCollector.IncrementTextureChanges();
        LastTexture = CompTexture;
    }
    
    ID3D11ShaderResourceView* TextureSRV = CompTexture->GetShaderResourceView();
    RHIDevice->PSSetDefaultSampler(0);
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &TextureSRV);
    RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(InTopology);
    RHIDevice->GetDeviceContext()->DrawIndexed(Comp->GetStaticMesh()->GetIndexCount(), 0, 0);
    StatsCollector.IncrementDrawCalls();
}

void URenderer::SetViewModeType(EViewModeIndex ViewModeIndex)
{
    RHIDevice->RSSetState(ViewModeIndex);
    if(ViewModeIndex == EViewModeIndex::VMI_Wireframe)
        RHIDevice->UpdateSetCBuffer(ColorBufferType{ FVector4(1.f, 0.f, 0.f, 1.f) });
    else
        RHIDevice->UpdateSetCBuffer(ColorBufferType{ FVector4(1.f, 1.f, 1.f, 0.f) });
}

void URenderer::EndFrame()
{
    // 렌더링 통계 수집 종료
    URenderingStatsCollector& StatsCollector = URenderingStatsCollector::GetInstance();
    StatsCollector.EndFrame();
    
    // 현재 프레임 통계를 업데이트
    const FRenderingStats& CurrentStats = StatsCollector.GetCurrentFrameStats();
    StatsCollector.UpdateFrameStats(CurrentStats);
    
    // 평균 통계를 얻어서 오버레이에 업데이트
    const FRenderingStats& AvgStats = StatsCollector.GetAverageStats();
    UStatsOverlayD2D::Get().UpdateRenderingStats(
        AvgStats.TotalDrawCalls,
        AvgStats.MaterialChanges,
        AvgStats.TextureChanges,
        AvgStats.ShaderChanges
    );
    
    RHIDevice->OMSetRenderTargets(ERenderTargetType::None);
    RHIDevice->PSSetRenderTargetSRV(ERenderTargetType::None);
    RHIDevice->Present();
}

void URenderer::OMSetDepthStencilState(EComparisonFunc Func)
{
    RHIDevice->OmSetDepthStencilState(Func);
}

void URenderer::RenderViewPorts(UWorld* World) 
{
    // 멀티 뷰포트 시스템을 통해 각 뷰포트별로 렌더링
    if (SMultiViewportWindow* MultiViewport = World->GetMultiViewportWindow())
    {
        MultiViewport->OnRender();
    }
}

void URenderer::RenderSceneDepthPass(UWorld* World, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix)
{
    OutputDebugStringA("[Renderer] RenderSceneDepthPass - Start\n");

    // +-+ Clear Depth Buffer +-+
    RHIDevice->ClearDepthBuffer(1.0f, 0);

    // +-+ Set Render State +-+
    RHIDevice->OMSetDepthOnlyTarget();     // DSV binding (no color target)
    RHIDevice->OmSetDepthStencilState(EComparisonFunc::LessEqual); // Enable depth write
    RHIDevice->OMSetBlendState(false);     // color write disabled
    RHIDevice->RSSetDefaultState();        // solid fill, back-face culling
    RHIDevice->IASetPrimitiveTopology();

    // +-+ Set Shader & Buffer +-+
    DepthOnlyShader = UResourceManager::GetInstance().Load<UShader>("DepthPrepassShader.hlsl");
    if (!DepthOnlyShader)
    {
        OutputDebugStringA("[Renderer] ERROR: Failed to load DepthPrepassShader.hlsl\n");
        return;
    }

    char shaderMsg[256];
    sprintf_s(shaderMsg, "[Renderer] DepthOnlyShader loaded: %p, VS: %p, PS: %p\n",
              DepthOnlyShader,
              DepthOnlyShader->GetVertexShader(),
              DepthOnlyShader->GetPixelShader());
    OutputDebugStringA(shaderMsg);

    PrepareShader(DepthOnlyShader);
    UpdateSetCBuffer(ViewProjBufferType(ViewMatrix, ProjectionMatrix));

    // +-+ Iterate and Draw Renderable Actors +-+
    const TArray<AActor*>& LevelActors = World->GetLevel()->GetActors();
    for (AActor* Actor : LevelActors)
    {
        if (!Actor || Actor->GetActorHiddenInGame())
            continue;

        for (UActorComponent* ActorComp : Actor->GetComponents())
        {
            if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(ActorComp))
            {
                if (!Primitive->IsActive())
                    continue;

                if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Primitive))
                {
                    UStaticMesh* Mesh = MeshComp->GetStaticMesh();
                    if (!Mesh)  continue;

                    const FMatrix& WorldMatrix = Primitive->GetWorldMatrix();
                    UpdateSetCBuffer(ModelBufferType(WorldMatrix));

                    ID3D11Buffer* VertexBuffer = Mesh->GetVertexBuffer();
                    ID3D11Buffer* IndexBuffer = Mesh->GetIndexBuffer();

                    UINT Stride = sizeof(FVertexDynamic);
                    UINT Offset = 0;
                    RHIDevice->GetDeviceContext()->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
                    RHIDevice->GetDeviceContext()->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

                    RHIDevice->GetDeviceContext()->DrawIndexed(Mesh->GetIndexCount(), 0, 0);
                    URenderingStatsCollector::GetInstance().IncrementDrawCalls();
                }
            }
        }
    }

    // +-+ Restore Render State +-+
    // Unbind DSV to allow it to be used as SRV
    ID3D11RenderTargetView* nullRTV[1] = { nullptr };
    RHIDevice->GetDeviceContext()->OMSetRenderTargets(0, nullRTV, nullptr);

    OutputDebugStringA("[Renderer] RenderSceneDepthPass - Complete\n");
}

void URenderer::RenderBasePass(UWorld* World, ACameraActor* Camera, FViewport* Viewport)
{
    // 뷰포트의 실제 크기로 aspect ratio 계산
    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0)
    {
        ViewportAspectRatio = 1.0f;
    }

    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);
   
    // 씬의 액터들을 렌더링
    // General Rendering (color + depth)
    RenderActorsInViewport(World, ViewMatrix, ProjectionMatrix, Viewport);
}

void URenderer::RenderScene(UWorld* World, ACameraActor* Camera, FViewport* Viewport)
{

    if (!World || !Camera || !Viewport)
        return;

    // +-+-+ Render Pass Structure +-+-+

    float ViewportAspectRatio = (Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f; // 0으로 나누기 방지
    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);
    UpdateSetCBuffer(ViewProjBufferType(ViewMatrix, ProjectionMatrix, Camera->GetActorLocation()));

    // Initialize Light Culling Manager if needed
    UINT screenWidth = Viewport->GetSizeX();
    UINT screenHeight = Viewport->GetSizeY();

    char rendererMsg[512];
    sprintf_s(rendererMsg, "[Renderer] RenderScene - Screen: %dx%d, bUseTiledCulling: %d, LightCullingManager: %p\n",
              screenWidth, screenHeight, bUseTiledCulling, LightCullingManager);
    OutputDebugStringA(rendererMsg);

    if (LightCullingManager && bUseTiledCulling)
    {
        // Only initialize once on first frame with the first viewport's size
        // All viewports will share the same culling data
        if (!LightCullingManager->IsInitialized())
        {
            OutputDebugStringA("[Renderer] Initializing LightCullingManager...\n");
            LightCullingManager->Initialize(RHIDevice->GetDevice(), screenWidth, screenHeight);
        }
        else
        {
            LightCullingManager->Resize(RHIDevice->GetDevice(), screenWidth, screenHeight);
        }
    }
    else
    {
        OutputDebugStringA("[Renderer] LightCullingManager is NULL or disabled!\n");
    }

    // Upload BRDF parameters for pixel shader
    //UpdateSetCBuffer(FBRDFInfoBufferType(BRDFRoughness, BRDFMetallic));
    switch (CurrentViewMode)
    {
    case EViewModeIndex::VMI_Lit:
    case EViewModeIndex::VMI_Unlit:
    case EViewModeIndex::VMI_Wireframe:
    {        
        // ? Step 1: Depth Prepass - 먼저 깊이 버퍼만 채우기
        OutputDebugStringA("[Renderer] Executing depth prepass...\n");
        RenderSceneDepthPass(World, ViewMatrix, ProjectionMatrix);

        // ?? DEBUG: 깊이 프리패스 결과 시각화 (옵션)
        // 주석 해제하면 깊이 프리패스 직후 깊이를 그레이스케일로 볼 수 있음
        //RenderSceneDepthVisualizePass(Camera);
        // return; // 이후 렌더링 스킵하고 깊이만 확인

        RenderDirectionalLightPass(World);
        RenderPointLightPass(World);
        RenderSpotLightPass(World);

        // Debug: Log light counts
        int pointLightCount = World->GetLevel()->GetComponentList<UPointLightComponent>().size();
        int spotLightCount = World->GetLevel()->GetComponentList<USpotLightComponent>().size();
        char lightMsg[256];
        sprintf_s(lightMsg, "[Renderer] Scene has %d Point Lights, %d Spot Lights\n",
                  pointLightCount, spotLightCount);
        OutputDebugStringA(lightMsg);

        // ? Step 2: Execute Tile-based Light Culling (이제 깊이 버퍼가 채워진 상태)
        OutputDebugStringA("[Renderer] Before ExecuteLightCulling check\n");
        if (LightCullingManager && bUseTiledCulling)
        {
            OutputDebugStringA("[Renderer] Getting depth SRV...\n");
            // Get depth SRV from RHI
            ID3D11ShaderResourceView* depthSRV = RHIDevice->GetDepthSRV();

            char depthMsg[256];
            sprintf_s(depthMsg, "[Renderer] DepthSRV: %p\n", depthSRV);
            OutputDebugStringA(depthMsg);

            if (depthSRV)
            {
                OutputDebugStringA("[Renderer] DepthSRV is valid, executing light culling...\n");

                // Unbind depth buffer from output to avoid resource hazard
                ID3D11RenderTargetView* nullRTV[1] = { nullptr };
                RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, nullRTV, nullptr);

                UCameraComponent* CamComp = Camera->GetCameraComponent();
                float nearPlane = CamComp ? CamComp->GetNearClip() : 0.1f;
                float farPlane = CamComp ? CamComp->GetFarClip() : 1000.0f;

                char camMsg[256];
                sprintf_s(camMsg, "[Renderer] Near: %.2f, Far: %.2f\n", nearPlane, farPlane);
                OutputDebugStringA(camMsg);

                // Get light buffers from PS (they were bound by RenderPointLightPass/RenderSpotLightPass)
                // PointLight is at b9, SpotLight is at b13
                ID3D11Buffer* pointLightBuffer = nullptr;
                ID3D11Buffer* spotLightBuffer = nullptr;
                RHIDevice->GetDeviceContext()->PSGetConstantBuffers(9, 1, &pointLightBuffer);
                RHIDevice->GetDeviceContext()->PSGetConstantBuffers(13, 1, &spotLightBuffer);

                char bufferMsg[256];
                sprintf_s(bufferMsg, "[Renderer] Got buffers - PointLight (b9): %p, SpotLight (b13): %p\n",
                          pointLightBuffer, spotLightBuffer);
                OutputDebugStringA(bufferMsg);

                if (pointLightBuffer && spotLightBuffer)
                {
                    LightCullingManager->ExecuteLightCulling(
                        RHIDevice->GetDeviceContext(),
                        depthSRV,
                        ViewMatrix,
                        ProjectionMatrix,
                        nearPlane,
                        farPlane,
                        pointLightBuffer,
                        spotLightBuffer
                    );

                    // Release references (PSGetConstantBuffers adds ref count)
                    pointLightBuffer->Release();
                    spotLightBuffer->Release();
                }
                else
                {
                    char errorMsg[256];
                    sprintf_s(errorMsg, "[Renderer] ERROR: Light buffers not bound! PointLight: %p, SpotLight: %p\n",
                              pointLightBuffer, spotLightBuffer);
                    OutputDebugStringA(errorMsg);
                }

                // Re-bind render targets for base pass (Frame + ID + Depth)
                RHIDevice->OMSetRenderTargets(ERenderTargetType::Frame | ERenderTargetType::ID);

                // Set debug visualization mode based on ShowFlag
                bool bDebugVisualizeTiles = Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_TileCullingDebug);
                char debugMsg[256];
                sprintf_s(debugMsg, "[Renderer] Setting debug visualization: %d\n", bDebugVisualizeTiles);
                OutputDebugStringA(debugMsg);

                LightCullingManager->SetDebugVisualization(bDebugVisualizeTiles);

                // 뷰포트 오프셋 가져오기
                D3D11_VIEWPORT d3d_vp;
                UINT num_vp = 1;
                RHIDevice->GetDeviceContext()->RSGetViewports(&num_vp, &d3d_vp);

                OutputDebugStringA("[Renderer] Binding results to PS...\n");
                LightCullingManager->BindResultsToPS(
                    RHIDevice->GetDeviceContext(),
                    d3d_vp.TopLeftX,  // 뷰포트 X 오프셋 전달
                    d3d_vp.TopLeftY   // 뷰포트 Y 오프셋 전달
                );
                OutputDebugStringA("[Renderer] Light culling complete!\n");
            }
            else
            {
                OutputDebugStringA("[Renderer] ERROR: DepthSRV is NULL!\n");
            }
        }
        else
        {
            OutputDebugStringA("[Renderer] Skipping light culling (manager NULL or disabled)\n");
        }

        RenderBasePass(World, Camera, Viewport);  // Full color + depth pass (Opaque geometry - per viewport)

        // Render tile culling debug visualization (full-screen overlay)
        if (LightCullingManager && bUseTiledCulling && Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_TileCullingDebug))
        {
            OutputDebugStringA("[Renderer] Rendering tile culling debug pass...\n");
            RenderTileCullingDebugPass(Camera);
        }

        // Unbind light culling resources after base pass
        if (LightCullingManager && bUseTiledCulling)
        {
            LightCullingManager->UnbindFromPS(RHIDevice->GetDeviceContext());
        }

        RenderFogPass(World,Camera,Viewport);
        RenderFXAAPaxx(World, Camera, Viewport);
        RenderEditorPass(World, Camera, Viewport);
        RenderSHAmbientLightPass(World);  // Capture SH after scene is rendered
        break;
    }
    case EViewModeIndex::VMI_SceneDepth:
    {
        RenderBasePass(World, Camera, Viewport);  // calls RenderScene, which executes the depth-only pass 
                                                  // (RenderSceneDepthPass) according to the current view mode
        RenderSceneDepthVisualizePass(Camera);    // Depth → Grayscale visualize

        RenderEditorPass(World, Camera, Viewport);
        break;
    }
    case EViewModeIndex::VMI_WorldNormal:
    {
        BeginLineBatch();
        OverrideShader = UResourceManager::GetInstance().Load<UShader>("WorldNormalShader.hlsl");
        RenderBasePass(World, Camera, Viewport);
        OverrideShader = nullptr;
        RenderEditorPass(World, Camera, Viewport);
        break;
    }
    default:
        break;
    }
    
    // Overlay (UI, debug visualization)
    RenderOverlayPass(World);
}

void URenderer::RenderEditorPass(UWorld* World, ACameraActor* Camera, FViewport* Viewport)
{
    // 뷰포트의 실제 크기로 aspect ratio 계산
    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0)
    {
        ViewportAspectRatio = 1.0f;
    }

    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);

    if (!World->IsPIEWorld())
    {
        RHIDevice->OMSetRenderTargets(ERenderTargetType::None);
        RHIDevice->PSSetRenderTargetSRV(ERenderTargetType::None);
        RHIDevice->OMSetRenderTargets(ERenderTargetType::Frame | ERenderTargetType::ID | ERenderTargetType::NoDepth);

        for (auto& Billboard : World->GetLevel()->GetComponentList<UBillboardComponent>())
        {
            Billboard->Render(this, ViewMatrix, ProjectionMatrix, Viewport->GetShowFlags());
        }
       
        if (AGizmoActor* Gizmo = World->GetGizmoActor())
        {
            Gizmo->Render(Camera, Viewport);
        }
    }
}

void URenderer::RenderActorsInViewport(UWorld* World, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, FViewport* Viewport)
{
    if (!World || !Viewport)
    {
        return;
    }
    if (!Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_Primitives))
    {
        return;
    }

    FFrustum ViewFrustum;
    ViewFrustum.Update(ViewMatrix * ProjectionMatrix);

    BeginLineBatch();
    SetViewModeType(CurrentViewMode);

    RenderPrimitives(World, ViewMatrix, ProjectionMatrix, Viewport);

    OMSetBlendState(false);
    RenderEngineActors(World->GetEngineActors(), ViewMatrix, ProjectionMatrix, Viewport);

    RenderDecals(World, ViewMatrix, ProjectionMatrix, Viewport);
    
    // BVH 바운드 시각화
    if (Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_BVH))
    {
        AddLines(World->GetBVH().GetBVHBoundsWire(), FVector4(0.5f, 0.5f, 1, 1));
    }

    // SpotLight cone visualization
    for (ULightComponent* LightComponent : World->GetLevel()->GetComponentList<ULightComponent>())
    {
        if (LightComponent)
        {
            LightComponent->DrawDebugLines(this, ViewMatrix, ProjectionMatrix);
            LightComponent->UpdateSpriteColor(LightComponent->GetTintColor());
        }        
    }

    EndLineBatch(FMatrix::Identity(), ViewMatrix, ProjectionMatrix);
    
}

void URenderer::RenderPrimitives(UWorld* World, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, FViewport* Viewport)
{
    USelectionManager& SelectionManager = USelectionManager::GetInstance();
    AActor* SelectedActor = SelectionManager.GetSelectedActor();

    for (UPrimitiveComponent* PrimitiveComponent : World->GetLevel()->GetComponentList<UPrimitiveComponent>())
    {
        // 안전성 체크: nullptr 또는 비활성 컴포넌트 스킵
        if (!PrimitiveComponent || !PrimitiveComponent->IsActive())
            continue;

        bool bIsSelected = false;
        if (Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_BoundingBoxes))
        {
            AddLines(PrimitiveComponent->GetBoundingBoxLines(), PrimitiveComponent->GetBoundingBoxColor());
        }
        if (PrimitiveComponent->GetOwner() == SelectedActor)
        {
            bIsSelected = true;
        }
        FVector rgb(1.0f, 1.0f, 1.0f);
        UpdateSetCBuffer(HighLightBufferType(bIsSelected, rgb, 0, 0, 0, 0));
        PrimitiveComponent->Render(this, ViewMatrix, ProjectionMatrix, Viewport->GetShowFlags());
    }
}

void URenderer::RenderDecals(UWorld* World, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, FViewport* Viewport)
{
    for (UDecalComponent* Decal : World->GetLevel()->GetComponentList<UDecalComponent>())
    {
        if (Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_BoundingBoxes))
        {
            AddLines(Decal->GetBoundingBoxLines(), Decal->GetBoundingBoxColor());
        }
        FOBB DecalWorldOBB = Decal->GetWorldOBB();

        if (World->GetUseBVH() && World->GetBVH().IsBuild())
        {
            TArray<UPrimitiveComponent*> CollisionPrimitives = World->GetBVH().GetCollisionWithOBB(DecalWorldOBB);
            for (UPrimitiveComponent* Primitive : CollisionPrimitives)
            {
                Decal->Render(this, Primitive, ViewMatrix, ProjectionMatrix, Viewport);
            }
        }
        else
        {
            for (UPrimitiveComponent* Primitive : World->GetLevel()->GetComponentList<UPrimitiveComponent>())
            {
                if (IntersectOBBAABB(DecalWorldOBB, Primitive->GetWorldAABB()))
                {
                    Decal->Render(this, Primitive, ViewMatrix, ProjectionMatrix, Viewport);
                }
            }
        }
    }
}

void URenderer::RenderEngineActors(const TArray<AActor*>& EngineActors, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, FViewport* Viewport)
{
    for (AActor* EngineActor : EngineActors)
    {
        if (!EngineActor || EngineActor->GetActorHiddenInGame())
        {
            continue;
        }

        if (Cast<AGridActor>(EngineActor) && !Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
        {
            continue;
        }

        if (Cast<AGizmoActor>(EngineActor))
        {
            continue;
        }

        for (UActorComponent* Component : EngineActor->GetComponents())
        {
            if (!Component)
            {
                continue;
            }

            if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
            {
                if (!ActorComp->IsActive())
                {
                    continue;
                }
            }

            if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
            {
                SetViewModeType(EViewModeIndex::VMI_Unlit);
                Primitive->Render(this, ViewMatrix, ProjectionMatrix, Viewport->GetShowFlags());
                OMSetDepthStencilState(EComparisonFunc::LessEqual);
            }
        }
        OMSetBlendState(false);
    }
}

void URenderer::RenderPostProcessing(UShader* Shader)
{
    OMSetBlendState(false);
    OMSetDepthStencilState(EComparisonFunc::Disable);

    ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();
    DeviceContext->IASetInputLayout(nullptr);
    DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    RHIDevice->PSSetDefaultSampler(0);

    //FrameBuffer -> TemporalBuffer
    PrepareShader(UResourceManager::GetInstance().Load<UShader>("Copy.hlsl"));
    RHIDevice->OMSetRenderTargets(ERenderTargetType::None);
    RHIDevice->PSSetRenderTargetSRV(ERenderTargetType::None);
    RHIDevice->PSSetRenderTargetSRV(ERenderTargetType::Frame);
    RHIDevice->OMSetRenderTargets(ERenderTargetType::Temporal);
   // RHIDevice->GetDeviceContext()->DrawIndexed(StaticMesh->GetIndexCount(), 0, 0);
    RHIDevice->GetDeviceContext()->Draw(6, 0);

    //TemporalBuffer ->FrameBuffer
    PrepareShader(Shader);
    RHIDevice->OMSetRenderTargets(ERenderTargetType::None);
    RHIDevice->PSSetRenderTargetSRV(ERenderTargetType::None);
    RHIDevice->PSSetRenderTargetSRV(ERenderTargetType::Temporal | ERenderTargetType::Depth);
    RHIDevice->OMSetRenderTargets(ERenderTargetType::Frame | ERenderTargetType::NoDepth);
   // RHIDevice->GetDeviceContext()->DrawIndexed(StaticMesh->GetIndexCount(), 0, 0);
    RHIDevice->GetDeviceContext()->Draw(6, 0);

}

void URenderer::RenderSceneToCubemapFace(UWorld* World, const FMatrix& ViewMatrix, const FMatrix& ProjMatrix, const FVector& ProbePosition, FViewport* Viewport)
{
    if (!World || !Viewport)
        return;

    // 1. View/Projection 상수 버퍼 업데이트 (ProbePosition을 카메라 위치로 사용)
    UpdateSetCBuffer(ViewProjBufferType(ViewMatrix, ProjMatrix, ProbePosition));

    // 2. 렌더 스테이트 설정
    RHIDevice->OMSetBlendState(false);
    RHIDevice->RSSetNoCullState();  // 큐브맵 렌더링 시 winding order 문제 방지
    RHIDevice->OmSetDepthStencilState(EComparisonFunc::LessEqual);
    RHIDevice->IASetPrimitiveTopology();

    // 3. 월드의 AAmbientActor만 렌더링
    USelectionManager& SelectionManager = USelectionManager::GetInstance();
    AActor* SelectedActor = SelectionManager.GetSelectedActor();

    for (AActor* Actor : World->GetLevel()->GetActors())
    {
        // AAmbientActor만 렌더링
        AAmbientActor* AmbientActor = Cast<AAmbientActor>(Actor);
        if (!AmbientActor)
            continue;

        // 액터의 모든 프리미티브 컴포넌트 렌더링
        for (UActorComponent* ActorComponent : AmbientActor->GetComponents())
        {
            UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ActorComponent);
            // 안전성 체크: nullptr 또는 비활성 컴포넌트 스킵
            if (!PrimitiveComponent || !PrimitiveComponent->IsActive())
                continue;

            // 빌보드나 에디터 전용 컴포넌트는 큐브맵에서 제외
            if (Cast<UBillboardComponent>(PrimitiveComponent))
                continue;

            bool bIsSelected = false;
            if (AmbientActor == SelectedActor)
            {
                bIsSelected = true;
            }

            FVector rgb(1.0f, 1.0f, 1.0f);
            UpdateSetCBuffer(HighLightBufferType(bIsSelected, rgb, 0, 0, 0, 0));
            PrimitiveComponent->Render(this, ViewMatrix, ProjMatrix, Viewport->GetShowFlags());
        }
    }
}

void URenderer::RenderFogPass(UWorld* World, ACameraActor* Camera, FViewport* Viewport)
{

    for (UExponentialHeightFogComponent* FogComponent : World->GetLevel()->GetComponentList<UExponentialHeightFogComponent>())
    {
        if (FogComponent->IsRender())
        {
            FogComponent->Render(this, Camera->GetActorLocation(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix(), Viewport);
            //첫번째 것만 그림
            break;
        }
   }

}

void URenderer::RenderFXAAPaxx(UWorld* World, ACameraActor* Camera, FViewport* Viewport)
{
    UpdateSetCBuffer(FGammaBufferType(Gamma));
    RenderPostProcessing(UResourceManager::GetInstance().Load<UShader>("Gamma.hlsl"));
    for (UFXAAComponent* FXAAComponent : World->GetLevel()->GetComponentList<UFXAAComponent>())
    {
        FXAAComponent->Render(this);
        //첫번째 것만 그림
        break;
    }
}

void URenderer::RenderPointLightPass(UWorld* World)
{
    if (!World) return;

    // 1?? 라이트 컴포넌트 수집 (PointLight, PointLight 등)
    FPointLightBufferType PointLightCB{};
    PointLightCB.PointLightCount=0;
    for (UPointLightComponent* PointLightComponent : World->GetLevel()->GetComponentList<UPointLightComponent>())
    {
        int idx = PointLightCB.PointLightCount++;
        if (idx >= MAX_POINT_LIGHTS) break;

        PointLightCB.PointLights[idx].Position = FVector4(
            PointLightComponent->GetWorldLocation(), PointLightComponent->GetRadius()
        );
        PointLightCB.PointLights[idx].Color = FVector4(
            PointLightComponent->GetFinalColor().R, PointLightComponent->GetFinalColor().G, PointLightComponent->GetFinalColor().B, PointLightComponent->GetRadiusFallOff()
        );
                
    }
    // 2?? 상수 버퍼 GPU로 업데이트
    UpdateSetCBuffer(PointLightCB);
    
}

void URenderer::RenderDirectionalLightPass(UWorld* World)
{
    if (!World) return;

    FDirectionalLightBufferType DirectionalLightCB{};
    DirectionalLightCB.DirectionalLightCount = 0;
    for (UDirectionalLightComponent* DirectionalLightComponent : World->GetLevel()->GetComponentList<UDirectionalLightComponent>())
    {
        int idx = DirectionalLightCB.DirectionalLightCount++;
        if (idx >= MAX_POINT_LIGHTS) break;

        DirectionalLightCB.DirectionalLights[idx].Direction = DirectionalLightComponent->GetDirection();
        DirectionalLightCB.DirectionalLights[idx].Color = FLinearColor(
            DirectionalLightComponent->GetFinalColor().R,DirectionalLightComponent->GetFinalColor().G,DirectionalLightComponent->GetFinalColor().B,DirectionalLightComponent->GetIntensity()
            );
        DirectionalLightCB.DirectionalLights[idx].bEnableSpecular = DirectionalLightComponent->IsEnabledSpecular();
    }
    UpdateSetCBuffer(DirectionalLightCB);    
}

void URenderer::RenderSpotLightPass(UWorld* World)
{
    if (!World) return;
     
    FSpotLightBufferType SpotLightCB{};
    SpotLightCB.SpotLightCount = 0;
    for (USpotLightComponent* PointLightComponent : World->GetLevel()->GetComponentList<USpotLightComponent>())
    {
        int idx = SpotLightCB.SpotLightCount++;
        if (idx >= MAX_POINT_LIGHTS) break;

        SpotLightCB.SpotLights[idx].Position = FVector4(
            PointLightComponent->GetWorldLocation(), PointLightComponent->GetRadius()
        );
        SpotLightCB.SpotLights[idx].Color = FVector4(
            PointLightComponent->GetFinalColor().R, PointLightComponent->GetFinalColor().G, PointLightComponent->GetFinalColor().B, PointLightComponent->GetRadiusFallOff()
        );
        // If set on component, propagate cone angles
        SpotLightCB.SpotLights[idx].InnerConeAngle = PointLightComponent->GetInnerConeAngle();
        SpotLightCB.SpotLights[idx].OuterConeAngle = PointLightComponent->GetOuterConeAngle();
        SpotLightCB.SpotLights[idx].Direction = PointLightComponent->GetDirection();   
        SpotLightCB.SpotLights[idx].InAndOutSmooth = PointLightComponent->GetInAndOutSmooth();
    }
    // 2?? 상수 버퍼 GPU로 업데이트
    UpdateSetCBuffer(SpotLightCB); 
}


void URenderer::RenderSHAmbientLightPass(UWorld* World)
{
    if (!World) return;

    // Find all active AmbientLightComponents in the world
    const auto& ProbeList = World->GetLevel()->GetComponentList<UAmbientLightComponent>();

    // Prepare multi-probe buffer
    FMultiSHProbeBufferType MultiProbeBuffer = {};
    MultiProbeBuffer.ProbeCount = 0;

    if (ProbeList.empty())
    {
        // No probes found - upload empty buffer
        UpdateSetCBuffer(MultiProbeBuffer);
        return;
    }

    // Collect up to MAX_SH_PROBES active probes
    for (UAmbientLightComponent* Probe : ProbeList)
    {
        if (!Probe || !Probe->IsActive())
            continue;

        if (MultiProbeBuffer.ProbeCount >= MAX_SH_PROBES)
            break;

        int32 idx = MultiProbeBuffer.ProbeCount;
        const FSHAmbientLightBufferType& SHBuffer = Probe->GetSHBuffer();
        FVector ProbePos = Probe->GetWorldLocation();
        FVector BoxExtent = Probe->GetBoxExtent();
        float ProbeFalloff = Probe->GetFalloff();

        // Copy probe data
        MultiProbeBuffer.Probes[idx].Position = FVector4(ProbePos.X, ProbePos.Y, ProbePos.Z, BoxExtent.Z); // w = BoxExtent.Z
        for (int32 i = 0; i < 9; ++i)
        {
            MultiProbeBuffer.Probes[idx].SHCoefficients[i] = SHBuffer.SHCoefficients[i];
        }
        MultiProbeBuffer.Probes[idx].Intensity = SHBuffer.Intensity;
        MultiProbeBuffer.Probes[idx].Falloff = ProbeFalloff;
        MultiProbeBuffer.Probes[idx].BoxExtent = FVector2D(BoxExtent.X, BoxExtent.Y);

        MultiProbeBuffer.ProbeCount++;
    }

    // Upload to GPU
    UpdateSetCBuffer(MultiProbeBuffer);
}

void URenderer::RenderOverlayPass(UWorld* World)
{
    // TODO: 오버레이(UI, 디버그 텍스트 등) 구현
}

void URenderer::RenderSceneDepthVisualizePass(ACameraActor* Camera)
{
    // +-+ 1. Save Original Viewport State +-+
    // 이 패스는 렌더 타겟 전체에 그리기 위해 뷰포트를 변경합니다.
    // 이후에 렌더링될 EditorPass (기즈모 등)가 올바른 뷰포트 영역에 그려지도록
    // 현재 뷰포트 상태를 미리 저장합니다.
    D3D11_VIEWPORT OriginalViewport;
    UINT NumViewports = 1;
    RHIDevice->GetDeviceContext()->RSGetViewports(&NumViewports, &OriginalViewport);

    // +-+ 2. Set Render State for Full-Screen Pass +-+
    // RTV(컬러)만 바인딩하고 DSV(뎁스)는 바인딩 해제(nullptr)합니다.
    // 뎁스 텍스처를 셰이더에서 읽기(SRV) 위해 DSV에서 바인딩 해제해야 합니다.
    ID3D11RenderTargetView* FrameRTV = static_cast<D3D11RHI*>(RHIDevice)->GetFrameRTV();
    RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &FrameRTV, nullptr);

    // 뎁스 테스트와 쓰기를 모두 비활성화합니다.
    RHIDevice->OmSetDepthStencilState(EComparisonFunc::Disable);

    // +-+ 3. Set Full-Screen Viewport +-+
    // DSV를 nullptr로 설정하면 D3D11 파이프라인이 뷰포트 설정을 무효화할 수 있습니다.
    // 따라서 렌더 타겟(프레임 버퍼) 크기에 맞는 "전체 화면" 뷰포트를 명시적으로 다시 설정합니다.
    D3D11_TEXTURE2D_DESC BackDesc{};
    ID3D11Texture2D* FrameBuffer = static_cast<D3D11RHI*>(RHIDevice)->GetFrameBuffer();
    if (FrameBuffer)
    {
        FrameBuffer->GetDesc(&BackDesc);
        D3D11_VIEWPORT FullScreenVP = {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<FLOAT>(BackDesc.Width),
            .Height = static_cast<FLOAT>(BackDesc.Height),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f
        };
        // 뷰포트를 전체 화면으로 리셋합니다.
        RHIDevice->GetDeviceContext()->RSSetViewports(1, &FullScreenVP);
    }

    // +-+ 4. Set Shader & Resources +-+
    SceneDepthVisualizeShader = UResourceManager::GetInstance().Load<UShader>("DepthVisualizeShader.hlsl");
    PrepareShader(SceneDepthVisualizeShader);
    if (Camera)
    {
        CameraInfoBufferType CameraInfo;
        CameraInfo.NearClip = Camera->GetCameraComponent()->GetNearClip();
        CameraInfo.FarClip = Camera->GetCameraComponent()->GetFarClip();
        UpdateSetCBuffer(CameraInfoBufferType(CameraInfo));
    }

    // 뎁스 텍스처를 픽셀 셰이더의 리소스(SRV)로 바인딩합니다.
    ID3D11ShaderResourceView* DepthSRV = static_cast<D3D11RHI*>(RHIDevice)->GetDepthSRV();
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &DepthSRV); // SET

    RHIDevice->PSSetDefaultSampler(0);
    RHIDevice->IASetPrimitiveTopology();

    // +-+ 5. Draw Full-Screen Triangle +-+
    // 정점 3개로 구성된 전체 화면 삼각형을 그립니다.
    RHIDevice->GetDeviceContext()->Draw(3, 0);

    // +-+ 6. Restore Original State +-+
    // 뎁스 텍스처를 SRV에서 바인딩 해제합니다 (이후 DSV로 다시 바인딩할 수 있도록).
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV);

    // 패스 시작 시 저장해두었던 원래 뷰포트로 복원합니다.
    // (이것이 기즈모 위치를 올바르게 수정하는 핵심입니다.)
    RHIDevice->GetDeviceContext()->RSSetViewports(1, &OriginalViewport);

    // 뎁스 상태를 기본값(테스트 및 쓰기 활성화)으로 복원합니다.
    RHIDevice->OmSetDepthStencilState(EComparisonFunc::LessEqual);
}

void URenderer::InitializeLineBatch()
{
    // Create UDynamicMesh for efficient line batching
    DynamicLineMesh = UResourceManager::GetInstance().Load<ULineDynamicMesh>("Line");
    
    // Initialize with maximum capacity (MAX_LINES * 2 vertices, MAX_LINES * 2 indices)
    uint32 maxVertices = MAX_LINES * 2;
    uint32 maxIndices = MAX_LINES * 2;
    DynamicLineMesh->Load(maxVertices, maxIndices, RHIDevice->GetDevice());

    // Create FMeshData for accumulating line data
    LineBatchData = new FMeshData();
    
    // Load line shader
    LineShader = UResourceManager::GetInstance().Load<UShader>("ShaderLine.hlsl");
}

void URenderer::BeginLineBatch()
{
    if (!LineBatchData) return;
    
    bLineBatchActive = true;
    
    // Clear previous batch data
    LineBatchData->Vertices.clear();
    LineBatchData->Color.clear();
    LineBatchData->Indices.clear();
}

void URenderer::AddLine(const FVector& Start, const FVector& End, const FVector4& Color)
{
    if (!bLineBatchActive || !LineBatchData) return;
    
    uint32 startIndex = static_cast<uint32>(LineBatchData->Vertices.size());
    
    // Add vertices
    LineBatchData->Vertices.push_back(Start);
    LineBatchData->Vertices.push_back(End);
    
    // Add colors
    LineBatchData->Color.push_back(Color);
    LineBatchData->Color.push_back(Color);
    
    // Add indices for line (2 vertices per line)
    LineBatchData->Indices.push_back(startIndex);
    LineBatchData->Indices.push_back(startIndex + 1);
}

void URenderer::AddLines(const TArray<FVector>& StartPoints, const TArray<FVector>& EndPoints, const TArray<FVector4>& Colors)
{
    if (!bLineBatchActive || !LineBatchData) return;
    
    // Validate input arrays have same size
    if (StartPoints.size() != EndPoints.size() || StartPoints.size() != Colors.size())
        return;
    
    uint32 startIndex = static_cast<uint32>(LineBatchData->Vertices.size());
    
    // Reserve space for efficiency
    size_t lineCount = StartPoints.size();
    LineBatchData->Vertices.reserve(LineBatchData->Vertices.size() + lineCount * 2);
    LineBatchData->Color.reserve(LineBatchData->Color.size() + lineCount * 2);
    LineBatchData->Indices.reserve(LineBatchData->Indices.size() + lineCount * 2);
    
    // Add all lines at once
    for (size_t i = 0; i < lineCount; ++i)
    {
        uint32 currentIndex = startIndex + static_cast<uint32>(i * 2);
        
        // Add vertices
        LineBatchData->Vertices.push_back(StartPoints[i]);
        LineBatchData->Vertices.push_back(EndPoints[i]);
        
        // Add colors
        LineBatchData->Color.push_back(Colors[i]);
        LineBatchData->Color.push_back(Colors[i]);
        
        // Add indices for line (2 vertices per line)
        LineBatchData->Indices.push_back(currentIndex);
        LineBatchData->Indices.push_back(currentIndex + 1);
    }
}

void URenderer::AddLines(const TArray<FVector>& LineList, const FVector4& Color)
{
    if (!bLineBatchActive || !LineBatchData) return;

    // Validate input arrays have same size
    if (LineList.size() < 2)
        return;

    uint32 StartIdx = static_cast<uint32>(LineBatchData->Vertices.size());
    size_t AddLineCount = LineList.size();
    LineBatchData->Vertices.reserve(LineBatchData->Vertices.size() + AddLineCount);
    LineBatchData->Color.reserve(LineBatchData->Color.size() + AddLineCount);
    LineBatchData->Indices.reserve(LineBatchData->Indices.size() + AddLineCount);

    for (int i = 0; i < AddLineCount; i++)
    {
        uint32 currentIndex = StartIdx + static_cast<uint32>(i);

        // Add vertices
        LineBatchData->Vertices.push_back(LineList[i]);

        // Add colors
        LineBatchData->Color.push_back(Color);

        // Add indices for line (2 vertices per line)
        LineBatchData->Indices.push_back(currentIndex);
    }
}

void URenderer::EndLineBatch(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix)
{
    if (!bLineBatchActive || !LineBatchData || !DynamicLineMesh || LineBatchData->Vertices.empty())
    {
        bLineBatchActive = false;
        return;
    }
    
    // Efficiently update dynamic mesh data (no buffer recreation!)
    if (!DynamicLineMesh->UpdateData(LineBatchData, RHIDevice->GetDeviceContext()))
    {
        bLineBatchActive = false;
        return;
    }
    
    // Set up rendering state
    UpdateSetCBuffer(ModelBufferType(ModelMatrix));
    PrepareShader(LineShader);
    OMSetBlendState(true);
    
    // Render using dynamic mesh
    if (DynamicLineMesh->GetCurrentVertexCount() > 0 && DynamicLineMesh->GetCurrentIndexCount() > 0)
    {
        UINT stride = sizeof(FVertexSimple);
        UINT offset = 0;  
        
        ID3D11Buffer* vertexBuffer = DynamicLineMesh->GetVertexBuffer();
        ID3D11Buffer* indexBuffer = DynamicLineMesh->GetIndexBuffer();
        
        RHIDevice->GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        RHIDevice->GetDeviceContext()->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        RHIDevice->GetDeviceContext()->DrawIndexed(DynamicLineMesh->GetCurrentIndexCount(), 0, 0);
        
        // 라인 렌더링에 대한 DrawCall 통계 추가
        URenderingStatsCollector::GetInstance().IncrementDrawCalls();
    }
    
    bLineBatchActive = false;
}

UPrimitiveComponent* URenderer::GetCollidedPrimitive(int MouseX, int MouseY) const
{
    //GPU와 동기화 문제 때문에 Map이 호출될때까지 기다려야해서 피킹 하는 프레임에 엄청난 프레임 드랍이 일어남.
    //******비동기 방식으로 무조건 바꿔야함****************
    uint32 PickedId = 0;

    ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();
    //스테이징 버퍼를 가져와야 하는데 이걸 Device 추상 클래스가 Getter로 가지고 있는게 좋은 설계가 아닌 것 같아서 일단 캐스팅함
    D3D11RHI* RHI = static_cast<D3D11RHI*>(RHIDevice);

    D3D11_BOX Box{};
    Box.left = MouseX;
    Box.right= MouseX+1;
    Box.top = MouseY;
    Box.bottom = MouseY+1;
    Box.front = 0;
    Box.back = 1;
    
    DeviceContext->CopySubresourceRegion(
        RHI->GetIdStagingBuffer(),
        0,
        0, 0, 0,
        RHI->GetIdBuffer(),
        0,
        &Box);
    D3D11_MAPPED_SUBRESOURCE MapResource{};
    if (SUCCEEDED(DeviceContext->Map(RHI->GetIdStagingBuffer(), 0, D3D11_MAP_READ, 0, &MapResource)))
    {
        PickedId = *static_cast<uint32*>(MapResource.pData);
        DeviceContext->Unmap(RHI->GetIdStagingBuffer(), 0);
    }

    if (PickedId == 0)
        return nullptr;
    return Cast<UPrimitiveComponent>(GUObjectArray[PickedId]);
}

void URenderer::ResetRenderStateTracking()
{
    LastMaterial = nullptr;
    LastShader = nullptr;
    LastTexture = nullptr;
}

void URenderer::ClearLineBatch()
{
    if (!LineBatchData) return;
    
    LineBatchData->Vertices.clear();
    LineBatchData->Color.clear();
    LineBatchData->Indices.clear();
    
    bLineBatchActive = false;
}

// Shading model accessors (used by UI)
void URenderer::SetShadingModel(ELightShadingModel Model)
{
    if (CurrentShadingModel == Model)
        return;

    CurrentShadingModel = Model;

    // Hot-reload affected shaders when shading model changes
    //TODO: 우버 쉐이더 리스틀르 만들어서, shader 전체순회가 아닌 우버쉐이더 순회로 변경하는 게 좋을 듯
    TArray<UShader*> AllShaders = UResourceManager::GetInstance().GetAll<UShader>();
    for (UShader* Shader : AllShaders)
    {
        if (!Shader) continue;
        const FString& Path = Shader->GetFilePath();
        // Reload only Uber shader variants for now
        if (Path.find("UberLit.hlsl") != std::string::npos)
        {
            Shader->ReloadForShadingModel(Model, RHIDevice->GetDevice());
        }
    }

    // Force a rebind so the next draw uses the refreshed shaders
    LastShader = nullptr;
}

ELightShadingModel URenderer::GetShadingModel() const
{
    return CurrentShadingModel;
}

void URenderer::RenderTileCullingDebugPass(ACameraActor* Camera)
{
    // Set render state for full-screen overlay
    RHIDevice->OMSetBlendState(true); // Enable alpha blending
    RHIDevice->OmSetDepthStencilState(EComparisonFunc::Disable); // Disable depth test

    // Load and prepare debug shader
    TileCullingDebugShader = UResourceManager::GetInstance().Load<UShader>("TileCullingDebug.hlsl");
    if (!TileCullingDebugShader)
    {
        OutputDebugStringA("[Renderer] ERROR: Failed to load TileCullingDebug.hlsl\n");
        return;
    }

    PrepareShader(TileCullingDebugShader);

    // Upload camera info (Near/Far clip planes) for depth linearization
    if (Camera)
    {
        CameraInfoBufferType CameraInfo;
        CameraInfo.NearClip = Camera->GetCameraComponent()->GetNearClip();
        CameraInfo.FarClip = Camera->GetCameraComponent()->GetFarClip();
        UpdateSetCBuffer(CameraInfoBufferType(CameraInfo));
    }

    // Bind depth texture to pixel shader (t12)
    ID3D11ShaderResourceView* DepthSRV = static_cast<D3D11RHI*>(RHIDevice)->GetDepthSRV();
    RHIDevice->GetDeviceContext()->PSSetShaderResources(12, 1, &DepthSRV);

    // Set sampler state
    RHIDevice->PSSetDefaultSampler(0);

    // Set primitive topology for full-screen triangle
    RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Clear input layout (using SV_VertexID in shader)
    RHIDevice->GetDeviceContext()->IASetInputLayout(nullptr);
    RHIDevice->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    RHIDevice->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

    // Draw full-screen triangle (3 vertices, no index buffer)
    RHIDevice->GetDeviceContext()->Draw(3, 0);

    // Unbind depth SRV
    ID3D11ShaderResourceView* nullSRV = nullptr;
    RHIDevice->GetDeviceContext()->PSSetShaderResources(12, 1, &nullSRV);

    OutputDebugStringA("[Renderer] Tile culling debug pass complete\n");
}
