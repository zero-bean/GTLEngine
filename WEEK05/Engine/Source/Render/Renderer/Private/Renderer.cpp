#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Component/Public/UUIDTextComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/Viewport.h"
#include "Editor/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"
#include "Level/Public/Level.h"

#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Optimization/Public/OcclusionCuller.h"

IMPLEMENT_SINGLETON_CLASS_BASE(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());
	ViewportClient = new FViewport();

	// 렌더링 상태 및 리소스 생성
	CreateRasterizerState();
	CreateDepthStencilState();
	CreateBlendState();
	CreateDefaultShader();
	CreateTextureShader();
	CreateConstantBuffers();

	// FontRenderer 초기화
	FontRenderer = new UFontRenderer();
	if (!FontRenderer->Initialize())
	{
		UE_LOG("FontRenderer 초기화 실패");
		SafeDelete(FontRenderer);
	}

	ViewportClient->InitializeLayout(DeviceResources->GetViewportInfo());
}

void URenderer::Release()
{
	ReleaseConstantBuffers();
	ReleaseDefaultShader();
	ReleaseDepthStencilState();
	ReleaseBlendState();
	ReleaseRasterizerState();

	SafeDelete(ViewportClient);
	SafeDelete(FontRenderer);
	SafeDelete(Pipeline);
	SafeDelete(DeviceResources);
}

void URenderer::CreateRasterizerState()
{
	// 현재는 GetRasterizerState에서 동적으로 생성하므로 비워둠
}

void URenderer::CreateDepthStencilState()
{
	// 3D Default Depth Stencil (Depth O, Stencil X)
	D3D11_DEPTH_STENCIL_DESC DefaultDescription = {};
	DefaultDescription.DepthEnable = TRUE;
	DefaultDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DefaultDescription.DepthFunc = D3D11_COMPARISON_LESS;
	DefaultDescription.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&DefaultDescription, &DefaultDepthStencilState);

	// Disabled Depth Stencil (Depth X, Stencil X)
	D3D11_DEPTH_STENCIL_DESC DisabledDescription = {};
	DisabledDescription.DepthEnable = FALSE;
	DisabledDescription.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&DisabledDescription, &DisabledDepthStencilState);
}

void URenderer::CreateBlendState()
{
    // Alpha Blending
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GetDevice()->CreateBlendState(&blendDesc, &AlphaBlendState);
}

void URenderer::CreateDefaultShader()
{
	TArray<D3D11_INPUT_ELEMENT_DESC> DefaultLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	CreateVertexShaderAndInputLayout(L"Asset/Shader/SampleShader.hlsl", DefaultLayout, &DefaultVertexShader, &DefaultInputLayout);
	CreatePixelShader(L"Asset/Shader/SampleShader.hlsl", &DefaultPixelShader);
	Stride = sizeof(FNormalVertex);
}

void URenderer::CreateTextureShader()
{
	TArray<D3D11_INPUT_ELEMENT_DESC> TextureLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	CreateVertexShaderAndInputLayout(L"Asset/Shader/TextureShader.hlsl", TextureLayout, &TextureVertexShader, &TextureInputLayout);
	CreatePixelShader(L"Asset/Shader/TextureShader.hlsl", &TexturePixelShader);
}

void URenderer::ReleaseRasterizerState()
{
	for (auto& Cache : RasterCache)
	{
		SafeRelease(Cache.second);
	}
	RasterCache.clear();
}

void URenderer::ReleaseDefaultShader()
{
	SafeRelease(DefaultInputLayout);
	SafeRelease(DefaultPixelShader);
	SafeRelease(DefaultVertexShader);
	SafeRelease(TextureInputLayout);
	SafeRelease(TexturePixelShader);
	SafeRelease(TextureVertexShader);
}

void URenderer::ReleaseDepthStencilState()
{
	SafeRelease(DefaultDepthStencilState);
	SafeRelease(DisabledDepthStencilState);
	if (GetDeviceContext())
	{
		GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}
}

void URenderer::ReleaseBlendState()
{
    SafeRelease(AlphaBlendState);
}

void URenderer::Update()
{
	RenderBegin();

	for (FViewportClient& ViewportClient : ViewportClient->GetViewports())
	{
		if (ViewportClient.GetViewportInfo().Width < 1.0f || ViewportClient.GetViewportInfo().Height < 1.0f) { continue; }

		ViewportClient.Apply(GetDeviceContext());

		UCamera* CurrentCamera = &ViewportClient.Camera;
		CurrentCamera->Update(ViewportClient.GetViewportInfo());
		UpdateConstantBuffer(ConstantBufferViewProj, CurrentCamera->GetFViewProjConstants(), 1, true);

		{
			TIME_PROFILE(RenderLevel)
			RenderLevel(CurrentCamera);
		}
		{
			TIME_PROFILE(RenderEditor)
			GEditor->GetEditorModule()->RenderEditor(CurrentCamera);
		}
	}

	{
		TIME_PROFILE(UUIManager)
		UUIManager::GetInstance().Render();
	}
	{
		TIME_PROFILE(UStatOverlay)
		UStatOverlay::GetInstance().Render();
	}

	RenderEnd();
}

void URenderer::RenderBegin() const
{
	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);
	auto* DepthStencilView = DeviceResources->GetDepthStencilView();
	GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView* rtvs[] = { RenderTargetView };
	GetDeviceContext()->OMSetRenderTargets(1, rtvs, DeviceResources->GetDepthStencilView());
	DeviceResources->UpdateViewport();
}

void URenderer::RenderLevel(UCamera* InCurrentCamera)
{
	const TObjectPtr<ULevel>& CurrentLevel = GWorld->GetLevel();
	if (!CurrentLevel) { return; }

	uint64 ShowFlags = CurrentLevel->GetShowFlags();
	if (!(ShowFlags & EEngineShowFlags::SF_Primitives)) { return; }

	// 오클루전 컬링 수행
	TIME_PROFILE(Occlusion)
	static COcclusionCuller Culler;
	const FViewProjConstants& ViewProj = InCurrentCamera->GetFViewProjConstants();
	Culler.InitializeCuller(ViewProj.View, ViewProj.Projection);
	TArray<TObjectPtr<UPrimitiveComponent>> FinalVisiblePrims = Culler.PerformCulling(
		InCurrentCamera->GetViewVolumeCuller().GetRenderableObjects(),
		InCurrentCamera->GetLocation()
	);
	TIME_PROFILE_END(Occlusion)

	// 렌더링할 컴포넌트 분류
	TArray<TObjectPtr<UStaticMeshComponent>> StaticMeshes;
	TArray<TObjectPtr<UPrimitiveComponent>> DefaultPrimitives;
	TArray<TObjectPtr<UBillBoardComponent>> BillBoards;
	TArray<TObjectPtr<UTextComponent>> Texts;
	StaticMeshes.reserve(FinalVisiblePrims.size());

	for (auto& Prim : FinalVisiblePrims)
	{
		if (auto StaticMesh = Cast<UStaticMeshComponent>(Prim))
		{
			StaticMeshes.push_back(StaticMesh);
		}
		else if (auto BillBoard = Cast<UBillBoardComponent>(Prim))
		{
			BillBoards.push_back(BillBoard);
		}
		else if (auto Text = Cast<UTextComponent>(Prim); Text && !Text->IsExactly(UUUIDTextComponent::StaticClass()))
		{
			Texts.push_back(Text);
		}
		else
		{
			DefaultPrimitives.push_back(Prim);
		}
	}

	// 각 타입별 렌더링 함수 호출
	RenderStaticMeshes(StaticMeshes);
	
	for (auto& PrimitiveComponent : DefaultPrimitives)
	{
		FRenderState RenderState = PrimitiveComponent->GetRenderState();
		if (GEditor->GetEditorModule()->GetViewMode() == EViewModeIndex::VMI_Wireframe)
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}
		RenderPrimitiveDefault(PrimitiveComponent, GetRasterizerState(RenderState));
	}
	
	RenderBillBoard(InCurrentCamera, BillBoards);
	RenderText(InCurrentCamera, Texts);

	if (ShowFlags & EEngineShowFlags::SF_BillboardText)
	{
		if (UUUIDTextComponent* PickedBillboard = GEditor->GetEditorModule()->GetPickedBillboard())
		{
			RenderUUID(PickedBillboard, InCurrentCamera);
		}
	}
}

void URenderer::RenderEditorPrimitive(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState, uint32 InStride, uint32 InIndexBufferStride)
{
    // Use the global stride if InStride is 0
    const uint32 FinalStride = (InStride == 0) ? Stride : InStride;

    // Allow for custom shaders, fallback to default
    FPipelineInfo PipelineInfo = {
        InPrimitive.InputLayout ? InPrimitive.InputLayout : DefaultInputLayout,
        InPrimitive.VertexShader ? InPrimitive.VertexShader : DefaultVertexShader,
        GetRasterizerState(InRenderState),
        InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState,
        InPrimitive.PixelShader ? InPrimitive.PixelShader : DefaultPixelShader,
        nullptr,
        InPrimitive.Topology
    };
    Pipeline->UpdatePipeline(PipelineInfo);

    // Update constant buffers
    UpdateConstantBuffer(ConstantBufferModels, FMatrix::GetModelMatrix(InPrimitive.Location, FVector::GetDegreeToRadian(InPrimitive.Rotation), InPrimitive.Scale), 0, true);
    UpdateConstantBuffer(ConstantBufferColor, InPrimitive.Color, 2, false);

    Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, FinalStride);

    // The core logic: check for an index buffer
    if (InPrimitive.IndexBuffer && InPrimitive.NumIndices > 0)
    {
        Pipeline->SetIndexBuffer(InPrimitive.IndexBuffer, InIndexBufferStride);
        Pipeline->DrawIndexed(InPrimitive.NumIndices, 0, 0);
    }
    else
    {
        Pipeline->Draw(InPrimitive.NumVertices, 0);
    }
}

void URenderer::RenderEnd() const
{
	TIME_PROFILE(DrawCall)
	GetSwapChain()->Present(0, 0);
	TIME_PROFILE_END(DrawCall)
}

void URenderer::RenderStaticMeshes(TArray<TObjectPtr<UStaticMeshComponent>>& MeshComponents)
{
	TIME_PROFILE(RenderStaticMeshes)
	sort(MeshComponents.begin(), MeshComponents.end(),
		[](TObjectPtr<UStaticMeshComponent> A, TObjectPtr<UStaticMeshComponent> B) {
			uint64_t MeshA = A->GetStaticMesh() ? A->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			uint64_t MeshB = B->GetStaticMesh() ? B->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			return MeshA < MeshB;
		});

	FStaticMesh* CurrentMeshAsset = nullptr;
	UMaterial* CurrentMaterial = nullptr;
	ID3D11RasterizerState* CurrentRasterizer = nullptr;
	const EViewModeIndex ViewMode = GEditor->GetEditorModule()->GetViewMode();

	for (UStaticMeshComponent* MeshComp : MeshComponents) 
	{
		if (!MeshComp->GetStaticMesh()) { continue; }
		FStaticMesh* MeshAsset = MeshComp->GetStaticMesh()->GetStaticMeshAsset();
		if (!MeshAsset) { continue; }

		FRenderState RenderState = MeshComp->GetRenderState();
		if (ViewMode == EViewModeIndex::VMI_Wireframe) 
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}
		ID3D11RasterizerState* RasterizerState = GetRasterizerState(RenderState);

		if (CurrentRasterizer != RasterizerState) 
		{
			static FPipelineInfo PipelineInfo = { TextureInputLayout, TextureVertexShader, nullptr, DefaultDepthStencilState, TexturePixelShader, nullptr };
			PipelineInfo.RasterizerState = RasterizerState;
			Pipeline->UpdatePipeline(PipelineInfo);
			CurrentRasterizer = RasterizerState;
		}

		if (CurrentMeshAsset != MeshAsset)
		{
			Pipeline->SetVertexBuffer(MeshComp->GetVertexBuffer(), sizeof(FNormalVertex));
			Pipeline->SetIndexBuffer(MeshComp->GetIndexBuffer(), 0);
			CurrentMeshAsset = MeshAsset;
		}

		UpdateConstantBuffer(ConstantBufferModels, MeshComp->GetWorldTransformMatrix(), 0, true);

		if (MeshAsset->MaterialInfo.empty() || MeshComp->GetStaticMesh()->GetNumMaterials() == 0) 
		{
			Pipeline->DrawIndexed(MeshAsset->Indices.size(), 0, 0);
			continue;
		}

		if (MeshComp->IsScrollEnabled()) 
		{
			MeshComp->SetElapsedTime(MeshComp->GetElapsedTime() + UTimeManager::GetInstance().GetDeltaTime());
		}

		for (const FMeshSection& section : MeshAsset->Sections)
		{
			UMaterial* Material = MeshComp->GetMaterial(section.MaterialSlot);
			if (CurrentMaterial != Material) {
				FMaterialConstants MaterialConstants = {};
				FVector AmbientColor = Material->GetAmbientColor(); MaterialConstants.Ka = FVector4(AmbientColor.X, AmbientColor.Y, AmbientColor.Z, 1.0f);
				FVector DiffuseColor = Material->GetDiffuseColor(); MaterialConstants.Kd = FVector4(DiffuseColor.X, DiffuseColor.Y, DiffuseColor.Z, 1.0f);
				FVector SpecularColor = Material->GetSpecularColor(); MaterialConstants.Ks = FVector4(SpecularColor.X, SpecularColor.Y, SpecularColor.Z, 1.0f);
				MaterialConstants.Ns = Material->GetSpecularExponent();
				MaterialConstants.Ni = Material->GetRefractionIndex();
				MaterialConstants.D = Material->GetDissolveFactor();
				MaterialConstants.MaterialFlags = 0;
				MaterialConstants.Time = MeshComp->GetElapsedTime();

				UpdateConstantBuffer(ConstantBufferMaterial, MaterialConstants, 2, false);

				if (UTexture* DiffuseTexture = Material->GetDiffuseTexture())
				{
					if(auto* Proxy = DiffuseTexture->GetRenderProxy())
					{
						Pipeline->SetTexture(0, false, Proxy->GetSRV());
						Pipeline->SetSamplerState(0, false, Proxy->GetSampler());
					}
				}
				if (UTexture* AmbientTexture = Material->GetAmbientTexture())
				{
					if(auto* Proxy = AmbientTexture->GetRenderProxy())
					{
						Pipeline->SetTexture(1, false, Proxy->GetSRV());
					}
				}
				if (UTexture* SpecularTexture = Material->GetSpecularTexture())
				{
					if(auto* Proxy = SpecularTexture->GetRenderProxy())
					{
						Pipeline->SetTexture(2, false, Proxy->GetSRV());
					}
				}
				if (UTexture* AlphaTexture = Material->GetAlphaTexture())
				{
					if(auto* Proxy = AlphaTexture->GetRenderProxy())
					{
						Pipeline->SetTexture(4, false, Proxy->GetSRV());
					}
				}
				
				CurrentMaterial = Material;
			}
			Pipeline->DrawIndexed(section.IndexCount, section.StartIndex, 0);
		}
	}
}

void URenderer::RenderBillBoard(UCamera* InCurrentCamera, TArray<TObjectPtr<UBillBoardComponent>>& InBillBoardComp)
{
	ID3D11RasterizerState* CurrentRasterizer = nullptr;
	const EViewModeIndex ViewMode = GEditor->GetEditorModule()->GetViewMode();

	Pipeline->SetTexture(1, false, nullptr);
	Pipeline->SetTexture(2, false, nullptr);
	Pipeline->SetTexture(4, false, nullptr);

	for (UBillBoardComponent* BillBoardComp : InBillBoardComp)
	{
		BillBoardComp->FaceCamera(InCurrentCamera->GetLocation(), InCurrentCamera->GetUp(), InCurrentCamera->GetRight());

		FRenderState RenderState = BillBoardComp->GetRenderState();
		if (ViewMode == EViewModeIndex::VMI_Wireframe)
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}
		ID3D11RasterizerState* RasterizerState = GetRasterizerState(RenderState);

		if (CurrentRasterizer != RasterizerState)
		{
			static FPipelineInfo PipelineInfo = { TextureInputLayout, TextureVertexShader, nullptr, DefaultDepthStencilState, TexturePixelShader, nullptr };
			PipelineInfo.RasterizerState = RasterizerState;
			Pipeline->UpdatePipeline(PipelineInfo);
			CurrentRasterizer = RasterizerState;
		}

		Pipeline->SetVertexBuffer(BillBoardComp->GetVertexBuffer(), sizeof(FNormalVertex));
		Pipeline->SetIndexBuffer(BillBoardComp->GetIndexBuffer(), 0);
		UpdateConstantBuffer(ConstantBufferModels, BillBoardComp->GetWorldTransformMatrix(), 0, true);
		Pipeline->SetTexture(0, false, BillBoardComp->GetSprite().second);
		Pipeline->SetSamplerState(0, false, const_cast<ID3D11SamplerState*>(BillBoardComp->GetSampler()));
		Pipeline->DrawIndexed(BillBoardComp->GetNumIndices(), 0, 0);
	}
}

void URenderer::RenderText(UCamera* InCurrentCamera, TArray<TObjectPtr<UTextComponent>>& InTextComp)
{
	const FViewProjConstants& ViewProj = InCurrentCamera->GetFViewProjConstants();
	for (const TObjectPtr<UTextComponent>& Text : InTextComp)
	{
		FontRenderer->RenderText(Text->GetText().c_str(), Text->GetWorldTransformMatrix(), ViewProj, 0.0f, -1.0f, 1.0f, 2.0f, true);
	}
}

void URenderer::RenderUUID(UUUIDTextComponent* InBillBoardComp, UCamera* InCurrentCamera)
{
	if (!InCurrentCamera) return;
	InBillBoardComp->UpdateRotationMatrix(InCurrentCamera->GetLocation());
	FString UUIDString = "UID: " + std::to_string(InBillBoardComp->GetUUID());
	FontRenderer->RenderText(UUIDString.c_str(), InBillBoardComp->GetRTMatrix(), InCurrentCamera->GetFViewProjConstants());
}

void URenderer::RenderPrimitiveDefault(UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState)
{
	FPipelineInfo PipelineInfo = { DefaultInputLayout, DefaultVertexShader, InRasterizerState, DefaultDepthStencilState, DefaultPixelShader, nullptr };
	Pipeline->UpdatePipeline(PipelineInfo);
	
	UpdateConstantBuffer(ConstantBufferModels, InPrimitiveComp->GetWorldTransformMatrix(), 0, true); 
	UpdateConstantBuffer(ConstantBufferColor, InPrimitiveComp->GetColor(), 2, true); 
    
	Pipeline->SetConstantBuffer(1, true, ConstantBufferViewProj);
    
	Pipeline->SetVertexBuffer(InPrimitiveComp->GetVertexBuffer(), Stride);

	if (InPrimitiveComp->GetIndexBuffer() && InPrimitiveComp->GetIndicesData())
	{
		Pipeline->SetIndexBuffer(InPrimitiveComp->GetIndexBuffer(), 0);
		Pipeline->DrawIndexed(InPrimitiveComp->GetNumIndices(), 0, 0);
	}
	else
	{
		Pipeline->Draw(static_cast<uint32>(InPrimitiveComp->GetNumVertices()), 0);
	}
}

ID3D11Buffer* URenderer::CreateVertexBuffer(FNormalVertex* InVertices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC Desc = { InByteWidth, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA InitData = { InVertices, 0, 0 };
	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&Desc, &InitData, &VertexBuffer);
	return VertexBuffer;
}

ID3D11Buffer* URenderer::CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const
{
	D3D11_BUFFER_DESC Desc = { InByteWidth, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	if (bCpuAccess)
	{
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	D3D11_SUBRESOURCE_DATA InitData = { InVertices, 0, 0 };
	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&Desc, &InitData, &VertexBuffer);
	return VertexBuffer;
}

ID3D11Buffer* URenderer::CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC Desc = { InByteWidth, D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA InitData = { InIndices, 0, 0 };
	ID3D11Buffer* IndexBuffer = nullptr;
	GetDevice()->CreateBuffer(&Desc, &InitData, &IndexBuffer);
	return IndexBuffer;
}

void URenderer::OnResize(uint32 InWidth, uint32 InHeight) const
{
	if (!DeviceResources || !GetDeviceContext() || !GetSwapChain()) return;

	DeviceResources->ReleaseFrameBuffer();
	DeviceResources->ReleaseDepthBuffer();
	GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

	if (FAILED(GetSwapChain()->ResizeBuffers(2, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0)))
	{
		UE_LOG("OnResize Failed");
		return;
	}

	DeviceResources->UpdateViewport();
	DeviceResources->CreateFrameBuffer();
	DeviceResources->CreateDepthBuffer();

	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	ID3D11RenderTargetView* RenderTargetViews[] = { RenderTargetView };
	GetDeviceContext()->OMSetRenderTargets(1, RenderTargetViews, DeviceResources->GetDepthStencilView());
}

void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
	SafeRelease(InVertexBuffer);
}

void URenderer::ReleaseIndexBuffer(ID3D11Buffer* InIndexBuffer)
{
	SafeRelease(InIndexBuffer);
}

void URenderer::CreateVertexShaderAndInputLayout(const wstring& InFilePath,
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescs,
	ID3D11VertexShader** OutVertexShader,
	ID3D11InputLayout** OutInputLayout)
{
	ID3DBlob* VertexShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", 0, 0, &VertexShaderBlob, &ErrorBlob);
	if (FAILED(Result))
	{
		if (ErrorBlob) { OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer())); SafeRelease(ErrorBlob); }
		SafeRelease(VertexShaderBlob);
		return;
	}

	GetDevice()->CreateVertexShader(VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), nullptr, OutVertexShader);
	GetDevice()->CreateInputLayout(InInputLayoutDescs.data(), static_cast<uint32>(InInputLayoutDescs.size()), VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), OutInputLayout);
	
	SafeRelease(VertexShaderBlob);
}

void URenderer::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader) const
{
	ID3DBlob* PixelShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", 0, 0, &PixelShaderBlob, &ErrorBlob);
	if (FAILED(Result))
	{
		if (ErrorBlob) { OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer())); SafeRelease(ErrorBlob); }
		SafeRelease(PixelShaderBlob);
		return;
	}

	GetDevice()->CreatePixelShader(PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(), nullptr, OutPixelShader);
	SafeRelease(PixelShaderBlob);
}

ID3D11SamplerState* URenderer::CreateSamplerState(D3D11_FILTER InFilter, D3D11_TEXTURE_ADDRESS_MODE InAddressMode) const
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = InFilter;
    samplerDesc.AddressU = InAddressMode;
    samplerDesc.AddressV = InAddressMode;
    samplerDesc.AddressW = InAddressMode;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* SamplerState = nullptr;
    if (FAILED(GetDevice()->CreateSamplerState(&samplerDesc, &SamplerState)))
    {
        UE_LOG_ERROR("Renderer: 샘플러 스테이트 생성 실패");
        return nullptr;
    }
    return SamplerState;
}

void URenderer::CreateConstantBuffers()
{
	ConstantBufferModels = CreateConstantBuffer<FMatrix>();
	ConstantBufferColor = CreateConstantBuffer<FVector4>();
	ConstantBufferViewProj = CreateConstantBuffer<FViewProjConstants>();
	ConstantBufferMaterial = CreateConstantBuffer<FMaterialConstants>();
}

void URenderer::ReleaseConstantBuffers()
{
	SafeRelease(ConstantBufferModels);
	SafeRelease(ConstantBufferColor);
	SafeRelease(ConstantBufferViewProj);
	SafeRelease(ConstantBufferMaterial);
}

bool URenderer::UpdateVertexBuffer(ID3D11Buffer* InVertexBuffer, const TArray<FVector>& InVertices) const
{
	if (!GetDeviceContext() || !InVertexBuffer || InVertices.empty()) return false;

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	if (FAILED(GetDeviceContext()->Map(InVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource))) return false;
	
	memcpy(MappedResource.pData, InVertices.data(), sizeof(FVector) * InVertices.size());
	GetDeviceContext()->Unmap(InVertexBuffer, 0);

	return true;
}

ID3D11RasterizerState* URenderer::GetRasterizerState(const FRenderState& InRenderState)
{
	const FRasterKey Key{ ToD3D11(InRenderState.FillMode), ToD3D11(InRenderState.CullMode) };
	if (auto Iter = RasterCache.find(Key); Iter != RasterCache.end())
	{
		return Iter->second;
	}

	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = Key.FillMode;
	RasterizerDesc.CullMode = Key.CullMode;
	RasterizerDesc.FrontCounterClockwise = TRUE;
	RasterizerDesc.DepthClipEnable = TRUE;

	ID3D11RasterizerState* RasterizerState = nullptr;
	if (FAILED(GetDevice()->CreateRasterizerState(&RasterizerDesc, &RasterizerState)))
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
	case ECullMode::Back: return D3D11_CULL_BACK;
	case ECullMode::Front: return D3D11_CULL_FRONT;
	case ECullMode::None: return D3D11_CULL_NONE;
	default: return D3D11_CULL_BACK;
	}
}

D3D11_FILL_MODE URenderer::ToD3D11(EFillMode InFill)
{
	switch (InFill)
	{
	case EFillMode::Solid: return D3D11_FILL_SOLID;
	case EFillMode::WireFrame: return D3D11_FILL_WIREFRAME;
	default: return D3D11_FILL_SOLID;
	}
}