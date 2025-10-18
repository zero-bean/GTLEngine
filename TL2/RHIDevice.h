#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include "Vector.h"
#include "CBufferTypes.h"

#define DECLARE_CBUFFER_UPDATE_FUNC(TYPE)\
    virtual void UpdateCBuffer(const TYPE& CBufferData) = 0;
#define DECLARE_CBUFFER_UPDATE_SET_FUNC(TYPE)\
    virtual void UpdateSetCBuffer(const TYPE& CBufferData) = 0;
#define DECLARE_CBUFFER_SET_FUNC(TYPE)\
    virtual void SetCBuffer(const TYPE& CBufferData) = 0;


enum class EComparisonFunc
{
    Always,
    LessEqual,
    GreaterEqual,
    // 필요하면 추가
      // 필요시 추가
    Disable,
    LessEqualReadOnly,
};

enum class ERenderTargetType
{
    None = 1 << 0,
    Frame = 1 << 1,
    ID = 1 << 2,
    Temporal = 1<< 3,
    //for SRV
    Depth = 1<<4,
    //for RTV
    NoDepth = 1<<5,
};

// 비트 OR 연산자
inline ERenderTargetType operator|(ERenderTargetType lhs, ERenderTargetType rhs)
{
    return static_cast<ERenderTargetType>(
        static_cast<int>(lhs) | static_cast<int>(rhs)
        );
}

// 비트 AND 연산자
inline ERenderTargetType operator&(ERenderTargetType lhs, ERenderTargetType rhs)
{
    return static_cast<ERenderTargetType>(
        static_cast<int>(lhs) & static_cast<int>(rhs)
        );
}
class URHIDevice
{
public:
    URHIDevice() {};
    virtual ~URHIDevice() {};

private:
    // 복사생성자, 연산자 금지
    URHIDevice(const URHIDevice& RHIDevice) = delete;
    URHIDevice& operator=(const URHIDevice& RHIDevice) = delete;

public:
    virtual void Initialize(HWND hWindow) = 0;
    virtual void Release() = 0;
public:
    //getter
    virtual ID3D11Device* GetDevice() = 0;
    virtual ID3D11DeviceContext* GetDeviceContext() = 0;
    virtual IDXGISwapChain* GetSwapChain() = 0;

    // create
    virtual void CreateDeviceAndSwapChain(HWND hWindow) = 0;;
    virtual void CreateFrameBuffer() = 0;
    virtual void CreateRasterizerState() = 0;
    virtual void CreateConstantBuffer() = 0;
    virtual void CreateBlendState() = 0;
	virtual void CreateDepthStencilState() = 0;

    // update
    CBUFFER_TYPE_LIST(DECLARE_CBUFFER_UPDATE_FUNC)
    CBUFFER_TYPE_LIST(DECLARE_CBUFFER_UPDATE_SET_FUNC)
    CBUFFER_TYPE_LIST(DECLARE_CBUFFER_SET_FUNC)

    // clear
    virtual void ClearBackBuffer() = 0;
    virtual void ClearDepthBuffer(float Depth, UINT Stenci) = 0;

    virtual void IASetPrimitiveTopology() = 0;
    virtual void RSSetViewport() = 0;
    virtual void RSSetState(EViewModeIndex ViewModeIndex) = 0;
    virtual void RSSetFrontCullState() = 0;
    virtual void RSSetNoCullState() = 0;
    virtual void RSSetDefaultState() = 0;
    virtual void RSSetDecalState() = 0;
    virtual void OMSetDepthOnlyTarget() = 0;
    virtual void OMSetRenderTargets(const ERenderTargetType RenderTargetType) = 0;
    virtual void PSSetRenderTargetSRV(const ERenderTargetType RenderTargetType) = 0;
    virtual void OMSetBlendState(bool bIsBlendMode) = 0;
    virtual void OmSetDepthStencilState(EComparisonFunc Func) = 0;
    virtual void Present() = 0;
    virtual void PSSetDefaultSampler(UINT StartSlot) = 0;
};

