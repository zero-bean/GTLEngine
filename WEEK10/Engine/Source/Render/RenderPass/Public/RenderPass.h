#pragma once
#include "Render/RenderPass/Public/RenderingContext.h"
#include <d3d11_1.h>

class UPipeline;

/**
 * @brief GPU 디버그 이벤트 마커 RAII 래퍼 (PIX, RenderDoc 등에서 표시)
 * 생성자에서 BeginEvent, 소멸자에서 EndEvent 자동 호출
 *
 * @note Execute 함수 Render 시작 지점에서 호출할 것, EndEvent는 자동 호출
 * void FooPass::Execute {
 *     GPU_EVENT(DeviceContext, "MyRenderPass");
 *     // Rendering Code
 * }
 */
class FGPUEventScope
{
public:
    FGPUEventScope(ID3D11DeviceContext* InContext, const char* InName)
        : Context(InContext), Annotation(nullptr)
    {
        if (!Context)
        {
	        return;
        }

        if (SUCCEEDED(Context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), reinterpret_cast<void**>(&Annotation))))
        {
            wstring WideName = ConvertToWide(InName);
            Annotation->BeginEvent(WideName.c_str());
        }
    }

    ~FGPUEventScope()
    {
        if (Annotation)
        {
            Annotation->EndEvent();
            Annotation->Release();
        }
    }

    FGPUEventScope(const FGPUEventScope&) = delete;
    FGPUEventScope& operator=(const FGPUEventScope&) = delete;

private:
    static wstring ConvertToWide(const char* InStr)
    {
        if (!InStr)
        {
	        return L"";
        }
        const int Len = MultiByteToWideChar(CP_UTF8, 0, InStr, -1, nullptr, 0);
        if (Len <= 0)
        {
	        return L"";
        }
        wstring Result(Len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, InStr, -1, Result.data(), Len);
        return Result;
    }

    ID3D11DeviceContext* Context;
    ID3DUserDefinedAnnotation* Annotation;
};

// GPU 이벤트 마킹 매크로
#define GPU_EVENT(Context, Name) FGPUEventScope TOKENPASTE(__GPUEvent_, __LINE__)(Context, Name)
#define TOKENPASTE(x, y) x##y

/**
 * @brief 특정 Primitive Type별로 달라지는 RenderPass를 관리하고 실행하도록 하는 기본 인터페이스
 */
class FRenderPass
{
public:
    FRenderPass(UPipeline* InPipeline)
        : Pipeline(InPipeline), ConstantBufferCamera(nullptr), ConstantBufferModel(nullptr) {}
    FRenderPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel)
        : Pipeline(InPipeline), ConstantBufferCamera(InConstantBufferCamera), ConstantBufferModel(InConstantBufferModel) {}

    virtual ~FRenderPass() = default;

	/**
	 * @brief 프레임마다 실행할 렌더 타겟 설정 함수
	 * Execute 전에 호출되어 해당 Pass의 렌더 타겟을 설정함
	 * @param DeviceResources RTV/DSV/Buffer 등을 담고 있는 DeviceResources 객체
	 */
	virtual void SetRenderTargets(class UDeviceResources* DeviceResources) = 0;

    /**
     * @brief 프레임마다 실행할 렌더 함수
     * @param Context 프레임 렌더링에 필요한 모든 정보를 담고 있는 객체
     */
    virtual void Execute(FRenderingContext& Context) = 0;

    /**
     * @brief 생성한 객체들 해제
     */
    virtual void Release() = 0;

protected:
    UPipeline* Pipeline;
    ID3D11Buffer* ConstantBufferCamera;
    ID3D11Buffer* ConstantBufferModel;
};
