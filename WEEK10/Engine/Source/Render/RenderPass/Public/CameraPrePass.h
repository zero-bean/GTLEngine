#pragma once
#include "RenderPass.h"

class FCameraPrePass : public FRenderPass
{
public:
	FCameraPrePass(UPipeline* InPipeline);

	void SetRenderTargets(UDeviceResources* DeviceResources) override;
	void Execute(FRenderingContext& Context) override;
	void Release() override;
};
