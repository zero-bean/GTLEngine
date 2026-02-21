#include "pch.h"
#include "Component/Public/BillBoardComponent.h""

#include "Manager/Asset/Public/AssetManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Physics/Public/AABB.h"
#include "Render/UI/Widget/Public/SpriteSelectionWidget.h"

IMPLEMENT_CLASS(UBillBoardComponent, UPrimitiveComponent)

UBillBoardComponent::UBillBoardComponent()
{
	Type = EPrimitiveType::Sprite;
	
    UAssetManager& ResourceManager = UAssetManager::GetInstance();

	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);

    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;       // UV가 범위를 벗어나면 클램프
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = URenderer::GetInstance().GetDevice()->CreateSamplerState(&SamplerDesc, &Sampler);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("CreateSamplerState failed (HRESULT: 0x%08lX)", hr);
        assert(false);
    }

    const TMap<FName, ID3D11ShaderResourceView*>& TextureCache = \
        UAssetManager::GetInstance().GetTextureCache();
    if (!TextureCache.empty())
        Sprite = *TextureCache.begin();
}

UBillBoardComponent::~UBillBoardComponent()
{
    if (Sampler)
        Sampler->Release();
}

void UBillBoardComponent::FaceCamera(
    const FVector& CameraPosition,
    const FVector& CameraUp,
    const FVector& FallbackUp
)
{
    // Front 방향
    FVector Front = (CameraPosition - GetRelativeLocation());
    Front.Normalize();

    // Right 계산
    FVector Right = CameraUp.Cross(Front);
    if (Right.Length() <= 0.0001f)
    {
        // CameraUp과 Front가 평행하면 FallbackUp 사용
        Right = FallbackUp.Cross(Front);
    }
    Right.Normalize();

    // Up 계산
    FVector Up = Front.Cross(Right);
    Up.Normalize();

    float XAngle = atan2(Up.Y, Up.Z);
    float YAngle = -asin(Up.X);
    float ZAngle = -atan2(-Right.X, Front.X);

    // 적용
    SetRelativeRotation(
        FVector(
            FVector::GetRadianToDegree(XAngle),
            FVector::GetRadianToDegree(YAngle),
            FVector::GetRadianToDegree(ZAngle)
        )
    );
}

const TPair<FName, ID3D11ShaderResourceView*>& UBillBoardComponent::GetSprite() const
{
    return Sprite;
}

void UBillBoardComponent::SetSprite(const TPair<FName, ID3D11ShaderResourceView*>& InSprite)
{
    Sprite = InSprite;
}

const ID3D11SamplerState* UBillBoardComponent::GetSampler() const
{ 
    return Sampler;
};

TObjectPtr<UClass> UBillBoardComponent::GetSpecificWidgetClass() const
{
    return USpriteSelectionWidget::StaticClass();
}