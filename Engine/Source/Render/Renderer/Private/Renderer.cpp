#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Component/Public/TextRenderComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/DecalComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/FireBallComponent.h"
#include "Core/Public/BVHierarchy.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/EditorEngine.h"
#include "Editor/Public/Viewport.h"
#include "Editor/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"
#include "Level/Public/Level.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"
#include "Source/Component/Mesh/Public/StaticMesh.h"

#include "Render/Renderer/Public/OcclusionRenderer.h"
#include "Render/Renderer/Public/FXAAPass.h"
#include "Physics/Public/AABB.h"

#include "Core/Public/ScopeCycleCounter.h"

#include "cpp-thread-pool/thread_pool.h"

#ifdef _
#define PROFILE_SCOPE(name, expr) \
    { \
        auto __start = std::chrono::high_resolution_clock::now(); \
        expr; \
        auto __end = std::chrono::high_resolution_clock::now(); \
        double __ms = std::chrono::duration<double, std::milli>(__end - __start).count(); \
        UE_LOG("%s took %.3f ms", name, __ms); \
    }
#endif

#define PROFILE_SCOPE(name, expr) \
{ \
	expr; \
}

IMPLEMENT_SINGLETON_CLASS_BASE(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());
	ViewportClient = new FViewport();

	CreateDepthStencilState();
	CreateDefaultShader();
	CreateTextureShader();
	CreateProjectionDecalShader();
    CreateSpotlightShader();
	CreateSceneDepthViewModeShader();
	CreateConstantBuffer();
	CreateBillboardResources();
	CreateSpotlightResrouces();
	CreateFireBallShader();
    FXAA = new UFXAAPass();
    FXAA->Initialize(DeviceResources);

	// FontRenderer 초기화
	FontRenderer = new UFontRenderer();
	if (!FontRenderer->Initialize())
	{
		UE_LOG("FontRenderer 초기화 실패");
		SafeDelete(FontRenderer);
	}

	ViewportClient->InitializeLayout(DeviceResources->GetViewportInfo());

	// =============================================================================== //
	// Occlusion Renderer Initialization

	RECT ClientRect;
	GetClientRect(InWindowHandle, &ClientRect);
	uint32 Width = ClientRect.right - ClientRect.left;
	uint32 Height = ClientRect.bottom - ClientRect.top;

	UOcclusionRenderer::GetInstance().Initialize(GetDevice(), GetDeviceContext(), Width, Height);

#ifdef MULTI_THREADING
	// Create deferred contexts and thread-specific constant buffers
	for (uint32 i = 0; i < NUM_WORKER_THREADS; ++i)
	{
		ID3D11DeviceContext* DeferredContext = nullptr;
		HRESULT hr = GetDevice()->CreateDeferredContext(0, &DeferredContext);
		if (FAILED(hr) || !DeferredContext)
		{
			// Log an error message indicating that deferred context creation failed.
			// Assuming UE_LOG is available for logging.
			UE_LOG("Failed to create deferred context! HRESULT: %x", hr);
			// Depending on severity, you might want to assert or throw here.
			continue; // Skip this thread's context if creation failed
		}
		DeferredContexts.push_back(DeferredContext);

		ID3D11Buffer* ThreadCBModels = nullptr;
		ID3D11Buffer* ThreadCBColors = nullptr;
		ID3D11Buffer* ThreadCBMaterials = nullptr;
		ID3D11Buffer* ThreadCBProjectionDecals = nullptr;

		// Create Model Constant Buffer
		D3D11_BUFFER_DESC ModelConstantBufferDescription = {};
		ModelConstantBufferDescription.ByteWidth = sizeof(FMatrix) + 0xf & 0xfffffff0;
		ModelConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
		ModelConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ModelConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		GetDevice()->CreateBuffer(&ModelConstantBufferDescription, nullptr, &ThreadCBModels);
		ThreadConstantBufferModels.push_back(ThreadCBModels);

		// Create Color Constant Buffer
		D3D11_BUFFER_DESC ColorConstantBufferDescription = {};
		ColorConstantBufferDescription.ByteWidth = sizeof(FVector4) + 0xf & 0xfffffff0;
		ColorConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
		ColorConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ColorConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		GetDevice()->CreateBuffer(&ColorConstantBufferDescription, nullptr, &ThreadCBColors);
		ThreadConstantBufferColors.push_back(ThreadCBColors);

		// Create Material Constant Buffer
		D3D11_BUFFER_DESC MaterialConstantBufferDescription = {};
		MaterialConstantBufferDescription.ByteWidth = sizeof(FMaterial) + 0xf & 0xfffffff0;
		MaterialConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
		MaterialConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		MaterialConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		GetDevice()->CreateBuffer(&MaterialConstantBufferDescription, nullptr, &ThreadCBMaterials);
		ThreadConstantBufferMaterials.push_back(ThreadCBMaterials);

		// Create Material Constant Buffer
		D3D11_BUFFER_DESC ProjectionDecalConstantBufferDescription = {};
		ProjectionDecalConstantBufferDescription.ByteWidth = sizeof(FMaterial) + 0xf & 0xfffffff0;
		ProjectionDecalConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
		ProjectionDecalConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ProjectionDecalConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		GetDevice()->CreateBuffer(&ProjectionDecalConstantBufferDescription, nullptr, &ThreadCBProjectionDecals);
		ThreadConstantBufferMaterials.push_back(ThreadCBProjectionDecals);
	}
#endif
}

void URenderer::Release()
{
	ReleaseConstantBuffer();
	ReleaseDefaultShader();
	ReleaseDepthStencilState();
	ReleaseRasterizerState();
	ReleaseBillboardResources();
	ReleaseSpotlightResources();
	ReleaseTextureShader();
	ReleaseProjectionDecalShader();
	ReleaseFireBallShader();

  if (FXAA) { FXAA->Release(); SafeDelete(FXAA); }

	SafeDelete(ViewportClient);

	// FontRenderer 해제
	SafeDelete(FontRenderer);

	SafeDelete(Pipeline);
	SafeDelete(DeviceResources);

#ifdef MULTI_THREADING
	for (ID3D11DeviceContext* Context : DeferredContexts)
	{
		if (Context)
		{
			Context->Release();
		}
	}
	DeferredContexts.clear();

	for (ID3D11Buffer* Buffer : ThreadConstantBufferModels)
	{
		if (Buffer)
		{
			Buffer->Release();
		}
	}
	ThreadConstantBufferModels.clear();

	for (ID3D11Buffer* Buffer : ThreadConstantBufferColors)
	{
		if (Buffer)
		{
			Buffer->Release();
		}
	}
	ThreadConstantBufferColors.clear();

	for (ID3D11Buffer* Buffer : ThreadConstantBufferMaterials)
	{
		if (Buffer)
		{
			Buffer->Release();
		}
	}
	ThreadConstantBufferMaterials.clear();

	for (ID3D11Buffer* Buffer : ThreadConstantBufferProjectionDecals)
	{
		if (Buffer)
		{
			Buffer->Release();
		}
	}
	ThreadConstantBufferProjectionDecals.clear();

	for (ID3D11CommandList* CommandList : CommandLists)
	{
		if (CommandList)
		{
			CommandList->Release();
		}
	}
	CommandLists.clear();
#endif
}

/**
 * @brief Renderer에서 주로 사용할 Depth-Stencil State를 생성하는 함수
 */
void URenderer::CreateDepthStencilState()
{
	// 3D Default Depth Stencil 설정 (Depth 판정 O, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC DefaultDescription = {};

	DefaultDescription.DepthEnable = TRUE;
	DefaultDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DefaultDescription.DepthFunc = D3D11_COMPARISON_LESS;

	DefaultDescription.StencilEnable = FALSE;
	DefaultDescription.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DefaultDescription.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	GetDevice()->CreateDepthStencilState(&DefaultDescription, &DefaultDepthStencilState);

	// Disabled Depth Stencil 설정 (Depth 판정 X, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC DisabledDescription = {};

	DisabledDescription.DepthEnable = FALSE;
	DisabledDescription.StencilEnable = FALSE;

	GetDevice()->CreateDepthStencilState(&DisabledDescription, &DisabledDepthStencilState);

	// Disabled Depth Stencil 설정 (Depth 판정 O, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC ProjectionDecalDescription = {};

	ProjectionDecalDescription.DepthEnable = TRUE;
	ProjectionDecalDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 버퍼에 쓰지 않음
	ProjectionDecalDescription.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	ProjectionDecalDescription.StencilEnable = FALSE;

	GetDevice()->CreateDepthStencilState(&ProjectionDecalDescription, &DisableDepthWriteDepthStencilState);
}

/**
 * @brief Shader 기반의 CSO 생성 함수
 */
void URenderer::CreateDefaultShader()
{
	ID3DBlob* VertexShaderCSO;
	ID3DBlob* PixelShaderCSO;

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&VertexShaderCSO, nullptr);

	GetDevice()->CreateVertexShader(VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), nullptr, &DefaultVertexShader);

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&PixelShaderCSO, nullptr);

	GetDevice()->CreatePixelShader(PixelShaderCSO->GetBufferPointer(),
		PixelShaderCSO->GetBufferSize(), nullptr, &DefaultPixelShader);

	D3D11_INPUT_ELEMENT_DESC DefaultLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	GetDevice()->CreateInputLayout(DefaultLayout, ARRAYSIZE(DefaultLayout), VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), &DefaultInputLayout);

	Stride = sizeof(FNormalVertex);

	VertexShaderCSO->Release();
	PixelShaderCSO->Release();
}

void URenderer::CreateTextureShader()
{
	ID3DBlob* TextureVSBlob;
	ID3DBlob* TexturePSBlob;

	D3DCompileFromFile(L"Asset/Shader/TextureShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&TextureVSBlob, nullptr);

	GetDevice()->CreateVertexShader(TextureVSBlob->GetBufferPointer(),
		TextureVSBlob->GetBufferSize(), nullptr, &TextureVertexShader);

	D3DCompileFromFile(L"Asset/Shader/TextureShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&TexturePSBlob, nullptr);

	GetDevice()->CreatePixelShader(TexturePSBlob->GetBufferPointer(),
		TexturePSBlob->GetBufferSize(), nullptr, &TexturePixelShader);

	D3D11_INPUT_ELEMENT_DESC TextureLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	GetDevice()->CreateInputLayout(TextureLayout, ARRAYSIZE(TextureLayout), TextureVSBlob->GetBufferPointer(),
		TextureVSBlob->GetBufferSize(), &TextureInputLayout);

	// TODO(KHJ): ShaderBlob 파일로 저장하고, 이후 이미 존재하는 경우 컴파일 없이 Blob을 로드할 수 있도록 할 것
	// TODO(KHJ): 실제 텍스처용 셰이더를 별도로 생성해야 함 (UV 좌표 포함)

	TextureVSBlob->Release();
	TexturePSBlob->Release();
}

void URenderer::CreateProjectionDecalShader()
{
	ID3DBlob* ProjectionDecalVSBlob;
	ID3DBlob* ProjectionDecalPSBlob;

	D3DCompileFromFile(L"Asset/Shader/ProjectionDecal.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&ProjectionDecalVSBlob, nullptr);

	GetDevice()->CreateVertexShader(ProjectionDecalVSBlob->GetBufferPointer(),
		ProjectionDecalVSBlob->GetBufferSize(), nullptr, &ProjectionDecalVertexShader);

	D3DCompileFromFile(L"Asset/Shader/ProjectionDecal.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&ProjectionDecalPSBlob, nullptr);

	GetDevice()->CreatePixelShader(ProjectionDecalPSBlob->GetBufferPointer(),
		ProjectionDecalPSBlob->GetBufferSize(), nullptr, &ProjectionDecalPixelShader);

	D3D11_INPUT_ELEMENT_DESC ProjectionDecalLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	GetDevice()->CreateInputLayout(ProjectionDecalLayout, ARRAYSIZE(ProjectionDecalLayout), ProjectionDecalVSBlob->GetBufferPointer(),
		ProjectionDecalVSBlob->GetBufferSize(), &ProjectionDecalInputLayout);

	ProjectionDecalVSBlob->Release();
	ProjectionDecalPSBlob->Release();

	if (!ProjectionDecalBlendState)
	{
		D3D11_BLEND_DESC BlendDesc = {};
		BlendDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT hr = GetDevice()->CreateBlendState(&BlendDesc, &ProjectionDecalBlendState);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("Renderer: Failed to create projection decal blend state (HRESULT: 0x%08lX)", hr);
		}
	}
}

void URenderer::CreateSpotlightShader()
{
	ID3DBlob* SpotLightVSBlob;
	ID3DBlob* SpotLightPSBlob;

	D3DCompileFromFile(L"Asset/Shader/SpotlightShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&SpotLightVSBlob, nullptr);

	GetDevice()->CreateVertexShader(SpotLightVSBlob->GetBufferPointer(),
		SpotLightVSBlob->GetBufferSize(), nullptr, &SpotlightVertexShader);

	D3DCompileFromFile(L"Asset/Shader/SpotlightShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&SpotLightPSBlob, nullptr);

	GetDevice()->CreatePixelShader(SpotLightPSBlob->GetBufferPointer(),
		SpotLightPSBlob->GetBufferSize(), nullptr, &SpotlightPixelShader);

	D3D11_INPUT_ELEMENT_DESC SpotlightLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	GetDevice()->CreateInputLayout(SpotlightLayout, ARRAYSIZE(SpotlightLayout), SpotLightVSBlob->GetBufferPointer(),
		SpotLightVSBlob->GetBufferSize(), &SpotlightInputLayout);

	SpotLightVSBlob->Release();
	SpotLightPSBlob->Release();
}

void URenderer::CreateSceneDepthViewModeShader()
{
	ID3DBlob* SceneDepthVSBlob;
	ID3DBlob* SceneDepthPSBlob;

	D3DCompileFromFile(L"Asset/Shader/SceneDepthShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&SceneDepthVSBlob, nullptr);

	GetDevice()->CreateVertexShader(SceneDepthVSBlob->GetBufferPointer(),
		SceneDepthVSBlob->GetBufferSize(), nullptr, &SceneDepthVertexShader);

	D3DCompileFromFile(L"Asset/Shader/SceneDepthShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&SceneDepthPSBlob, nullptr);

	GetDevice()->CreatePixelShader(SceneDepthPSBlob->GetBufferPointer(),
		SceneDepthPSBlob->GetBufferSize(), nullptr, &SceneDepthPixelShader);

	SceneDepthVSBlob->Release();
	SceneDepthPSBlob->Release();
}

/**
 * @brief 래스터라이저 상태를 해제하는 함수
 */
void URenderer::ReleaseRasterizerState()
{
	for (auto& Cache : RasterCache)
	{
		SafeRelease(Cache.second);
	}
	RasterCache.clear();
}

void URenderer::ReleaseBillboardResources()
{
	SafeRelease(BillboardVertexBuffer);
	SafeRelease(BillboardIndexBuffer);
	SafeRelease(BillboardBlendState);
}

void URenderer::ReleaseSpotlightResources()
{
	SafeRelease(SpotlightVertexShader);
	SafeRelease(SpotlightPixelShader);
	SafeRelease(SpotlightInputLayout);
	SafeRelease(SpotlightBlendState);
}

void URenderer::ReleaseTextureShader()
{
	SafeRelease(TextureVertexShader);
	SafeRelease(TexturePixelShader);
	SafeRelease(TextureInputLayout);
}

void URenderer::ReleaseProjectionDecalShader()
{
	SafeRelease(ProjectionDecalInputLayout);
	SafeRelease(ProjectionDecalVertexShader);
	SafeRelease(ProjectionDecalPixelShader);
	SafeRelease(ProjectionDecalBlendState);
}

void URenderer::ReleaseSpotlightShader()
{
	SafeRelease(SpotlightVertexShader);
	SafeRelease(SpotlightInputLayout);
	SafeRelease(SpotlightBlendState);
	SafeRelease(SpotlightPixelShader);
}

void URenderer::ReleaseFireBallShader()
{
	SafeRelease(FireBallInputLayout);
	SafeRelease(FireBallVertexShader);
	SafeRelease(FireBallPixelShader);
	SafeRelease(FireBallBlendState);
}

void URenderer::ReleaseSceneDepthViewModeShader()
{
	SafeRelease(SceneDepthVertexShader);
	SafeRelease(SceneDepthPixelShader);
}

/**
 * @brief Shader Release
 */
void URenderer::ReleaseDefaultShader()
{
	SafeRelease(DefaultInputLayout);
	SafeRelease(DefaultPixelShader);
	SafeRelease(DefaultVertexShader);
}

void URenderer::ReleaseDepthStencilState()
{
	SafeRelease(DefaultDepthStencilState);
	SafeRelease(DisabledDepthStencilState);
	SafeRelease(DisableDepthWriteDepthStencilState);

	// 렌더 타겟을 초기화
	if (GetDeviceContext())
	{
		GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}
}

// Renderer.cpp
void URenderer::Tick(float DeltaSeconds)
{
	RenderBegin();
	// FViewportClient로부터 모든 뷰포트를 가져옵니다.
	for (FViewportClient& ViewportClient : ViewportClient->GetViewports())
	{
		// 0. 현재 뷰포트가 닫혀있다면 렌더링을 하지 않습니다.
		if (ViewportClient.GetViewportInfo().Width < 1.0f || ViewportClient.GetViewportInfo().Height < 1.0f) { continue; }

		// 1. 현재 뷰포트의 영역을 설정합니다.
		ViewportClient.Apply(GetDeviceContext());

		// 2. 현재 뷰포트의 카메라 정보를 가져옵니다.
		UCamera* CurrentCamera = &ViewportClient.Camera;

		// 3. 해당 카메라의 View/Projection 행렬로 상수 버퍼를 업데이트합니다.
		CurrentCamera->Update(ViewportClient.GetViewportInfo());
		Pipeline->SetConstantBuffer(1, true, ConstantBufferViewProj);
		UpdateConstant(ConstantBufferViewProj, CurrentCamera->GetFViewProjConstants(), 1, true, true);

		// 4. 씬(레벨, 에디터 요소 등)을 이 뷰포트와 카메라 기준으로 렌더링합니다.
		RenderLevel(CurrentCamera, ViewportClient);

		// 5. 에디터를 렌더링합니다.
		GEngine->GetEditor()->RenderEditor(*Pipeline, CurrentCamera);
	}

    // Apply post-process (FXAA) before UI so UI remains crisp
    if (bFXAAEnabled && FXAA
    	&& GEngine->GetEditor()->GetViewMode() != EViewModeIndex::VMI_SceneDepth
    	&& GEngine->GetEditor()->GetViewMode() != EViewModeIndex::VMI_SceneDepth2D)
    {
        FXAA->Apply(Pipeline, ViewportClient, DisabledDepthStencilState, ClearColor);
    }

	// 최상위 에디터/GUI는 프레임에 1회만
	UUIManager::GetInstance().Render();
	UStatOverlay::GetInstance().Render();

	RenderEnd(); // Present 1회
}


/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin() const
{
    // Render scene into FXAA scene color target when enabled and not in SceneDepth view mode
    const bool bUseFXAAPath = bFXAAEnabled && FXAA && (GEngine->GetEditor()->GetViewMode() != EViewModeIndex::VMI_SceneDepth);
    ID3D11RenderTargetView* RenderTargetView = bUseFXAAPath ? FXAA->GetSceneRTV() : DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);

	if (GEngine->GetEditor()->GetViewMode() == EViewModeIndex::VMI_SceneDepth)
	{
		float DepthClearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		GetDeviceContext()->ClearRenderTargetView(DeviceResources->GetRenderTargetView(), DepthClearColor);
	}

	auto* DepthStencilView = DeviceResources->GetDepthStencilView();
	GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView* rtvs[] = { RenderTargetView }; // 배열 생성

	GetDeviceContext()->OMSetRenderTargets(1, rtvs, DeviceResources->GetDepthStencilView());
	DeviceResources->UpdateViewport();
}

/**
 * @brief Buffer에 데이터 입력 및 Draw
 */
void URenderer::RenderLevel(UCamera* InCurrentCamera, FViewportClient& InViewportClient)
{
	if (!GEngine->GetCurrentLevel())
	{
		return;
	}

	const auto PrimitiveComponents = GEngine->GetCurrentLevel()->GetVisiblePrimitiveComponents(InCurrentCamera);

	// ImGui 창 옆 키고 끌 수 있는 메뉴 넣기
	if (bOcclusionCulling)
	{
		PerformOcclusionCulling(InCurrentCamera, PrimitiveComponents);
	}

#ifdef MULTI_THREADING
	RenderLevel_MultiThreaded(InCurrentCamera, InViewportClient, PrimitiveComponents);
#else
	RenderLevel_SingleThreaded(InCurrentCamera, InViewportClient, PrimitiveComponents);
#endif
}

void URenderer::PerformOcclusionCulling(UCamera* InCurrentCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents)
{
	auto& OcclusionRenderer = UOcclusionRenderer::GetInstance();

	if (bIsFirstPass)
	{
		//bIsFirstPass = false;
		return;
	}

	GetDeviceContext()->Flush();

	PROFILE_SCOPE("GenerateHiZ",
		OcclusionRenderer.GenerateHiZ(GetDevice(), GetDeviceContext(), DeviceResources->GetPreviousFrameDepthSRV())
	);

	PROFILE_SCOPE("BuildScreenSpaceBoundingVolumes",
		OcclusionRenderer.BuildScreenSpaceBoundingVolumes(GetDeviceContext(), InCurrentCamera, InPrimitiveComponents)
	);

	PROFILE_SCOPE("OcclusionTest",
		OcclusionRenderer.OcclusionTest(GetDevice(), GetDeviceContext())
	);

	ID3D11RenderTargetView* RTV = (bFXAAEnabled && FXAA && (GEngine->GetEditor()->GetViewMode() != EViewModeIndex::VMI_SceneDepth) && FXAA->GetSceneRTV()) ? FXAA->GetSceneRTV() : DeviceResources->GetRenderTargetView();
	GetDeviceContext()->OMSetRenderTargets(1, &RTV, DeviceResources->GetDepthStencilView());
}

void URenderer::RenderPrimitiveComponent(UPipeline& InPipeline, UPrimitiveComponent* InPrimitiveComponent, ID3D11RasterizerState* InRasterizerState, ID3D11Buffer* InConstantBufferModels, ID3D11Buffer* InConstantBufferColor, ID3D11Buffer* InConstantBufferMaterial)
{
	switch (InPrimitiveComponent->GetPrimitiveType())
	{
	case EPrimitiveType::TextRender:
	case EPrimitiveType::Billboard:
		// Billboards and text are rendered on the main thread after all other primitives
		break;
	case EPrimitiveType::Decal:
		RenderPrimitiveLine(InPipeline, InPrimitiveComponent, InRasterizerState, InConstantBufferModels, InConstantBufferColor);
		break;
	case EPrimitiveType::Spotlight:
		RenderPrimitiveLine(InPipeline, InPrimitiveComponent, InRasterizerState, InConstantBufferModels, InConstantBufferColor);
		break;
	case EPrimitiveType::StaticMesh:
		RenderStaticMesh(InPipeline, Cast<UStaticMeshComponent>(InPrimitiveComponent), InRasterizerState, InConstantBufferModels, InConstantBufferMaterial);
		break;
	default:
		RenderPrimitiveDefault(InPipeline, InPrimitiveComponent, InRasterizerState, InConstantBufferModels, InConstantBufferColor);
		break;
	}
}

void URenderer::RenderLevel_SingleThreaded(UCamera* InCurrentCamera, FViewportClient& InViewportClient, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents)
{
	auto& OcclusionRenderer = UOcclusionRenderer::GetInstance();
	TObjectPtr<UTextRenderComponent> TextRender = nullptr;
	TArray<TObjectPtr<UBillboardComponent>> Billboards;
	TArray<TObjectPtr<UDecalComponent>> Decals;
	TArray<TObjectPtr<USpotLightComponent>> SpotLights;
	TArray<TObjectPtr<UPrimitiveComponent>> PrimitivesToPostUpdate;
	TArray<TObjectPtr<UFireBallComponent>> FireBalls;

	for (size_t i = 0; i < InPrimitiveComponents.size(); ++i)
	{
		auto PrimitiveComponent = InPrimitiveComponents[i];

		if (!PrimitiveComponent || !PrimitiveComponent->IsVisible() || !OcclusionRenderer.IsPrimitiveVisible(PrimitiveComponent))
		{
			continue;
		}

		FRenderState RenderState = PrimitiveComponent->GetRenderState();
		const EViewModeIndex ViewMode = GEngine->GetEditor()->GetViewMode();
		if (ViewMode == EViewModeIndex::VMI_Wireframe)
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}
		ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

		if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::Decal)
		{
			Decals.push_back(Cast<UDecalComponent>(PrimitiveComponent));
		}
		if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::Spotlight)
		{
			SpotLights.push_back(Cast<USpotLightComponent>(PrimitiveComponent));
		}
		else if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::TextRender)
		{
			TextRender = Cast<UTextRenderComponent>(PrimitiveComponent);
		}
		else if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::Billboard)
		{
			Billboards.push_back(Cast<UBillboardComponent>(PrimitiveComponent));
		}
		else if (PrimitiveComponent->IsA(UDecalComponent::StaticClass()))
		{
			Decals.push_back(Cast<UDecalComponent>(PrimitiveComponent));
		}
		else if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::FireBall)
		{
			FireBalls.push_back(Cast<UFireBallComponent>(PrimitiveComponent));
		}
		else
		{
			PrimitivesToPostUpdate.push_back(PrimitiveComponent);
		}
	}

	// 1. 먼저 일반 프리미티브들(StaticMesh 등)을 렌더링합니다.
	for (UPrimitiveComponent* PrimitiveComponent : PrimitivesToPostUpdate)
	{
		FRenderState RenderState = PrimitiveComponent->GetRenderState();
		const EViewModeIndex ViewMode = GEngine->GetEditor()->GetViewMode();
		if (ViewMode == EViewModeIndex::VMI_Wireframe)
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}
		ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

		RenderPrimitiveComponent(*Pipeline, PrimitiveComponent, LoadedRasterizerState, ConstantBufferModels, ConstantBufferColor, ConstantBufferMaterial);
	}

	// 2. 데칼의 시각용 큐브 라인을 렌더링합니다.
	if (GEngine->GetCurrentLevel()->GetOwningWorld()->GetWorldType() == EWorldType::Editor)
	{
		for (UPrimitiveComponent* PrimitiveComponent : Decals)
		{
			FRenderState RenderState = PrimitiveComponent->GetRenderState();
			ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

			RenderPrimitiveComponent(*Pipeline, PrimitiveComponent, LoadedRasterizerState, ConstantBufferModels, ConstantBufferColor, ConstantBufferMaterial);
		}
	}

	for (UPrimitiveComponent* PrimitiveComponent : SpotLights)
	{
		FRenderState RenderState = PrimitiveComponent->GetRenderState();
		ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

		RenderPrimitiveComponent(*Pipeline, PrimitiveComponent, LoadedRasterizerState, ConstantBufferModels, ConstantBufferColor, ConstantBufferMaterial);
	}

	// 3. 그 다음, 렌더링된 프리미티브 위에 데칼을 렌더링합니다.
	RenderDecals(InCurrentCamera, Decals, PrimitivesToPostUpdate);
	RenderLights(InCurrentCamera, SpotLights, PrimitivesToPostUpdate);
	RenderFireBalls(InCurrentCamera, FireBalls, PrimitivesToPostUpdate);

	if (GEngine->GetEditor()->GetViewMode() == EViewModeIndex::VMI_SceneDepth2D)
	{
		RenderSceneDepthView(InCurrentCamera, InViewportClient);
	}
	else if (GEngine->GetEditor()->GetViewMode() != EViewModeIndex::VMI_SceneDepth)
	{
		RenderDecals(InCurrentCamera, Decals, PrimitivesToPostUpdate);
		RenderLights(InCurrentCamera, SpotLights, PrimitivesToPostUpdate);
	}
	for (TObjectPtr<UBillboardComponent> BillboardComponent : Billboards)
	{
		if (BillboardComponent)
		{
			RenderBillboard(BillboardComponent.Get(), InCurrentCamera);
		}
	}

	if (TextRender)
	{
		RenderText(TextRender, InCurrentCamera);
	}
}

void URenderer::RenderLevel_MultiThreaded(UCamera* InCurrentCamera, FViewportClient& InViewportClient, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents)
{
	static ThreadPool Pool(NUM_WORKER_THREADS);

	const size_t NumPrimitives = InPrimitiveComponents.size();
	const size_t ChunkSize = (NumPrimitives + NUM_WORKER_THREADS - 1) / NUM_WORKER_THREADS;

	CommandLists.clear();
	CommandLists.resize(NUM_WORKER_THREADS, nullptr);

	std::vector<std::future<void>> Futures;
	for (size_t i = 0; i < NUM_WORKER_THREADS; ++i)
	{
		const size_t StartIndex = i * ChunkSize;
		const size_t EndIndex = std::min(StartIndex + ChunkSize, NumPrimitives);

		if (StartIndex >= EndIndex) continue;

		Futures.emplace_back(Pool.Enqueue([this, StartIndex, EndIndex, &InPrimitiveComponents, i, &InViewportClient]()
		{
			ID3D11DeviceContext* DeferredContext = DeferredContexts[i];
			if (!DeferredContext)
			{
				UE_LOG("DeferredContext is null in worker thread %d! Skipping rendering for this chunk.", i);
				return;
			}
			UPipeline ThreadPipeline(DeferredContext);
			InViewportClient.Apply(DeferredContext);

			auto* RTV = (bFXAAEnabled && FXAA && (GEngine->GetEditor()->GetViewMode() != EViewModeIndex::VMI_SceneDepth) && FXAA->GetSceneRTV()) ? FXAA->GetSceneRTV() : DeviceResources->GetRenderTargetView();
			auto* DSV = DeviceResources->GetDepthStencilView();
			DeferredContext->OMSetRenderTargets(1, &RTV, DSV);
			ThreadPipeline.SetConstantBuffer(1, true, ConstantBufferViewProj);

			ID3D11Buffer* ThreadCBModels = ThreadConstantBufferModels[i];
			ID3D11Buffer* ThreadCBColors = ThreadConstantBufferColors[i];
			ID3D11Buffer* ThreadCBMaterials = ThreadConstantBufferMaterials[i];

			for (size_t j = StartIndex; j < EndIndex; ++j)
			{
				UPrimitiveComponent* PrimitiveComponent = InPrimitiveComponents[j];
				if (!PrimitiveComponent || !PrimitiveComponent->IsVisible() || !UOcclusionRenderer::GetInstance().IsPrimitiveVisible(PrimitiveComponent))
				{
					continue;
				}

				FRenderState RenderState = PrimitiveComponent->GetRenderState();
				const EViewModeIndex ViewMode = GEngine->GetEditor()->GetViewMode();
				if (ViewMode == EViewModeIndex::VMI_Wireframe)
				{
					RenderState.CullMode = ECullMode::None;
					RenderState.FillMode = EFillMode::WireFrame;
				}
				ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

				RenderPrimitiveComponent(ThreadPipeline, PrimitiveComponent, LoadedRasterizerState, ThreadCBModels, ThreadCBColors, ThreadCBMaterials);
			}
			DeferredContext->FinishCommandList(FALSE, &CommandLists[i]);
		}));
	}

	for (auto& Future : Futures)
	{
		Future.get();
	}

	for (ID3D11CommandList* CommandList : CommandLists)
	{
		if (CommandList)
		{
			GetDeviceContext()->ExecuteCommandList(CommandList, TRUE);
			CommandList->Release();
		}
	}
	CommandLists.clear();

	TObjectPtr<UTextRenderComponent> TextRender = nullptr;
	TArray<TObjectPtr<UBillboardComponent>> Billboards;
	auto& OcclusionRenderer = UOcclusionRenderer::GetInstance();

	for (size_t i = 0; i < InPrimitiveComponents.size(); ++i)
	{
		UPrimitiveComponent* PrimitiveComponent = InPrimitiveComponents[i];
		if (!PrimitiveComponent || !PrimitiveComponent->IsVisible() || !OcclusionRenderer.IsPrimitiveVisible(PrimitiveComponent))
		{
			continue;
		}

		if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::TextRender)
		{
			// Render the last text component found (they typically represent selection info)
			TextRender = Cast<UTextRenderComponent>(PrimitiveComponent);
		}
		else if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::Billboard)
		{
			Billboards.push_back(TObjectPtr(Cast<UBillboardComponent>(PrimitiveComponent)));
		}
	}

	for (TObjectPtr<UBillboardComponent> BillboardComponent : Billboards)
	{
		if (BillboardComponent)
		{
			RenderBillboard(BillboardComponent.Get(), InCurrentCamera);
		}
	}

	if (TextRender)
	{
		RenderText(TextRender, InCurrentCamera);
	}
}

/**
 * @brief Editor용 Primitive를 렌더링하는 함수 (Gizmo, Axis 등)
 * @param InEditorPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 */
void URenderer::RenderEditorPrimitive(UPipeline& InPipeline, const FEditorPrimitive& InEditorPrimitive, const FRenderState& InRenderState)
{
    // Always visible 옵션에 따라 Depth 테스트 여부 결정
    ID3D11DepthStencilState* DepthStencilState =
        InEditorPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState;

    ID3D11RasterizerState* RasterizerState = GetRasterizerState(InRenderState);

    // Pipeline 정보 구성
    FPipelineInfo PipelineInfo = {
        DefaultInputLayout,
        DefaultVertexShader,
        RasterizerState,
        DepthStencilState,
        DefaultPixelShader,
        nullptr,
        InEditorPrimitive.Topology
    };

    InPipeline.UpdatePipeline(PipelineInfo);

    // Update constant buffers
	FMatrix ModelMatrix = FMatrix::GetModelMatrix(InEditorPrimitive.Location, FVector::GetDegreeToRadian(InEditorPrimitive.Rotation), InEditorPrimitive.Scale);
	UpdateConstant(ConstantBufferModels, ModelMatrix, 0, true, false);

	UpdateConstant(ConstantBufferColor, InEditorPrimitive.Color, 2, false, true);

    // Set vertex buffer and draw
    InPipeline.SetVertexBuffer(InEditorPrimitive.Vertexbuffer, Stride);
    InPipeline.Draw(InEditorPrimitive.NumVertices, 0);
}
/**
 * @brief Index Buffer를 사용하는 Editor Primitive 렌더링 함수
 * @param InEditorPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 * @param bInUseBaseConstantBuffer 기본 상수 버퍼 사용 여부
 * @param InStride 정점 스트라이드
 * @param InIndexBufferStride 인덱스 버퍼 스트라이드
 */
void URenderer::RenderEditorPrimitiveIndexed(UPipeline& InPipeline, const FEditorPrimitive& InEditorPrimitive, const FRenderState& InRenderState,
    bool bInUseBaseConstantBuffer, uint32 InStride, uint32 InIndexBufferStride)
{
    // Always visible 옵션에 따라 Depth 테스트 여부 결정
    ID3D11DepthStencilState* DepthStencilState =
        InEditorPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState;

    ID3D11RasterizerState* RasterizerState = GetRasterizerState(InRenderState);

    // 커스텀 셰이더가 있으면 사용, 없으면 기본 셰이더 사용
    ID3D11InputLayout* InputLayout = InEditorPrimitive.InputLayout ? InEditorPrimitive.InputLayout : DefaultInputLayout;
    ID3D11VertexShader* VertexShader = InEditorPrimitive.VertexShader ? InEditorPrimitive.VertexShader : DefaultVertexShader;
    ID3D11PixelShader* PixelShader = InEditorPrimitive.PixelShader ? InEditorPrimitive.PixelShader : DefaultPixelShader;

    // Pipeline 정보 구성
    FPipelineInfo PipelineInfo = {
        InputLayout,
        VertexShader,
        RasterizerState,
        DepthStencilState,
        PixelShader,
        nullptr,
        InEditorPrimitive.Topology
    };

    InPipeline.UpdatePipeline(PipelineInfo);

    // 기본 상수 버퍼 사용하는 경우에만 업데이트
	if (bInUseBaseConstantBuffer)
	{
		FMatrix ModelMatrix = FMatrix::GetModelMatrix(InEditorPrimitive.Location, FVector::GetDegreeToRadian(InEditorPrimitive.Rotation), InEditorPrimitive.Scale);
		UpdateConstant(ConstantBufferModels, ModelMatrix, 0, true, false);

		UpdateConstant(ConstantBufferColor, InEditorPrimitive.Color, 2, false, true);
	}

    // Set buffers and draw indexed
    InPipeline.SetIndexBuffer(InEditorPrimitive.IndexBuffer, InIndexBufferStride);
    InPipeline.SetVertexBuffer(InEditorPrimitive.Vertexbuffer, InStride);
    InPipeline.DrawIndexed(InEditorPrimitive.NumIndices, 0, 0);
}

void URenderer::RenderDecals(UCamera* InCurrentCamera, const TArray<TObjectPtr<UDecalComponent>>& InDecals,
	const TArray<TObjectPtr<UPrimitiveComponent>>& InVisiblePrimitives)
{
	TIME_PROFILE(Decal)

	// 0. 데칼이 없으면 함수를 종료합니다.
	if (InDecals.empty()) { return; }

    // Helper to quickly test if decal OBB projects inside the view. Conservative and cheap.
    auto IsDecalRoughlyOnScreen = [&](const FOBB& OBB) -> bool
    {
        const FViewProjConstants& VP = InCurrentCamera->GetFViewProjConstants();
        FMatrix ViewProj = VP.View * VP.Projection;

        // OBB axes (world space)
        const FVector ax(OBB.Orientation.Data[0][0], OBB.Orientation.Data[0][1], OBB.Orientation.Data[0][2]);
        const FVector ay(OBB.Orientation.Data[1][0], OBB.Orientation.Data[1][1], OBB.Orientation.Data[1][2]);
        const FVector az(OBB.Orientation.Data[2][0], OBB.Orientation.Data[2][1], OBB.Orientation.Data[2][2]);

        const FVector e = OBB.Extents;

        FVector corners[8];
        int ci = 0;
        for (int sx : { -1, 1 })
        for (int sy : { -1, 1 })
        for (int sz : { -1, 1 })
        {
            corners[ci++] = OBB.Center + ax * (e.X * (float)sx) + ay * (e.Y * (float)sy) + az * (e.Z * (float)sz);
        }

        float ndcMinX = +FLT_MAX, ndcMinY = +FLT_MAX;
        float ndcMaxX = -FLT_MAX, ndcMaxY = -FLT_MAX;
        bool anyInFront = false;

        for (int i = 0; i < 8; ++i)
        {
            FVector4 p = FVector4(corners[i].X, corners[i].Y, corners[i].Z, 1.0f);
            p = p * ViewProj;
            if (p.W <= 0.0f) { continue; }
            anyInFront = true;
            const float invW = 1.0f / p.W;
            const float x = p.X * invW;
            const float y = p.Y * invW;
            ndcMinX = std::min(ndcMinX, x);
            ndcMaxX = std::max(ndcMaxX, x);
            ndcMinY = std::min(ndcMinY, y);
            ndcMaxY = std::max(ndcMaxY, y);
        }

        if (!anyInFront) return false; // completely behind camera

        // Test overlap with the NDC clip rectangle [-1,1]^2
        if (ndcMaxX < -1.0f || ndcMinX > 1.0f) return false;
        if (ndcMaxY < -1.0f || ndcMinY > 1.0f) return false;
        return true;
    };

	// 1. 파이프라인을 데칼 렌더링용으로 설정합니다.
	FRenderState DecalRenderState = {};
	DecalRenderState.CullMode = ECullMode::Back;
	DecalRenderState.FillMode = EFillMode::Solid;

	FPipelineInfo PipelineInfo = {
		ProjectionDecalInputLayout,
		ProjectionDecalVertexShader,
		GetRasterizerState(DecalRenderState),
		DisableDepthWriteDepthStencilState,
		ProjectionDecalPixelShader,
		ProjectionDecalBlendState,
	};

	// 2. 모든 데칼 컴포넌트에 대해 반복합니다.
	for (UDecalComponent* Decal : InDecals)
	{
		// 데칼이 보이지 않거나 바운딩 볼륨이 없으면 건너뜁니다.
		if (!Decal) { continue; }

		UMaterial* DecalMaterial = Decal->GetDecalMaterial();
		if (DecalMaterial == nullptr || DecalMaterial->GetDiffuseTexture() == nullptr) { continue; }

		if (auto* Proxy = DecalMaterial->GetDiffuseTexture()->GetRenderProxy())
		{
			Pipeline->SetTexture(0, false, Proxy->GetSRV());
			Pipeline->SetSamplerState(0, false, Proxy->GetSampler());
		}
		Pipeline->UpdatePipeline(PipelineInfo);

		// 3. 데칼의 월드 변환 역행렬을 계산하여 셰이더로 전달합니다.
		FDecalConstants DecalData(Decal->GetWorldTransformMatrix(), Decal->GetWorldTransformMatrixInverse(),
			Decal->GetFadeAlpha(), Decal->GetFadeStyle());
		UpdateConstant(ConstantBufferProjectionDecal, DecalData, 3, true, true);


		// 데칼의 바운딩 볼륨을 가져옵니다.
		const FOBB* DecalBounds = Decal->GetProjectionBox();

        // Quick off-screen reject for decal to avoid BVH query cost
        if (!DecalBounds || !IsDecalRoughlyOnScreen(*DecalBounds))
        {
            continue;
        }

		// 4. 화면에 보이는 모든 프리미티브와 데칼 볼륨의 충돌 검사를 수행합니다.
		TArray<UPrimitiveComponent*> HitComponents;
		UBVHierarchy::GetInstance().CheckOBBoxCollision(*DecalBounds, HitComponents);
		for (UPrimitiveComponent* Primitive : HitComponents)
		{
			// 데칼 액터의 시각화 컴포넌트에는 데칼을 적용하지 않도록 예외 처리합니다.
			if (!Primitive || !Primitive->GetBoundingBox()) { continue; }
			if (Primitive->IsA(UDecalComponent::StaticClass())) { continue; }
			if (Primitive->IsA(USpotLightComponent::StaticClass())) { continue; }

			// 5. 교차하는 프리미티브를 데칼 셰이더로 다시 그립니다.
			FModelConstants ModelConstants(Primitive->GetWorldTransformMatrix(),
				Primitive->GetWorldTransformMatrixInverse().Transpose());
			UpdateConstant(ConstantBufferModels, ModelConstants, 0, true, false);

			// 프리미티브의 버텍스/인덱스 버퍼를 설정합니다.
			Pipeline->SetVertexBuffer(Primitive->GetVertexBuffer(), sizeof(FNormalVertex));
			if (Primitive->GetIndexBuffer() && Primitive->GetNumIndices() > 0)
			{
				Pipeline->SetIndexBuffer(Primitive->GetIndexBuffer(), 0);
				Pipeline->DrawIndexed(Primitive->GetNumIndices(), 0, 0);
			}
			else
			{
				Pipeline->Draw(Primitive->GetNumVertices(), 0);
			}
		}
	}
}

void URenderer::RenderLights(UCamera* InCurrentCamera, const TArray<TObjectPtr<USpotLightComponent>>& InSpotlights,
	const TArray<TObjectPtr<UPrimitiveComponent>>& InVisiblePrimitives)
{
	if (InSpotlights.empty()) { return; }

	// 1. 파이프라인을 light용으로 수정
	FRenderState LightRenderState = {};
	LightRenderState.CullMode = ECullMode::Back;
	LightRenderState.FillMode = EFillMode::Solid;

	FPipelineInfo PipelineInfo = {
		SpotlightInputLayout, // just Spotlight for now
		SpotlightVertexShader,
		GetRasterizerState(LightRenderState),
		DisableDepthWriteDepthStencilState,
		SpotlightPixelShader,
		SpotlightBlendState,
	};

	for (USpotLightComponent* Light : InSpotlights)
	{
		if (!Light) { continue; }

		Pipeline->UpdatePipeline(PipelineInfo);

		// 월드 변환 역행렬을 계산하여 셰이더로 전달
		FLightConstants LightData(Light->GetWorldTransformMatrix(), Light->GetWorldTransformMatrixInverse());
		UpdateConstant(ConstantBufferSpotlight, LightData, 3, true, true);
		FVector4 LightColor = Light->GetLightColor();
		UpdateConstant(ConstantBufferColor, LightColor, 2, true, true);

		// 데칼의 바운딩 볼륨을 가져옵니다.
		const IBoundingVolume* LightBounds = Light->GetBoundingBox();

		// 4. 화면에 보이는 모든 프리미티브와 데칼 볼륨의 충돌 검사를 수행합니다.
		for (UPrimitiveComponent* Primitive : InVisiblePrimitives)
		{
			if (!Primitive || !Primitive->GetBoundingBox()) { continue; }

			// 데칼 액터의 시각화 컴포넌트에는 데칼을 적용하지 않도록 예외 처리합니다.
			if (Primitive == Light) { continue; }

			// AABB(Axis-Aligned Bounding Box) 교차 검사
			if (LightBounds->Intersects(*Primitive->GetBoundingBox()))
			{
				// 5. 교차하는 프리미티브를 데칼 셰이더로 다시 그립니다.
				FModelConstants ModelConstants(Primitive->GetWorldTransformMatrix(),
					Primitive->GetWorldTransformMatrixInverse().Transpose());
				UpdateConstant(ConstantBufferModels, ModelConstants, 0, true, false);

				// 프리미티브의 버텍스/인덱스 버퍼를 설정합니다.
				Pipeline->SetVertexBuffer(Primitive->GetVertexBuffer(), sizeof(FNormalVertex));
				if (Primitive->GetIndexBuffer() && Primitive->GetNumIndices() > 0)
				{
					Pipeline->SetIndexBuffer(Primitive->GetIndexBuffer(), 0);
					Pipeline->DrawIndexed(Primitive->GetNumIndices(), 0, 0);
				}
				else
				{
					Pipeline->Draw(Primitive->GetNumVertices(), 0);
				}
			}
		}
	}
}

void URenderer::RenderFireBalls(UCamera* InCurrentCamera, const TArray<TObjectPtr<UFireBallComponent>>& InFireBalls, const TArray<TObjectPtr<UPrimitiveComponent>>& InVisiblePrimitives)
{
	if (InFireBalls.empty()) { return; }

	// 1. 파이프라인을 FireBall 렌더링용으로 설정
	FRenderState FireBallRenderState = {};
	FireBallRenderState.CullMode = ECullMode::Back;
	FireBallRenderState.FillMode = EFillMode::Solid;

	FPipelineInfo PipelineInfo = {
		FireBallInputLayout,
		FireBallVertexShader,
		GetRasterizerState(FireBallRenderState),
		DisableDepthWriteDepthStencilState,
		FireBallPixelShader,
		FireBallBlendState, // Additive Blending
	};

	// 2. 모든 파이어볼 컴포넌트에 대해 반복
	for (UFireBallComponent* FireBall : InFireBalls)
	{
		if (!FireBall) { continue; }

		Pipeline->UpdatePipeline(PipelineInfo);

		// 3. 파이어볼 데이터를 셰이더로 전달
		FFireBallConstants FireBallData;
		const FVector& WorldLocation = FireBall->GetWorldLocation();
		FireBallData.WorldPosition = { WorldLocation.X, WorldLocation.Y, WorldLocation.Z };

		FireBallData.Radius = FireBall->GetRadius();
		FireBallData.Color = FireBall->GetLinearColor().Color;
		FireBallData.Intensity = FireBall->GetIntensity();
		FireBallData.RadiusFallOff = FireBall->GetRadiusFallOff();
		UpdateConstant(ConstantBufferFireBall, FireBallData, 4, true, true);

		const IBoundingVolume* FireBallBounds = FireBall->GetBoundingBox();

		// 4. 파이어볼 경계 볼륨과 교차하는 프리미티브들을 다시 렌더링
		for (UPrimitiveComponent* Primitive : InVisiblePrimitives)
		{
			if (!Primitive || !Primitive->GetBoundingBox() || Primitive->IsA(UDecalComponent::StaticClass())) { continue; }

			if (FireBallBounds->Intersects(*Primitive->GetBoundingBox()))
			{
				// 5. 교차하는 프리미티브를 파이어볼 셰이더로 다시 그립니다.
				FModelConstants ModelConstants(Primitive->GetWorldTransformMatrix(),
					Primitive->GetWorldTransformMatrixInverse().Transpose());
				UpdateConstant(ConstantBufferModels, ModelConstants, 0, true, false);

				Pipeline->SetVertexBuffer(Primitive->GetVertexBuffer(), sizeof(FNormalVertex));
				if (Primitive->GetIndexBuffer() && Primitive->GetNumIndices() > 0)
				{
					Pipeline->SetIndexBuffer(Primitive->GetIndexBuffer(), 0);
					Pipeline->DrawIndexed(Primitive->GetNumIndices(), 0, 0);
				}
				else
				{
					Pipeline->Draw(Primitive->GetNumVertices(), 0);
				}
			}
		}
	}
}

void URenderer::RenderSceneDepthView(UCamera* InCurrentCamera, const FViewportClient& InViewportClient)
{
	FRenderState SceneDepthState = {};
	SceneDepthState.CullMode = ECullMode::Front;
	SceneDepthState.FillMode = EFillMode::Solid;

	// Pipeline 정보 구성
	FPipelineInfo PipelineInfo = {
		nullptr,
		SceneDepthVertexShader,
		GetRasterizerState(SceneDepthState),
		DisabledDepthStencilState,
		SceneDepthPixelShader,
		nullptr,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	// constant buffer update
	FDepthConstants2D SceneDepthData{};

	// 1) InvViewProj 계산
	UCamera* Cam = InCurrentCamera;
	const FViewProjConstants& VP = Cam->GetFViewProjConstants();
	FMatrix View      = VP.View;
	FMatrix Proj      = VP.Projection;
	FMatrix ViewProj  = View * Proj;
	FMatrix InvVP     = ViewProj.Inverse();

	SceneDepthData.InvViewProj = InvVP;
	const FVector CameraPos = Cam->GetLocation();
	SceneDepthData.CameraPosWSAndNear = FVector4(CameraPos.X, CameraPos.Y, CameraPos.Z, Cam->GetNearZ());
	SceneDepthData.FarAndPadding = FVector4(Cam->GetFarZ(), 0.0f, 0.0f, 0.0f);

	const D3D11_VIEWPORT& FullViewport = DeviceResources->GetViewportInfo();
	const D3D11_VIEWPORT& SubViewport  = InViewportClient.GetViewportInfo();

	const float InvFullWidth  = FullViewport.Width  > 0.0f ? 1.0f / FullViewport.Width  : 0.0f;
	const float InvFullHeight = FullViewport.Height > 0.0f ? 1.0f / FullViewport.Height : 0.0f;

	SceneDepthData.ViewportRect = FVector4(
		SubViewport.TopLeftX * InvFullWidth,
		SubViewport.TopLeftY * InvFullHeight,
		SubViewport.Width    * InvFullWidth,
		SubViewport.Height   * InvFullHeight
	);

	UpdateConstant(ConstantBufferDepth2D, SceneDepthData, 0, true, true);

	ID3D11RenderTargetView* CurrentRTV = DeviceResources->GetRenderTargetView();
	ID3D11DepthStencilView* CurrentDSV = DeviceResources->GetDepthStencilView();

	ID3D11RenderTargetView* Targets[1] = { CurrentRTV };
	GetDeviceContext()->OMSetRenderTargets(1, Targets, nullptr);

	InViewportClient.Apply(GetDeviceContext());

	// Bind SRV, Sampler
	ID3D11ShaderResourceView* depthSRV = DeviceResources->GetDetphShaderResourceView();

	Pipeline->SetTexture(0, false, depthSRV);
	Pipeline->SetSamplerState(0, false, DeviceResources->GetDepthSamplerState());

	Pipeline->SetVertexBuffer(nullptr, 0);
	Pipeline->SetIndexBuffer(nullptr, 0);

	Pipeline->Draw(3, 0);

	//  depth SRV 언바인드
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV);

	Targets[0] = CurrentRTV;
	GetDeviceContext()->OMSetRenderTargets(1, Targets, CurrentDSV);
}

/**
 * @brief 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
 */
void URenderer::RenderEnd() const
{
	//DeviceResources->CopyDepthSRVToPreviousFrameSRV();

	GetSwapChain()->Present(0, 0); // 1: VSync 활성화
}

void URenderer::RenderStaticMesh(UPipeline& InPipeline, UStaticMeshComponent* InMeshComp, ID3D11RasterizerState* InRasterizerState, ID3D11Buffer* InConstantBufferModels, ID3D11Buffer* InConstantBufferMaterial)
{
    if (!InMeshComp || !InMeshComp->GetStaticMesh()) return;

    FStaticMesh* MeshAsset = InMeshComp->GetStaticMesh()->GetStaticMeshAsset();
    if (!MeshAsset)    return;

    // Pipeline setting
    FPipelineInfo PipelineInfo = {
        TextureInputLayout,
        TextureVertexShader,
        InRasterizerState,
        DefaultDepthStencilState,
        TexturePixelShader,
        nullptr,
    };
    InPipeline.UpdatePipeline(PipelineInfo);

	UpdateConstant(InConstantBufferModels, InMeshComp->GetWorldTransformMatrix(), 0, true, false);

    {
        // Use user-defined component color; apply a lightweight highlight if selected (render-time only).
        const bool bIsSelected = (GEngine->GetCurrentLevel() && InMeshComp->GetOwner() == GEngine->GetCurrentLevel()->GetSelectedActor());
        FVector4 C = InMeshComp->GetColor();
        if (bIsSelected)
        {
            C.X *= 1.2f; C.Y *= 1.2f; C.Z *= 1.2f; // simple brighten
        }
        // Bind as DebugParams.TotalColor (b3) for TextureShader tint and as debug params for depth viz
        const bool bIsDepthView = (GEngine->GetEditor()->GetViewMode() == EViewModeIndex::VMI_SceneDepth);
        float Color[4] = { C.X, C.Y, C.Z, C.W };
        FDepthConstants DepthData(bIsDepthView ? 1u : 0u, 0.1f, 500.0f, 0.8f, Color);
        UpdateConstant(ConstantBufferDepth, DepthData, 3, true, true);
    }

    InPipeline.SetVertexBuffer(InMeshComp->GetVertexBuffer(), sizeof(FNormalVertex));
    InPipeline.SetIndexBuffer(InMeshComp->GetIndexBuffer(), 0);

    // If no material is assigned, render the entire mesh using the default shader
    if (MeshAsset->MaterialInfo.empty() || InMeshComp->GetStaticMesh()->GetNumMaterials() == 0)
    {
        InPipeline.DrawIndexed(MeshAsset->Indices.size(), 0, 0);
        return;
    }

    if (InMeshComp->IsScrollEnabled())
    {
        UTimeManager& TimeManager = UTimeManager::GetInstance();
        InMeshComp->SetElapsedTime(InMeshComp->GetElapsedTime() + TimeManager.GetDeltaSeconds());
    }

    for (const FMeshSection& Section : MeshAsset->Sections)
    {
        UMaterial* Material = InMeshComp->GetMaterial(Section.MaterialSlot);
        if (Material)
        {
            FMaterialConstants MaterialConstants = {};
            FVector AmbientColor = Material->GetAmbientColor();
            MaterialConstants.Ka = FVector4(AmbientColor.X, AmbientColor.Y, AmbientColor.Z, 1.0f);
            FVector DiffuseColor = Material->GetDiffuseColor();
            MaterialConstants.Kd = FVector4(DiffuseColor.X, DiffuseColor.Y, DiffuseColor.Z, 1.0f);
            FVector SpecularColor = Material->GetSpecularColor();
            MaterialConstants.Ks = FVector4(SpecularColor.X, SpecularColor.Y, SpecularColor.Z, 1.0f);
            MaterialConstants.Ns = Material->GetSpecularExponent();
            MaterialConstants.Ni = Material->GetRefractionIndex();
            MaterialConstants.D = Material->GetDissolveFactor();
            MaterialConstants.MaterialFlags = 0; // Placeholder
            MaterialConstants.Time = InMeshComp->GetElapsedTime();

            // Update Constant Buffer
            UpdateConstant(InConstantBufferMaterial, MaterialConstants, 2, false, true);

            if (Material->GetDiffuseTexture()){
                auto* Proxy = Material->GetDiffuseTexture()->GetRenderProxy();
                InPipeline.SetTexture(0, false, Proxy->GetSRV());
                InPipeline.SetSamplerState(0, false, Proxy->GetSampler());
            }
            if (Material->GetAmbientTexture()){
                auto* Proxy = Material->GetAmbientTexture()->GetRenderProxy();
                InPipeline.SetTexture(1, false, Proxy->GetSRV());
                InPipeline.SetSamplerState(1, false, Proxy->GetSampler());
            }
            if (Material->GetSpecularTexture()){
                auto* Proxy = Material->GetSpecularTexture()->GetRenderProxy();
                InPipeline.SetTexture(2, false, Proxy->GetSRV());
                InPipeline.SetSamplerState(2, false, Proxy->GetSampler());
            }
            if (Material->GetAlphaTexture()){
                auto* Proxy = Material->GetAlphaTexture()->GetRenderProxy();
                InPipeline.SetTexture(4, false, Proxy->GetSRV());
                InPipeline.SetSamplerState(4, false, Proxy->GetSampler());
            }
        }
        InPipeline.DrawIndexed(Section.IndexCount, Section.StartIndex, 0);
    }
}

void URenderer::RenderBillboard(UBillboardComponent* InBillboardComp, UCamera* InCurrentCamera)
{
	if (!InBillboardComp || !InCurrentCamera || !Pipeline)
	{
		return;
	}

	UTexture* Sprite = InBillboardComp->GetSprite();
	if (!Sprite)
	{
		return;
	}

	const FTextureRenderProxy* RenderProxy = Sprite->GetRenderProxy();
	if (!RenderProxy || !RenderProxy->GetSRV() || !RenderProxy->GetSampler())
	{
		UE_LOG_WARNING("Renderer: Billboard sprite '%s' does not have a valid render proxy", Sprite->GetName().ToString().c_str());
		return;
	}

	if (!BillboardVertexBuffer || !BillboardIndexBuffer || !BillboardBlendState)
	{
		CreateBillboardResources();
		if (!BillboardVertexBuffer || !BillboardIndexBuffer || !BillboardBlendState)
		{
			return;
		}
	}

	InBillboardComp->UpdateRotationMatrix(InCurrentCamera);
	FMatrix ModelMatrix = FMatrix::ScaleMatrix(InBillboardComp->GetRelativeScale3D());
	ModelMatrix *= InBillboardComp->GetRTMatrix();

	FRenderState BillboardRenderState = InBillboardComp->GetRenderState();
	ID3D11RasterizerState* RasterizerState = GetRasterizerState(BillboardRenderState);
	ID3D11DepthStencilState* DepthStencilState = DefaultDepthStencilState;

	if (!RasterizerState)
	{
		FRenderState DefaultState;
		RasterizerState = GetRasterizerState(DefaultState);
	}

	FPipelineInfo PipelineInfo = {
		TextureInputLayout,
		TextureVertexShader,
		RasterizerState,
		DepthStencilState,
		TexturePixelShader,
		BillboardBlendState,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	Pipeline->UpdatePipeline(PipelineInfo);

	UpdateConstant(ConstantBufferModels, ModelMatrix, 0, true, false);
	Pipeline->SetConstantBuffer(1, true, ConstantBufferViewProj);
	//UpdateConstant(ConstantBufferViewProj, ConstantBufferViewProj, 1, true, false);

	constexpr uint32 MATERIAL_FLAG_DIFFUSE_MAP = 1 << 0;
	FMaterialConstants MaterialConstants = {};
	MaterialConstants.MaterialFlags = MATERIAL_FLAG_DIFFUSE_MAP;
	MaterialConstants.Kd = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	MaterialConstants.D = 1.0f;
    UpdateConstant(ConstantBufferMaterial, MaterialConstants, 2, false, true);

    // Bind debug/tint params (slot b3) so TextureShader can apply TotalColor tint if any
    {
        const bool bIsDepthView = (GEngine->GetEditor()->GetViewMode() == EViewModeIndex::VMI_SceneDepth);
        const bool bIsSelected = (GEngine->GetCurrentLevel() && InBillboardComp->GetOwner() == GEngine->GetCurrentLevel()->GetSelectedActor());
        FVector4 C = InBillboardComp->GetColor();
        if (bIsSelected)
        {
            C.X *= 1.2f; C.Y *= 1.2f; C.Z *= 1.2f;
        }
        float Color[4] = { C.X, C.Y, C.Z, C.W };
        FDepthConstants DepthData(bIsDepthView ? 1u : 0u, 0.1f, 500.0f, 0.8f, Color);
        UpdateConstant(ConstantBufferDepth, DepthData, 3, true, true);
    }

	Pipeline->SetTexture(0, false, RenderProxy->GetSRV());
	Pipeline->SetSamplerState(0, false, RenderProxy->GetSampler());

	Pipeline->SetVertexBuffer(BillboardVertexBuffer, sizeof(FNormalVertex));
	Pipeline->SetIndexBuffer(BillboardIndexBuffer, 0);
	Pipeline->DrawIndexed(6, 0, 0);
}

void URenderer::RenderText(UTextRenderComponent* InTextRenderComp, UCamera* InCurrentCamera)
{
	if (!InCurrentCamera)	return;

	FString RenderedString;

	if (InTextRenderComp->bIsUUIDText)
	{
		RenderedString = "UID: " + std::to_string(InTextRenderComp->GetUUID());
	}
	else
	{
		RenderedString = InTextRenderComp->GetText();
		if (strlen(RenderedString.c_str()) == 0)
		{
			RenderedString = "Insert Text";
		}
	}

	const FViewProjConstants& ViewProjConstData = InCurrentCamera->GetFViewProjConstants();
	FMatrix Translation = FMatrix::TranslationMatrix(InTextRenderComp->GetOwner()->GetActorLocation() + InTextRenderComp->GetRelativeLocation());
	FMatrix Rotation = FMatrix::RotationMatrix(InTextRenderComp->GetRelativeRotation());
	FMatrix Scale = FMatrix::ScaleMatrix(InTextRenderComp->GetRelativeScale3D());
	FontRenderer->RenderText(RenderedString.c_str(), Scale * Rotation * Translation, ViewProjConstData);
}

void URenderer::RenderPrimitiveDefault(UPipeline& InPipeline, UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState, ID3D11Buffer* InConstantBufferModels, ID3D11Buffer* InConstantBufferColor)
{
    // Update pipeline info
    FPipelineInfo PipelineInfo = {
        DefaultInputLayout,
        DefaultVertexShader,
        InRasterizerState,
        DefaultDepthStencilState,
        DefaultPixelShader,
        nullptr,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    };
    InPipeline.UpdatePipeline(PipelineInfo);

	ID3D11ShaderResourceView* const pSRV[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	InPipeline.GetDeviceContext()->PSSetShaderResources(0, 5, pSRV);
	ID3D11SamplerState* const pSamplers[5] = { nullptr };
	InPipeline.GetDeviceContext()->PSSetSamplers(0, 5, pSamplers);
    // Update pipeline buffers
    UpdateConstant(InConstantBufferModels, InPrimitiveComp->GetWorldTransformMatrix(), 0, true, false);
    // Color: use component color and brighten slightly if this primitive belongs to the selected actor (render-time only)
    {
        FVector4 C = InPrimitiveComp->GetColor();
        if (GEngine->GetCurrentLevel() && InPrimitiveComp->GetOwner() == GEngine->GetCurrentLevel()->GetSelectedActor())
        {
            C.X *= 1.2f; C.Y *= 1.2f; C.Z *= 1.2f;
        }
        UpdateConstant(InConstantBufferColor, C, 2, false, true);
    }

	if (GEngine->GetEditor()->GetViewMode() == EViewModeIndex::VMI_SceneDepth)
	{
		float Color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		FDepthConstants DepthData(1, 0.1f, 500.0f, 0.8f, Color);
		UpdateConstant(ConstantBufferDepth, DepthData, 3, true, true);
	}
	else
	{
		float Color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		FDepthConstants DepthData(0, 0.1f, 500.0f, 0.8f, Color);
		UpdateConstant(ConstantBufferDepth, DepthData, 3, true, true);
	}

    // Bind vertex buffer
    InPipeline.SetVertexBuffer(InPrimitiveComp->GetVertexBuffer(), Stride);

    // Draw vertex + index
    if (InPrimitiveComp->GetIndexBuffer() && InPrimitiveComp->GetIndicesData())
    {
        InPipeline.SetIndexBuffer(InPrimitiveComp->GetIndexBuffer(), 0);
        InPipeline.DrawIndexed(InPrimitiveComp->GetNumIndices(), 0, 0);
    }
    // Draw vertex
    else
    {
        InPipeline.Draw(static_cast<uint32>(InPrimitiveComp->GetNumVertices()), 0);
    }
}

void URenderer::RenderPrimitiveLine(UPipeline& InPipeline, UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState, ID3D11Buffer* InConstantBufferModels, ID3D11Buffer* InConstantBufferColor)
{
	// Update pipeline info
	FPipelineInfo PipelineInfo = {
		DefaultInputLayout,
		DefaultVertexShader,
		InRasterizerState,
		DefaultDepthStencilState,
		DefaultPixelShader,
		nullptr,
		D3D11_PRIMITIVE_TOPOLOGY_LINELIST
	};
	InPipeline.UpdatePipeline(PipelineInfo);

	// Update pipeline buffers
	UpdateConstant(InConstantBufferModels, InPrimitiveComp->GetWorldTransformMatrix(), 0, true, false);

	// TODO: 현재 하드 코딩으로 강제로 색을 지정함, 추후 변경을 반드시 해야 함 (PYB, 25.10.10)
	const FVector4 SolidWhiteColor = FVector4(0.0f, 1.0f, 0.0f, 1.0f);
	UpdateConstant(InConstantBufferColor, SolidWhiteColor, 2, false, true);

	// Bind vertex buffer
	InPipeline.SetVertexBuffer(InPrimitiveComp->GetVertexBuffer(), Stride);

	// Draw vertex + index
	if (InPrimitiveComp->GetIndexBuffer() && InPrimitiveComp->GetIndicesData())
	{
		InPipeline.SetIndexBuffer(InPrimitiveComp->GetIndexBuffer(), 0);
		InPipeline.DrawIndexed(InPrimitiveComp->GetNumIndices(), 0, 0);
	}
	// Draw vertex
	else
	{
		InPipeline.Draw(static_cast<uint32>(InPrimitiveComp->GetNumVertices()), 0);
	}
}

/**
 * @brief FVertex 타입용 정점 Buffer 생성 함수
 * @param InVertices 정점 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @return 생성된 D3D11 정점 버퍼
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FNormalVertex* InVertices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC VertexBufferDescription = {};
	VertexBufferDescription.ByteWidth = InByteWidth;
	VertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE; // 변경되지 않는 정적 데이터
	VertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexBufferInitData = { InVertices };

	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&VertexBufferDescription, &VertexBufferInitData, &VertexBuffer);

	return VertexBuffer;
}

/**
 * @brief FVector 타입용 정점 Buffer 생성 함수
 * @param InVertices 정점 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @param bCpuAccess CPU에서 접근 가능한 동적 버퍼 여부
 * @return 생성된 D3D11 정점 버퍼
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const
{
	D3D11_BUFFER_DESC VertexBufferDescription = {};
	VertexBufferDescription.ByteWidth = InByteWidth;
	VertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE; // 기본값: 정적 데이터
	VertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	// CPU 접근이 필요한 경우 동적 버퍼로 변경
	if (bCpuAccess)
	{
		VertexBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // CPU에서 자주 수정할 경우
		VertexBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU 쓰기 가능
		VertexBufferDescription.MiscFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA VertexBufferInitData = { InVertices };

	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&VertexBufferDescription, &VertexBufferInitData, &VertexBuffer);

	return VertexBuffer;
}

/**
 * @brief Index Buffer 생성 함수
 * @param InIndices 인덱스 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @return 생성된 D3D11 인덱스 버퍼
 */
ID3D11Buffer* URenderer::CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC IndexBufferDescription = {};
	IndexBufferDescription.ByteWidth = InByteWidth;
	IndexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	IndexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IndexBufferInitData = {};
	IndexBufferInitData.pSysMem = InIndices;

	ID3D11Buffer* IndexBuffer = nullptr;
	GetDevice()->CreateBuffer(&IndexBufferDescription, &IndexBufferInitData, &IndexBuffer);
	return IndexBuffer;
}

/**
 * @brief 창 크기 변경 시 렌더 타곟 및 버퍼를 재설정하는 함수
 * @param InWidth 새로운 창 너비
 * @param InHeight 새로운 창 높이
 */
void URenderer::OnResize(uint32 InWidth, uint32 InHeight)
{
	// 필수 리소스가 유효하지 않으면 Early Return
	if (!DeviceResources || !GetDevice() || !GetDeviceContext() || !GetSwapChain())
	{
		return;
	}

	if (InWidth == 0 || InHeight == 0) return;

	UStatOverlay::GetInstance().PreResize();

	DeviceResources->OnWindowSizeChanged(InWidth, InHeight);
	// Recreate FXAA render target to match new size
	if (FXAA) FXAA->OnResize();

	// 새로운 렌더 타겟 바인딩
	auto* RenderTargetView = (bFXAAEnabled && FXAA && (GEngine->GetEditor()->GetViewMode() != EViewModeIndex::VMI_SceneDepth) && FXAA->GetSceneRTV()) ? FXAA->GetSceneRTV() : DeviceResources->GetRenderTargetView();
	ID3D11RenderTargetView* RenderTargetViews[] = { RenderTargetView };
	GetDeviceContext()->OMSetRenderTargets(1, RenderTargetViews, DeviceResources->GetDepthStencilView());
	UStatOverlay::GetInstance().OnResize();

	UOcclusionRenderer::GetInstance().Resize(InWidth, InHeight);

	bIsFirstPass = true;
}

/**
 * @brief Vertex Buffer 해제 함수
 * @param InVertexBuffer 해제할 정점 버퍼
 */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
	if (InVertexBuffer)
	{
		InVertexBuffer->Release();
	}
}

/**
 * @brief Index Buffer 해제 함수
 * @param InIndexBuffer 해제할 인덱스 버퍼
 */
void URenderer::ReleaseIndexBuffer(ID3D11Buffer* InIndexBuffer)
{
	if (InIndexBuffer)
	{
		InIndexBuffer->Release();
	}
}

/**
 * @brief 커스텀 Vertex Shader와 Input Layout을 생성하는 함수
 * @param InFilePath 셰이더 파일 경로
 * @param InInputLayoutDescs Input Layout 스팩 배열
 * @param OutVertexShader 출력될 Vertex Shader 포인터
 * @param OutInputLayout 출력될 Input Layout 포인터
 */
void URenderer::CreateVertexShaderAndInputLayout(const wstring& InFilePath,
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescs,
	ID3D11VertexShader** OutVertexShader,
	ID3D11InputLayout** OutInputLayout)
{
	ID3DBlob* VertexShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	// Vertex Shader 컴파일
	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, nullptr, "mainVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&VertexShaderBlob, &ErrorBlob);

	// 컴파일 실패 시 에러 처리
	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			ErrorBlob->Release();
		}
		return;
	}

	// Vertex Shader 객체 생성
	GetDevice()->CreateVertexShader(VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(), nullptr, OutVertexShader);

	// Input Layout 생성
	GetDevice()->CreateInputLayout(InInputLayoutDescs.data(), static_cast<uint32>(InInputLayoutDescs.size()),
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(), OutInputLayout);

	// TODO(KHJ): 이 값이 여기에 있는 게 맞나? 검토 필요
	Stride = sizeof(FNormalVertex);

	VertexShaderBlob->Release();
}

/**
 * @brief 커스텀 Pixel Shader를 생성하는 함수
 * @param InFilePath 셰이더 파일 경로
 * @param OutPixelShader 출력될 Pixel Shader 포인터
 */
void URenderer::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader) const
{
	ID3DBlob* PixelShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	// Pixel Shader 컴파일
	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, nullptr, "mainPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&PixelShaderBlob, &ErrorBlob);

	// 컴파일 실패 시 에러 처리
	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			ErrorBlob->Release();
		}
		return;
	}

	// Pixel Shader 객체 생성
	GetDevice()->CreatePixelShader(PixelShaderBlob->GetBufferPointer(),
		PixelShaderBlob->GetBufferSize(), nullptr, OutPixelShader);

	PixelShaderBlob->Release();
}

/**
 * @brief 렌더링에 사용될 상수 버퍼들을 생성하는 함수
 */
void URenderer::CreateConstantBuffer()
{
	// 모델 변환 행렬용 상수 버퍼 생성 (Slot 0)
	{
		D3D11_BUFFER_DESC ModelConstantBufferDescription = {};
		ModelConstantBufferDescription.ByteWidth = sizeof(FMatrix) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ModelConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ModelConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ModelConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ModelConstantBufferDescription, nullptr, &ConstantBufferModels);
	}

	// 색상 정보용 상수 버퍼 생성 (Slot 2)
	{
		D3D11_BUFFER_DESC ColorConstantBufferDescription = {};
		ColorConstantBufferDescription.ByteWidth = sizeof(FVector4) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ColorConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ColorConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ColorConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ColorConstantBufferDescription, nullptr, &ConstantBufferColor);
	}

	// 카메라 View/Projection 행렬용 상수 버퍼 생성 (Slot 1)
	{
		D3D11_BUFFER_DESC ViewProjConstantBufferDescription = {};
		ViewProjConstantBufferDescription.ByteWidth = sizeof(FViewProjConstants) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ViewProjConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ViewProjConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ViewProjConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ViewProjConstantBufferDescription, nullptr, &ConstantBufferViewProj);
	}

	{
		D3D11_BUFFER_DESC MaterialConstantBufferDescription = {};
		MaterialConstantBufferDescription.ByteWidth = sizeof(FMaterial) + 0xf & 0xfffffff0;
		MaterialConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		MaterialConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		MaterialConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&MaterialConstantBufferDescription, nullptr, &ConstantBufferMaterial);
	}

	{
		D3D11_BUFFER_DESC ProjectionDecalConstantBufferDescription = {};
		ProjectionDecalConstantBufferDescription.ByteWidth = sizeof(FDecalConstants) + 0xf & 0xfffffff0;
		ProjectionDecalConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ProjectionDecalConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ProjectionDecalConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ProjectionDecalConstantBufferDescription, nullptr, &ConstantBufferProjectionDecal);
	}

	{
		D3D11_BUFFER_DESC SpotlightConstantBufferDescription = {};
		SpotlightConstantBufferDescription.ByteWidth = sizeof(FLightConstants) + 0xf & 0xfffffff0;
		SpotlightConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		SpotlightConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		SpotlightConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&SpotlightConstantBufferDescription, nullptr, &ConstantBufferSpotlight);
	}

	{
		D3D11_BUFFER_DESC FireBallConstantBufferDescription = {};
		FireBallConstantBufferDescription.ByteWidth = sizeof(FFireBallConstants) + 0xf & 0xfffffff0;
		FireBallConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
		FireBallConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		FireBallConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		GetDevice()->CreateBuffer(&FireBallConstantBufferDescription, nullptr, &ConstantBufferFireBall);

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth      = (sizeof(FDepthConstants2D) + 0xF) & ~0xF; // 16바이트 맞춤
		desc.Usage          = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&desc, nullptr, &ConstantBufferDepth2D);
	}

	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth      = (sizeof(FDepthConstants) + 0xF) & ~0xF; // 16바이트 맞춤
		desc.Usage          = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&desc, nullptr, &ConstantBufferDepth);
	}
}

void URenderer::CreateBillboardResources()
{
	if (BillboardVertexBuffer && BillboardIndexBuffer && BillboardBlendState)
	{
		return;
	}

	FNormalVertex BillboardVertices[4] = {};
	BillboardVertices[0].Position = FVector(-0.5f, -0.5f, 0.0f);
	BillboardVertices[0].Normal = FVector(0.0f, 0.0f, 1.0f);
	BillboardVertices[0].Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	BillboardVertices[0].TexCoord = FVector2(0.0f, 1.0f);

	BillboardVertices[1].Position = FVector(0.5f, -0.5f, 0.0f);
	BillboardVertices[1].Normal = FVector(0.0f, 0.0f, 1.0f);
	BillboardVertices[1].Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	BillboardVertices[1].TexCoord = FVector2(1.0f, 1.0f);

	BillboardVertices[2].Position = FVector(0.5f, 0.5f, 0.0f);
	BillboardVertices[2].Normal = FVector(0.0f, 0.0f, 1.0f);
	BillboardVertices[2].Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	BillboardVertices[2].TexCoord = FVector2(1.0f, 0.0f);

	BillboardVertices[3].Position = FVector(-0.5f, 0.5f, 0.0f);
	BillboardVertices[3].Normal = FVector(0.0f, 0.0f, 1.0f);
	BillboardVertices[3].Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	BillboardVertices[3].TexCoord = FVector2(0.0f, 0.0f);

	if (!BillboardVertexBuffer)
	{
		BillboardVertexBuffer = CreateVertexBuffer(BillboardVertices, static_cast<uint32>(sizeof(BillboardVertices)));
		if (!BillboardVertexBuffer)
		{
			UE_LOG_ERROR("Renderer: Failed to create billboard vertex buffer");
		}
	}

	uint32 BillboardIndices[6] = { 0, 1, 2, 0, 2, 3 };
	if (!BillboardIndexBuffer)
	{
		BillboardIndexBuffer = CreateIndexBuffer(BillboardIndices, static_cast<uint32>(sizeof(BillboardIndices)));
		if (!BillboardIndexBuffer)
		{
			UE_LOG_ERROR("Renderer: Failed to create billboard index buffer");
		}
	}

	if (!BillboardBlendState)
	{
		D3D11_BLEND_DESC BlendDesc = {};
		BlendDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT ResultHandle = GetDevice()->CreateBlendState(&BlendDesc, &BillboardBlendState);
		if (FAILED(ResultHandle))
		{
			UE_LOG_ERROR("Renderer: Failed to create billboard blend state (HRESULT: 0x%08lX)", ResultHandle);
		}
	}
}

void URenderer::CreateSpotlightResrouces()
{
	if (!SpotlightBlendState)
	{
		D3D11_BLEND_DESC BlendDesc = {};
		BlendDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT ResultHandle = GetDevice()->CreateBlendState(&BlendDesc, &SpotlightBlendState);
		if (FAILED(ResultHandle))
		{
			UE_LOG_ERROR("Renderer: Failed to create billboard blend state (HRESULT: 0x%08lX)", ResultHandle);
		}
	}
}

void URenderer::CreateFireBallShader()
{
	ID3DBlob* FireBallVSBlob;
	ID3DBlob* FireBallPSBlob;

	D3DCompileFromFile(L"Asset/Shader/FireBallShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &FireBallVSBlob, nullptr);
	GetDevice()->CreateVertexShader(FireBallVSBlob->GetBufferPointer(), FireBallVSBlob->GetBufferSize(), nullptr, &FireBallVertexShader);

	D3DCompileFromFile(L"Asset/Shader/FireBallShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &FireBallPSBlob, nullptr);
	GetDevice()->CreatePixelShader(FireBallPSBlob->GetBufferPointer(), FireBallPSBlob->GetBufferSize(), nullptr, &FireBallPixelShader);

	// InputLayout은 보통 동일합니다.
	D3D11_INPUT_ELEMENT_DESC FireBallLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	GetDevice()->CreateInputLayout(FireBallLayout, ARRAYSIZE(FireBallLayout), FireBallVSBlob->GetBufferPointer(), FireBallVSBlob->GetBufferSize(), &FireBallInputLayout);

	FireBallVSBlob->Release();
	FireBallPSBlob->Release();

	if (!FireBallBlendState)
	{
		D3D11_BLEND_DESC BlendDesc = {};
		BlendDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;       // 원본색 기여도 1
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;      // 대상색 기여도 1
		BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;    // 두 색을 더함
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		GetDevice()->CreateBlendState(&BlendDesc, &FireBallBlendState);
	}

}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
	SafeRelease(ConstantBufferModels);
	SafeRelease(ConstantBufferColor);
	SafeRelease(ConstantBufferViewProj);
	SafeRelease(ConstantBufferMaterial);
	SafeRelease(ConstantBufferProjectionDecal);
	SafeRelease(ConstantBufferBatchLine);
	SafeRelease(ConstantBufferSpotlight);
	SafeRelease(ConstantBufferFireBall);
	SafeRelease(ConstantBufferDepth);
	SafeRelease(ConstantBufferDepth2D);
}

bool URenderer::UpdateVertexBuffer(ID3D11Buffer* InVertexBuffer, const TArray<FVector>& InVertices) const
{
	if (!GetDeviceContext() || !InVertexBuffer || InVertices.empty())
	{
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT ResultHandle = GetDeviceContext()->Map(
		InVertexBuffer,
		0, // 서브리소스 인덱스 (버퍼는 0)
		D3D11_MAP_WRITE_DISCARD, // 전체 갱신
		0, // 플래그 없음
		&MappedResource
	);

	if (FAILED(ResultHandle))
	{
		return false;
	}

	// GPU 메모리에 새 데이터 복사
	// TODO: 어쩔 때 한번 read access violation 걸림
	memcpy(MappedResource.pData, InVertices.data(), sizeof(FVector) * InVertices.size());

	// GPU 접근 재허용
	GetDeviceContext()->Unmap(InVertexBuffer, 0);

	return true;
}

ID3D11RasterizerState* URenderer::GetRasterizerState(const FRenderState& InRenderState)
{
	D3D11_FILL_MODE FillMode = ToD3D11(InRenderState.FillMode);
	D3D11_CULL_MODE CullMode = ToD3D11(InRenderState.CullMode);

	const FRasterKey Key{ FillMode, CullMode };
	if (auto Iter = RasterCache.find(Key); Iter != RasterCache.end())
	{
		return Iter->second;
	}

	ID3D11RasterizerState* RasterizerState = nullptr;
	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = FillMode;
	RasterizerDesc.CullMode = CullMode;
	RasterizerDesc.FrontCounterClockwise = TRUE;
	RasterizerDesc.DepthClipEnable = TRUE;

	HRESULT ResultHandle = GetDevice()->CreateRasterizerState(&RasterizerDesc, &RasterizerState);

	if (FAILED(ResultHandle))
	{
		return nullptr;
	}

	RasterCache.emplace(Key, RasterizerState);
	return RasterizerState;
}

D3D11_CULL_MODE URenderer::ToD3D11(ECullMode InCull)
{
	switch (InCull)
	{
	case ECullMode::Back:
		return D3D11_CULL_BACK;
	case ECullMode::Front:
		return D3D11_CULL_FRONT;
	case ECullMode::None:
		return D3D11_CULL_NONE;
	default:
		return D3D11_CULL_BACK;
	}
}

D3D11_FILL_MODE URenderer::ToD3D11(EFillMode InFill)
{
	switch (InFill)
	{
	case EFillMode::Solid:
		return D3D11_FILL_SOLID;
	case EFillMode::WireFrame:
		return D3D11_FILL_WIREFRAME;
	default:
		return D3D11_FILL_SOLID;
	}
}
