#include "pch.h"
#include "Render/Public/Renderer.h"

#include "Level/Public/Level.h"
#include "Render/Public/Pipeline.h"
#include "Manager/ImGui/Public/ImGuiManager.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Render/Public/DeviceResources.h"
#include "Mesh/Public/Actor.h"

IMPLEMENT_SINGLETON(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());

	// 래스터라이저 상태 생성
	CreateRasterizerState();
	CreateDefaultShader();
	CreateConstantBuffer();

	UImGuiManager::GetInstance().Init(InWindowHandle);
}

void URenderer::Release()
{
	UImGuiManager::GetInstance().Release();

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

	GatherRenderableObjects();
	Render();
	UImGuiManager::GetInstance().Render();

	RenderEnd();
}

/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin()
{
	auto* rtv = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(rtv, ClearColor);

	GetDeviceContext()->RSSetViewports(1, &DeviceResources->GetViewportInfo());

	ID3D11RenderTargetView* rtvs[] = { rtv };  // 배열 생성
	GetDeviceContext()->OMSetRenderTargets(1, rtvs, nullptr);
}

/**
 * @brief Gathering Renderable Object
 */
void URenderer::GatherRenderableObjects()
{
	PrimitiveComponents.clear();
	for (auto& Object : ULevelManager::GetInstance().GetCurrentLevel()->GetLevelObjects())
	{
		AActor* Actor = dynamic_cast<AActor*>(Object);
		if (!Actor)
			continue;
		for (auto& Component : Actor->OwnedComponents)
		{
			UPrimitiveComponent* Primitive = dynamic_cast<UPrimitiveComponent*>(Component);
			if (!Primitive)
				continue;
			PrimitiveComponents.push_back(Primitive);
		}
	}
}

/**
 * @brief Buffer에 데이터 입력 및 Draw
 */
void URenderer::Render()
{
	for (auto& PrimitiveComponent : PrimitiveComponents)
	{
		FPipelineInfo PipelineInfo = {
			DefaultInputLayout,
			DefaultVertexShader,
			RasterizerState,
			DefaultPixelShader,
			nullptr,
		};
		Pipeline->UpdatePipeline(PipelineInfo);

		Pipeline->SetConstantBuffer(0, true, ConstantBuffer);
		UpdateConstant(
			PrimitiveComponent->GetRelativeLocation(),
			PrimitiveComponent->GetRelativeRotation(),
			PrimitiveComponent->GetRelativeScale3D() );

		Pipeline->SetVertexBuffer(PrimitiveComponent->GetVertexBuffer(), Stride);
		Pipeline->Draw(PrimitiveComponent->GetVerticesData()->size(), 0);
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
 * @brief Line Segments 그리는 함수
 * Concave Collider를 위해 구현되었음
 */
void URenderer::RenderLines(const FVertex* InVertices, UINT InCount) const
{
	if (!InVertices || InCount == 0)
	{
		return;
	}

	ID3D11Buffer* tempVB = CreateVertexBuffer(const_cast<FVertex*>(InVertices),
	                                          sizeof(FVertex) * InCount);

	UINT Offset = 0;
	GetDeviceContext()->IASetVertexBuffers(0, 1, &tempVB, &Stride, &Offset);
	GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	GetDeviceContext()->Draw(InCount, 0);

	ReleaseVertexBuffer(tempVB);

	// 복원: 이후 삼각형 렌더링을 위해 TRIANGLE LIST로 되돌림
	GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
	D3D11_BUFFER_DESC constantbufferdesc = {};
	constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;
	// ensure constant buffer size is multiple of 16 bytes
	constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
	constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	GetDevice()->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}
}

/**
 * @brief 상수 버퍼 업데이트 함수
 * @param InOffset
 * @param InScale Ball Size
 */
void URenderer::UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const
{
	if (ConstantBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		// update constant buffer every frame
		FConstants* constants = (FConstants*)constantbufferMSR.pData;
		{
			const float Roll = FVector::GetDegreeToRadian(InRotation.X);
			const float Pitch = FVector::GetDegreeToRadian(InRotation.Y);
			const float Yaw = FVector::GetDegreeToRadian(InRotation.Z);

			FConstants C = FConstants::TranslationMatrix(FVector(0, 0, 0));
			FConstants S = FConstants::ScaleMatrix(InScale);
			FConstants R = FConstants::RotationMatrix({ Roll, Pitch, Yaw });
			FConstants T = FConstants::TranslationMatrix(InPosition);
			*constants = C * S * R * T;
		}
		GetDeviceContext()->Unmap(ConstantBuffer, 0);
	}
}
