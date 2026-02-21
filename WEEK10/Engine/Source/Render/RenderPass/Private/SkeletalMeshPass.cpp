#include "pch.h"

#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Render/RenderPass/Public/ShadowMapPass.h"
#include "Render/RenderPass/Public/SkeletalMeshPass.h"
#include "Render/Renderer/Public/DeviceResources.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Texture/Public/Texture.h"

struct FShadowMapResource;

FSkeletalMeshPass::FSkeletalMeshPass(UPipeline* InPipeline,
                                     ID3D11Buffer* InConstantBufferViewProj,
                                     ID3D11Buffer* InConstantBufferModel,
                                     ID3D11VertexShader* InVS,
                                     ID3D11PixelShader* InPS,
                                     ID3D11InputLayout* InLayout,
                                     ID3D11DepthStencilState* InDS)
	: FRenderPass(InPipeline, InConstantBufferViewProj, InConstantBufferModel)
	, VS(InVS)
	, PS(InPS)
	, InputLayout(InLayout)
	, DS(InDS)
{
	ConstantBufferMaterial = FRenderResourceFactory::CreateConstantBuffer<FMaterialConstants>();
}

void FSkeletalMeshPass::SetRenderTargets(class UDeviceResources* DeviceResources)
{
	ID3D11RenderTargetView* RTVs[] = { DeviceResources->GetDestinationRTV(), DeviceResources->GetNormalBufferRTV() };
	ID3D11DepthStencilView* DSV = DeviceResources->GetDepthBufferDSV();
	Pipeline->SetRenderTargets(2, RTVs, DSV);
}

void FSkeletalMeshPass::Execute(FRenderingContext& Context)
{
	const auto& Renderer = URenderer::GetInstance();
	GPU_EVENT(Renderer.GetDeviceContext(), "SkeletalMeshPass");
	FRenderState RenderState = UStaticMeshComponent::GetClassDefaultRenderState();
	if (Context.ViewMode == EViewModeIndex::VMI_Wireframe)
	{
		RenderState.CullMode = ECullMode::None;
		RenderState.FillMode = EFillMode::WireFrame;
	}
	else
	{
		VS = Renderer.GetVertexShader(Context.ViewMode);
		PS = Renderer.GetPixelShader(Context.ViewMode);
	}

	ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState(RenderState);
	FPipelineInfo PipelineInfo = { InputLayout, VS, RS, DS, PS, nullptr, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	Pipeline->UpdatePipeline(PipelineInfo);

	Pipeline->SetSamplerState(0, EShaderType::PS, Renderer.GetDefaultSampler());

	Pipeline->SetSamplerState(1, EShaderType::PS, Renderer.GetShadowComparisonSampler());

	Pipeline->SetSamplerState(2, EShaderType::PS, Renderer.GetVarianceShadowSampler());

	Pipeline->SetSamplerState(3, EShaderType::PS, Renderer.GetPointShadowSampler());

	FShadowMapPass* ShadowPass = Renderer.GetShadowMapPass();
	if (ShadowPass)
	{
		FShadowMapResource* ShadowAtlas = ShadowPass->GetShadowAtlas();
		if (ShadowAtlas && ShadowAtlas->IsValid())
		{
			Pipeline->SetShaderResourceView(10, EShaderType::PS, ShadowAtlas->ShadowSRV.Get());
			Pipeline->SetShaderResourceView(11, EShaderType::PS, ShadowAtlas->VarianceShadowSRV.Get());
		}
	}

	Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
	Pipeline->SetConstantBuffer(1, EShaderType::VS | EShaderType::PS, ConstantBufferCamera);

	if (!(Context.ShowFlags & EEngineShowFlags::SF_SkeletalMesh)) { return; }
	TArray<USkeletalMeshComponent*>& MeshComponents = Context.SkeletalMeshes;
	sort(MeshComponents.begin(), MeshComponents.end(),
		[](USkeletalMeshComponent* A, USkeletalMeshComponent* B) {
			int32 MeshA = A->GetSkeletalMeshAsset()->GetStaticMesh() ? A->GetSkeletalMeshAsset()->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			int32 MeshB = B->GetSkeletalMeshAsset()->GetStaticMesh() ? B->GetSkeletalMeshAsset()->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			return MeshA < MeshB;
		});

	FStaticMesh* CurrentMeshAsset = nullptr;
	UMaterial* CurrentMaterial = nullptr;

	for (USkeletalMeshComponent* MeshComp : Context.SkeletalMeshes)
	{
		if (!MeshComp->IsVisible()) { continue; }
		if (!MeshComp->GetSkeletalMeshAsset()) { continue; }
		FStaticMesh* MeshAsset = MeshComp->GetSkeletalMeshAsset()->GetStaticMesh()->GetStaticMeshAsset();

		// 스켈레탈 메시도 FNormalVertex를 사용
		Pipeline->SetVertexBuffer(MeshComp->GetVertexBuffer(), sizeof(FNormalVertex));
		Pipeline->SetIndexBuffer(MeshComp->GetIndexBuffer(), 0);
		CurrentMeshAsset = MeshAsset;

		FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, MeshComp->GetWorldTransformMatrix());
		Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

		if (MeshAsset->MaterialInfo.IsEmpty() || MeshComp->GetSkeletalMeshAsset()->GetStaticMesh()->GetNumMaterials() == 0)
		{
			Pipeline->DrawIndexed(static_cast<uint32>(MeshAsset->Indices.Num()), 0, 0);
			continue;
		}

		for (const FMeshSection& Section : MeshAsset->Sections)
		{
			UMaterial* Material = MeshComp->GetMaterial(Section.MaterialSlot);
			if (Material == nullptr)
			{
				FMaterialConstants MaterialConstants = {};
				MaterialConstants.Ka = FVector4(0.2f, 0.2f, 0.2f, 1.0f);
				MaterialConstants.Kd = FVector4(0.9f, 0.9f, 0.9f, 1.0f);
				MaterialConstants.Ks = FVector4(0.5f, 0.5f, 0.5f, 1.0f);
				MaterialConstants.Ns = 32.0f;
				//MaterialConstants.Ni = Material->GetRefractionIndex();
				MaterialConstants.D = 1.0f;
				MaterialConstants.MaterialFlags = 0;

				FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferMaterial, MaterialConstants);
				Pipeline->SetConstantBuffer(2, EShaderType::VS | EShaderType::PS, ConstantBufferMaterial);
			}
			else if (CurrentMaterial != Material) {
				FMaterialConstants MaterialConstants = {};
				FVector AmbientColor = Material->GetAmbientColor(); MaterialConstants.Ka = FVector4(AmbientColor.X, AmbientColor.Y, AmbientColor.Z, 1.0f);
				FVector DiffuseColor = Material->GetDiffuseColor(); MaterialConstants.Kd = FVector4(DiffuseColor.X, DiffuseColor.Y, DiffuseColor.Z, 1.0f);
				FVector SpecularColor = Material->GetSpecularColor(); MaterialConstants.Ks = FVector4(SpecularColor.X, SpecularColor.Y, SpecularColor.Z, 1.0f);
				MaterialConstants.Ns = Material->GetSpecularExponent();
				MaterialConstants.Ni = Material->GetRefractionIndex();
				MaterialConstants.D = Material->GetDissolveFactor();
				MaterialConstants.MaterialFlags = 0;
				if (Material->GetDiffuseTexture())  { MaterialConstants.MaterialFlags |= HAS_DIFFUSE_MAP; }
				if (Material->GetAmbientTexture())  { MaterialConstants.MaterialFlags |= HAS_AMBIENT_MAP; }
				if (Material->GetSpecularTexture()) { MaterialConstants.MaterialFlags |= HAS_SPECULAR_MAP; }
				if (Material->GetNormalTexture())   { MaterialConstants.MaterialFlags |= HAS_NORMAL_MAP; }
				if (!MeshComp->IsNormalMapEnabled())
				{
					MaterialConstants.MaterialFlags &= ~HAS_NORMAL_MAP;
				}
				if (Material->GetAlphaTexture())    { MaterialConstants.MaterialFlags |= HAS_ALPHA_MAP; }
				if (Material->GetBumpTexture())     { MaterialConstants.MaterialFlags |= HAS_BUMP_MAP; }
				MaterialConstants.Time = UTimeManager::GetInstance().GetGameTime();

				FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferMaterial, MaterialConstants);
				Pipeline->SetConstantBuffer(2, EShaderType::VS | EShaderType::PS, ConstantBufferMaterial);

				if (UTexture* DiffuseTexture = Material->GetDiffuseTexture())
				{
					Pipeline->SetShaderResourceView(0, EShaderType::PS, DiffuseTexture->GetTextureSRV());
					Pipeline->SetSamplerState(0, EShaderType::PS, DiffuseTexture->GetTextureSampler());
				}
				if (UTexture* AmbientTexture = Material->GetAmbientTexture())
				{
					Pipeline->SetShaderResourceView(1, EShaderType::PS, AmbientTexture->GetTextureSRV());
				}
				if (UTexture* SpecularTexture = Material->GetSpecularTexture())
				{
					Pipeline->SetShaderResourceView(2, EShaderType::PS, SpecularTexture->GetTextureSRV());
				}
				if (Material->GetNormalTexture() && MeshComp->IsNormalMapEnabled())
				{
					Pipeline->SetShaderResourceView(3, EShaderType::PS, Material->GetNormalTexture()->GetTextureSRV());
				}
				if (UTexture* AlphaTexture = Material->GetAlphaTexture())
				{
					Pipeline->SetShaderResourceView(4, EShaderType::PS, AlphaTexture->GetTextureSRV());
				}
				if (UTexture* BumpTexture = Material->GetBumpTexture())
				{ // 범프 텍스처 추가 그러나 범프 텍스처 사용하지 않아서 없을 것임. 무시 ㄱㄱ
					Pipeline->SetShaderResourceView(5, EShaderType::PS, BumpTexture->GetTextureSRV());
					// 필요한 경우 샘플러 지정
					// Pipeline->SetSamplerState(5, false, BumpTexture->GetTextureSampler());
				}
				CurrentMaterial = Material;
			}
				Pipeline->DrawIndexed(Section.IndexCount, Section.StartIndex, 0);
		}
	}
	Pipeline->SetConstantBuffer(2, EShaderType::PS, nullptr);

	// Unbind shadow maps to prevent resource hazards
	Pipeline->SetShaderResourceView(10, EShaderType::PS, nullptr);  // Shadow Atlas
	Pipeline->SetShaderResourceView(11, EShaderType::PS, nullptr);  // Variance Shadow Atlas
	Pipeline->SetShaderResourceView(12, EShaderType::PS, nullptr);  // Directional Light Tile Position
	Pipeline->SetShaderResourceView(13, EShaderType::PS, nullptr);  // Spotlight Tile Position
	Pipeline->SetShaderResourceView(14, EShaderType::PS, nullptr);  // Point Light Tile Position
}

void FSkeletalMeshPass::Release()
{
	SafeRelease(ConstantBufferMaterial);
}

