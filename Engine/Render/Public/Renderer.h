#pragma once
#include "Core/Public/Object.h"

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
 * @param SimpleVertexShader
 * @param SimplePixelShader
 * @param SimpleInputLayout
 * @param Stride
 *
 * @param vertexBufferSphere
 * @param numVerticesSphere
 */
class URenderer :
	public UObject
{
DECLARE_SINGLETON(URenderer)

public:
	void Create(HWND InWindowHandle);
	void CreateDeviceAndSwapChain(HWND InWindowHandle);
	void ReleaseDeviceAndSwapChain();
	void CreateFrameBuffer();
	void ReleaseFrameBuffer();
	void CreateRasterizerState();
	void ReleaseRasterizerState();

	void ReleaseResource();

	// 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
	void SwapBuffer() const;

	void CreateShader();
	void ReleaseShader();
	void Prepare() const;
	void PrepareShader() const;
	void RenderPrimitive() const;
	void RenderRectangle() const;
	void RenderTriangle() const;
	void RenderLines(const FVertex* InVertices, UINT InCount) const;

	ID3D11Buffer* CreateVertexBuffer(FVertex* InVertices, UINT InByteWidth) const;
	ID3D11Buffer* CreateIndexBuffer(const void* InIndices, UINT InByteWidth) const;
	static void ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer);
	void CreateConstantBuffer();
	void ReleaseConstantBuffer();
	void UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const;

	void Init(HWND InWindowHandle);
	void Release();

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;
	ID3D11RasterizerState* RasterizerState = nullptr;
	ID3D11Buffer* ConstantBuffer = nullptr;

	FLOAT ClearColor[4] = {0.025f, 0.025f, 0.025f, 1.0f};
	D3D11_VIEWPORT ViewportInfo;
	D3D11_VIEWPORT UIViewportInfo;

	ID3D11VertexShader* SimpleVertexShader;
	ID3D11PixelShader* SimplePixelShader;
	ID3D11InputLayout* SimpleInputLayout;
	unsigned int Stride;

	ID3D11Buffer* vertexBufferSphere;
	UINT numVerticesSphere;

	// Rectangle용 버퍼
	ID3D11Buffer* vertexBufferRectangle = nullptr;
	ID3D11Buffer* indexBufferRectangle = nullptr;
	UINT numIndicesRectangle = 0;

	// Triangle용 버퍼
	ID3D11Buffer* vertexBufferTriangle = nullptr;
};
