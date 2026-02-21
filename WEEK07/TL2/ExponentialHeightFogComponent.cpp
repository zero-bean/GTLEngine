#include "pch.h"
#include "ExponentialHeightFogComponent.h"
#include "MeshLoader.h"
#include "ImGui/imgui.h"

void UExponentialHeightFogComponent::Render(URenderer* Renderer, const FVector& CameraPosition, const FMatrix& View, const FMatrix& Projection, FViewport* Viewport)
{

	//상수버퍼 업데이트
	Renderer->UpdateSetCBuffer(FHeightFogBufferType(
		FogInscatteringColor,
		FogDensity,
		FogHeightFalloff,
		StartDistance,
		FogCutoffDistance,
		FogMaxOpacityDistance,
		FogMaxOpacity,
		//2열에 있던 Z가 Y로 가서 Y를 써야하는 게 아니라 이미 Z가 up으로 변환된 상태(바뀐 기저축상의 점을 월드좌표계로 나타냄)이므로
		//Z값을 써야 이미 바뀐 높이값을 쓸 수 있음(Y는 이미 높이가 아니라 깊이를 나타냄)
		this->GetWorldLocation().Z)); 

	FMatrix ViewProj = View * Projection;

	Renderer->UpdateSetCBuffer(ViewProjBufferType(View, Projection, CameraPosition));
	Renderer->UpdateSetCBuffer(FViewProjectionInverse(ViewProj.Inverse()));

	Renderer->RenderPostProcessing(UResourceManager::GetInstance().Load<UShader>("HeightFogShader.hlsl"));
}

UObject* UExponentialHeightFogComponent::Duplicate()
{
	UExponentialHeightFogComponent* DuplicatedComponent = Cast<UExponentialHeightFogComponent>(NewObject(GetClass()));
	if (DuplicatedComponent)
	{
		CopyCommonProperties(DuplicatedComponent);
		DuplicatedComponent->FogDensity = FogDensity;
		DuplicatedComponent->FogHeightFalloff = FogHeightFalloff;
		DuplicatedComponent->StartDistance = StartDistance;
		DuplicatedComponent->FogCutoffDistance = FogCutoffDistance;
		DuplicatedComponent->FogMaxOpacityDistance = FogMaxOpacityDistance;
		DuplicatedComponent->FogMaxOpacity = FogMaxOpacity;

		DuplicatedComponent->FogInscatteringColor = FogInscatteringColor;
	}
	DuplicatedComponent->DuplicateSubObjects();

	return DuplicatedComponent;
}

void UExponentialHeightFogComponent::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();
}

void UExponentialHeightFogComponent::RenderDetails()
{
	FFogInfo FogInfo = GetFogInfo();
	ImGui::Text("Exponential Height Fog Component");

	ImGui::DragFloat("Fog Density", &FogInfo.FogDensity, 0.001f, 0.0f, 10.0f);
	ImGui::DragFloat("Fog Height Falloff", &FogInfo.FogHeightFalloff, 0.0001f, 0.0f, 10.0f);
	ImGui::DragFloat("Start Distance", &FogInfo.StartDistance, 0.1f, 0.0f);
	ImGui::DragFloat("Fog Max Opacity", &FogInfo.FogMaxOpacity, 0.001f, 0.0f, 1.0f);
	ImGui::DragFloat("Fog Max Opacity Distance", &FogInfo.FogMaxOpacityDistance, 100.0f, 0.0f);
	ImGui::DragFloat("Fog Cutoff Distance", &FogInfo.FogCutoffDistance, 100.0f, 0.0f);
	float Color[3]{ FogInfo.FogInscatteringColor.R, FogInfo.FogInscatteringColor.G, FogInfo.FogInscatteringColor.B };
	if (ImGui::ColorEdit3("Fog Inscattering Color", Color))
	{
		FogInfo.FogInscatteringColor.R = Color[0];
		FogInfo.FogInscatteringColor.G = Color[1];
		FogInfo.FogInscatteringColor.B = Color[2];
	}
	bool bIsRender = IsRender();
	ImGui::Checkbox("Render", &bIsRender);
	SetShowflag(bIsRender);
	SetFogInfo(FogInfo);
}
