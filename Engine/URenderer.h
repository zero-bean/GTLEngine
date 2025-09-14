#pragma once
#include "stdafx.h"
#include "UMesh.h"
#include "Matrix.h"
#include "UEngineSubsystem.h"

// URenderer.h or cpp 상단
struct CBTransform
{
	// HLSL의 row_major float4x4와 메모리 호환을 위해 float[16]로 보냄
	float MVP[16];
	float MeshColor[4];
	float IsSelected;
	float padding[3];
};

// 행벡터 규약: p' = p * M
static inline void TransformPosRow(float& x, float& y, float& z, const FMatrix& M) {
	const float X = x, Y = y, Z = z;
	x = X * M.M[0][0] + Y * M.M[1][0] + Z * M.M[2][0] + M.M[3][0];
	y = X * M.M[0][1] + Y * M.M[1][1] + Z * M.M[2][1] + M.M[3][1];
	z = X * M.M[0][2] + Y * M.M[1][2] + Z * M.M[2][2] + M.M[3][2];
}
struct FBatchLineList
{
	TArray<FVertexPosColor4> Vertices{};
	TArray<uint32> Indices{};
	ID3D11VertexShader* vertexShader = nullptr;
	ID3D11PixelShader* pixelShader = nullptr;
	ID3D11InputLayout* inputLayout = nullptr;
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;   
	size_t MaxVertex = 0;
	size_t MaxIndex = 0;

	void Clear()
	{
		Vertices.clear();
		Indices.clear();
	}
};

class URenderer : UEngineSubsystem
{
	DECLARE_UCLASS(URenderer, UEngineSubsystem)
public:
	void SubmitLineList(const TArray<FVertexPosColor4>& vertices,
		const TArray<uint32>& indices,
		const FMatrix& model); // NEW
private:
	// Batch Rendering
	FBatchLineList batchLineList;
	// URenderer.h (선언부)

	// Core D3D11 objects
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	IDXGISwapChain* swapChain;
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;
	ID3D11RasterizerState* rasterizerState;

	// Shader objects
	/* Comment: 이제 TMap으로 관리합니다. 
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* inputLayout;
	*/

	// TMap으로 관리
	TMap<FString, ID3D11InputLayout*> InputLayouts;
	TMap<FString, ID3D11PixelShader*> PixelShaders;
	TMap<FString, ID3D11VertexShader*> VertexShaders;

	// Constant buffer
	ID3D11Buffer* constantBuffer;

	// Viewport
	D3D11_VIEWPORT viewport;
	D3D11_VIEWPORT currentViewport; // 실제로 사용할 뷰포트(레터박스/필러박스 포함)
	float targetAspect = 16.0f / 9.0f;

	// Window handle
	HWND hWnd;

	// Render state
	bool bIsInitialized;

	FMatrix mVP;                 // 프레임 캐시
	CBTransform   mCBData;

public:
	URenderer();
	~URenderer();

	// Initialization and cleanup
	bool Initialize(HWND windowHandle);
	bool CreateShader();
	bool CreateRasterizerState();
	bool CreateConstantBuffer();
	void Release();
	void ReleaseShader();
	void ReleaseConstantBuffer();

	// Buffer creation
	ID3D11Buffer* CreateVertexBuffer(const void* data, size_t sizeInBytes);
	ID3D11Buffer* CreateIndexBuffer(const void* data, size_t sizeInBytes);
	bool ReleaseVertexBuffer(ID3D11Buffer* buffer);
	bool ReleaseIndexBuffer(ID3D11Buffer* buffer);

	// Texture creation
	ID3D11Texture2D* CreateTexture2D(int32 width, int32 height, DXGI_FORMAT format,
		const void* data = nullptr);
	ID3D11ShaderResourceView* CreateShaderResourceView(ID3D11Texture2D* texture);
	bool ReleaseTexture(ID3D11Texture2D* texture);
	bool ReleaseShaderResourceView(ID3D11ShaderResourceView* srv);

	// Rendering operations
	void Prepare();
	void PrepareShader();
	void SwapBuffer();
	void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

	// Drawing operations
	void DrawIndexed(UINT indexCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0);
	void Draw(UINT vertexCount, UINT startVertexLocation = 0);
	void DrawMesh(UMesh* mesh);
	void DrawMeshOnTop(UMesh* mesh);

	// Resource binding
	void SetVertexBuffer(ID3D11Buffer* buffer, UINT stride, UINT offset = 0);
	void SetIndexBuffer(ID3D11Buffer* buffer, DXGI_FORMAT format = DXGI_FORMAT_R32_UINT);
	void SetConstantBuffer(ID3D11Buffer* buffer, UINT slot = 0);
	void SetTexture(ID3D11ShaderResourceView* srv, UINT slot = 0);

	// Constant buffer updates
	bool UpdateConstantBuffer(const void* data, size_t sizeInBytes);

	// Batch Mode only for LineList
	void BeginBatchLineList();       
	void SubmitLineList(const UMesh* mesh); 
	void SubmitLineList(const TArray<FVertexPosColor4>& vertices, const TArray<uint32>& indices);
	void FlushBatchLineList();  // Draw Call 1회 처리

	// Window resize handling
	bool ResizeBuffers(int32 width, int32 height);

	// Getters
	ID3D11Device* GetDevice() const { return device; }
	ID3D11DeviceContext* GetDeviceContext() const { return deviceContext; }
	IDXGISwapChain* GetSwapChain() const { return swapChain; }
	bool IsInitialized() const { return bIsInitialized; }

	ID3D11InputLayout* GetInputLayout(const FString& name) { return InputLayouts[name]; }
	ID3D11VertexShader* GetVertexShader(const FString& name) { return VertexShaders[name]; }
	ID3D11PixelShader* GetPixelShader(const FString& name) { return PixelShaders[name]; }


	// Utility functions
	bool CheckDeviceState();
	void GetBackBufferSize(int32& width, int32& height);

private:
	// Internal helper functions
	bool CreateDeviceAndSwapChain(HWND windowHandle);
	bool CreateRenderTargetView();
	bool CreateDepthStencilView(int32 width, int32 height);
	bool SetupViewport(int32 width, int32 height);
	
	// Internal helper - Batch Rendering
	void EnsureBatchCapacity(FBatchLineList& B, size_t vNeed, size_t iNeed);

	// Error handling
	void LogError(const char* function, HRESULT hr);
	bool CheckResult(HRESULT hr, const char* function);

	// 행렬 복사 핼퍼
	static inline void CopyRowMajor(float dst[16], const FMatrix& src)
	{
		for (int32 r = 0; r < 4; ++r)
			for (int32 c = 0; c < 4; ++c)
				dst[r * 4 + c] = src.M[r][c];
	}
public:
	void SetViewProj(const FMatrix& V, const FMatrix& P); // 내부에 VP 캐시
	void SetModel(const FMatrix& M, const FVector4& color, bool IsSelected); // M*VP → b0 업로드
	void SetTargetAspect(float a) { if (a > 0.f) targetAspect = a; }
	// targetAspect를 내부에서 사용 (카메라에 의존 X)
	D3D11_VIEWPORT MakeAspectFitViewport(int32 winW, int32 winH) const;
	// 드래그 중 호출: currentViewport만 갈아끼움
	void UseAspectFitViewport(int32 winW, int32 winH)
	{
		currentViewport = MakeAspectFitViewport(winW, winH);
	}
	// 평소엔 풀 윈도우
	void UseFullWindowViewport()
	{
		currentViewport = viewport;
	}

};