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

URenderer::URenderer(URHIDevice* InDevice) : RHIDevice(InDevice)
{
    InitializeLineBatch();
}

URenderer::~URenderer()
{
    if (LineBatchData)
    {
        delete LineBatchData;
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
    // 셰이더 변경 추적
    if (LastShader != InShader)
    {
        URenderingStatsCollector::GetInstance().IncrementShaderChanges();
        LastShader = InShader;
    }
    
    RHIDevice->GetDeviceContext()->VSSetShader(InShader->GetVertexShader(), nullptr, 0);
    RHIDevice->GetDeviceContext()->PSSetShader(InShader->GetPixelShader(), nullptr, 0);
    RHIDevice->GetDeviceContext()->IASetInputLayout(InShader->GetInputLayout());
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
    case EVertexLayoutType::PositionUV:
        stride = sizeof(FVertexUV);
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
            const UMaterial* const Material = UResourceManager::GetInstance().Get<UMaterial>(InComponentMaterialSlots[i].MaterialName);
            const FObjMaterialInfo& MaterialInfo = Material->GetMaterialInfo();
            bool bHasTexture = !(MaterialInfo.DiffuseTextureFileName == FName::None());
            bool bHasNormalTexture = !(MaterialInfo.NormalTextureFileName == FName::None());

            // 재료 변경 추적
            if (LastMaterial != Material)
            {
                StatsCollector.IncrementMaterialChanges();
                LastMaterial = const_cast<UMaterial*>(Material);
            }

            // Diffuse Texture
            if (bHasTexture)
            {
                FTextureData* TextureData = UResourceManager::GetInstance().CreateOrGetTextureData(MaterialInfo.DiffuseTextureFileName);

                // 텍스처 변경 추적 (임시로 FTextureData*를 UTexture*로 캠스트)
                UTexture* CurrentTexture = reinterpret_cast<UTexture*>(TextureData);
                if (LastTexture != CurrentTexture)
                {
                    StatsCollector.IncrementTextureChanges();
                    LastTexture = CurrentTexture;
                }

                RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &(TextureData->TextureSRV));
            }

            // Normal Texture
            if (bHasNormalTexture)
            {
                FTextureData* NormalTextureData = UResourceManager::GetInstance().CreateOrGetTextureData(MaterialInfo.NormalTextureFileName);
                RHIDevice->GetDeviceContext()->PSSetShaderResources(1, 1, &(NormalTextureData->TextureSRV));
            }

            RHIDevice->UpdateSetCBuffer({ FMaterialInPs(MaterialInfo), (uint32)true, (uint32)bHasTexture, (uint32)bHasNormalTexture }); // PSSet도 해줌

            // DrawCall 수실행 및 통계 추가
            RHIDevice->GetDeviceContext()->DrawIndexed(MeshGroupInfos[i].IndexCount, MeshGroupInfos[i].StartIndex, 0);
            StatsCollector.IncrementDrawCalls();
        }
    }
    else
    {
        FObjMaterialInfo ObjMaterialInfo;
        RHIDevice->UpdateSetCBuffer({ FMaterialInPs(ObjMaterialInfo), (uint32)false, (uint32)false, (uint32)false }); // PSSet도 해줌
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
    // +-+ Set Render State +-+
    RHIDevice->OMSetDepthOnlyTarget();     // DSV binding
    RHIDevice->OMSetBlendState(false);     // color write mask = 0
    RHIDevice->RSSetDefaultState();        // solid fill, back-face culling
    RHIDevice->IASetPrimitiveTopology();

    // +-+ Set Shader & Buffer +-+
    DepthOnlyShader = UResourceManager::GetInstance().Load<UShader>("DepthPrepassShader.hlsl");
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
    // DSV un-binding
    ID3D11RenderTargetView* nullRTV[1] = { nullptr };
    RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, nullRTV, nullptr);
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

//void URenderer::RenderPointLightShadowPass(UWorld* World)
//{
//    for (auto& Light : World->PointLights)
//    {
//        if (!Light->CastShadows)
//            continue;
//
//        const FVector LightPos = Light->Position;
//        const float NearZ = 1.0f;
//        const float FarZ = Light->Radius;
//
//        // 6방향 뷰행렬 구성
//        FMatrix LightViews[6];
//        LightViews[0] = FMatrix::LookAt(LightPos, LightPos + FVector(1, 0, 0), FVector(0, 1, 0));   // +X
//        LightViews[1] = FMatrix::LookAt(LightPos, LightPos + FVector(-1, 0, 0), FVector(0, 1, 0));  // -X
//        LightViews[2] = FMatrix::LookAt(LightPos, LightPos + FVector(0, 1, 0), FVector(0, 0, -1));  // +Y
//        LightViews[3] = FMatrix::LookAt(LightPos, LightPos + FVector(0, -1, 0), FVector(0, 0, 1));  // -Y
//        LightViews[4] = FMatrix::LookAt(LightPos, LightPos + FVector(0, 0, 1), FVector(0, 1, 0));   // +Z
//        LightViews[5] = FMatrix::LookAt(LightPos, LightPos + FVector(0, 0, -1), FVector(0, 1, 0));  // -Z
//
//        const FMatrix LightProj = FMatrix::PerspectiveFovLH(PI / 2.0f, 1.0f, NearZ, FarZ);
//
//        for (int Face = 0; Face < 6; ++Face)
//        {
//            RHI->SetRenderTarget(Light->ShadowCubeMap, Face);
//            RHI->ClearDepth(1.0f);
//            RHI->OMSetDepthStencilState(EComparisonFunc::LessEqualWrite);
//
//            UpdateShadowBuffer(LightViews[Face], LightProj, LightPos);
//            RenderSceneDepthOnly(World); // 깊이만 렌더
//        }
//    }
//}


void URenderer::RenderScene(UWorld* World, ACameraActor* Camera, FViewport* Viewport)
{

    if (!World || !Camera || !Viewport)
        return;

    // +-+-+ Render Pass Structure +-+-+

    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f; // 0으로 나누기 방지
    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);
    UpdateSetCBuffer(ViewProjBufferType(ViewMatrix, ProjectionMatrix, Camera->GetActorLocation()));
    switch (CurrentViewMode)
    {
    case EViewModeIndex::VMI_Lit:
    case EViewModeIndex::VMI_Unlit:
    case EViewModeIndex::VMI_Wireframe:
    {
        RenderPointLightPass(World);
        RenderBasePass(World, Camera, Viewport);  // Full color + depth pass (Opaque geometry - per viewport)
        RenderFogPass(World,Camera,Viewport);
        RenderFXAAPaxx(World, Camera, Viewport);
        RenderEditorPass(World, Camera, Viewport);
        break;
    }
    case EViewModeIndex::VMI_SceneDepth:
    {
        RenderBasePass(World, Camera, Viewport);  // calls RenderScene, which executes the depth-only pass 
                                                  // (RenderSceneDepthPass) according to the current view mode
        RenderSceneDepthVisualizePass(Camera);    // Depth → Grayscale visualize
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


    //const TArray<AActor*>& LevelActors = World->GetLevel() ? World->GetLevel()->GetActors() : TArray<AActor*>();
    //USelectionManager& SelectionManager = USelectionManager::GetInstance();

    //// 특수 처리가 필요한 컴포넌트들
    //TArray<UDecalComponent*> Decals;
    //TArray<UPrimitiveComponent*> RenderPrimitivesWithOutDecal;
    //TArray<UBillboardComponent*> BillboardComponentList;
    // 
    //// 액터별로 순회하며 렌더링
    //for (AActor* Actor : LevelActors)
    //{
    //    if (!Actor || Actor->GetActorHiddenInGame())
    //    {
    //        continue;
    //    }

    //    bool bIsSelected = SelectionManager.IsActorSelected(Actor);

    //    for (UActorComponent* ActorComp : Actor->GetComponents())
    //    {
    //        if (!ActorComp)
    //        {
    //            continue;
    //        }

    //        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(ActorComp))
    //        {
    //            // 바운딩 박스 그리기
    //            if (Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_BoundingBoxes))
    //            {
    //                AddLines(Primitive->GetBoundingBoxLines(), Primitive->GetBoundingBoxColor());
    //            }

    //            // 데칼 컴포넌트는 나중에 처리
    //            if (UDecalComponent* Decal = Cast<UDecalComponent>(ActorComp))
    //            {
    //                Decals.Add(Decal);
    //                continue;
    //            }
    //            if (UBillboardComponent* Billboard = Cast<UBillboardComponent>(ActorComp))
    //            {
    //                BillboardComponentList.Add(Billboard);
    //                continue;
    //            }

    //            RenderPrimitivesWithOutDecal.Add(Primitive);

    //            FVector rgb(1.0f, 1.0f, 1.0f);
    //            UpdateSetCBuffer(HighLightBufferType(bIsSelected, rgb, 0, 0, 0, 0));
    //            Primitive->Render(this, ViewMatrix, ProjectionMatrix, Viewport->GetShowFlags());
    //        }
    //    }
    //}

    //OMSetBlendState(false);
    //RenderEngineActors(World->GetEngineActors(), ViewMatrix, ProjectionMatrix, Viewport);

    //// 데칼 렌더링
    //if (Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_Decals))
    //{
    //    Decals.Sort([](const UDecalComponent* A, const UDecalComponent* B)
    //    {
    //        return A->GetSortOrder() < B->GetSortOrder();
    //    });

    //    for (UDecalComponent* Decal : Decals)
    //    {
    //        FOBB DecalWorldOBB = Decal->GetWorldOBB();

    //        if (World->GetUseBVH() && World->GetBVH().IsBuild())
    //        {
    //            TArray<UPrimitiveComponent*> CollisionPrimitives = World->GetBVH().GetCollisionWithOBB(DecalWorldOBB);
    //            for (UPrimitiveComponent* Primitive : CollisionPrimitives)
    //            {
    //                Decal->Render(this, Primitive, ViewMatrix, ProjectionMatrix, Viewport);
    //            }
    //        }
    //        else
    //        {
    //            for (UPrimitiveComponent* Primitive : RenderPrimitivesWithOutDecal)
    //            {
    //                if (IntersectOBBAABB(DecalWorldOBB, Primitive->GetWorldAABB()))
    //                {
    //                    Decal->Render(this, Primitive, ViewMatrix, ProjectionMatrix, Viewport);
    //                }
    //            }
    //        }
    //    }
    //}

    // BVH 바운드 시각화
    if (Viewport->IsShowFlagEnabled(EEngineShowFlags::SF_BVH))
    {
        AddLines(World->GetBVH().GetBVHBoundsWire(), FVector4(0.5f, 0.5f, 1, 1));
    }

    EndLineBatch(FMatrix::Identity(), ViewMatrix, ProjectionMatrix);

    // 빌보드는 마지막에 렌더링
   /* for (auto& Billboard : World->GetLevel()->GetComponentList<UBillboardComponent>())
    {
        Billboard->Render(this, ViewMatrix, ProjectionMatrix, Viewport->GetShowFlags());
    }*/

  
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
    /*UINT Stride = sizeof(FVertexUV);
    UINT Offset = 0;
    UStaticMesh* StaticMesh = UResourceManager::GetInstance().Load<UStaticMesh>("ScreenQuad");
    ID3D11Buffer* VertexBuffer = StaticMesh->GetVertexBuffer();
    ID3D11Buffer* IndexBuffer = StaticMesh->GetIndexBuffer();
    RHIDevice->GetDeviceContext()->IASetVertexBuffers(
        0, 1, &VertexBuffer, &Stride, &Offset
    );

    RHIDevice->GetDeviceContext()->IASetIndexBuffer(s
        IndexBuffer, DXGI_FORMAT_R32_UINT, 0
    );*/
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

    // 1️⃣ 라이트 컴포넌트 수집 (PointLight, PointLight 등)
    FPointLightBufferType PointLightCB{};
    PointLightCB.PointLightCount=0;
    for (UPointLightComponent* PointLightComponent : World->GetLevel()->GetComponentList<UPointLightComponent>())
    {
        int idx = PointLightCB.PointLightCount++;
        if (idx >= MAX_POINT_LIGHTS) break;

        PointLightCB.PointLights[idx].Position = FVector4(
            PointLightComponent->GetWorldLocation(), PointLightComponent->PointData.Radius
        );
        PointLightCB.PointLights[idx].Color = FVector4(
            PointLightComponent->PointData.Color.R, PointLightComponent->PointData.Color.G, PointLightComponent->PointData.Color.B, PointLightComponent->PointData.Intensity
        );
        PointLightCB.PointLights[idx].FallOff = PointLightComponent->PointData.RadiusFallOff;
    }
    // 2️⃣ 상수 버퍼 GPU로 업데이트
    UpdateSetCBuffer(PointLightCB);

    /*const TArray<AActor*>& Actors = World->GetLevel()->GetActors();se

    for (AActor* Actor : Actors)
    {
        if (!Actor) continue;
        for (UActorComponent* Comp : Actor->GetComponents())
        {
            if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
            {
                if (UPointLightComponent* Point = Cast<UPointLightComponent>(Prim))
                {
                    int idx = PointLightCB.PointLightCount++;
                    if (idx >= MAX_POINT_LIGHTS) break;

                    PointLightCB.PointLights[idx].Position = FVector4(
                        Point->GetWorldLocation(), Point->PointData.Radius
                    );
                    PointLightCB.PointLights[idx].Color = FVector4(
                        Point->PointData.Color.R, Point->PointData.Color.G, Point->PointData.Color.B, Point->PointData.Intensity
                    );
                    PointLightCB.PointLights[idx].FallOff = Point->PointData.RadiusFallOff;
                }
            }
        }
    }*/

    
}

void URenderer::RenderOverlayPass(UWorld* World)
{
    // TODO: 오버레이(UI, 디버그 텍스트 등) 구현
}

void URenderer::RenderSceneDepthVisualizePass(ACameraActor* Camera)
{
    // +-+ Set Render State +-+
    // Bind only RTV (DSV = nullptr)
    ID3D11RenderTargetView* FrameRTV = static_cast<D3D11RHI*>(RHIDevice)->GetFrameRTV();
    RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &FrameRTV, nullptr);
    RHIDevice->OmSetDepthStencilState(EComparisonFunc::Disable);

    // +-+ Re-set Viewport +-+
    // Because the DSV is set to nullptr
    // Unbinding DSV invalidates the current viewport state, causing it to appear smaller.
    // Get viewport size from the current framebuffer (Texture2D)
    D3D11_TEXTURE2D_DESC BackDesc{};
    ID3D11Texture2D* FrameBuffer = static_cast<D3D11RHI*>(RHIDevice)->GetFrameBuffer();
    if (FrameBuffer)
    {
        FrameBuffer->GetDesc(&BackDesc);
        D3D11_VIEWPORT vp = {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<FLOAT>(BackDesc.Width),
            .Height = static_cast<FLOAT>(BackDesc.Height),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f
        };
        // Reset Viewport
        RHIDevice->GetDeviceContext()->RSSetViewports(1, &vp);
    }

    // +-+ Set Shader & Buffer +-+
    SceneDepthVisualizeShader = UResourceManager::GetInstance().Load<UShader>("DepthVisualizeShader.hlsl");
    PrepareShader(SceneDepthVisualizeShader);
    if (Camera)
    {
        CameraInfoBufferType CameraInfo;
        CameraInfo.NearClip = Camera->GetCameraComponent()->GetNearClip();
        CameraInfo.FarClip = Camera->GetCameraComponent()->GetFarClip();
        UpdateSetCBuffer(CameraInfoBufferType(CameraInfo));
    }

    // +-+ Set Shader Resources (Texture) +-+
    // Bind depth SRV
    // If DSV were still bound, the set call would fail and result in a null binding.
    // TODO: Abstracting RHI access to the depth SRV
    ID3D11ShaderResourceView* DepthSRV = static_cast<D3D11RHI*>(RHIDevice)->GetDepthSRV();
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &DepthSRV);   // SET
    
    //ID3D11ShaderResourceView* currentSRV = nullptr;
    //RHIDevice->GetDeviceContext()->PSGetShaderResources(0, 1, &currentSRV); // GET
    //UE_LOG("Currently bound SRV at slot 0 = %p", currentSRV);      // null!!! if bind both RTV + DSV

    // +-+ Connect to PS s0 slot +-+
    RHIDevice->PSSetDefaultSampler(0);
    RHIDevice->IASetPrimitiveTopology();

    // +-+ Draw Full-Screen Triangle +-+
    RHIDevice->GetDeviceContext()->Draw(3, 0);

    // +-+ Restore Render State +-+
    // Unbind SRV to allow re-binding the depth texture as DSV
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV);
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

