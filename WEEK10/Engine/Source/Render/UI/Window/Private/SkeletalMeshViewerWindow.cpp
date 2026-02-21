#include "pch.h"
#include "Render/UI/Window/Public/SkeletalMeshViewerWindow.h"
#include "ImGui/imgui_internal.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/SceneView.h"
#include "Render/Renderer/Public/SceneViewFamily.h"
#include "Render/Renderer/Public/SceneRenderer.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Core/Public/NewObject.h"
#include "Global/Quaternion.h"
#include "Editor/Public/BatchLines.h"
#include "Render/UI/Widget/Public/SkeletalMeshViewerToolbarWidget.h"
#include "Component/Public/AmbientLightComponent.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Actor/Public/AmbientLight.h"
#include "Actor/Public/DirectionalLight.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/Axis.h"
#include "Editor/Public/GizmoHelper.h"
#include "Render/HitProxy/Public/HitProxy.h"
#include "Manager/Input/Public/InputManager.h"
#include "Render/UI/Overlay/Public/D2DOverlayManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Texture/Public/Texture.h"

IMPLEMENT_CLASS(USkeletalMeshViewerWindow, UUIWindow)

/**
 * @brief SkeletalMeshViewerWindow 생성자
 * 독립적인 플로팅 윈도우로 설정
 */
USkeletalMeshViewerWindow::USkeletalMeshViewerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "SkeletalMeshViewer";
	Config.DefaultSize = ImVec2(1400, 800);
	Config.DefaultPosition = ImVec2(100, 100);
	Config.DockDirection = EUIDockDirection::None; // 독립적인 플로팅 윈도우
	Config.Priority = 100;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.InitialState = EUIWindowState::Hidden; // 기본적으로 숨김

	// 일반 윈도우 플래그
	Config.WindowFlags = ImGuiWindowFlags_None;

	SetConfig(Config);

	UE_LOG("SkeletalMeshViewerWindow: 생성자 호출됨");
}

/**
 * @brief 소멸자
 */
USkeletalMeshViewerWindow::~USkeletalMeshViewerWindow()
{
	Cleanup();
}

/**
 * @brief 초기화 함수 - 독립적인 뷰포트와 카메라 생성
 */
void USkeletalMeshViewerWindow::Initialize()
{
	if (bIsInitialized)
	{
		UE_LOG_WARNING("SkeletalMeshViewerWindow: 이미 초기화됨 - 중복 초기화 방지");
		return;
	}

	// Viewport 생성
	ViewerViewport = new FViewport();
	ViewerViewport->SetSize(ViewerWidth, ViewerHeight);
	ViewerViewport->SetInitialPosition(0, 0);

	// ViewportClient 생성
	ViewerViewportClient = new FViewportClient();
	ViewerViewportClient->SetOwningViewport(ViewerViewport);
	ViewerViewportClient->SetViewType(EViewType::Perspective);
	ViewerViewportClient->SetViewMode(EViewModeIndex::VMI_BlinnPhong);

	// 뷰어의 카메라는 ImGui 마우스 델타를 직접 사용하므로 Camera::UpdateInput() 비활성화
	if (ViewerViewportClient->GetCamera())
	{
		ViewerViewportClient->GetCamera()->SetInputEnabled(false);
		// 카메라를 -Y 위치에 두고 +Y 방향(스켈레탈 메쉬 정면)을 보도록 설정
		ViewerViewportClient->GetCamera()->SetLocation(FVector(0.0f, -5.0f, 4.0f));
		ViewerViewportClient->GetCamera()->SetRotation(FVector(0.0f, 0.0f, 90.0f));
		UE_LOG("SkeletalMeshViewerWindow: 카메라 위치 설정 완료: (0, -5, 4), Roll: 90");
	}

	ViewerViewport->SetViewportClient(ViewerViewportClient);

	// 렌더 타겟 생성
	CreateRenderTarget(ViewerWidth, ViewerHeight);

	// BatchLines 생성 및 Grid 초기화
	ViewerBatchLines = new UBatchLines();
	if (ViewerBatchLines)
	{
		ViewerBatchLines->UpdateUGridVertices(GridCellSize); // 그리드 생성
		ViewerBatchLines->UpdateVertexBuffer();
	}

	// 툴바 위젯 생성 및 초기화
	ToolbarWidget = new USkeletalMeshViewerToolbarWidget();
	if (ToolbarWidget)
	{
		ToolbarWidget->Initialize();
		ToolbarWidget->SetViewportClient(ViewerViewportClient);
		ToolbarWidget->SetOwningWindow(this);
	}

	// Preview World 생성 (EWorldType::EditorPreview)
	PreviewWorld = new UWorld(EWorldType::EditorPreview);
	if (PreviewWorld)
	{
		// 새 레벨 생성
		PreviewWorld->CreateNewLevel(FName("PreviewLevel"));

		// Preview World BeginPlay 호출 (중요!)
		PreviewWorld->BeginPlay();

		UE_LOG("SkeletalMeshViewerWindow: Preview World 생성 및 BeginPlay 완료");

		// Ambient Light Actor 스폰
		PreviewAmbientLightActor = PreviewWorld->SpawnActor(AAmbientLight::StaticClass());
		if (PreviewAmbientLightActor)
		{
			if (AAmbientLight* AmbientLight = Cast<AAmbientLight>(PreviewAmbientLightActor))
			{
				if (UAmbientLightComponent* LightComp = AmbientLight->GetAmbientLightComponent())
				{
					LightComp->SetLightColor(FVector(0.3f, 0.3f, 0.3f));
					LightComp->SetIntensity(1.0f);
				}
			}
			UE_LOG("SkeletalMeshViewerWindow: Ambient Light Actor 스폰 완료");
		}

		// Directional Light Actor 스폰
		PreviewDirectionalLightActor = PreviewWorld->SpawnActor(ADirectionalLight::StaticClass());
		if (PreviewDirectionalLightActor)
		{
			PreviewDirectionalLightActor->SetActorLocation(FVector(0, 0, 100));
			FRotator Rotator(-45.0f, 45.0f, 0.0f);
			PreviewDirectionalLightActor->SetActorRotation(Rotator.Quaternion());

			if (ADirectionalLight* DirLight = Cast<ADirectionalLight>(PreviewDirectionalLightActor))
			{
				if (UDirectionalLightComponent* LightComp = DirLight->GetDirectionalLightComponent())
				{
					LightComp->SetLightColor(FVector(1.0f, 1.0f, 1.0f));
					LightComp->SetIntensity(0.8f);
				}
			}
			UE_LOG("SkeletalMeshViewerWindow: Directional Light Actor 스폰 완료");
		}

		PreviewSkeletalMeshActor = PreviewWorld->SpawnActor(AActor::StaticClass());
	}

	// Gizmo 생성
	ViewerGizmo = new UGizmo();
	if (ViewerGizmo)
	{
		// 뷰어의 Gizmo는 ViewportManager 대신 자체 툴바 설정 사용
		ViewerGizmo->SetUseCustomLocationSnap(true);
		ViewerGizmo->SetUseCustomRotationSnap(true);
		ViewerGizmo->SetUseCustomScaleSnap(true);
		UE_LOG("SkeletalMeshViewerWindow: Gizmo 생성 및 초기 선택 완료");
	}

	// ObjectPicker 생성
	ViewerObjectPicker = new UObjectPicker();
	if (ViewerObjectPicker)
	{
		UE_LOG("SkeletalMeshViewerWindow: ObjectPicker 생성 완료");
	}

	// Lock/Unlock 아이콘 로드
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	std::string IconBasePath = "Engine/Asset/Icon/";
	LockIcon = AssetManager.LoadTexture((IconBasePath + "Lock.png").data());
	UnlockIcon = AssetManager.LoadTexture((IconBasePath + "Unlock.png").data());
	if (LockIcon && UnlockIcon)
	{
		UE_LOG("SkeletalMeshViewerWindow: Lock/Unlock 아이콘 로드 성공");
	}
	else
	{
		UE_LOG_WARNING("SkeletalMeshViewerWindow: Lock/Unlock 아이콘 로드 실패");
	}

	bIsInitialized = true;
	bIsCleanedUp = false;

	SetWindowState(EUIWindowState::Hidden);

	UE_LOG("SkeletalMeshViewerWindow: 독립적인 뷰포트 및 Preview World 초기화 완료");
}

/**
 * @brief 정리 함수 - 뷰포트와 카메라 해제
 */
void USkeletalMeshViewerWindow::Cleanup()
{
	if (bIsCleanedUp)
	{
		UE_LOG_WARNING("SkeletalMeshViewerWindow: 이미 정리됨 - 중복 정리 방지");
		return;
	}

	// 렌더 타겟 해제
	ReleaseRenderTarget();

	// BatchLines 삭제
	if (ViewerBatchLines)
	{
		SafeDelete(ViewerBatchLines);
	}

	// 툴바 위젯 삭제
	if (ToolbarWidget)
	{
		delete ToolbarWidget;
		ToolbarWidget = nullptr;
	}

	// Gizmo 삭제
	if (ViewerGizmo)
	{
		delete ViewerGizmo;
		ViewerGizmo = nullptr;
	}

	// ObjectPicker 삭제
	if (ViewerObjectPicker)
	{
		delete ViewerObjectPicker;
		ViewerObjectPicker = nullptr;
	}

	// Preview World 정리 (Actor들은 World 소멸 시 자동으로 정리됨)
	if (PreviewWorld)
	{
		// Actor 참조 초기화
		PreviewSkeletalMeshActor = nullptr;
		PreviewAmbientLightActor = nullptr;
		PreviewDirectionalLightActor = nullptr;

		// World 종료 및 삭제
		PreviewWorld->EndPlay();
		delete PreviewWorld;
		PreviewWorld = nullptr;

		UE_LOG("SkeletalMeshViewerWindow: Preview World 정리 완료");
	}

	// ViewportClient와 Viewport의 연결을 먼저 끊음
	if (ViewerViewportClient && ViewerViewport)
	{
		ViewerViewportClient->SetOwningViewport(nullptr);
		ViewerViewport->SetViewportClient(nullptr);
	}

	// ViewportClient 삭제
	if (ViewerViewportClient)
	{
		SafeDelete(ViewerViewportClient);
	}

	// 그 다음 Viewport 삭제
	if (ViewerViewport)
	{
		SafeDelete(ViewerViewport);
	}

	bIsCleanedUp = true;
	bIsInitialized = false;

	UE_LOG("SkeletalMeshViewerWindow: 정리 완료");
}

void USkeletalMeshViewerWindow::OpenViewer(USkeletalMeshComponent* InSkeletalMeshComponent)
{
	assert(InSkeletalMeshComponent);

	// 이전에 등록된 SkeletalMeshComponent 제거
	if (SkeletalMeshComponent && PreviewSkeletalMeshActor)
	{
		PreviewSkeletalMeshActor->RemoveComponent(SkeletalMeshComponent);
	}

	SkeletalMeshComponent = nullptr;
	SelectedBoneIndex = INDEX_NONE;
	bShowAllBones = false;

	// SkeletalMeshComponent 설정
	SetWindowState(EUIWindowState::Visible);
	DuplicateSkeletalMeshComponent(InSkeletalMeshComponent);
	assert(SkeletalMeshComponent);

	// SkeletalMesh 유효성 검사 및 변수 할당
	USkeletalMesh* SkeletalMesh = nullptr;
	FReferenceSkeleton RefSkeleton;
	int32 NumBones = 0;
	bool bValid = CheckSkeletalValidity(SkeletalMesh, RefSkeleton, NumBones, false);

	// TempBoneSpaceTransforms 초기화
	if (bValid)
	{
		const int32 NumBones = RefSkeleton.GetRawBoneNum();
		TempBoneSpaceTransforms.Reset(NumBones);
		for (int32 i = 0; i < NumBones; ++i)
		{
			TempBoneSpaceTransforms[i] = SkeletalMeshComponent->GetBoneTransformLocal(i);
		}
	}

	// SkeletalMesh Actor 스폰
	if (PreviewWorld)
	{
		if (PreviewSkeletalMeshActor)
		{
			PreviewSkeletalMeshActor->SetActorLocation(FVector(0, 0, 0));
			PreviewSkeletalMeshActor->SetActorRotation(FQuaternion::Identity());

			SkeletalMeshComponent->SetWorldLocation(FVector(0, 0, 0));
			SkeletalMeshComponent->SetWorldRotation(FQuaternion::Identity());
			SkeletalMeshComponent->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));

			PreviewSkeletalMeshActor->RegisterComponent(SkeletalMeshComponent);

			UE_LOG("SkeletalMeshViewerWindow: 편집용 Actor 스폰 완료");
		}

		// 초기에는 아무것도 선택하지 않음
		SelectedBoneIndex = INDEX_NONE;
	}

	// 카메라를 초기 위치로 리셋
	if (ViewerViewportClient && ViewerViewportClient->GetCamera())
	{
		ViewerViewportClient->GetCamera()->SetLocation(FVector(0.0f, -5.0f, 4.0f));
		ViewerViewportClient->GetCamera()->SetRotation(FVector(0.0f, 0.0f, 90.0f));
		UE_LOG("SkeletalMeshViewerWindow: OpenViewer - 카메라 초기 위치로 리셋");
	}
}
bool USkeletalMeshViewerWindow::OnWindowClose()
{
	if (SkeletalMeshComponent && PreviewSkeletalMeshActor)
	{
		PreviewSkeletalMeshActor->RemoveComponent(SkeletalMeshComponent);
	}
	SkeletalMeshComponent = nullptr;

	return true;
}

void USkeletalMeshViewerWindow::DuplicateSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
{
	if(SkeletalMeshComponent = Cast<USkeletalMeshComponent>(InSkeletalMeshComponent->Duplicate()))
	{
		OriginalSkeletalMeshComponent = InSkeletalMeshComponent;
		UE_LOG("SkeletalMeshViewerWindow: SkeletalMeshComponent 복제 완료");
	}
	else
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: SkeletalMeshComponent 복제 실패");
	}
}

/**
 * @brief 렌더 타겟 생성
 */
void USkeletalMeshViewerWindow::CreateRenderTarget(uint32 Width, uint32 Height)
{
	// 기존 렌더 타겟 해제
	ReleaseRenderTarget();

	if (Width == 0 || Height == 0)
	{
		UE_LOG_WARNING("SkeletalMeshViewerWindow: 잘못된 렌더 타겟 크기 (%d x %d)", Width, Height);
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();

	// 렌더 타겟 텍스처 생성 (D2D 호환을 위해 B8G8R8A8 포맷 사용)
	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // D2D 호환을 위해 UNORM 사용 (TYPELESS는 D2D와 호환 안됨)
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	HRESULT hr = Device->CreateTexture2D(&TextureDesc, nullptr, &ViewerRenderTargetTexture);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 렌더 타겟 텍스처 생성 실패");
		return;
	}

	// 렌더 타겟 뷰 생성 (D2D 호환을 위해 B8G8R8A8_UNORM 사용)
	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // 텍스처와 동일한 포맷
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;

	hr = Device->CreateRenderTargetView(ViewerRenderTargetTexture, &RTVDesc, &ViewerRenderTargetView);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 렌더 타겟 뷰 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// 셰이더 리소스 뷰 생성 (D2D 호환을 위해 B8G8R8A8_UNORM 사용)
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // 텍스처와 동일한 포맷
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = Device->CreateShaderResourceView(ViewerRenderTargetTexture, &SRVDesc, &ViewerShaderResourceView);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 셰이더 리소스 뷰 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// 깊이 스텐실 텍스처 생성
	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = Width;
	DepthDesc.Height = Height;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.SampleDesc.Quality = 0;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthDesc.CPUAccessFlags = 0;
	DepthDesc.MiscFlags = 0;

	hr = Device->CreateTexture2D(&DepthDesc, nullptr, &ViewerDepthStencilTexture);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 깊이 스텐실 텍스처 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// 깊이 스텐실 뷰 생성
	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
	DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSVDesc.Texture2D.MipSlice = 0;

	hr = Device->CreateDepthStencilView(ViewerDepthStencilTexture, &DSVDesc, &ViewerDepthStencilView);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 깊이 스텐실 뷰 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// HitProxy 렌더 타겟 텍스처 생성 (R8G8B8A8)
	D3D11_TEXTURE2D_DESC HitProxyTextureDesc = {};
	HitProxyTextureDesc.Width = Width;
	HitProxyTextureDesc.Height = Height;
	HitProxyTextureDesc.MipLevels = 1;
	HitProxyTextureDesc.ArraySize = 1;
	HitProxyTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	HitProxyTextureDesc.SampleDesc.Count = 1;
	HitProxyTextureDesc.SampleDesc.Quality = 0;
	HitProxyTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	HitProxyTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	HitProxyTextureDesc.CPUAccessFlags = 0;
	HitProxyTextureDesc.MiscFlags = 0;

	hr = Device->CreateTexture2D(&HitProxyTextureDesc, nullptr, &ViewerHitProxyTexture);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: HitProxy 텍스처 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// HitProxy 렌더 타겟 뷰 생성
	D3D11_RENDER_TARGET_VIEW_DESC HitProxyRTVDesc = {};
	HitProxyRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	HitProxyRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	HitProxyRTVDesc.Texture2D.MipSlice = 0;

	hr = Device->CreateRenderTargetView(ViewerHitProxyTexture, &HitProxyRTVDesc, &ViewerHitProxyRTV);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: HitProxy RTV 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// HitProxy 셰이더 리소스 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC HitProxySRVDesc = {};
	HitProxySRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	HitProxySRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	HitProxySRVDesc.Texture2D.MostDetailedMip = 0;
	HitProxySRVDesc.Texture2D.MipLevels = 1;

	hr = Device->CreateShaderResourceView(ViewerHitProxyTexture, &HitProxySRVDesc, &ViewerHitProxySRV);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: HitProxy SRV 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// D2D 렌더 타겟 생성 (오버레이용)
	ID2D1Factory* D2DFactory = Renderer.GetDeviceResources()->GetD2DFactory();
	if (D2DFactory)
	{
		// DXGI Surface 가져오기
		IDXGISurface* DXGISurface = nullptr;
		hr = ViewerRenderTargetTexture->QueryInterface(__uuidof(IDXGISurface), (void**)&DXGISurface);
		if (SUCCEEDED(hr))
		{
			// D2D RenderTarget 속성 설정 (메인과 동일하게 B8G8R8A8 사용)
			D2D1_RENDER_TARGET_PROPERTIES Props = D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			);

			hr = D2DFactory->CreateDxgiSurfaceRenderTarget(DXGISurface, &Props, &ViewerD2DRenderTarget);
			if (SUCCEEDED(hr))
			{
				UE_LOG("SkeletalMeshViewerWindow: D2D RenderTarget 생성 성공");
			}
			else
			{
				UE_LOG_WARNING("SkeletalMeshViewerWindow: D2D RenderTarget 생성 실패 (HRESULT: 0x%08X)", hr);
			}

			DXGISurface->Release();
		}
		else
		{
			UE_LOG_WARNING("SkeletalMeshViewerWindow: DXGI Surface 가져오기 실패 (HRESULT: 0x%08X)", hr);
		}
	}

	ViewerWidth = Width;
	ViewerHeight = Height;

	UE_LOG("SkeletalMeshViewerWindow: 렌더 타겟 생성 완료 (%d x %d)", Width, Height);
}

/**
 * @brief 렌더 타겟 해제
 */
void USkeletalMeshViewerWindow::ReleaseRenderTarget()
{
	SafeRelease(ViewerD2DRenderTarget);
	SafeRelease(ViewerHitProxySRV);
	SafeRelease(ViewerHitProxyRTV);
	SafeRelease(ViewerHitProxyTexture);
	SafeRelease(ViewerDepthStencilView);
	SafeRelease(ViewerDepthStencilTexture);
	SafeRelease(ViewerShaderResourceView);
	SafeRelease(ViewerRenderTargetView);
	SafeRelease(ViewerRenderTargetTexture);
}

/**
 * @brief 윈도우 렌더링 전 호출
 */
void USkeletalMeshViewerWindow::OnPreRenderWindow(float MenuBarOffset)
{
	// 부모 클래스의 PreRender 호출
	UUIWindow::OnPreRenderWindow(MenuBarOffset);
}

/**
 * @brief 윈도우 렌더링 후 호출 - 여기서 실제 레이아웃을 그림
 */
void USkeletalMeshViewerWindow::OnPostRenderWindow()
{
	RenderLayout();

	// 부모 클래스의 PostRender 호출
	UUIWindow::OnPostRenderWindow();
}

/**
 * @brief 3-패널 레이아웃 렌더링
 * ImGui Child 영역을 사용하여 화면을 3개로 분할하고 Splitter로 크기 조절 가능
 */
void USkeletalMeshViewerWindow::RenderLayout()
{
	const ImVec2 ContentRegion = ImGui::GetContentRegionAvail();
	const float TotalWidth = ContentRegion.x;
	const float PanelHeight = ContentRegion.y;

	// 중앙 패널은 남은 공간 사용
	const float CenterPanelWidthRatio = 1.0f - LeftPanelWidthRatio - RightPanelWidthRatio;

	const float LeftPanelWidth = TotalWidth * LeftPanelWidthRatio;
	const float CenterPanelWidth = TotalWidth * CenterPanelWidthRatio;
	const float RightPanelWidth = TotalWidth * RightPanelWidthRatio;


	// SkeletalMesh 유효성 검사 및 변수 할당
	USkeletalMesh* SkeletalMesh = nullptr;
	FReferenceSkeleton RefSkeleton;
	int32 NumBones = 0;
	bool bValid = CheckSkeletalValidity(SkeletalMesh, RefSkeleton, NumBones, false);

	// SkeletalMeshComponent에 임시 본 트랜스폼 적용
	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->RefreshBoneTransforms();
		SkeletalMeshComponent->UpdateSkinnedVertices();
	}

	// 본 피라미드 정점들 업데이트
	if (bShowAllBones)
	{
		ViewerBatchLines->UpdateAllBonePyramidVertices(SkeletalMeshComponent, SelectedBoneIndex);
	}
	else
	{
		ViewerBatchLines->UpdateBonePyramidVertices(SkeletalMeshComponent, SelectedBoneIndex);
	}
	ViewerBatchLines->UpdateVertexBuffer();

	// === 좌측 패널: Skeleton Tree ===
	if (ImGui::BeginChild("SkeletonTreePanel", ImVec2(LeftPanelWidth - SplitterWidth * 0.5f, PanelHeight), true))
	{
		RenderSkeletonTreePanel(SkeletalMesh, RefSkeleton, NumBones);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// === 첫 번째 Splitter (좌측-중앙 사이) ===
	RenderVerticalSplitter("##LeftCenterSplitter", LeftPanelWidthRatio, MinPanelRatio, MaxPanelRatio);

	ImGui::SameLine();

	// === 중앙 패널: 3D Viewport ===
	if (ImGui::BeginChild("3DViewportPanel", ImVec2(CenterPanelWidth - SplitterWidth, PanelHeight), true, ImGuiWindowFlags_MenuBar))
	{
		Render3DViewportPanel();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// === 두 번째 Splitter (중앙-우측 사이) ===
	RenderVerticalSplitter("##CenterRightSplitter", RightPanelWidthRatio, MinPanelRatio, MaxPanelRatio, true);

	ImGui::SameLine();

	// === 우측 패널: Edit Tools ===
	if (ImGui::BeginChild("EditToolsPanel", ImVec2(0, PanelHeight), true))
	{
		RenderEditToolsPanel(SkeletalMesh, RefSkeleton, NumBones);
	}
	ImGui::EndChild();
}

/**
 * @brief 좌측 패널: Skeleton Tree (Placeholder)
 */
void USkeletalMeshViewerWindow::RenderSkeletonTreePanel(const USkeletalMesh* InSkeletalMesh, const FReferenceSkeleton& InRefSkeleton, const int32 InNumBones)
{
	/*assert(InSkeletalMesh);
	assert(InNumBones > 0);*/
	bool bValid = CheckSkeletalValidity(const_cast<USkeletalMesh*&>(InSkeletalMesh), const_cast<FReferenceSkeleton&>(InRefSkeleton), const_cast<int32&>(InNumBones), true);
	if(bValid == false)
	{
		return;
	}

	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Skeleton Tree");
	ImGui::Separator();
	ImGui::Spacing();

	// 본 정보 표시
	ImGui::Text("Total Bones: %d", InNumBones);
	ImGui::SameLine();
	ImGui::Checkbox("Show All Bone Names", &bShowAllBoneNames);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// 검색 기능
	static char SearchBuffer[128] = "";
	ImGui::SetNextItemWidth(-1);
	ImGui::InputTextWithHint("##BoneSearch", "Search bones...", SearchBuffer, IM_ARRAYSIZE(SearchBuffer));
	ImGui::Spacing();

	// 스크롤 가능한 영역
	if (ImGui::BeginChild("BoneTreeScroll", ImVec2(0, 0), false))
	{
		const TArray<FMeshBoneInfo>& BoneInfoArray = InRefSkeleton.GetRawRefBoneInfo();
		const TArray<FTransform>& BonePoseArray = InRefSkeleton.GetRawRefBonePose();

		// 검색 필터링
		FString SearchStr = SearchBuffer;
		bool bHasSearchFilter = !SearchStr.empty();

		// 루트 본들을 찾아서 재귀적으로 렌더링
		for (int32 BoneIndex = 0; BoneIndex < InNumBones; ++BoneIndex)
		{
			const FMeshBoneInfo& BoneInfo = BoneInfoArray[BoneIndex];

			// 루트 본만 처리 (부모가 없는 본)
			if (BoneInfo.ParentIndex == INDEX_NONE)
			{
				RenderBoneTreeNode(BoneIndex, InRefSkeleton, SearchStr, bHasSearchFilter);
			}
		}
	}
	ImGui::EndChild();
}

void USkeletalMeshViewerWindow::RenderBoneTreeNode(int32 BoneIndex, const FReferenceSkeleton& RefSkeleton, const FString& SearchFilter, bool bHasSearchFilter)
{
	const TArray<FMeshBoneInfo>& BoneInfoArray = RefSkeleton.GetRawRefBoneInfo();
	//const TArray<FTransform>& BonePoseArray = RefSkeleton.GetRawRefBonePose();
	const FMeshBoneInfo& BoneInfo = BoneInfoArray[BoneIndex];

	FString BoneName = BoneInfo.Name.ToString();

	// 자식 본들 찾기
	TArray<int32> ChildBoneIndices;
	for (int32 ChildIndex = 0; ChildIndex < BoneInfoArray.Num(); ++ChildIndex)
	{
		if (BoneInfoArray[ChildIndex].ParentIndex == BoneIndex)
		{
			ChildBoneIndices.Add(ChildIndex);
		}
	}

	// 트리 노드 플래그 설정
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	// 아래 조건 만족 시, 모든 노드를 기본으로 열림
	if (bHasSearchFilter || bShowAllBoneNames)
	{
		NodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
	}

	// 자식이 없으면 Leaf 플래그 추가
	if (ChildBoneIndices.IsEmpty())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	// 선택 상태 표시
	if (SelectedBoneIndex == BoneIndex)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	// 검색 필터에 매칭되면 하이라이트
	bool bIsMatching = false;
	if (bHasSearchFilter && BoneName.Contains(SearchFilter))
	{
		bIsMatching = true;
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.4f, 1.0f)); // 노란색
	}

	// 트리 노드 렌더링
	bool bNodeOpen = ImGui::TreeNodeEx(
		(void*)(intptr_t)BoneIndex,
		NodeFlags,
		"[%d] %s",
		BoneIndex,
		BoneName.c_str()
	);

	if (bIsMatching)
	{
		ImGui::PopStyleColor();
	}

	// 툴팁: 본 정보 표시
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("Parent Index: %d", BoneInfo.ParentIndex);

		if (BoneInfo.ParentIndex != INDEX_NONE)
		{
			ImGui::Text("Parent Name: %s", BoneInfoArray[BoneInfo.ParentIndex].Name.ToString().c_str());
		}
		else
		{
			ImGui::Text("Parent Name: (Root)");
		}

		ImGui::EndTooltip();
	}

	// 클릭 시 본 선택
	if (ImGui::IsItemClicked())
	{
		SelectedBoneIndex = BoneIndex;

		// 기즈모 위치를 선택된 본의 월드 위치로 설정
		if (ViewerGizmo && SkeletalMeshComponent)
		{
			FVector BoneWorldLocation = GetBoneWorldLocation(BoneIndex);
			ViewerGizmo->SetFixedLocation(BoneWorldLocation);
		}
	}

	// 자식 본들 재귀적으로 렌더링
	if (bNodeOpen && !ChildBoneIndices.IsEmpty())
	{
		for (int32 ChildBoneIndex : ChildBoneIndices)
		{
			RenderBoneTreeNode(ChildBoneIndex, RefSkeleton, SearchFilter, bHasSearchFilter);
		}
		ImGui::TreePop();
	}
}

/**
 * @brief 툴바 업데이트 및 기즈모 동기화 처리
 */
void USkeletalMeshViewerWindow::UpdateToolbarAndGizmoSync()
{
	if (!ToolbarWidget) return;

	ToolbarWidget->Update();
	ToolbarWidget->RenderWidget();

	// 툴바와 ViewerGizmo 간 양방향 동기화
	if (ViewerGizmo)
	{
		static EGizmoMode PreToolbarGizmoMode = ToolbarWidget->GetCurrentGizmoMode();
		static EGizmoMode PreViewerGizmoMode = ViewerGizmo->GetGizmoMode();

		EGizmoMode ToolbarGizmoMode = ToolbarWidget->GetCurrentGizmoMode();
		EGizmoMode ViewerGizmoMode = ViewerGizmo->GetGizmoMode();

		if (PreToolbarGizmoMode != ToolbarGizmoMode)
		{
			ToolbarWidget->SetCurrentGizmoMode(ToolbarGizmoMode);
			ViewerGizmo->SetGizmoMode(ToolbarGizmoMode);
			PreToolbarGizmoMode = ToolbarGizmoMode;
			PreViewerGizmoMode = ViewerGizmoMode;
		}
		else if (PreViewerGizmoMode != ViewerGizmoMode)
		{
			ToolbarWidget->SetCurrentGizmoMode(ViewerGizmoMode);
			ViewerGizmo->SetGizmoMode(ViewerGizmoMode);
			PreToolbarGizmoMode = ToolbarGizmoMode;
			PreViewerGizmoMode = ViewerGizmoMode;
		}
	}
}

/**
 * @brief 뷰포트 크기 변경 시 렌더 타겟 업데이트
 */
void USkeletalMeshViewerWindow::UpdateViewportRenderTarget(uint32 NewWidth, uint32 NewHeight)
{
	if (NewWidth > 0 && NewHeight > 0 && (NewWidth != ViewerWidth || NewHeight != ViewerHeight))
	{
		CreateRenderTarget(NewWidth, NewHeight);
		if (ViewerViewport)
		{
			ViewerViewport->SetSize(NewWidth, NewHeight);
		}
	}
}

/**
 * @brief 뷰포트 렌더 타겟에 3D 씬 렌더링
 */
void USkeletalMeshViewerWindow::RenderToViewportTexture()
{
	if (!ViewerRenderTargetView || !ViewerDepthStencilView || !ViewerViewportClient) return;

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* Context = Renderer.GetDeviceContext();

	// 렌더 타겟 클리어
	const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	Context->ClearRenderTargetView(ViewerRenderTargetView, ClearColor);
	Context->ClearDepthStencilView(ViewerDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 렌더 타겟 설정
	Context->OMSetRenderTargets(1, &ViewerRenderTargetView, ViewerDepthStencilView);

	// 뷰포트 설정
	D3D11_VIEWPORT D3DViewport = {};
	D3DViewport.TopLeftX = 0.0f;
	D3DViewport.TopLeftY = 0.0f;
	D3DViewport.Width = static_cast<float>(ViewerWidth);
	D3DViewport.Height = static_cast<float>(ViewerHeight);
	D3DViewport.MinDepth = 0.0f;
	D3DViewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &D3DViewport);

	// Viewport의 RenderRect 설정
	ViewerViewport->SetRenderRect(D3DViewport);

	// Preview World 렌더링
	if (PreviewWorld)
	{
		ULevel* Level = PreviewWorld->GetLevel();
		if (Level)
		{
			const TArray<AActor*>& Actors = Level->GetLevelActors();
			static bool bLoggedOnce = false;
			if (!bLoggedOnce)
			{
				UE_LOG("SkeletalMeshViewerWindow: Preview World has %d actors", Actors.Num());
				for (AActor* Actor : Actors)
				{
					if (Actor)
					{
						FString ClassName = Actor->GetClass()->GetName().ToString();
						UE_LOG("  - Actor: %s", ClassName.c_str());
					}
				}
				bLoggedOnce = true;
			}
		}

		ViewerViewportClient->PrepareCamera(D3DViewport);
		ViewerViewportClient->UpdateVisiblePrimitives(PreviewWorld);

		const TArray<UPrimitiveComponent*>& VisiblePrims = ViewerViewportClient->GetVisiblePrimitives();
		static bool bLoggedVisibles = false;
		if (!bLoggedVisibles)
		{
			UE_LOG("SkeletalMeshViewerWindow: Visible primitives count: %d", VisiblePrims.Num());
			bLoggedVisibles = true;
		}

		UCamera* Camera = ViewerViewportClient->GetCamera();
		if (Camera)
		{
			const FCameraConstants& CameraConst = Camera->GetCameraConstants();

			FSceneView* View = new FSceneView();
			View->InitializeWithMatrices(
				CameraConst.View,
				CameraConst.Projection,
				Camera->GetLocation(),
				Camera->GetRotation(),
				ViewerViewport,
				PreviewWorld,
				ViewerViewportClient->GetViewMode(),
				Camera->GetFovY(),
				CameraConst.NearClip,
				CameraConst.FarClip
			);

			View->SetViewport(ViewerViewport);
			Renderer.RenderPreviewWorld(PreviewWorld, View, ViewerRenderTargetView, ViewerDepthStencilView, D3DViewport);

			// Grid 렌더링
			if (ViewerBatchLines)
			{
				ID3D11Buffer* ConstantBufferViewProj = Renderer.GetConstantBufferViewProj();
				UPipeline* Pipeline = Renderer.GetPipeline();

				if (ConstantBufferViewProj && Pipeline)
				{
					FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewProj, CameraConst);
					Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);
				}

				ViewerBatchLines->Render();
			}

			// Gizmo 렌더링 (본 선택 시)
			if (ViewerGizmo && SelectedBoneIndex != INDEX_NONE)
			{
				// 본 선택 시: Local 모드 또는 Scale 모드일 때 선택된 본 자체의 월드 회전을 기즈모에 설정
				// 드래그 중이 아닐 때만 업데이트 (드래그 시작 회전값을 보존하기 위해)
				if (SelectedBoneIndex != INDEX_NONE && SkeletalMeshComponent && !ViewerGizmo->IsDragging())
				{
					FQuaternion GizmoRotation = FQuaternion::Identity();  // World 모드: Identity

					// Local 모드 또는 Scale 모드(항상 로컬)일 때 선택된 본 자체의 회전 사용
					if (!ViewerGizmo->IsWorldMode() || ViewerGizmo->GetGizmoMode() == EGizmoMode::Scale)
					{
						// 선택된 본 자체의 월드 회전 (본의 로컬 좌표계)
						GizmoRotation = GetBoneWorldRotation(SelectedBoneIndex);
					}

					// 기즈모의 DragStartActorRotationQuat 설정
					// World 모드: Identity (단, Scale 모드는 예외)
					// Local 모드: 선택된 본의 회전
					// Scale 모드: 항상 선택된 본의 회전
					ViewerGizmo->SetDragStartActorRotationQuat(GizmoRotation);
				}

				if (ToolbarWidget)
				{
					ViewerGizmo->SetCustomLocationSnapEnabled(ToolbarWidget->IsLocationSnapEnabled());
					ViewerGizmo->SetCustomLocationSnapValue(ToolbarWidget->GetLocationSnapValue());
					ViewerGizmo->SetCustomRotationSnapEnabled(ToolbarWidget->IsRotationSnapEnabled());
					ViewerGizmo->SetCustomRotationSnapAngle(ToolbarWidget->GetRotationSnapAngle());
					ViewerGizmo->SetCustomScaleSnapEnabled(ToolbarWidget->IsScaleSnapEnabled());
					ViewerGizmo->SetCustomScaleSnapValue(ToolbarWidget->GetScaleSnapValue());
				}

				ViewerGizmo->UpdateScale(Camera, D3DViewport);
				ViewerGizmo->RenderGizmo(Camera, D3DViewport);
			}

			delete View;
		}

		// D2D 오버레이 렌더링
		if (ViewerD2DRenderTarget && Camera)
		{
			FD2DOverlayManager& OverlayManager = FD2DOverlayManager::GetInstance();
			OverlayManager.BeginCollect(Camera, D3DViewport);

			if (ViewerGizmo && ToolbarWidget)
			{
				const bool bSnapEnabled = ToolbarWidget->IsRotationSnapEnabled();
				const float SnapAngle = ToolbarWidget->GetRotationSnapAngle();
				ViewerGizmo->CollectRotationAngleOverlay(OverlayManager, Camera, D3DViewport, false, bSnapEnabled, SnapAngle);
			}

			FAxis::CollectDrawCommands(OverlayManager, Camera, D3DViewport);

			static bool bLoggedD2DRender = false;
			if (!bLoggedD2DRender)
			{
				UE_LOG("SkeletalMeshViewerWindow: D2D 오버레이 렌더링 (RenderTarget: %p, Viewport: %.0fx%.0f)",
					ViewerD2DRenderTarget, D3DViewport.Width, D3DViewport.Height);
				bLoggedD2DRender = true;
			}
			OverlayManager.FlushAndRender(ViewerD2DRenderTarget);
		}
	}

	// 렌더 타겟 복구
	ID3D11RenderTargetView* MainRTV = Renderer.GetDeviceResources()->GetBackBufferRTV();
	ID3D11DepthStencilView* MainDSV = Renderer.GetDeviceResources()->GetDepthBufferDSV();
	Context->OMSetRenderTargets(1, &MainRTV, MainDSV);

	const D3D11_VIEWPORT& MainViewport = Renderer.GetDeviceResources()->GetViewportInfo();
	Context->RSSetViewports(1, &MainViewport);
}

/**
 * @brief 뷰포트 이미지 표시 및 정보 오버레이 렌더링
 */
void USkeletalMeshViewerWindow::DisplayViewportImage(const ImVec2& ViewportSize)
{
	if (!ViewerShaderResourceView)
	{
		const char* PlaceholderText = "Render Target Not Available";
		const ImVec2 TextSize = ImGui::CalcTextSize(PlaceholderText);
		ImGui::SetCursorPos(ImVec2((ViewportSize.x - TextSize.x) * 0.5f, ViewportSize.y * 0.5f));
		ImGui::TextDisabled("%s", PlaceholderText);
		return;
	}

	ImTextureID TextureID = reinterpret_cast<ImTextureID>(ViewerShaderResourceView);
	ImVec2 CursorPos = ImGui::GetCursorScreenPos();

	ImGui::Image(TextureID, ViewportSize);

	ImGui::SetCursorScreenPos(CursorPos);
	ImGui::InvisibleButton("##ViewportInteraction", ViewportSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

	bool bViewerHasFocus = ImGui::IsItemActive() || ImGui::IsItemHovered();
	if (bViewerHasFocus)
	{
		ImGuiIO& IO = ImGui::GetIO();
		IO.WantCaptureMouse = true;
		IO.WantCaptureKeyboard = true;
	}

	// 뷰포트 상태 표시
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImVec2 WindowPos = ImGui::GetCursorScreenPos();
	WindowPos.y -= ViewportSize.y;

	ImVec2 InfoPos = ImVec2(WindowPos.x + 10, WindowPos.y + 10);
	ImVec2 InfoSize = ImVec2(280, 70);

	DrawList->AddRectFilled(InfoPos, ImVec2(InfoPos.x + InfoSize.x, InfoPos.y + InfoSize.y),
	                        IM_COL32(0, 0, 0, 180), 4.0f);

	// 화면 사이즈 정보
	DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 10), IM_COL32(80, 200, 200, 255),
	                  (FString("Size: ") + std::to_string(ViewerWidth) + " x " + std::to_string(ViewerHeight)).c_str());

	// 단축키 정보
	DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 30), IM_COL32(150, 150, 150, 255), "RMB: Rotate | MMB: Pan");
	DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 50), IM_COL32(150, 150, 150, 255), "Wheel: Zoom | Q/W/E/R: Gizmo");
}

/**
 * @brief 뷰포트 입력 처리 (키보드, 마우스, 기즈모, 카메라)
 */
void USkeletalMeshViewerWindow::ProcessViewportInput(bool bViewerHasFocus, const ImVec2& ViewportWindowPos)
{
	if (ImGui::IsItemHovered())
	{
		// 키보드 입력 처리
		if (ViewerGizmo)
		{
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_GraveAccent))
			{
				ViewerGizmo->IsWorldMode() ? ViewerGizmo->SetLocal() : ViewerGizmo->SetWorld();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Space))
			{
				ViewerGizmo->ChangeGizmoMode();
			}
		}

		UCamera* Camera = ViewerViewportClient->GetCamera();
		if (Camera)
		{
			ImVec2 MousePos = ImGui::GetMousePos();
			const ImVec2& WindowPos = ViewportWindowPos;

			float MouseX = MousePos.x - WindowPos.x;
			float MouseY = MousePos.y - WindowPos.y;

			const float NdcX = (MouseX / ViewerWidth) * 2.0f - 1.0f;
			const float NdcY = -((MouseY / ViewerHeight) * 2.0f - 1.0f);

			FRay WorldRay = Camera->ConvertToWorldRay(NdcX, NdcY);

			// 기즈모 호버링 (본 선택 시)
			FVector CollisionPoint;
			if (ViewerGizmo && SelectedBoneIndex != INDEX_NONE && ViewerObjectPicker && !ViewerGizmo->IsDragging())
			{
				ViewerObjectPicker->PickGizmo(Camera, WorldRay, *ViewerGizmo, CollisionPoint);
			}

			// 기즈모 클릭
			if (ViewerGizmo && ViewerGizmo->GetGizmoDirection() != EGizmoDirection::None)
			{
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					ViewerGizmo->OnMouseDragStart(CollisionPoint);

					// 본 선택 시: 본의 월드 트랜스폼을 기즈모에 설정
					if (SelectedBoneIndex != INDEX_NONE && SkeletalMeshComponent)
					{
						FVector BoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
						FQuaternion BoneWorldRotation = GetBoneWorldRotation(SelectedBoneIndex);

						// TempBoneSpaceTransforms가 초기화되어 있는지 확인
						if (!TempBoneSpaceTransforms.IsValidIndex(SelectedBoneIndex))
						{
							USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
							if (SkeletalMesh)
							{
								int32 NumBones = SkeletalMesh->GetRefSkeleton().GetRawBoneNum();
								TempBoneSpaceTransforms.Reset(NumBones);
								for (int32 i = 0; i < NumBones; ++i)
								{
									TempBoneSpaceTransforms[i] = SkeletalMeshComponent->GetBoneTransformLocal(i);
								}
							}
						}

						FVector BoneLocalScale = TempBoneSpaceTransforms.IsValidIndex(SelectedBoneIndex)
							? TempBoneSpaceTransforms[SelectedBoneIndex].Scale
							: FVector::OneVector();

						ViewerGizmo->SetDragStartActorLocation(BoneWorldLocation);
						ViewerGizmo->SetDragStartActorRotationQuat(BoneWorldRotation);
						ViewerGizmo->SetDragStartActorScale(BoneLocalScale);
						ViewerGizmo->SetDragStartMouseLocation(CollisionPoint);
					}

					ImVec2 CurrentMousePos = ImGui::GetMousePos();
					const FVector2 ViewerLocalScreenPos(CurrentMousePos.x - ViewportWindowPos.x, CurrentMousePos.y - ViewportWindowPos.y);
					ViewerGizmo->SetPreviousScreenPos(ViewerLocalScreenPos);
					ViewerGizmo->SetDragStartScreenPos(ViewerLocalScreenPos);
				}
			}
			else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				// 기즈모가 선택되지 않은 상태에서 왼쪽 클릭
				// → 본 피킹 시도 또는 선택 해제

				// 본 피킹 시도 (bShowAllBones가 켜져있을 때만)
				bool bBonePicked = false;
				if (bShowAllBones && SkeletalMeshComponent)
				{
					// HitProxy 패스 렌더링
					RenderHitProxyPassForViewer();

					// 마우스 위치에서 HitProxy ID 읽기
					int32 LocalMouseX = static_cast<int32>(MouseX);
					int32 LocalMouseY = static_cast<int32>(MouseY);
					FHitProxyId HitProxyId = ReadHitProxyAtViewerLocation(LocalMouseX, LocalMouseY);

					if (HitProxyId.IsValid())
					{
						// HitProxyManager에서 HitProxy 조회
						FHitProxyManager& HitProxyManager = FHitProxyManager::GetInstance();
						HHitProxy* HitProxy = HitProxyManager.GetHitProxy(HitProxyId);

						if (HitProxy && HitProxy->IsBone())
						{
							HBone* BoneProxy = static_cast<HBone*>(HitProxy);
							if (BoneProxy->SkeletalMeshComponent == SkeletalMeshComponent)
							{
								// 본 선택
								SelectedBoneIndex = BoneProxy->BoneIndex;
								bBonePicked = true;

								// 기즈모를 선택된 본 위치로 이동
								if (ViewerGizmo)
								{
									FVector BoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
									ViewerGizmo->SetFixedLocation(BoneWorldLocation);
								}
							}
						}
					}

					// HitProxy 매니저 정리
					FHitProxyManager::GetInstance().ClearAllHitProxies();
				}

				// 본이 피킹되지 않았으면 선택 해제
				if (!bBonePicked && SelectedBoneIndex != INDEX_NONE)
				{
					SelectedBoneIndex = INDEX_NONE;
					if (ViewerGizmo)
					{
						ViewerGizmo->ClearFixedLocation();
					}
				}
			}
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			bIsDraggingRightButton = true;
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
		{
			bIsDraggingMiddleButton = true;
		}
	}

	if (bViewerHasFocus)
	{
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			bIsDraggingRightButton = false;
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
		{
			bIsDraggingMiddleButton = false;
		}
	}

	if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		bIsDraggingRightButton = false;
	}

	if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
	{
		bIsDraggingMiddleButton = false;
	}

	// 기즈모 드래그 릴리즈
	if (ViewerGizmo && ViewerGizmo->IsDragging())
	{
		bool bIsGlobalMouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		if (!bIsGlobalMouseDown)
		{
			ViewerGizmo->EndDrag();
			ViewerGizmo->SetGizmoDirection(EGizmoDirection::None);
		}
	}

	// 기즈모 드래그 처리 (본)
	if (ViewerGizmo && ViewerGizmo->IsDragging() && SelectedBoneIndex != INDEX_NONE && ViewerViewportClient)
	{
		UCamera* Camera = ViewerViewportClient->GetCamera();
		if (Camera)
		{
			ImVec2 MousePos = ImGui::GetMousePos();

			float MouseX = MousePos.x - ViewportWindowPos.x;
			float MouseY = MousePos.y - ViewportWindowPos.y;

			const float NdcX = (MouseX / ViewerWidth) * 2.0f - 1.0f;
			const float NdcY = -((MouseY / ViewerHeight) * 2.0f - 1.0f);

			FRay WorldRay = Camera->ConvertToWorldRay(NdcX, NdcY);

			// 본 선택 시: 본의 로컬 트랜스폼 직접 수정 (UI 슬라이더 방식)
			if (SelectedBoneIndex != INDEX_NONE && SkeletalMeshComponent)
			{
				if (!TempBoneSpaceTransforms.IsValidIndex(SelectedBoneIndex))
				{
					// TempBoneSpaceTransforms 초기화 (아직 초기화되지 않은 경우)
					USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
					if (SkeletalMesh)
					{
						int32 NumBones = SkeletalMesh->GetRefSkeleton().GetRawBoneNum();
						TempBoneSpaceTransforms.Reset(NumBones);
						for (int32 i = 0; i < NumBones; ++i)
						{
							TempBoneSpaceTransforms[i] = SkeletalMeshComponent->GetBoneTransformLocal(i);
						}
					}
				}

				FTransform& BoneLocalTransform = TempBoneSpaceTransforms[SelectedBoneIndex];

				switch (ViewerGizmo->GetGizmoMode())
				{
					case EGizmoMode::Translate:
					{
						// 기즈모로부터 월드 좌표 얻기
						FVector GizmoDragLocation = FGizmoHelper::ProcessDragLocation(ViewerGizmo, ViewerObjectPicker, Camera, WorldRay, true);

						// 월드 좌표 → 본 로컬 좌표 변환
						FVector BoneLocalLocation = WorldToLocalBoneTranslation(SelectedBoneIndex, GizmoDragLocation);

						// 본 로컬 트랜스폼 업데이트
						BoneLocalTransform.Translation = BoneLocalLocation;

						// SkeletalMeshComponent에 적용
						SkeletalMeshComponent->SetBoneTransformLocal(SelectedBoneIndex, BoneLocalTransform);

						// 기즈모 위치 업데이트 (다음 프레임을 위해)
						ViewerGizmo->SetFixedLocation(GizmoDragLocation);
						break;
					}
					case EGizmoMode::Rotate:
					{
						// 뷰어의 ViewportRect 생성
						FRect ViewportRect;
						ViewportRect.Left = static_cast<uint32>(ViewportWindowPos.x);
						ViewportRect.Top = static_cast<uint32>(ViewportWindowPos.y);
						ViewportRect.Width = ViewerWidth;
						ViewportRect.Height = ViewerHeight;

						// 기즈모로부터 월드 회전 얻기
						FQuaternion GizmoDragRotation = FGizmoHelper::ProcessDragRotation(ViewerGizmo, Camera, WorldRay, ViewportRect, true);

						// 월드 회전 → 본 로컬 회전 변환
						FQuaternion BoneLocalRotation = WorldToLocalBoneRotation(SelectedBoneIndex, GizmoDragRotation);

						// 본 로컬 트랜스폼 업데이트
						BoneLocalTransform.Rotation = BoneLocalRotation;

						// SkeletalMeshComponent에 적용
						SkeletalMeshComponent->SetBoneTransformLocal(SelectedBoneIndex, BoneLocalTransform);

						// 기즈모 위치는 그대로 유지 (회전 시 위치는 변하지 않음)
						FVector BoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
						ViewerGizmo->SetFixedLocation(BoneWorldLocation);
						break;
					}
					case EGizmoMode::Scale:
					{
						// 기즈모로부터 스케일 얻기
						FVector GizmoDragScale = FGizmoHelper::ProcessDragScale(ViewerGizmo, ViewerObjectPicker, Camera, WorldRay);

						// 스케일은 로컬 스케일을 직접 사용 (월드→로컬 변환 불필요)
						BoneLocalTransform.Scale = GizmoDragScale;

						// SkeletalMeshComponent에 적용
						SkeletalMeshComponent->SetBoneTransformLocal(SelectedBoneIndex, BoneLocalTransform);

						// 기즈모 위치는 그대로 유지
						FVector BoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
						ViewerGizmo->SetFixedLocation(BoneWorldLocation);
						break;
					}
				}
			}
		}
	}
	else if ((bIsDraggingRightButton || bIsDraggingMiddleButton || ImGui::IsItemHovered()) && ViewerViewportClient)
	{
		UCamera* Camera = ViewerViewportClient->GetCamera();
		if (Camera)
		{
			ImGuiIO& IO = ImGui::GetIO();

			if (bIsDraggingRightButton && ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				FVector MouseDelta = FVector(IO.MouseDelta.x, IO.MouseDelta.y, 0);

				if (Camera->GetCameraType() == ECameraType::ECT_Perspective)
				{
					const float YawDelta = MouseDelta.X * KeySensitivityDegPerPixel * 2;
					const float PitchDelta = -MouseDelta.Y * KeySensitivityDegPerPixel * 2;

					FRotator CurrentRot = Camera->GetRotationRotator();
					CurrentRot.Yaw += YawDelta;
					CurrentRot.Pitch += PitchDelta;
					CurrentRot.Roll = 0.0f;

					constexpr float MaxPitch = 89.9f;
					CurrentRot.Pitch = clamp(CurrentRot.Pitch, -MaxPitch, MaxPitch);

					Camera->SetRotation(CurrentRot);

					FVector Direction = FVector::Zero();
					if (ImGui::IsKeyDown(ImGuiKey_A)) { Direction += -Camera->GetRight() * 2; }
					if (ImGui::IsKeyDown(ImGuiKey_D)) { Direction += Camera->GetRight() * 2; }
					if (ImGui::IsKeyDown(ImGuiKey_W)) { Direction += Camera->GetForward() * 2; }
					if (ImGui::IsKeyDown(ImGuiKey_S)) { Direction += -Camera->GetForward() * 2; }
					if (ImGui::IsKeyDown(ImGuiKey_Q)) { Direction += FVector(0, 0, -2); }
					if (ImGui::IsKeyDown(ImGuiKey_E)) { Direction += FVector(0, 0, 2); }

					if (Direction.LengthSquared() > MATH_EPSILON)
					{
						Direction.Normalize();
						Camera->SetLocation(Camera->GetLocation() + Direction * Camera->GetMoveSpeed() * DT);
					}
				}
				else if (Camera->GetCameraType() == ECameraType::ECT_Orthographic)
				{
					const float PanSpeed = 0.1f;
					FVector PanDelta = Camera->GetRight() * -MouseDelta.X * PanSpeed + Camera->GetUp() * MouseDelta.Y * PanSpeed;
					Camera->SetLocation(Camera->GetLocation() + PanDelta);
				}
			}

			if (bIsDraggingMiddleButton && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				FVector MouseDelta = FVector(IO.MouseDelta.x, IO.MouseDelta.y, 0);

				const float PanSpeed = 0.5f;
				FVector PanDelta = Camera->GetRight() * -MouseDelta.X * PanSpeed + Camera->GetUp() * MouseDelta.Y * PanSpeed;
				Camera->SetLocation(Camera->GetLocation() + PanDelta);
			}

			if (IO.MouseWheel != 0.0f)
			{
				if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
				{
					float CurrentSpeed = Camera->GetMoveSpeed();
					float NewSpeed = CurrentSpeed + IO.MouseWheel * 1.0f;
					NewSpeed = clamp(NewSpeed, 0.1f, 100.0f);
					Camera->SetMoveSpeed(NewSpeed);
				}
				else
				{
					if (Camera->GetCameraType() == ECameraType::ECT_Perspective)
					{
						float NewFOV = Camera->GetFovY() - IO.MouseWheel * 5.0f;
						NewFOV = clamp(NewFOV, 10.0f, 120.0f);
						Camera->SetFovY(NewFOV);
					}
					else if (Camera->GetCameraType() == ECameraType::ECT_Orthographic)
					{
						float NewZoom = Camera->GetOrthoZoom() * (1.0f - IO.MouseWheel * 0.1f);
						NewZoom = clamp(NewZoom, 10.0f, 10000.0f);
						Camera->SetOrthoZoom(NewZoom);
					}
				}
			}
		}
	}
}

/**
 * @brief 중앙 패널: 3D Viewport
 * 독립적인 카메라를 사용한 3D 렌더링 뷰포트
 */
void USkeletalMeshViewerWindow::Render3DViewportPanel()
{
	// 1. 툴바 업데이트 및 기즈모 동기화
	UpdateToolbarAndGizmoSync();

	// 2. 뷰포트 크기 가져오기
	const ImVec2 ViewportSize = ImGui::GetContentRegionAvail();
	const uint32 NewWidth = static_cast<uint32>(ViewportSize.x);
	const uint32 NewHeight = static_cast<uint32>(ViewportSize.y);

	// 3. 렌더 타겟 업데이트 (크기 변경 시)
	UpdateViewportRenderTarget(NewWidth, NewHeight);

	// 4. 3D 씬을 렌더 타겟에 렌더링
	RenderToViewportTexture();

	// 5. ImGui에 렌더 결과 표시 (뷰포트 시작 위치 저장)
	const ImVec2 ViewportWindowPos = ImGui::GetCursorScreenPos();
	DisplayViewportImage(ViewportSize);

	// 6. 입력 처리 (마우스/키보드/기즈모/카메라)
	bool bViewerHasFocus = ImGui::IsItemActive() || ImGui::IsItemHovered();
	ProcessViewportInput(bViewerHasFocus, ViewportWindowPos);
}

/**
 * @brief 우측 패널: Edit Tools (Placeholder)
 */
void USkeletalMeshViewerWindow::RenderEditToolsPanel(const USkeletalMesh* InSkeletalMesh, const FReferenceSkeleton& InRefSkeleton, const int32 InNumBones)
{
	bool bValid = CheckSkeletalValidity(const_cast<USkeletalMesh*&>(InSkeletalMesh), const_cast<FReferenceSkeleton&>(InRefSkeleton), const_cast<int32&>(InNumBones), true);
	if (bValid == false)
	{
		return;
	}

	ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.8f, 1.0f), "Edit Tools");
	ImGui::Separator();
	ImGui::Spacing();

	// 선택된 본이 없는 경우
	if (SelectedBoneIndex == INDEX_NONE)
	{
		ImGui::TextDisabled("No bone selected");
		ImGui::Spacing();
		ImGui::TextWrapped("Select a bone from the Skeleton Tree to view and edit its properties.");
		return;
	}

	// 선택된 본 인덱스 유효성 검사
	assert(( 0 <= SelectedBoneIndex && SelectedBoneIndex < InNumBones) && "Invalid bone index selected");

	const TArray<FMeshBoneInfo>& BoneInfoArray = InRefSkeleton.GetRawRefBoneInfo();
	const TArray<FTransform>& RefBonePoses = InRefSkeleton.GetRawRefBonePose();
	const FMeshBoneInfo& BoneInfo = BoneInfoArray[SelectedBoneIndex];

	// ===================================================================
	// Reference Pose (읽기 전용)
	// ===================================================================
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.25f, 0.35f, 1.0f));

	if (ImGui::CollapsingHeader("Reference Pose (Read Only)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const FTransform& RefPose = RefBonePoses[SelectedBoneIndex];

		ImGui::Indent();

		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Translation:");
		ImGui::Text("  X: %.3f  Y: %.3f  Z: %.3f", RefPose.Translation.X, RefPose.Translation.Y, RefPose.Translation.Z);

		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Rotation (Quaternion):");
		ImGui::Text("  X: %.3f  Y: %.3f  Z: %.3f  W: %.3f", RefPose.Rotation.X, RefPose.Rotation.Y, RefPose.Rotation.Z, RefPose.Rotation.W);

		ImGui::Spacing();

		FVector EulerRot = RefPose.Rotation.ToEuler();
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Rotation (Euler):");
		ImGui::Text("  Pitch: %.3f  Yaw: %.3f  Roll: %.3f", EulerRot.X, EulerRot.Y, EulerRot.Z);

		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Scale:");
		ImGui::Text("  X: %.3f  Y: %.3f  Z: %.3f", RefPose.Scale.X, RefPose.Scale.Y, RefPose.Scale.Z);

		ImGui::Unindent();
	}

	ImGui::PopStyleColor(3);
	ImGui::Spacing();

	// ===================================================================
	// Bone Space Transform (편집 가능)
	// ===================================================================
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.3f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.4f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.35f, 0.25f, 1.0f));

	if (ImGui::CollapsingHeader("Bone Space Transform (Editable)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		FTransform& TempTransform = TempBoneSpaceTransforms[SelectedBoneIndex];

		ImGui::Indent();

		// Translation
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Translation:");

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		bDirtyBoneTransforms = false;

		// X
		ImVec2 PosX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##TransX", &TempTransform.Translation.X, 0.1f, 0.0f, 0.0f, "%.3f"))
		{
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2),
			IM_COL32(255, 0, 0, 255), 2.0f);
		ImGui::SameLine();

		// Y
		ImVec2 PosY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##TransY", &TempTransform.Translation.Y, 0.1f, 0.0f, 0.0f, "%.3f"))
		{
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2),
			IM_COL32(0, 255, 0, 255), 2.0f);
		ImGui::SameLine();

		// Z
		ImVec2 PosZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##TransZ", &TempTransform.Translation.Z, 0.1f, 0.0f, 0.0f, "%.3f"))
		{
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2),
			IM_COL32(0, 0, 255, 255), 2.0f);

		ImGui::Spacing();

		// Rotation (Euler) - 캐싱 방식으로 90도 제한 해결
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Rotation (Euler):");

		// Static 변수로 캐싱된 Euler 각도 관리
		static FVector CachedRotation = FVector::ZeroVector();
		static bool bIsDraggingRotation = false;
		static int32 LastSelectedBoneIndex = INDEX_NONE;

		// 본이 변경되었을 때 캐시 초기화
		if (LastSelectedBoneIndex != SelectedBoneIndex)
		{
			CachedRotation = TempTransform.Rotation.ToEuler();
			LastSelectedBoneIndex = SelectedBoneIndex;
			bIsDraggingRotation = false;
		}

		// 드래그 중이 아닐 때만 동기화
		if (!bIsDraggingRotation)
		{
			CachedRotation = TempTransform.Rotation.ToEuler();
		}

		// 작은 값은 0으로 스냅 (부동소수점 오차 제거)
		constexpr float ZeroSnapThreshold = 0.0001f;
		if (std::abs(CachedRotation.X) < ZeroSnapThreshold) CachedRotation.X = 0.0f;
		if (std::abs(CachedRotation.Y) < ZeroSnapThreshold) CachedRotation.Y = 0.0f;
		if (std::abs(CachedRotation.Z) < ZeroSnapThreshold) CachedRotation.Z = 0.0f;

		float EulerAngles[3] = { CachedRotation.X, CachedRotation.Y, CachedRotation.Z };
		bool bRotationChanged = false;

		// Pitch (X)
		ImVec2 RotX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##RotX", &EulerAngles[0], 1.0f, 0.0f, 0.0f, "%.3f"))
		{
			bRotationChanged = true;
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeRotX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotX.x + 5, RotX.y + 2), ImVec2(RotX.x + 5, RotX.y + SizeRotX.y - 2),
			IM_COL32(255, 0, 0, 255), 2.0f);
		
		bool bIsAnyRotationItemActive = ImGui::IsItemActive();
		ImGui::SameLine();

		// Yaw (Y)
		ImVec2 RotY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##RotY", &EulerAngles[1], 1.0f, 0.0f, 0.0f, "%.3f"))
		{
			bRotationChanged = true;
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeRotY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotY.x + 5, RotY.y + 2), ImVec2(RotY.x + 5, RotY.y + SizeRotY.y - 2),
			IM_COL32(0, 255, 0, 255), 2.0f);
		
		bIsAnyRotationItemActive |= ImGui::IsItemActive();
		ImGui::SameLine();

		// Roll (Z)
		ImVec2 RotZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##RotZ", &EulerAngles[2], 1.0f, 0.0f, 0.0f, "%.3f"))
		{
			bRotationChanged = true;
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeRotZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotZ.x + 5, RotZ.y + 2), ImVec2(RotZ.x + 5, RotZ.y + SizeRotZ.y - 2),
			IM_COL32(0, 0, 255, 255), 2.0f);

		bIsAnyRotationItemActive |= ImGui::IsItemActive();

		// 회전값이 변경되었을 때 캐시 업데이트 및 Quaternion 변환
		if (bRotationChanged)
		{
			CachedRotation.X = EulerAngles[0];
			CachedRotation.Y = EulerAngles[1];
			CachedRotation.Z = EulerAngles[2];
			TempTransform.Rotation = FQuaternion::FromEuler(CachedRotation);
		}

		// 드래그 상태 업데이트
		bIsDraggingRotation = bIsAnyRotationItemActive;

		ImGui::Spacing();

		// Scale
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Scale:");
		ImGui::SameLine();

		// Lock button (Uniform Scale toggle) - 이미지 렌더링
		UTexture* CurrentLockIcon = bUniformBoneScale ? LockIcon : UnlockIcon;
		if (CurrentLockIcon)
		{
			ID3D11ShaderResourceView* LockSRV = CurrentLockIcon->GetTextureSRV();
			ImTextureID LockTexID = reinterpret_cast<ImTextureID>(LockSRV);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

			ImVec2 ButtonSize(16.0f, 16.0f);
			if (ImGui::ImageButton("##LockBoneScale", LockTexID, ButtonSize))
			{
				bUniformBoneScale = !bUniformBoneScale;
			}

			ImGui::PopStyleColor(3);

			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(bUniformBoneScale ? "Locked: Uniform Scale" : "Unlocked: Non-uniform Scale");
			}
		}

		// 이전 스케일 값 저장 (Uniform 모드용)
		static FVector PrevScale = TempTransform.Scale;

		// Scale X
		ImVec2 ScaleX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##ScaleX", &TempTransform.Scale.X, 0.01f, 0.0f, 0.0f, "%.3f"))
		{
			if (bUniformBoneScale)
			{
				// Uniform: X축 변화 비율을 Y, Z축에도 적용 (언리얼 방식)
				constexpr float MinScaleThreshold = 0.0001f;
				if (std::abs(PrevScale.X) > MinScaleThreshold)
				{
					const float ScaleRatio = TempTransform.Scale.X / PrevScale.X;
					TempTransform.Scale.Y = PrevScale.Y * ScaleRatio;
					TempTransform.Scale.Z = PrevScale.Z * ScaleRatio;
				}
				else
				{
					// PrevScale.X가 0에 가까우면 현재 값을 다른 축에도 적용
					TempTransform.Scale.Y = TempTransform.Scale.X;
					TempTransform.Scale.Z = TempTransform.Scale.X;
				}
			}
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeScaleX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleX.x + 5, ScaleX.y + 2), ImVec2(ScaleX.x + 5, ScaleX.y + SizeScaleX.y - 2),
			IM_COL32(255, 0, 0, 255), 2.0f);
		ImGui::SameLine();

		// Scale Y
		ImVec2 ScaleY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##ScaleY", &TempTransform.Scale.Y, 0.01f, 0.0f, 0.0f, "%.3f"))
		{
			if (bUniformBoneScale)
			{
				// Uniform: Y축 변화 비율을 X, Z축에도 적용 (언리얼 방식)
				constexpr float MinScaleThreshold = 0.0001f;
				if (std::abs(PrevScale.Y) > MinScaleThreshold)
				{
					const float ScaleRatio = TempTransform.Scale.Y / PrevScale.Y;
					TempTransform.Scale.X = PrevScale.X * ScaleRatio;
					TempTransform.Scale.Z = PrevScale.Z * ScaleRatio;
				}
				else
				{
					// PrevScale.Y가 0에 가까우면 현재 값을 다른 축에도 적용
					TempTransform.Scale.X = TempTransform.Scale.Y;
					TempTransform.Scale.Z = TempTransform.Scale.Y;
				}
			}
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeScaleY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleY.x + 5, ScaleY.y + 2), ImVec2(ScaleY.x + 5, ScaleY.y + SizeScaleY.y - 2),
			IM_COL32(0, 255, 0, 255), 2.0f);
		ImGui::SameLine();

		// Scale Z
		ImVec2 ScaleZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::DragFloat("##ScaleZ", &TempTransform.Scale.Z, 0.01f, 0.0f, 0.0f, "%.3f"))
		{
			if (bUniformBoneScale)
			{
				// Uniform: Z축 변화 비율을 X, Y축에도 적용 (언리얼 방식)
				constexpr float MinScaleThreshold = 0.0001f;
				if (std::abs(PrevScale.Z) > MinScaleThreshold)
				{
					const float ScaleRatio = TempTransform.Scale.Z / PrevScale.Z;
					TempTransform.Scale.X = PrevScale.X * ScaleRatio;
					TempTransform.Scale.Y = PrevScale.Y * ScaleRatio;
				}
				else
				{
					// PrevScale.Z가 0에 가까우면 현재 값을 다른 축에도 적용
					TempTransform.Scale.X = TempTransform.Scale.Z;
					TempTransform.Scale.Y = TempTransform.Scale.Z;
				}
			}
			bDirtyBoneTransforms = true;
		}
		ImVec2 SizeScaleZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleZ.x + 5, ScaleZ.y + 2), ImVec2(ScaleZ.x + 5, ScaleZ.y + SizeScaleZ.y - 2),
			IM_COL32(0, 0, 255, 255), 2.0f);

		// 다음 프레임을 위해 현재 스케일 저장
		PrevScale = TempTransform.Scale;

		ImGui::PopStyleColor(3);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (bDirtyBoneTransforms)
		{
			SkeletalMeshComponent->SetBoneTransformLocal(SelectedBoneIndex, TempTransform);
			// UI 슬라이더로 위치 변경 시 기즈모 위치도 업데이트
			if (ViewerGizmo)
			{
				FVector NewBoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
				ViewerGizmo->SetFixedLocation(NewBoneWorldLocation);
			}
		}

		//Reset 버튼들
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.4f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.25f, 0.15f, 1.0f));

		if (ImGui::Button("Reset to Last Value", ImVec2(-1, 30)))
		{
			TempBoneSpaceTransforms[SelectedBoneIndex] = OriginalSkeletalMeshComponent->GetBoneTransformLocal(SelectedBoneIndex);
			SkeletalMeshComponent->SetBoneTransformLocal(SelectedBoneIndex, TempBoneSpaceTransforms[SelectedBoneIndex]);
			// 본 트랜스폼 갱신
			SkeletalMeshComponent->RefreshBoneTransforms();
			// 기즈모 위치 업데이트
			if (ViewerGizmo)
			{
				FVector BoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
				ViewerGizmo->SetFixedLocation(BoneWorldLocation);
			}
			// 캐시 초기화
			CachedRotation = TempBoneSpaceTransforms[SelectedBoneIndex].Rotation.ToEuler();
			bIsDraggingRotation = false;
			UE_LOG("SkeletalMeshViewerWindow: Reset temp bone transforms to last value");
		}

		if(ImGui::Button("Reset to Reference Pose", ImVec2(-1, 30)))
		{
			TempBoneSpaceTransforms[SelectedBoneIndex] = RefBonePoses[SelectedBoneIndex];
			SkeletalMeshComponent->SetBoneTransformLocal(SelectedBoneIndex, TempBoneSpaceTransforms[SelectedBoneIndex]);
			// 본 트랜스폼 갱신
			SkeletalMeshComponent->RefreshBoneTransforms();
			// 기즈모 위치 업데이트
			if (ViewerGizmo)
			{
				FVector BoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
				ViewerGizmo->SetFixedLocation(BoneWorldLocation);
			}
			// 캐시 초기화
			CachedRotation = TempBoneSpaceTransforms[SelectedBoneIndex].Rotation.ToEuler();
			bIsDraggingRotation = false;
			UE_LOG("SkeletalMeshViewerWindow: Reset temp bone transforms to reference pose");
		}

		ImGui::PopStyleColor(3);

		ImGui::Unindent();
	}

	ImGui::PopStyleColor(3);
	ImGui::Spacing();

	// ===================================================================
	// Mesh Info
	// ===================================================================
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	if (ImGui::CollapsingHeader("Mesh Info"))
	{
		if (InSkeletalMesh)
		{
			ImGui::Text("Loaded Mesh: %s", InSkeletalMesh->GetName().ToString().c_str());
			ImGui::Text("Bones: %d", InNumBones);
		}
		else
		{
			ImGui::TextDisabled("Loaded Mesh: (없음)");
			ImGui::TextDisabled("Bones: 0");
		}
	}

	ImGui::PopStyleColor(3);

	ImGui::Spacing();

	ImGui::Checkbox("Show All Bones", &bShowAllBones);

	// All Value Change 버튼들 - 우측 하단에 고정
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.15f, 1.0f));

	if (ImGui::Button("Reset All Value to Last Value", ImVec2(-1, 30)))
	{
		for (int32 i = 0; i < InNumBones; ++i)
		{
			TempBoneSpaceTransforms[i] = OriginalSkeletalMeshComponent->GetBoneTransformLocal(i);
			SkeletalMeshComponent->SetBoneTransformLocal(i, TempBoneSpaceTransforms[i]);
		}
		// 본 트랜스폼 갱신
		SkeletalMeshComponent->RefreshBoneTransforms();
		// 선택된 본이 있으면 기즈모 위치 업데이트
		if (SelectedBoneIndex != -1 && ViewerGizmo)
		{
			FVector BoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
			ViewerGizmo->SetFixedLocation(BoneWorldLocation);
		}
		UE_LOG("SkeletalMeshViewerWindow: Reset temp bone transforms to last value");
	}

	if (ImGui::Button("Reset All Value to Reference Pose", ImVec2(-1, 30)))
	{
		for (int32 i = 0; i < InNumBones; ++i)
		{
			TempBoneSpaceTransforms[i] = RefBonePoses[i];
			SkeletalMeshComponent->SetBoneTransformLocal(i, TempBoneSpaceTransforms[i]);
		}
		// 본 트랜스폼 갱신
		SkeletalMeshComponent->RefreshBoneTransforms();
		// 선택된 본이 있으면 기즈모 위치 업데이트
		if (SelectedBoneIndex != -1 && ViewerGizmo)
		{
			FVector BoneWorldLocation = GetBoneWorldLocation(SelectedBoneIndex);
			ViewerGizmo->SetFixedLocation(BoneWorldLocation);
		}
		UE_LOG("SkeletalMeshViewerWindow: Reset temp bone transforms to reference pose");
	}

	if (ImGui::Button("Apply All Changes", ImVec2(-1, 30)))
	{
		for (int32 i = 0; i < InNumBones; ++i)
		{
			OriginalSkeletalMeshComponent->SetBoneTransformLocal(i, TempBoneSpaceTransforms[i]);
		}
		OriginalSkeletalMeshComponent->RefreshBoneTransforms();
		OriginalSkeletalMeshComponent->UpdateSkinnedVertices();
		UE_LOG("SkeletalMeshViewerWindow: Applied bone transform changes");
	}
	ImGui::PopStyleColor(3);
}

/**
 * @brief 카메라 컨트롤 UI 렌더링
 */
void USkeletalMeshViewerWindow::RenderCameraControls(UCamera& InCamera)
{
	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Camera Settings");
	ImGui::Separator();
	ImGui::Spacing();

	// 카메라 타입
	ECameraType CameraType = InCamera.GetCameraType();
	const char* CameraTypeNames[] = { "Perspective", "Orthographic" };
	int CurrentType = static_cast<int>(CameraType);

	if (ImGui::Combo("Camera Type", &CurrentType, CameraTypeNames, 2))
	{
		InCamera.SetCameraType(static_cast<ECameraType>(CurrentType));
	}

	ImGui::Spacing();

	// 위치
	FVector Location = InCamera.GetLocation();
	float Loc[3] = { Location.X, Location.Y, Location.Z };
	if (ImGui::DragFloat3("Location", Loc, 1.0f))
	{
		InCamera.SetLocation(FVector(Loc[0], Loc[1], Loc[2]));
	}

	// 회전
	FVector Rotation = InCamera.GetRotation();
	float Rot[3] = { Rotation.X, Rotation.Y, Rotation.Z };
	if (ImGui::DragFloat3("Rotation", Rot, 1.0f))
	{
		InCamera.SetRotation(FVector(Rot[0], Rot[1], Rot[2]));
	}

	ImGui::Spacing();

	// Perspective 전용 설정
	if (CameraType == ECameraType::ECT_Perspective)
	{
		float FOV = InCamera.GetFovY();
		if (ImGui::SliderFloat("FOV", &FOV, 10.0f, 120.0f, "%.1f"))
		{
			InCamera.SetFovY(FOV);
		}
	}
	// Orthographic 전용 설정
	else
	{
		float OrthoZoom = InCamera.GetOrthoZoom();
		if (ImGui::SliderFloat("Ortho Zoom", &OrthoZoom, 10.0f, 10000.0f, "%.1f"))
		{
			InCamera.SetOrthoZoom(OrthoZoom);
		}
	}

	// Near/Far Clip
	float NearZ = InCamera.GetNearZ();
	float FarZ = InCamera.GetFarZ();

	if (ImGui::DragFloat("Near Clip", &NearZ, 0.1f, 0.1f, 100.0f, "%.2f"))
	{
		InCamera.SetNearZ(NearZ);
	}

	if (ImGui::DragFloat("Far Clip", &FarZ, 10.0f, 100.0f, 100000.0f, "%.1f"))
	{
		InCamera.SetFarZ(FarZ);
	}

	ImGui::Spacing();

	// 이동 속도
	float MoveSpeed = InCamera.GetMoveSpeed();
	if (ImGui::SliderFloat("Move Speed", &MoveSpeed, 1.0f, 100.0f, "%.1f"))
	{
		InCamera.SetMoveSpeed(MoveSpeed);
	}

	ImGui::Spacing();
	ImGui::Separator();

	// 카메라 리셋 버튼
	if (ImGui::Button("Reset Camera", ImVec2(-1, 0)))
	{
		InCamera.SetLocation(FVector(0.0f, -5.0f, 4.0f));
		InCamera.SetRotation(FVector(0.0f, 0.0f, 90.0f));
		InCamera.SetFovY(90.0f);
		InCamera.SetOrthoZoom(1000.0f);
		InCamera.SetMoveSpeed(10.0f);
	}
}

bool USkeletalMeshViewerWindow::CheckSkeletalValidity(USkeletalMesh*& OutSkeletalMesh, FReferenceSkeleton& OutRefSkeleton, int32& OutNumBones, bool bLogging) const
{
	// SkeletalMeshComponent 유효성 검사
	if (!SkeletalMeshComponent)
	{
		if (bLogging)
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No SkeletalMeshComponent assigned");
			ImGui::Spacing();
			ImGui::TextWrapped("Select a SkeletalMeshComponent from the Detail panel to view its skeleton.");
		}
		return false;
	}

	// SkeletalMesh 가져오기
	OutSkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!OutSkeletalMesh)
	{
		if (bLogging)
		{
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "No SkeletalMesh found");
			ImGui::Spacing();
			ImGui::TextWrapped("The component has no mesh asset assigned.");
		}
		return false;
	}

	// ReferenceSkeleton 가져오기
	OutRefSkeleton = OutSkeletalMesh->GetRefSkeleton();
	OutNumBones = OutRefSkeleton.GetRawBoneNum();

	if (OutNumBones == 0)
	{
		if (bLogging)
		{
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "No bones in skeleton");
		}
		return false;
	}

	return true;
}

/**
 * @brief 수직 Splitter (드래그 가능한 구분선) 렌더링
 * @param bInvertDirection false면 좌측 패널용 (오른쪽 드래그 = 증가), true면 우측 패널용 (왼쪽 드래그 = 증가)
 */
void USkeletalMeshViewerWindow::RenderVerticalSplitter(const char* SplitterID, float& Ratio, float MinRatio, float MaxRatio, bool bInvertDirection)
{
	const ImVec2 ContentRegion = ImGui::GetContentRegionAvail();
	const float SplitterHeight = ContentRegion.y;

	// Splitter 영역 설정
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 0.7f));

	ImGui::Button(SplitterID, ImVec2(SplitterWidth, SplitterHeight));

	ImGui::PopStyleColor(3);

	// 호버 시 커서 변경
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}

	// 드래그 처리
	if (ImGui::IsItemActive())
	{
		// 마우스 이동량 계산
		const float MouseDeltaX = ImGui::GetIO().MouseDelta.x;
		const float TotalWidth = ImGui::GetWindowWidth();

		// 비율 조정
		// 좌측 패널: 오른쪽 드래그 = 증가 (양수)
		// 우측 패널: 왼쪽 드래그 = 증가 (음수를 곱해서 반전)
		const float DirectionMultiplier = bInvertDirection ? -1.0f : 1.0f;
		const float DeltaRatio = (MouseDeltaX / TotalWidth) * DirectionMultiplier;
		Ratio += DeltaRatio;

		// 비율 제한
		Ratio = ImClamp(Ratio, MinRatio, MaxRatio);
	}
}

/**
 * @brief 그리드 셀 크기 설정
 */
void USkeletalMeshViewerWindow::SetGridCellSize(float NewCellSize)
{
	GridCellSize = NewCellSize;

	// ViewerBatchLines가 존재하면 그리드 업데이트
	if (ViewerBatchLines)
	{
		ViewerBatchLines->UpdateUGridVertices(GridCellSize);
		ViewerBatchLines->UpdateVertexBuffer();
	}
}

/**
 * @brief 본의 월드 스페이스 위치 계산
 *
 * ComponentSpace Transform을 월드 스페이스로 변환합니다.
 * 이 함수는 UI 슬라이더가 사용하는 본 데이터와 동일한 소스를 사용합니다.
 */
FVector USkeletalMeshViewerWindow::GetBoneWorldLocation(int32 BoneIndex) const
{
	if (!SkeletalMeshComponent || BoneIndex == INDEX_NONE)
	{
		return FVector::ZeroVector();
	}

	// ComponentSpace Transform 가져오기
	const TArray<FTransform>& ComponentSpaceTransforms = SkeletalMeshComponent->GetComponentSpaceTransforms();
	if (!ComponentSpaceTransforms.IsValidIndex(BoneIndex))
	{
		return FVector::ZeroVector();
	}

	// ComponentSpace → WorldSpace 변환
	FTransform ComponentWorldTransform;
	ComponentWorldTransform.Translation = SkeletalMeshComponent->GetWorldLocation();
	ComponentWorldTransform.Rotation = SkeletalMeshComponent->GetWorldRotationAsQuaternion();
	ComponentWorldTransform.Scale = SkeletalMeshComponent->GetWorldScale3D();

	// ComponentSpace 위치를 월드 스페이스로 변환
	FMatrix ComponentWorldMatrix = ComponentWorldTransform.ToMatrixWithScale();
	FVector WorldLocation = ComponentWorldMatrix.TransformPosition(ComponentSpaceTransforms[BoneIndex].Translation);

	return WorldLocation;
}

/**
 * @brief 본의 월드 스페이스 회전 계산
 *
 * ComponentSpace Transform의 회전을 월드 스페이스로 변환합니다.
 */
FQuaternion USkeletalMeshViewerWindow::GetBoneWorldRotation(int32 BoneIndex) const
{
	if (!SkeletalMeshComponent || BoneIndex == INDEX_NONE)
	{
		return FQuaternion::Identity();
	}

	const TArray<FTransform>& ComponentSpaceTransforms = SkeletalMeshComponent->GetComponentSpaceTransforms();
	if (!ComponentSpaceTransforms.IsValidIndex(BoneIndex))
	{
		return FQuaternion::Identity();
	}

	// ComponentSpace → WorldSpace 변환
	FQuaternion ComponentWorldRotation = SkeletalMeshComponent->GetWorldRotationAsQuaternion();
	FQuaternion WorldRotation = ComponentWorldRotation * ComponentSpaceTransforms[BoneIndex].Rotation;

	return WorldRotation;
}

/**
 * @brief 월드 스페이스 위치를 본의 로컬 스페이스 위치로 변환
 *
 * 이 함수는 기즈모에서 계산된 월드 좌표를 본의 로컬 좌표로 변환하여
 * UI 슬라이더와 동일한 방식으로 TempBoneSpaceTransforms를 업데이트할 수 있게 합니다.
 */
FVector USkeletalMeshViewerWindow::WorldToLocalBoneTranslation(int32 BoneIndex, const FVector& WorldLocation) const
{
	if (!SkeletalMeshComponent || BoneIndex == INDEX_NONE)
	{
		return FVector::ZeroVector();
	}

	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!SkeletalMesh)
	{
		return FVector::ZeroVector();
	}

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	int32 ParentIndex = RefSkeleton.GetRawParentIndex(BoneIndex);

	// ComponentWorld Transform
	FTransform ComponentWorldTransform;
	ComponentWorldTransform.Translation = SkeletalMeshComponent->GetWorldLocation();
	ComponentWorldTransform.Rotation = SkeletalMeshComponent->GetWorldRotationAsQuaternion();
	ComponentWorldTransform.Scale = SkeletalMeshComponent->GetWorldScale3D();

	if (ParentIndex == INDEX_NONE)
	{
		// 루트 본: WorldSpace → ComponentSpace (ComponentSpace == LocalSpace for root)
		FMatrix InvComponentWorldMatrix = ComponentWorldTransform.ToInverseMatrixWithScale();
		return InvComponentWorldMatrix.TransformPosition(WorldLocation);
	}
	else
	{
		// 자식 본: WorldSpace → ParentBoneSpace (LocalSpace)
		const TArray<FTransform>& ComponentSpaceTransforms = SkeletalMeshComponent->GetComponentSpaceTransforms();
		if (!ComponentSpaceTransforms.IsValidIndex(ParentIndex))
		{
			return FVector::ZeroVector();
		}

		// ParentBone의 WorldSpace Transform 계산
		FTransform ParentComponentSpace = ComponentSpaceTransforms[ParentIndex];
		FMatrix ComponentWorldMatrix = ComponentWorldTransform.ToMatrixWithScale();
		FVector ParentWorldLocation = ComponentWorldMatrix.TransformPosition(ParentComponentSpace.Translation);
		FQuaternion ParentWorldRotation = ComponentWorldTransform.Rotation * ParentComponentSpace.Rotation;
		FVector ParentWorldScale = ComponentWorldTransform.Scale * ParentComponentSpace.Scale;

		FTransform ParentWorldTransform;
		ParentWorldTransform.Translation = ParentWorldLocation;
		ParentWorldTransform.Rotation = ParentWorldRotation;
		ParentWorldTransform.Scale = ParentWorldScale;

		// WorldSpace를 ParentBoneSpace(LocalSpace)로 변환
		FMatrix InvParentWorldMatrix = ParentWorldTransform.ToInverseMatrixWithScale();
		return InvParentWorldMatrix.TransformPosition(WorldLocation);
	}
}

/**
 * @brief 월드 스페이스 회전을 본의 로컬 스페이스 회전으로 변환
 *
 * 기즈모의 월드 회전을 본의 로컬 회전으로 변환합니다.
 */
FQuaternion USkeletalMeshViewerWindow::WorldToLocalBoneRotation(int32 BoneIndex, const FQuaternion& WorldRotation) const
{
	if (!SkeletalMeshComponent || BoneIndex == INDEX_NONE)
	{
		return FQuaternion::Identity();
	}

	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!SkeletalMesh)
	{
		return FQuaternion::Identity();
	}

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	int32 ParentIndex = RefSkeleton.GetRawParentIndex(BoneIndex);

	FQuaternion ComponentWorldRotation = SkeletalMeshComponent->GetWorldRotationAsQuaternion();

	if (ParentIndex == INDEX_NONE)
	{
		// 루트 본: WorldSpace → ComponentSpace
		return ComponentWorldRotation.Inverse() * WorldRotation;
	}
	else
	{
		// 자식 본: WorldSpace → ParentBoneSpace
		const TArray<FTransform>& ComponentSpaceTransforms = SkeletalMeshComponent->GetComponentSpaceTransforms();
		if (!ComponentSpaceTransforms.IsValidIndex(ParentIndex))
		{
			return FQuaternion::Identity();
		}

		// ParentBone의 WorldSpace Rotation 계산
		FQuaternion ParentWorldRotation = ComponentWorldRotation * ComponentSpaceTransforms[ParentIndex].Rotation;

		// WorldSpace를 ParentBoneSpace(LocalSpace)로 변환
		return ParentWorldRotation.Inverse() * WorldRotation;
	}
}

void USkeletalMeshViewerWindow::RenderHitProxyPassForViewer()
{
	if (!SkeletalMeshComponent || !ViewerHitProxyRTV || !bShowAllBones)
	{
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();
	UPipeline* Pipeline = Renderer.GetPipeline();

	// 현재 렌더 타겟 저장
	ID3D11RenderTargetView* OldRTV = nullptr;
	ID3D11DepthStencilView* OldDSV = nullptr;
	DeviceContext->OMGetRenderTargets(1, &OldRTV, &OldDSV);

	// 현재 뷰포트 저장
	UINT NumViewports = 1;
	D3D11_VIEWPORT OldViewport = {};
	DeviceContext->RSGetViewports(&NumViewports, &OldViewport);

	// HitProxy 렌더 타겟 설정
	Pipeline->SetRenderTargets(1, &ViewerHitProxyRTV, ViewerDepthStencilView);

	// 렌더 타겟 클리어 (검은색 = Invalid HitProxy)
	const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	DeviceContext->ClearRenderTargetView(ViewerHitProxyRTV, ClearColor);
	DeviceContext->ClearDepthStencilView(ViewerDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 뷰포트 설정
	D3D11_VIEWPORT Viewport = {};
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = static_cast<float>(ViewerWidth);
	Viewport.Height = static_cast<float>(ViewerHeight);
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	DeviceContext->RSSetViewports(1, &Viewport);

	// 카메라 설정
	UCamera* Camera = ViewerViewportClient ? ViewerViewportClient->GetCamera() : nullptr;
	if (!Camera)
	{
		// 렌더 타겟 복원
		Pipeline->SetRenderTargets(1, &OldRTV, OldDSV);
		DeviceContext->RSSetViewports(1, &OldViewport);
		SafeRelease(OldRTV);
		SafeRelease(OldDSV);
		return;
	}

	// 카메라 상수 버퍼 업데이트
	const FCameraConstants& CameraConstants = Camera->GetCameraConstants();
	ID3D11Buffer* ConstantBufferViewProj = Renderer.GetConstantBufferViewProj();
	FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewProj, CameraConstants);

	// 본 피라미드 HitProxy 렌더링
	if (ViewerBatchLines)
	{
		ViewerBatchLines->RenderBonePyramidsForHitProxy(SkeletalMeshComponent);
	}

	// 렌더 타겟 복원
	Pipeline->SetRenderTargets(1, &OldRTV, OldDSV);
	DeviceContext->RSSetViewports(1, &OldViewport);

	// 참조 카운트 해제
	SafeRelease(OldRTV);
	SafeRelease(OldDSV);

	// HitProxy 매니저 정리는 다음 프레임 시작 시 자동으로 수행됨
}

FHitProxyId USkeletalMeshViewerWindow::ReadHitProxyAtViewerLocation(int32 X, int32 Y)
{
	if (!ViewerHitProxyTexture || X < 0 || Y < 0 || X >= static_cast<int32>(ViewerWidth) || Y >= static_cast<int32>(ViewerHeight))
	{
		return InvalidHitProxyId;
	}

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	// Staging Texture 생성 (CPU에서 읽기 가능)
	static ID3D11Texture2D* StagingTexture = nullptr;
	static uint32 StagingWidth = 0;
	static uint32 StagingHeight = 0;

	if (!StagingTexture || StagingWidth != ViewerWidth || StagingHeight != ViewerHeight)
	{
		if (StagingTexture)
		{
			StagingTexture->Release();
			StagingTexture = nullptr;
		}

		D3D11_TEXTURE2D_DESC StagingDesc = {};
		StagingDesc.Width = ViewerWidth;
		StagingDesc.Height = ViewerHeight;
		StagingDesc.MipLevels = 1;
		StagingDesc.ArraySize = 1;
		StagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		StagingDesc.SampleDesc.Count = 1;
		StagingDesc.SampleDesc.Quality = 0;
		StagingDesc.Usage = D3D11_USAGE_STAGING;
		StagingDesc.BindFlags = 0;
		StagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		StagingDesc.MiscFlags = 0;

		if (FAILED(Device->CreateTexture2D(&StagingDesc, nullptr, &StagingTexture)))
		{
			UE_LOG_ERROR("SkeletalMeshViewerWindow: Failed to create staging texture for HitProxy reading");
			return InvalidHitProxyId;
		}

		StagingWidth = ViewerWidth;
		StagingHeight = ViewerHeight;
	}

	// HitProxy 텍스처를 Staging 텍스처로 복사
	DeviceContext->CopyResource(StagingTexture, ViewerHitProxyTexture);

	// Staging 텍스처에서 픽셀 읽기
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	if (FAILED(DeviceContext->Map(StagingTexture, 0, D3D11_MAP_READ, 0, &MappedResource)))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: Failed to map staging texture");
		return InvalidHitProxyId;
	}

	// 픽셀 데이터 읽기 (R8G8B8A8)
	const uint8* PixelData = static_cast<const uint8*>(MappedResource.pData);
	const uint32 PixelOffset = (Y * MappedResource.RowPitch) + (X * 4); // 4 bytes per pixel (RGBA)

	uint8 R = PixelData[PixelOffset + 0];
	uint8 G = PixelData[PixelOffset + 1];
	uint8 B = PixelData[PixelOffset + 2];

	DeviceContext->Unmap(StagingTexture, 0);

	// RGB를 HitProxyId로 변환
	FHitProxyId HitProxyId(R, G, B);
	return HitProxyId;
}
