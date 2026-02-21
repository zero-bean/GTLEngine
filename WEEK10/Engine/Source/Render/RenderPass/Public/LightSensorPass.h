#pragma once
#include "Render/RenderPass/Public/RenderPass.h"
#include <queue>
#include <functional>

/**
 * @brief Light Sensor 요청 구조체
 */
struct FLightSensorRequest
{
	FVector WorldPosition;
	std::function<void(float)> Callback;
};

/**
 * @brief GPU 처리 중인 Readback 대기 구조체
 */
struct FPendingReadback
{
	TArray<FLightSensorRequest> Requests;
	uint32 Count;
};

/**
 * @brief 특정 월드 좌표의 조명 세기를 측정하는 Render Pass
 *
 * ULightSensorComponent의 요청을 배치 처리하여 GPU에서 조명 계산을 수행합니다.
 * FLightPass가 설정한 리소스를 FRenderingContext를 통해 재사용하며,
 * Compute Shader를 사용하여 Shadow Map을 포함한 정확한 조명 계산을 수행합니다.
 *
 * 실행 흐름:
 *   Frame N:   Component::EnqueueRequest() → RequestQueue
 *   Frame N+1: Execute() → Dispatch CS → CopyResource → PendingReadbacks
 *   Frame N+2: Execute() → Map() → Callback() → Component
 */
class FLightSensorPass : public FRenderPass
{
public:
	FLightSensorPass(UPipeline* InPipeline, ID3D11Buffer* InCameraConstantBuffer);

	void SetRenderTargets(class UDeviceResources* DeviceResources) override;
	virtual void Execute(FRenderingContext& Context) override;
	virtual void Release() override;

	/**
	 * @brief 조명 측정 요청을 큐에 추가
	 * @param Request 월드 좌표와 콜백을 포함한 요청
	 */
	void EnqueueRequest(const FLightSensorRequest& Request);

	/**
	 * @brief 대기 중인 요청 개수 반환
	 */
	uint32 GetPendingRequestCount() const { return static_cast<uint32>(RequestQueue.size()); }

private:
	/**
	 * @brief 이전 프레임에서 Dispatch한 결과를 읽어 Callback 호출
	 */
	void ProcessPendingReadbacks();

	/**
	 * @brief RequestQueue에서 요청을 가져와 GPU Dispatch
	 */
	void ProcessNewRequests(FRenderingContext& Context);

private:
	/** 배치 처리 최대 크기 (Compute Shader의 numthreads와 일치) */
	static constexpr uint32 MaxBatchSize = 32;

	/** Compute Shader */
	ID3D11ComputeShader* LightSensorCS = nullptr;

	/** Sensor 위치 배열 Constant Buffer (cbuffer SensorParams) */
	ID3D11Buffer* SensorParamsConstantBuffer = nullptr;

	/** 출력 버퍼 (RWStructuredBuffer<float>) */
	ID3D11Buffer* OutputBuffer = nullptr;

	/** CPU Readback용 Staging Buffer */
	ID3D11Buffer* ReadbackBuffer = nullptr;

	/** Output Buffer UAV */
	ID3D11UnorderedAccessView* OutputBufferUAV = nullptr;

	/** 대기 중인 요청 큐 */
	std::queue<FLightSensorRequest> RequestQueue;

	/** GPU 처리 중인 Readback 대기 큐 */
	std::queue<FPendingReadback> PendingReadbacks;
};
