#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"

#include "Level/Public/Level.h"

#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Mesh/Public/Actor.h"
#include "Render/Gizmo/Public/Gizmo.h"
#include "Render/Renderer/Public/Pipeline.h"

IMPLEMENT_SINGLETON(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());

	// 래스터라이저 상태 생성
	CreateRasterizerState();
	CreateDepthStencilState();
	CreateDefaultShader();
	CreateConstantBuffer();
}

void URenderer::Release()
{
	ReleaseVertexBuffer(this->vertexBufferSphere);
	ReleaseConstantBuffer();
	ReleaseDefaultShader();
	ReleaseResource();
}

/**
 * @brief 래스터라이저 상태를 생성하는 함수
 */
void URenderer::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerdesc = {};
	rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
	rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

	GetDevice()->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
}

void URenderer::CreateDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};

	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	HRESULT hr = DeviceResources->GetDevice()->CreateDepthStencilState(
		&depthStencilDesc,
		&DepthStencilState
	);
}

/**
 * @brief 래스터라이저 상태를 해제하는 함수
 */
void URenderer::ReleaseRasterizerState()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
}

/**
 * @brief 렌더러에 사용된 모든 리소스를 해제하는 함수
 */
void URenderer::ReleaseResource()
{
	RasterizerState->Release();

	// 렌더 타겟을 초기화
	GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

	delete Pipeline;
	delete DeviceResources;
}

/**
 * @brief Shader 기반의 CSO 생성 함수
 */
void URenderer::CreateDefaultShader()
{
	ID3DBlob* VertexShaderCSO;
	ID3DBlob* PixelShaderCSO;

	D3DCompileFromFile(L"Shader/SampleShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
	                   &VertexShaderCSO, nullptr);

	GetDevice()->CreateVertexShader(VertexShaderCSO->GetBufferPointer(),
	                           VertexShaderCSO->GetBufferSize(), nullptr, &DefaultVertexShader);

	D3DCompileFromFile(L"Shader/SampleShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
	                   &PixelShaderCSO, nullptr);

	GetDevice()->CreatePixelShader(PixelShaderCSO->GetBufferPointer(),
	                          PixelShaderCSO->GetBufferSize(), nullptr, &DefaultPixelShader);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), VertexShaderCSO->GetBufferPointer(),
	                          VertexShaderCSO->GetBufferSize(), &DefaultInputLayout);

	Stride = sizeof(FVertex);

	VertexShaderCSO->Release();
	PixelShaderCSO->Release();
}

/**
 * @brief Shader Release
 */
void URenderer::ReleaseDefaultShader()
{
	if (DefaultInputLayout)
	{
		DefaultInputLayout->Release();
		DefaultInputLayout = nullptr;
	}

	if (DefaultPixelShader)
	{
		DefaultPixelShader->Release();
		DefaultPixelShader = nullptr;
	}

	if (DefaultVertexShader)
	{
		DefaultVertexShader->Release();
		DefaultVertexShader = nullptr;
	}
}

void URenderer::Update()
{
	RenderBegin();

	RenderLevel();
	RenderEditor();
	//RenderLines();

	UUIManager::GetInstance().Render();

	RenderEnd();
}

/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin()
{
	auto* rtv = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(rtv, ClearColor);
	auto* dsv = DeviceResources->GetDepthStencilView();
	GetDeviceContext()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

	GetDeviceContext()->RSSetViewports(1, &DeviceResources->GetViewportInfo());

	ID3D11RenderTargetView* rtvs[] = { rtv };  // 배열 생성

	GetDeviceContext()->OMSetRenderTargets(1, rtvs, DeviceResources->GetDepthStencilView());
}

/**
 * @brief Buffer에 데이터 입력 및 Draw
 */
void URenderer::RenderLevel()
{
	//
	// 여기에 카메라 VP 업데이트 한 번 싹
	//
	if (!ULevelManager::GetInstance().GetCurrentLevel())
		return;

	for (auto& PrimitiveComponent : ULevelManager::GetInstance().GetCurrentLevel()->GetLevelPrimitiveComponents())
	{
		if (!PrimitiveComponent) continue;

		FPipelineInfo PipelineInfo = {
			DefaultInputLayout,
			DefaultVertexShader,
			RasterizerState,
			DepthStencilState,
			DefaultPixelShader,
			nullptr,
		};
		Pipeline->UpdatePipeline(PipelineInfo);

		Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
		UpdateConstant(
			PrimitiveComponent->GetRelativeLocation(),
			PrimitiveComponent->GetRelativeRotation(),
			PrimitiveComponent->GetRelativeScale3D() );

		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
		UpdateConstant(PrimitiveComponent->GetColor());

		Pipeline->SetVertexBuffer(PrimitiveComponent->GetVertexBuffer(), Stride);
		Pipeline->Draw(static_cast<UINT>(PrimitiveComponent->GetVerticesData()->size()), 0);
	}
}

void URenderer::RenderEditor()
{
	if (!ULevelManager::GetInstance().GetCurrentLevel())
		return;

	for (auto& PrimitiveComponent :  ULevelManager::GetInstance().GetCurrentLevel()->GetEditorPrimitiveComponents())
	{
		if (!PrimitiveComponent) continue;

		FPipelineInfo PipelineInfo = {
			DefaultInputLayout,
			DefaultVertexShader,
			RasterizerState,
			DepthStencilState,
			DefaultPixelShader,
			nullptr,
			PrimitiveComponent->GetTopology()
		};

		Pipeline->UpdatePipeline(PipelineInfo);

		Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
		UpdateConstant(
			PrimitiveComponent);

		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
		UpdateConstant(PrimitiveComponent->GetColor());

		Pipeline->SetVertexBuffer(PrimitiveComponent->GetVertexBuffer(), Stride);
		Pipeline->Draw(static_cast<UINT>(PrimitiveComponent->GetVerticesData()->size()), 0);
	}
}

/**
 * @brief 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
 */
void URenderer::RenderEnd()
{
	GetSwapChain()->Present(0, 0); // 1: VSync 활성화
}

/**
 * @brief 정점 Buffer 생성 함수
 * @param InVertices
 * @param InByteWidth
 * @return
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FVertex* InVertices, UINT InByteWidth) const
{
	// 2. Create a vertex buffer
	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = InByteWidth;
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = {InVertices};

	ID3D11Buffer* vertexBuffer;

	GetDevice()->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &vertexBuffer);

	return vertexBuffer;
}

/**
 * @brief Index Buffer 생성 함수
 * @param InIndices
 * @param InByteWidth
 * @return
 */
ID3D11Buffer* URenderer::CreateIndexBuffer(const void* InIndices, UINT InByteWidth) const
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = InByteWidth;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA srd = {};
	srd.pSysMem = InIndices;

	ID3D11Buffer* buffer = nullptr;
	GetDevice()->CreateBuffer(&desc, &srd, &buffer);
	return buffer;
}

/**
 * @brief Vertex Buffer 소멸 함수
 * @param InVertexBuffer
 */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
	InVertexBuffer->Release();
}

/**
 * @brief 상수 버퍼 생성 함수
 */
void URenderer::CreateConstantBuffer()
{
	/**
	 * @brief 모델에 사용될 상수 버퍼 생성
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FMatrix) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferModels);
	}

	/**
	 * @brief 색상 수정에 사용할 상수 버퍼
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FVector4) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferColor);
	}

	/**
	 * @brief 카메라에 사용될 상수 버퍼 생성
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FViewProjConstants) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferViewProj);
	}
}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBufferModels)
	{
		ConstantBufferModels->Release();
		ConstantBufferModels = nullptr;
	}

	if (ConstantBufferColor)
	{
		ConstantBufferColor->Release();
		ConstantBufferColor = nullptr;
	}

	if (ConstantBufferViewProj)
	{
		ConstantBufferViewProj->Release();
		ConstantBufferViewProj = nullptr;
	}
}


void URenderer::UpdateConstant(const UPrimitiveComponent* Primitive)
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		// update constant buffer every frame
		FMatrix* constants = (FMatrix*)constantbufferMSR.pData;
		{
			*constants = FMatrix::GetModelMatrix(Primitive->GetRelativeLocation(), FVector::GetDegreeToRadian(Primitive->GetRelativeRotation()), Primitive->GetRelativeScale3D());
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}
/**
 * @brief 상수 버퍼 업데이트 함수
 * @param InOffset
 * @param InScale Ball Size
 */
void URenderer::UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		// update constant buffer every frame
		FMatrix* constants = (FMatrix*)constantbufferMSR.pData;
		{
			*constants = FMatrix::GetModelMatrix(InPosition, FVector::GetDegreeToRadian(InRotation), InScale);
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

void URenderer::UpdateConstant(const FViewProjConstants& InViewProjConstants) const
{
	Pipeline->SetConstantBuffer(1, true, ConstantBufferViewProj);

	if (ConstantBufferViewProj)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FViewProjConstants* ViewProjectionConstants = (FViewProjConstants*)ConstantBufferMSR.pData;
		{
			ViewProjectionConstants->View = InViewProjConstants.View;
			ViewProjectionConstants->Projection = InViewProjConstants.Projection;
		}
		GetDeviceContext()->Unmap(ConstantBufferViewProj, 0);
	}
}

void URenderer::UpdateConstant(const FVector4& Color) const
{
	Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);

	if (ConstantBufferColor)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FVector4* ColorConstants = (FVector4*)ConstantBufferMSR.pData;
		{
			ColorConstants->X = Color.X;
			ColorConstants->Y = Color.Y;
			ColorConstants->Z = Color.Z;
			ColorConstants->W = Color.W;
		}
		GetDeviceContext()->Unmap(ConstantBufferColor, 0);
	}
}
