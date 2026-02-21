#pragma once
#include "DeviceResources.h"
#include "Core/Public/Object.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Component/Public/BillBoardComponent.h"
#include "Component/Public/TextComponent.h"

class UDeviceResources;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UUUIDTextComponent;
class AActor;
class AGizmo;
class UEditor;
class UFontRenderer;
class FViewport;
class UCamera;
class UPipeline;

/**
 * @brief Rendering Pipeline 전반을 처리하는 클래스
 */
UCLASS()
class URenderer :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(URenderer, UObject)

public:
	void Init(HWND InWindowHandle);
	void Release();

	// Initialize
	void CreateRasterizerState();
	void CreateDepthStencilState();
	void CreateBlendState();
	void CreateDefaultShader();
	void CreateTextureShader();
	void CreateConstantBuffers();

	// Release
	void ReleaseConstantBuffers();
	void ReleaseDefaultShader();
	void ReleaseDepthStencilState();
	void ReleaseBlendState();
	void ReleaseRasterizerState();

	// Render
	void Update();
	void RenderBegin() const;
	void RenderLevel(UCamera* InCurrentCamera);
	void RenderEnd() const;
	void RenderStaticMeshes(TArray<TObjectPtr<UStaticMeshComponent>>& MeshComponents);
	void RenderBillBoard(UCamera* InCurrentCamera, TArray<TObjectPtr<UBillBoardComponent>>& InBillBoardComp);
	void RenderText(UCamera* InCurrentCamera, TArray<TObjectPtr<UTextComponent>>& InTextComp);
	void RenderUUID(UUUIDTextComponent* InBillBoardComp, UCamera* InCurrentCamera);
	void RenderPrimitiveDefault(UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState);
	void RenderEditorPrimitive(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState, uint32 InStride = 0, uint32 InIndexBufferStride = 0);

	void OnResize(uint32 Inwidth = 0, uint32 InHeight = 0) const;

	// Create function
	void CreateVertexShaderAndInputLayout(const wstring& InFilePath,
									  const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescriptions,
									  ID3D11VertexShader** OutVertexShader, ID3D11InputLayout** OutInputLayout);
	ID3D11Buffer* CreateVertexBuffer(FNormalVertex* InVertices, uint32 InByteWidth) const;
	ID3D11Buffer* CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const;
	ID3D11Buffer* CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const;
	void CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** InPixelShader) const;
	ID3D11SamplerState* CreateSamplerState(D3D11_FILTER InFilter, D3D11_TEXTURE_ADDRESS_MODE InAddressMode) const;

	bool UpdateVertexBuffer(ID3D11Buffer* InVertexBuffer, const TArray<FVector>& InVertices) const;

	template<typename T>
	ID3D11Buffer* CreateConstantBuffer() const
	{
		D3D11_BUFFER_DESC Desc = {};
		Desc.ByteWidth = sizeof(T) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		ID3D11Buffer* Buffer = nullptr;
		GetDevice()->CreateBuffer(&Desc, nullptr, &Buffer);
		return Buffer;
	}

	template<typename T>
	void UpdateConstantBuffer(ID3D11Buffer* Buffer, const T& Data, int SlotIndex = -1, bool IsVertexShader = true) const
	{
		if (!Buffer) { return; }

		if (SlotIndex >= 0) { Pipeline->SetConstantBuffer(SlotIndex, IsVertexShader, Buffer); }

		D3D11_MAPPED_SUBRESOURCE MappedResource = {};
		GetDeviceContext()->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &Data, sizeof(T));
		GetDeviceContext()->Unmap(Buffer, 0);
	}

	static void ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer);
	static void ReleaseIndexBuffer(ID3D11Buffer* InIndexBuffer);

	// Helper function
	static D3D11_CULL_MODE ToD3D11(ECullMode InCull);
	static D3D11_FILL_MODE ToD3D11(EFillMode InFill);

	// Getter & Setter
	ID3D11Device* GetDevice() const { return DeviceResources->GetDevice(); }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceResources->GetDeviceContext(); }
	IDXGISwapChain* GetSwapChain() const { return DeviceResources->GetSwapChain(); }
	ID3D11RenderTargetView* GetRenderTargetView() const { return DeviceResources->GetRenderTargetView(); }
	UDeviceResources* GetDeviceResources() const { return DeviceResources; }
	FViewport* GetViewportClient() const { return ViewportClient; }
	UPipeline* GetPipeline() const { return Pipeline; }
	bool GetIsResizing() const { return bIsResizing; }

	ID3D11RasterizerState* GetRasterizerState(const FRenderState& InRenderState);
	ID3D11DepthStencilState* GetDefaultDepthStencilState() const { return DefaultDepthStencilState; }
	ID3D11DepthStencilState* GetDisabledDepthStencilState() const { return DisabledDepthStencilState; }
	ID3D11BlendState* GetAlphaBlendState() const { return AlphaBlendState; }
	ID3D11Buffer* GetConstantBufferModels() const { return ConstantBufferModels; }
	ID3D11Buffer* GetConstantBufferViewProj() const { return ConstantBufferViewProj; }

	void SetIsResizing(bool isResizing) { bIsResizing = isResizing; }

private:
	UPipeline* Pipeline = nullptr;
	UDeviceResources* DeviceResources = nullptr;
	UFontRenderer* FontRenderer = nullptr;
	TArray<UPrimitiveComponent*> PrimitiveComponents;

	// States
	ID3D11DepthStencilState* DefaultDepthStencilState = nullptr;
	ID3D11DepthStencilState* DisabledDepthStencilState = nullptr;
	ID3D11BlendState* AlphaBlendState = nullptr;

	// Constant Buffers
	ID3D11Buffer* ConstantBufferModels = nullptr;
	ID3D11Buffer* ConstantBufferViewProj = nullptr;
	ID3D11Buffer* ConstantBufferColor = nullptr;
	ID3D11Buffer* ConstantBufferBatchLine = nullptr;
	ID3D11Buffer* ConstantBufferMaterial = nullptr;

	FLOAT ClearColor[4] = {0.025f, 0.025f, 0.025f, 1.0f};

	// Default Shaders
	ID3D11VertexShader* DefaultVertexShader = nullptr;
	ID3D11PixelShader* DefaultPixelShader = nullptr;
	ID3D11InputLayout* DefaultInputLayout = nullptr;
	
	// Texture Shaders
	ID3D11VertexShader* TextureVertexShader = nullptr;
	ID3D11PixelShader* TexturePixelShader = nullptr;
	ID3D11InputLayout* TextureInputLayout = nullptr;
	
	uint32 Stride = 0;

	FViewport* ViewportClient = nullptr;

	struct FRasterKey
	{
		D3D11_FILL_MODE FillMode = {};
		D3D11_CULL_MODE CullMode = {};

		bool operator==(const FRasterKey& InKey) const
		{
			return FillMode == InKey.FillMode && CullMode == InKey.CullMode;
		}
	};

	struct FRasterKeyHasher
	{
		size_t operator()(const FRasterKey& InKey) const noexcept
		{
			auto Mix = [](size_t& H, size_t V)
			{
				H ^= V + 0x9e3779b97f4a7c15ULL + (H << 6) + (H << 2);
			};

			size_t H = 0;
			Mix(H, (size_t)InKey.FillMode);
			Mix(H, (size_t)InKey.CullMode);

			return H;
		}
	};

	TMap<FRasterKey, ID3D11RasterizerState*, FRasterKeyHasher> RasterCache;
	
	bool bIsResizing = false;
};