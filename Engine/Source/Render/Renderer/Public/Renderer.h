#pragma once
#include "Component/Public/BillboardComponent.h"

#ifdef MULTI_THREADING
#include <mutex>
#endif

#include "DeviceResources.h"
#include "Core/Public/Object.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/EditorPrimitive.h"

class UPipeline;
class UDeviceResources;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UDecalComponent;
class USpotLightComponent;
class AActor;
class AGizmo;
class UEditor;
class UFontRenderer;
class FViewport;
class UCamera;
class FViewportClient;

/**
 * @brief Rendering Pipeline 전반을 처리하는 클래스
 *
 * Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
 * @param Device GPU와 통신하기 위한 Direct3D 장치
 * @param DeviceContext GPU 명령 실행을 담당하는 컨텍스트
 * @param SwapChain 프레임 버퍼를 교체하는 데 사용되는 스왑 체인
 *
 * // 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
 * @param FrameBuffer 화면 출력용 텍스처
 * @param FrameBufferRTV 텍스처를 렌더 타겟으로 사용하는 뷰
 * @param RasterizerState 래스터라이저 상태(컬링, 채우기 모드 등 정의)
 * @param ConstantBuffer 쉐이더에 데이터를 전달하기 위한 상수 버퍼
 *
 * @param ClearColor 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
 * @param ViewportInfo 렌더링 영역을 정의하는 뷰포트 정보
 *
 * @param DefaultVertexShader
 * @param DefaultPixelShader
 * @param DefaultInputLayout
 * @param Stride
 *
 * @param vertexBufferSphere
 * @param numVerticesSphere
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
	void CreateConstantBuffer();
	void CreateRasterizerState();
	void CreateDepthStencilState();

	void CreateBillboardResources();
	void CreateSpotlightResources();

	void CreateDefaultShader();
	void CreateTextureShader();
	void CreateProjectionDecalShader();
	void CreateSpotlightShader();
	void CreateSceneDepthViewModeShader();
	void CreateHeightFogShader();

	// Release
	void ReleaseConstantBuffer();
	void ReleaseRasterizerState();
	void ReleaseDepthStencilState();

	void ReleaseBillboardResources();
	void ReleaseSpotlightResources();

	void ReleaseDefaultShader();
	void ReleaseTextureShader();
	void ReleaseProjectionDecalShader();
	void ReleaseSpotlightShader();
	void ReleaseSceneDepthViewModeShader();
	void ReleaseHeightFogShader();

	// Render
	void Tick(float DeltaSeconds);
	void RenderBegin() const;
	void RenderLevel(UCamera* InCurrentCamera, FViewportClient& InViewportClient);
	void RenderEnd() const;

	void RenderStaticMesh(UPipeline& InPipeline, UStaticMeshComponent* InMeshComp, ID3D11RasterizerState* InRasterizerState, ID3D11Buffer* InConstantBufferModels, ID3D11Buffer* InConstantBufferMaterial);
	void RenderBillboard(UBillboardComponent* InBillboardComp, UCamera* InCurrentCamera);
	void RenderText(UTextRenderComponent* InBillBoardComp, UCamera* InCurrentCamera);
	void RenderPrimitiveDefault(UPipeline& InPipeline, UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState, ID3D11Buffer* InConstantBufferModels, ID3D11Buffer* InConstantBufferColor);
	void RenderPrimitiveLine(UPipeline& InPipeline, UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState, ID3D11Buffer* InConstantBufferModels, ID3D11Buffer* InConstantBufferColor);
	void RenderEditorPrimitive(UPipeline& InPipeline, const FEditorPrimitive& InEditorPrimitive, const FRenderState& InRenderState);
	void RenderEditorPrimitiveIndexed(UPipeline& InPipeline, const FEditorPrimitive& InEditorPrimitive, const FRenderState& InRenderState,
	                            bool bInUseBaseConstantBuffer, uint32 InStride, uint32 InIndexBufferStride);
	void RenderDecals(UCamera* InCurrentCamera, const TArray<TObjectPtr<UDecalComponent>>& InDecals,
		const TArray<TObjectPtr<UPrimitiveComponent>>& InVisiblePrimitives);
	void RenderLights(UCamera* InCurrentCamera, const TArray<TObjectPtr<USpotLightComponent>>& InSpotlights,
		const TArray<TObjectPtr<UPrimitiveComponent>>& InVisiblePrimitives);
	void RenderSceneDepthView(UCamera* InCurrentCamera, const FViewportClient& InViewportClient);
	void RenderHeightFog(UCamera* InCurrentCamera, const FViewportClient& InViewportClient);

	void OnResize(uint32 Inwidth = 0, uint32 InHeight = 0);

	// Create function
	void CreateVertexShaderAndInputLayout(const wstring& InFilePath,
		const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescriptions,
		ID3D11VertexShader** OutVertexShader, ID3D11InputLayout** OutInputLayout);
	ID3D11Buffer* CreateVertexBuffer(FNormalVertex* InVertices, uint32 InByteWidth) const;
	ID3D11Buffer* CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const;
	ID3D11Buffer* CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const;
	void CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** InPixelShader) const;

	bool UpdateVertexBuffer(ID3D11Buffer* InVertexBuffer, const TArray<FVector>& InVertices) const;

	template<typename T>
	void UpdateConstant(ID3D11Buffer* InBuffer, const T& InData, uint32 InSlot,
		bool bBindToVertexShader, bool bBindToPixelShader)
	{

		if (!InBuffer) return;

		// 1. 상수 버퍼의 메모리에 접근하여 데이터를 업데이트합니다.
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		HRESULT hr = GetDeviceContext()->Map(InBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		if (FAILED(hr)) return;

		memcpy(MappedResource.pData, &InData, sizeof(T));
		GetDeviceContext()->Unmap(InBuffer, 0);

		// 2. 지정된 슬롯에 상수 버퍼를 바인딩합니다.
		if (bBindToVertexShader)
		{
			GetDeviceContext()->VSSetConstantBuffers(InSlot, 1, &InBuffer);
		}
		if (bBindToPixelShader)
		{
			GetDeviceContext()->PSSetConstantBuffers(InSlot, 1, &InBuffer);
		}
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
	bool GetIsResizing() const { return bIsResizing; }
	bool GetOcclusionCullingEnabled() const { return bOcclusionCulling; }
	bool GetFXAAEnabled() const { return bFXAAEnabled; }

	void SetIsResizing(bool isResizing) { bIsResizing = isResizing; }
	void SetOcclusionCullingEnabled(bool bEnabled) { bOcclusionCulling = bEnabled; }
	void ResetOcclusionCullingState() { bIsFirstPass = true; }
    void SetFXAAEnabled(bool bEnabled) { bFXAAEnabled = bEnabled; }

private:
	void PerformOcclusionCulling(UCamera* InCurrentCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents);
	void RenderPrimitiveComponent(UPipeline& InPipeline, UPrimitiveComponent* InPrimitiveComponent, ID3D11RasterizerState* InRasterizerState, ID3D11Buffer* InConstantBufferModels, ID3D11Buffer* InConstantBufferColor, ID3D11Buffer* InConstantBufferMaterial);
	void RenderLevel_SingleThreaded(UCamera* InCurrentCamera, FViewportClient& InViewportClient, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents);
	void RenderLevel_MultiThreaded(UCamera* InCurrentCamera, FViewportClient& InViewportClient, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents);

	UPipeline* Pipeline = nullptr;
	UDeviceResources* DeviceResources = nullptr;
	UFontRenderer* FontRenderer = nullptr;
	TArray<UPrimitiveComponent*> PrimitiveComponents;

	ID3D11DepthStencilState* DefaultDepthStencilState = nullptr;
	ID3D11DepthStencilState* DisabledDepthStencilState = nullptr;
	ID3D11DepthStencilState* DisableDepthWriteDepthStencilState = nullptr;
	ID3D11Buffer* ConstantBufferModels = nullptr;
	ID3D11Buffer* ConstantBufferViewProj = nullptr;
	ID3D11Buffer* ConstantBufferColor = nullptr;
	ID3D11Buffer* ConstantBufferBatchLine = nullptr;
	ID3D11Buffer* ConstantBufferMaterial = nullptr;
	ID3D11Buffer* ConstantBufferProjectionDecal = nullptr;
	ID3D11Buffer* ConstantBufferSpotlight = nullptr;
	ID3D11Buffer* ConstantBufferDepth2D = nullptr;
	ID3D11Buffer* ConstantBufferDepth = nullptr;
	ID3D11Buffer* ConstantBufferHeightFog = nullptr;

	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };

	ID3D11VertexShader* DefaultVertexShader = nullptr;
	ID3D11PixelShader* DefaultPixelShader = nullptr;
	ID3D11InputLayout* DefaultInputLayout = nullptr;

	ID3D11VertexShader* TextureVertexShader = nullptr;
	ID3D11PixelShader* TexturePixelShader = nullptr;
	ID3D11InputLayout* TextureInputLayout = nullptr;

	ID3D11Buffer* BillboardVertexBuffer = nullptr;
	ID3D11Buffer* BillboardIndexBuffer = nullptr;
	ID3D11BlendState* BillboardBlendState = nullptr;

	ID3D11VertexShader* ProjectionDecalVertexShader = nullptr;
	ID3D11PixelShader* ProjectionDecalPixelShader = nullptr;
	ID3D11InputLayout* ProjectionDecalInputLayout = nullptr;
	ID3D11BlendState* ProjectionDecalBlendState = nullptr;

	ID3D11VertexShader* SpotlightVertexShader = nullptr;
	ID3D11PixelShader* SpotlightPixelShader = nullptr;
	ID3D11InputLayout* SpotlightInputLayout = nullptr;
	ID3D11BlendState* SpotlightBlendState = nullptr;

	ID3D11VertexShader* SceneDepthVertexShader = nullptr;
	ID3D11PixelShader* SceneDepthPixelShader= nullptr;

	ID3D11VertexShader* HeightFogVertexShader = nullptr;
	ID3D11PixelShader* HeightFogPixelShader= nullptr;

	class UFXAAPass* FXAA = nullptr;

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

	ID3D11RasterizerState* GetRasterizerState(const FRenderState& InRenderState);

	bool bIsResizing = false;

	bool bIsFirstPass = true;
	bool bOcclusionCulling = true;
	bool bFXAAEnabled = true;

	bool bIsSceneDepth = true;

	constexpr static size_t NUM_WORKER_THREADS = 4;

	TArray<ID3D11DeviceContext*> DeferredContexts;
	TArray<ID3D11Buffer*> ThreadConstantBufferModels;
	TArray<ID3D11Buffer*> ThreadConstantBufferColors;
	TArray<ID3D11Buffer*> ThreadConstantBufferMaterials;
	TArray<ID3D11Buffer*> ThreadConstantBufferProjectionDecals;
	TArray<ID3D11CommandList*> CommandLists;
};
