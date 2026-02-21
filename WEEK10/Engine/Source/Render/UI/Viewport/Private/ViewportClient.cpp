#include "pch.h"

#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Level/Public/Level.h"
#include "Level/Public/World.h"

FViewportClient::FViewportClient()
{
    ViewportCamera = NewObject<UCamera>();

if (ViewportCamera)
    {
        ViewportCamera->SetMoveSpeed(UViewportManager::GetInstance().GetEditorCameraSpeed());
    }
}

FViewportClient::~FViewportClient()
{
    SafeDelete(ViewportCamera);
}

void FViewportClient::SetViewType(EViewType InType)
{
    ViewType = InType;
    
    if (!ViewportCamera) return;
    
    // Set camera type and orientation based on view type
    switch (InType)
    {
    case EViewType::Perspective:
        ViewportCamera->SetCameraType(ECameraType::ECT_Perspective);
        // Restore saved perspective location, rotation and far clip
        ViewportCamera->SetLocation(SavedPerspectiveLocation);
        ViewportCamera->SetRotation(SavedPerspectiveRotation);
        ViewportCamera->SetFarZ(SavedPerspectiveFarZ);
        break;
        
    case EViewType::OrthoTop:
        // Save current state if coming from perspective
        if (ViewportCamera->GetCameraType() == ECameraType::ECT_Perspective)
        {
            SavedPerspectiveLocation = ViewportCamera->GetLocation();
            SavedPerspectiveRotation = ViewportCamera->GetRotation();
            SavedPerspectiveFarZ = ViewportCamera->GetFarZ();
        }

        ViewportCamera->SetCameraType(ECameraType::ECT_Orthographic);
        ViewportCamera->SetFarZ(5000.0f);
        ViewportCamera->SetOrthoZoom(UViewportManager::GetInstance().GetSharedOrthoZoom());
        // Top view: Looking down (-Z direction)
        ViewportCamera->SetLocation(FVector(0.0f, 0.0f, 100.0f));
        ViewportCamera->SetForward(FVector(0.0f, 0.0f, -1.0f));
        ViewportCamera->SetRight(FVector(0.0f, -1.0f, 0.0f));
        ViewportCamera->SetUp(FVector(-1.0f, 0.0f, 0.0f));
        break;
        
    case EViewType::OrthoBottom:
        if (ViewportCamera->GetCameraType() == ECameraType::ECT_Perspective)
        {
            SavedPerspectiveLocation = ViewportCamera->GetLocation();
            SavedPerspectiveRotation = ViewportCamera->GetRotation();
            SavedPerspectiveFarZ = ViewportCamera->GetFarZ();
        }

        ViewportCamera->SetCameraType(ECameraType::ECT_Orthographic);
        ViewportCamera->SetFarZ(5000.0f);
        ViewportCamera->SetOrthoZoom(UViewportManager::GetInstance().GetSharedOrthoZoom());
        // Bottom view: Looking up (+Z direction)
        ViewportCamera->SetLocation(FVector(0.0f, 0.0f, -100.0f));
        ViewportCamera->SetForward(FVector(0.0f, 0.0f, 1.0f));
        ViewportCamera->SetRight(FVector(0.0f, 1.0f, 0.0f));
        ViewportCamera->SetUp(FVector(-1.0f, 0.0f, 0.0f));
        break;
        
    case EViewType::OrthoFront:
        if (ViewportCamera->GetCameraType() == ECameraType::ECT_Perspective)
        {
            SavedPerspectiveLocation = ViewportCamera->GetLocation();
            SavedPerspectiveRotation = ViewportCamera->GetRotation();
            SavedPerspectiveFarZ = ViewportCamera->GetFarZ();
        }

        ViewportCamera->SetCameraType(ECameraType::ECT_Orthographic);
        ViewportCamera->SetFarZ(5000.0f);
        ViewportCamera->SetOrthoZoom(UViewportManager::GetInstance().GetSharedOrthoZoom());
        // Front view: Looking forward (+X direction)
        ViewportCamera->SetLocation(FVector(-100.0f, 0.0f, 0.0f));
        ViewportCamera->SetForward(FVector(1.0f, 0.0f, 0.0f));
        ViewportCamera->SetRight(FVector(0.0f, -1.0f, 0.0f));
        ViewportCamera->SetUp(FVector(0.0f, 0.0f, 1.0f));
        break;

    case EViewType::OrthoBack:
        if (ViewportCamera->GetCameraType() == ECameraType::ECT_Perspective)
        {
            SavedPerspectiveLocation = ViewportCamera->GetLocation();
            SavedPerspectiveRotation = ViewportCamera->GetRotation();
            SavedPerspectiveFarZ = ViewportCamera->GetFarZ();
        }

        ViewportCamera->SetCameraType(ECameraType::ECT_Orthographic);
        ViewportCamera->SetFarZ(5000.0f);
        ViewportCamera->SetOrthoZoom(UViewportManager::GetInstance().GetSharedOrthoZoom());
        // Back view: Looking backward (-X direction)
        ViewportCamera->SetLocation(FVector(100.0f, 0.0f, 0.0f));
        ViewportCamera->SetForward(FVector(-1.0f, 0.0f, 0.0f));
        ViewportCamera->SetRight(FVector(0.0f, 1.0f, 0.0f));
        ViewportCamera->SetUp(FVector(0.0f, 0.0f, 1.0f));
        break;
        
    case EViewType::OrthoRight:
        if (ViewportCamera->GetCameraType() == ECameraType::ECT_Perspective)
        {
            SavedPerspectiveLocation = ViewportCamera->GetLocation();
            SavedPerspectiveRotation = ViewportCamera->GetRotation();
            SavedPerspectiveFarZ = ViewportCamera->GetFarZ();
        }

        ViewportCamera->SetCameraType(ECameraType::ECT_Orthographic);
        ViewportCamera->SetFarZ(5000.0f);
        ViewportCamera->SetOrthoZoom(UViewportManager::GetInstance().GetSharedOrthoZoom());
        // Right view: Looking right (+Y direction)
        ViewportCamera->SetLocation(FVector(0.0f, -100.0f, 0.0f));
        ViewportCamera->SetForward(FVector(0.0f, 1.0f, 0.0f));
        ViewportCamera->SetRight(FVector(-1.0f, 0.0f, 0.0f));
        ViewportCamera->SetUp(FVector(0.0f, 0.0f, 1.0f));
        break;
        
    case EViewType::OrthoLeft:
        if (ViewportCamera->GetCameraType() == ECameraType::ECT_Perspective)
        {
            SavedPerspectiveLocation = ViewportCamera->GetLocation();
            SavedPerspectiveRotation = ViewportCamera->GetRotation();
            SavedPerspectiveFarZ = ViewportCamera->GetFarZ();
        }

        ViewportCamera->SetCameraType(ECameraType::ECT_Orthographic);
        ViewportCamera->SetFarZ(5000.0f);
        ViewportCamera->SetOrthoZoom(UViewportManager::GetInstance().GetSharedOrthoZoom());
        // Left view: Looking left (-Y direction)
        ViewportCamera->SetLocation(FVector(0.0f, 100.0f, 0.0f));
        ViewportCamera->SetForward(FVector(0.0f, -1.0f, 0.0f));
        ViewportCamera->SetRight(FVector(1.0f, 0.0f, 0.0f));
        ViewportCamera->SetUp(FVector(0.0f, 0.0f, 1.0f));
        break;
    }
}

// TODO: This function is not called anywhere (dead code since Oct 20, 2025)
// Investigate original intention and either remove or integrate into rendering pipeline
void FViewportClient::Tick() const
{
    // FutureEngine 철학: Camera는 Viewport의 크기 정보를 받아야 함
    if (OwningViewport && ViewportCamera)
    {
        const D3D11_VIEWPORT ViewportRect = OwningViewport->GetRenderRect();
        ViewportCamera->Update(ViewportRect);
    }
}

// TODO: This function is not called anywhere (dead code since Oct 20, 2025)
// Consider using this or a new method to update camera before rendering
// Should handle both ActiveCamera (PIE/Game) and ViewportCamera (Editor)
void FViewportClient::Draw(const FViewport* InViewport) const
{
    if (!InViewport) { return; }

    const float Aspect = InViewport->GetAspect();
    ViewportCamera->SetAspect(Aspect);

    if (IsOrtho())
    {
        ViewportCamera->UpdateMatrixByOrth();
    }
    else
    {
        ViewportCamera->UpdateMatrixByPers();
    }
}

void FViewportClient::SetPlayerCameraManager(APlayerCameraManager* Manager)
{
    PlayerCameraManager = Manager;
}

void FViewportClient::PrepareCamera(const D3D11_VIEWPORT& InViewport)
{
    if (PlayerCameraManager)  // PIE/Game mode
    {
        // Update aspect ratio from viewport (marks camera as dirty)
        if (InViewport.Width > 0.f && InViewport.Height > 0.f)
        {
            PlayerCameraManager->SetDefaultAspectRatio(InViewport.Width / InViewport.Height);
        }
        // Camera will be updated automatically in GetCameraCachePOV() if dirty
    }
    else if (ViewportCamera)  // Editor mode
    {
        ViewportCamera->Update(InViewport);
    }
}

const FCameraConstants& FViewportClient::GetCameraConstants() const
{
    // PIE/Game mode: Use player camera manager
    if (PlayerCameraManager)
    {
        return PlayerCameraManager->GetCameraCachePOV().CameraConstants;
    }

    // Editor mode: Use editor camera
    if (ViewportCamera)
    {
        return ViewportCamera->GetCameraConstants();
    }

    // Fallback: return empty camera constants
    static const FCameraConstants EmptyCameraConstants;
    return EmptyCameraConstants;
}

FMinimalViewInfo FViewportClient::GetViewInfo() const
{
    // PIE/Game mode: Use player camera manager
    if (PlayerCameraManager)
    {
        return PlayerCameraManager->GetCameraCachePOV();
    }

    // Editor mode: Build FMinimalViewInfo from editor camera
    if (ViewportCamera)
    {
        FMinimalViewInfo ViewInfo;
        ViewInfo.Location = ViewportCamera->GetLocation();
        ViewInfo.Rotation = ViewportCamera->GetRotationQuat();
        ViewInfo.FOV = ViewportCamera->GetFovY();
        ViewInfo.AspectRatio = ViewportCamera->GetAspect();
        ViewInfo.NearClipPlane = ViewportCamera->GetNearZ();
        ViewInfo.FarClipPlane = ViewportCamera->GetFarZ();
        ViewInfo.ProjectionMode = (ViewportCamera->GetCameraType() == ECameraType::ECT_Orthographic)
            ? ECameraProjectionMode::Orthographic
            : ECameraProjectionMode::Perspective;
        ViewInfo.OrthoWidth = ViewportCamera->GetOrthoWidth();
        ViewInfo.CameraConstants = ViewportCamera->GetCameraConstants();
        return ViewInfo;
    }

    // Fallback: return default view info
    return FMinimalViewInfo();
}

const TArray<UPrimitiveComponent*>& FViewportClient::GetVisiblePrimitives() const
{
    return ViewFrustumCuller.GetRenderableObjects();
}

void FViewportClient::UpdateVisiblePrimitives(UWorld* InWorld)
{
    if (!InWorld || !InWorld->GetLevel())
    {
        return;
    }

    // Get camera constants from the appropriate source (PIE or Editor)
    const FCameraConstants& CameraConst = GetCameraConstants();

    // Perform frustum culling
    TArray<UPrimitiveComponent*> DynamicPrimitives = InWorld->GetLevel()->GetDynamicPrimitives();
    ViewFrustumCuller.Cull(
        InWorld->GetLevel()->GetStaticOctree(),
        DynamicPrimitives,
        CameraConst
    );
}
