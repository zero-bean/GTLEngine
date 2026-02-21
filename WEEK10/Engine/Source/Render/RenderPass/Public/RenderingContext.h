#pragma once
#include "Global/CameraTypes.h"

struct FSharedLightResources;

struct FRenderingContext
{
    FRenderingContext(){}

    // Constructor with FMinimalViewInfo (world-specific camera data)
    FRenderingContext(const FMinimalViewInfo& InViewInfo, EViewModeIndex InViewMode, uint64 InShowFlags, const D3D11_VIEWPORT& InViewport, const FVector2& InRenderTargetSize)
        : ViewInfo(InViewInfo), ViewProjConstants(&ViewInfo.CameraConstants), ViewMode(InViewMode), ShowFlags(InShowFlags), Viewport(InViewport), RenderTargetSize(InRenderTargetSize) {}

    // Complete view information (world-specific camera data)
    FMinimalViewInfo ViewInfo;

    const FCameraConstants* ViewProjConstants = nullptr;
    EViewModeIndex ViewMode;
    uint64 ShowFlags;
    D3D11_VIEWPORT Viewport;
    D3D11_VIEWPORT LocalViewport;
    FVector2 RenderTargetSize;

    TArray<class UPrimitiveComponent*> AllPrimitives;
    // Components By Render Pass
    TArray<class UStaticMeshComponent*> StaticMeshes;
	TArray<class USkeletalMeshComponent*> SkeletalMeshes;
    TArray<class UBillBoardComponent*> BillBoards;
    TArray<class UEditorIconComponent*> EditorIcons;
    TArray<class UTextComponent*> Texts;
    TArray<class UUUIDTextComponent*> UUIDs;
    TArray<class UDecalComponent*> Decals;
    TArray<class UPointLightComponent*> PointLights;
    TArray<class USpotLightComponent*> SpotLights;
    TArray<class UDirectionalLightComponent*> DirectionalLights;
    TArray<class UAmbientLightComponent*> AmbientLights;
    TArray<class UHeightFogComponent*> Fogs;

    /**
     * @brief FLightPass가 생성한 Light 리소스 참조
     * FLightPass::Execute()에서 설정되며, 같은 프레임 내에서만 유효합니다.
     * 소유권은 FLightPass에 있으며, 이 포인터는 읽기 전용 참조입니다.
     */
    const FSharedLightResources* SharedLightResources = nullptr;
};
