#include "pch.h"
#include "Component/Public/LightSensorComponent.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/RenderPass/Public/LightSensorPass.h"
#include "Render/UI/Widget/Public/LightSensorComponentWidget.h"
#include "Component/Public/ScriptComponent.h"  // FDelegateInfo 템플릿 구현용

IMPLEMENT_CLASS(ULightSensorComponent, USceneComponent)

ULightSensorComponent::ULightSensorComponent()
{
	bCanEverTick = true;
}

void ULightSensorComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("FramesPerRequest"))
		{
			bool success = false;
			FramesPerRequest = InOutHandle["FramesPerRequest"].ToInt(success);
			if (!success) FramesPerRequest = 5;
		}

		if (InOutHandle.hasKey("Enabled"))
		{
			bool success = false;
			bEnabled = InOutHandle["Enabled"].ToBool(success);
			if (!success) bEnabled = true;
		}
	}
	else
	{
		InOutHandle["FramesPerRequest"] = FramesPerRequest;
		InOutHandle["Enabled"] = bEnabled;
	}
}

UObject* ULightSensorComponent::Duplicate()
{
	ULightSensorComponent* Clone = NewObject<ULightSensorComponent>();
	Clone->FramesPerRequest = FramesPerRequest;
	Clone->bEnabled = bEnabled;
	Clone->CurrentLuminance = CurrentLuminance;
	Clone->PreviousLuminance = PreviousLuminance;

	// Transform 복사
	Clone->SetRelativeLocation(GetRelativeLocation());
	Clone->SetRelativeRotation(GetRelativeRotation());
	Clone->SetRelativeScale3D(GetRelativeScale3D());

	return Clone;
}

void ULightSensorComponent::BeginPlay()
{
	Super::BeginPlay();

	// 초기 측정
	FrameCounter = 0;
	CurrentLuminance = 0.0f;
	PreviousLuminance = 0.0f;
	bPendingRequest = false;
}

void ULightSensorComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	if (!bEnabled)
		return;

	// 프레임 카운터 증가
	FrameCounter++;

	// FramesPerRequest마다 측정 요청
	if (FrameCounter >= FramesPerRequest)
	{
		FrameCounter = 0;

		// 이미 요청 중이 아니면 새 요청
		if (!bPendingRequest)
		{
			RequestCalculation();
		}
	}
}

void ULightSensorComponent::EndPlay()
{
	Super::EndPlay();
}

UClass* ULightSensorComponent::GetSpecificWidgetClass() const
{
	return ULightSensorComponentWidget::StaticClass();
}

TArray<FDelegateInfoBase*> ULightSensorComponent::GetDelegates() const
{
	TArray<FDelegateInfoBase*> Result;

	// const_cast 필요 (Delegate는 mutable)
	ULightSensorComponent* MutableThis = const_cast<ULightSensorComponent*>(this);

	Result.Add(MakeDelegateInfo("OnLightIntensityChanged", &MutableThis->OnLightIntensityChanged));

	return Result;
}

void ULightSensorComponent::RequestCalculation()
{
	// Renderer를 통해 LightSensorPass 접근
	URenderer& Renderer = URenderer::GetInstance();
	FLightSensorPass* SensorPass = Renderer.GetLightSensorPass();

	if (!SensorPass)
	{
		UE_LOG("[LightSensor] ERROR: SensorPass is nullptr!");
		return;
	}

	// 요청 생성
	FLightSensorRequest Request;
	Request.WorldPosition = GetWorldLocation();
	Request.Callback = [this](float Luminance)
	{
		this->OnCalculationComplete(Luminance);
	};

	// 큐에 추가
	SensorPass->EnqueueRequest(Request);
	bPendingRequest = true;
}

void ULightSensorComponent::OnCalculationComplete(float Luminance)
{
	bPendingRequest = false;

	// 이전 값 저장
	PreviousLuminance = CurrentLuminance;

	// 현재 값 업데이트
	CurrentLuminance = Luminance;

	// Delegate 호출 (Lua 스크립트로 전달)
	OnLightIntensityChanged.Broadcast(CurrentLuminance, PreviousLuminance);
}
