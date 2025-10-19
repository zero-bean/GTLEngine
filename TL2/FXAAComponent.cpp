#include "pch.h"
#include "FXAAComponent.h"
#include "ImGui/imgui.h"

void UFXAAComponent::Render(URenderer* Renderer)
{
	Renderer->UpdateSetCBuffer(FXAABufferType(SlideX, SpanMax, (ReduceMin / 128.0f), ReduceMul));
	Renderer->RenderPostProcessing(UResourceManager::GetInstance().Load<UShader>("FXAA.hlsl"));
}

UObject* UFXAAComponent::Duplicate()
{
	UFXAAComponent* DuplicatedComponent = Cast<UFXAAComponent>(NewObject(GetClass()));
	if (DuplicatedComponent)
	{
		CopyCommonProperties(DuplicatedComponent);
		DuplicatedComponent->SlideX = SlideX;
		DuplicatedComponent->SpanMax = SpanMax;
		DuplicatedComponent->ReduceMin = ReduceMin;
		DuplicatedComponent->ReduceMul = ReduceMul;
	}
	DuplicatedComponent->DuplicateSubObjects();

	return DuplicatedComponent;
}

void UFXAAComponent::DuplicateSubObjects()
{
	USceneComponent::DuplicateSubObjects();
}

void UFXAAComponent::RenderDetails()
{
	float SlideX = GetSlideX();
	float SpanMax = GetSpanMax();
	int ReduceMin = GetReduceMin();
	float ReduceMul = GetReduceMul();
	if (ImGui::DragFloat("SlideX", &SlideX, 0.01f, 0, 1))
	{
		SetSlideX(SlideX);
	}
	if (ImGui::DragFloat("SpanMax", &SpanMax, 0.01f, 0, 12))
	{
		SetSpanMax(SpanMax);
	}
	if (ImGui::DragInt("ReduceMin", &ReduceMin, 1.0f, 0, 128))
	{
		SetReduceMin(ReduceMin);
	}
	if (ImGui::DragFloat("ReduceMul", &ReduceMul, 0.01f, 0, 1))
	{
		SetReduceMul(ReduceMul);
	}
}