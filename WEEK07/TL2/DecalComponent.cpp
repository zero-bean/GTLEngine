// DecalComponent.cpp
#include "pch.h"
#include "DecalComponent.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "StaticMeshComponent.h"
#include "SceneLoader.h"
#include "ImGui/imgui.h"
#include <filesystem>

// Helper function to extract base filename without extension
static inline FString GetBaseNameNoExt(const FString& Path)
{
	const size_t sep = Path.find_last_of("/\\");
	const size_t start = (sep == FString::npos) ? 0 : sep + 1;

	const FString ext = ".obj";
	size_t end = Path.size();
	if (end >= ext.size() && Path.compare(end - ext.size(), ext.size(), ext) == 0)
	{
		end -= ext.size();
	}
	if (start <= end) return Path.substr(start, end - start);
	return Path;
}

// Helper function to get icon files from Editor/Icon folder
static TArray<FString> GetIconFiles()
{
	TArray<FString> iconFiles;
	try
	{
		std::filesystem::path iconPath = "Editor/Icon";
		if (std::filesystem::exists(iconPath) && std::filesystem::is_directory(iconPath))
		{
			for (const auto& entry : std::filesystem::directory_iterator(iconPath))
			{
				if (entry.is_regular_file())
				{
					auto filename = entry.path().filename().string();
					// .dds 확장자만 포함
					if (filename.ends_with(".dds"))
					{
						// 상대경로 포맷으로 저장 (Editor/Icon/filename.dds)
						FString relativePath = "Editor/Icon/" + filename;
						iconFiles.push_back(relativePath);
					}
				}
			}
		}
	}
	catch (const std::exception&)
	{
		// 파일 시스템 오류 발생 시 기본값으로 폴백
		iconFiles.push_back("Editor/Icon/Pawn_64x.dds");
		iconFiles.push_back("Editor/Icon/PointLight_64x.dds");
		iconFiles.push_back("Editor/Icon/SpotLight_64x.dds");
	}
	return iconFiles;
}

IMPLEMENT_CLASS(UDecalComponent)

UDecalComponent::UDecalComponent()
{
    bCanEverTick = true;

    // 기본 큐브 메쉬 로드 (데칼 볼륨으로 사용)
    DecalBoxMesh = UResourceManager::GetInstance().Load<UStaticMesh>("Data/Cube.obj");
    // 기본 데칼 텍스처 로드
    LocalAABB = FAABB(FVector(-0.5f, -0.5f, -0.5f), FVector(0.5f, 0.5f, 0.5f));
    TexturePath = "Editor/Decal/BattleGround.dds";

    SetMaterial("DecalShader.hlsl");
  
    if (Material)
    {
        Material->Load(TexturePath, UResourceManager::GetInstance().GetDevice());
    }
    UpdateDecalProjectionMatrix();
}

UDecalComponent::~UDecalComponent()
{
}

void UDecalComponent::UpdateDecalProjectionMatrix()
{
    //스케일을 그대로 프로젝션에 적용하면 스케일과 데칼 사이즈를 바꾸면서 타일링이 같이 되버림.(역행렬 곱하면서 데칼 스케일의 역을 곱했는데 그 이후에
    //프로젝션 하면서 원상태로 복구시킴. )
    float Right = DecalSize.Y/2.0f;
    float Left = -DecalSize.Y/2.0f;
    float Top = DecalSize.Z/2.0f;
    float Bottom = -DecalSize.Z/2.0f;
    float Near = 0.0f;
    float Far = DecalSize.X * 2;

    FMatrix OrthoMatrix = FMatrix::OffCenterOrthoLH(Left, Right, Top, Bottom, Near, Far);

    //// UV 타일링을 위한 스케일 행렬 생성
    //FMatrix UVScale = FMatrix::Identity();
    //UVScale.M[0][0] =  UVTiling.X;
    //UVScale.M[1][1] =  UVTiling.Y;
    //UVScale.M[2][2] = 1.0f;
    //// 중심 보정 (짝수일 때 반 픽셀 밀림 방지)
    //// 중심 보정 - 타일링된 텍스처를 중앙에 배치
    //UVScale.M[3][0] = -(UVTiling.X - 1.0f) / 2.0f;
    //UVScale.M[3][1] = -(UVTiling.Y - 1.0f) / 2.0f;
    DecalProjectionMatrix = OrthoMatrix ;
}

void UDecalComponent::TickComponent(float DeltaSeconds)
{
    LifetimeTimer += DeltaSeconds;
    
    const float FadeInEndTime = FadeInStartDelay + FadeInDuration;
    const float FadeOutStartTime = FadeInEndTime + FadeStartDelay;

    float FadeInAlpha = 1.0f;
    if (FadeInDuration > 0.0f)
    {
        float FadeInProgress = (LifetimeTimer - FadeInStartDelay) / FadeInDuration;
        FadeInAlpha = std::max(0.0f, std::min(1.0f, FadeInProgress));
    }

    float FadeOutAlpha = 1.0f;
    if (FadeDuration > 0.0f)
    {
        float FadeOutProgress = (LifetimeTimer - FadeOutStartTime) / FadeDuration;
        FadeOutAlpha = 1.0f - std::max(0.0f, std::min(1.0f, FadeOutProgress));
    }

    CurrentAlpha = std::min(FadeInAlpha, FadeOutAlpha);
    //UE_LOG("Tick - Delta: %.3f, Lifetime: %.3f, InAlpha: %.3f, OutAlpha: %.3f, FinalAlpha: %.3f", DeltaSeconds, LifetimeTimer, FadeInAlpha, FadeOutAlpha, CurrentAlpha);
}

void UDecalComponent::Render(URenderer* Renderer, UPrimitiveComponent* Component, const FMatrix& View, const FMatrix& Proj,FViewport* Viewport)
{
    if (!DecalBoxMesh || !Material)
        return;
    if (Material)
    {
        Material->Load(TexturePath, UResourceManager::GetInstance().GetDevice());
    }
    //일단 프로젝션 데칼 테스트용으로 StaticMesh만 처리
    UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component);
    if (!StaticMeshComponent)
    {
        return;
    }
    UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
    if (!StaticMesh)
    {
        return;
    }
    
    float LifeTimeAlpha = CurrentAlpha;
    float ScreenFadeAlpha = 1.0f;
    if (FadeScreenSize > 0.0f)
    {
        // Calculate an approximate bounding sphere radius from the decal's size
        const FVector DecalCenter = GetWorldLocation();
        const float DecalRadius =  GetWorldOBB().Extents.Size() / 2.0f;

        // Get the camera's position
        const FMatrix InvView = View.Inverse();
        const FVector CameraPos = FVector(InvView.Rows[3].X, InvView.Rows[3].Y, InvView.Rows[3].Z);
        const float Distance = FVector::Distance(CameraPos, DecalCenter);

        if (Distance > 0.001f)
        {
            // Approximate the decal's apparent size
            float ApparentSize = DecalRadius / Distance;
            ScreenFadeAlpha = std::min(1.0f, ApparentSize / FadeScreenSize);
        }
    }
    const float FinalAlpha = std::min(LifeTimeAlpha, ScreenFadeAlpha);

    // 월드/역월드
    // DecalSize를 스케일로 적용, 데칼 world inverse를 구하기 위함
    FMatrix DecalWorldMatrix = GetWorldMatrix();           // 데칼의 원본 월드 행렬
    FMatrix DecalWorldMatrixInverse = DecalWorldMatrix.InverseAffine();

    //데칼 world inverse를 구했으므로 Component의 worldMatrix를 구해줌
    FMatrix MeshWorldMatrix = Component->GetWorldMatrix();

    // 상수 버퍼 업데이트
    //MeshWorldMatrix = 데칼을 투영할 Component의 WorldMatrix
    Renderer->UpdateSetCBuffer(ModelBufferType({ MeshWorldMatrix }));
    //DecalWorldMatrix = 데칼의 원본 월드 행렬
    //DecalWorldMatrixInverse = 데칼의 역 월드 행렬
    //DecalProjectionMatrix = 데칼 투영 행렬
    Renderer->UpdateSetCBuffer(DecalMatrixCB({ DecalWorldMatrix, DecalWorldMatrixInverse, DecalProjectionMatrix , DecalSize}));
    Renderer->UpdateSetCBuffer(DecalAlphaBufferType({ FinalAlpha, UVTiling}));

    // 셰이더/블렌드 셋업
    Renderer->PrepareShader(Material->GetShader());
    Renderer->OMSetBlendState(true);                  // (SrcAlpha, InvSrcAlpha)인지 내부 확인

    // =========================
    // RTV와 DSV 모두 유지
    // =========================
    ID3D11RenderTargetView* currentRTV = nullptr;
    ID3D11DepthStencilView* currentDSV = nullptr;
    ID3D11DeviceContext* ctx = Renderer->GetRHIDevice()->GetDeviceContext();

    ctx->OMGetRenderTargets(1, &currentRTV, &currentDSV);        // 현재 RTV와 DSV 핸들 얻고
    ctx->OMSetRenderTargets(1, &currentRTV, currentDSV);         // RTV와 DSV 모두 유지
    if (currentRTV) currentRTV->Release();                       // 로컬 ref release
    if (currentDSV) currentDSV->Release();                       // 로컬 ref release

    // 데칼은 깊이 "읽기"만 (LessEqual + DepthWrite Off)
    Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqualReadOnly);

    // DepthBias 적용된 데칼 Rasterizer State 사용
    Renderer->GetRHIDevice()->RSSetDecalState();

    // 입력 어셈블러
    UINT stride = sizeof(FVertexDynamic);
    UINT offset = 0;
    ID3D11Buffer* vb = StaticMesh->GetVertexBuffer();
    ID3D11Buffer* ib = StaticMesh->GetIndexBuffer();;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 텍스처 & 샘플러
    if (Material->GetTexture())
    {
        ID3D11ShaderResourceView* texSRV = Material->GetTexture()->GetShaderResourceView();
        ctx->PSSetShaderResources(0, 1, &texSRV);
    }
    Renderer->GetRHIDevice()->PSSetDefaultSampler(0);

    // 서브매시 처리도 해줘야함.
    ctx->DrawIndexed(StaticMesh->GetIndexCount() , 0, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSRV);

    // 원래 DSV/RTV 복원 (렌더러가 백버퍼/DSV 재바인딩)
    Renderer->GetRHIDevice()->OMSetRenderTargets(ERenderTargetType::Frame | ERenderTargetType::ID);

    // 상태 복원
    Renderer->OMSetBlendState(false);
    Renderer->RSSetDefaultState();
    Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqual); // 기본 상태로 복원
}


void UDecalComponent::SetDecalTexture( FString NewTexturePath)
{

    if (!Material)
        return;

    // 멤버 변수 업데이트
    TexturePath = NewTexturePath;

    // TextRenderComponent와 동일한 방식으로 텍스처 로드
    Material->Load(TexturePath, UResourceManager::GetInstance().GetDevice());
}

void UDecalComponent::SetDecalSize(FVector InDecalSize)
{
    DecalSize = InDecalSize;
    UpdateDecalProjectionMatrix();
}
const FOBB UDecalComponent::GetWorldOBB() const
{
    FAABB SizeApplied = LocalAABB;
    SizeApplied.Max = SizeApplied.Max * DecalSize;
    SizeApplied.Min = SizeApplied.Min * DecalSize;
    return FOBB(SizeApplied, GetWorldTransform());
}

const TArray<FVector> UDecalComponent::GetBoundingBoxLines() const
{
    return GetWorldOBB().GetWireLine();
}

UObject* UDecalComponent::Duplicate()
{
    UDecalComponent* DuplicatedComponent = Cast<UDecalComponent>(NewObject(GetClass()));
    if (DuplicatedComponent)
    {
        CopyCommonProperties(DuplicatedComponent);
        DuplicatedComponent->UVTiling = UVTiling;
        DuplicatedComponent->BlendMode = BlendMode;
        DuplicatedComponent->TexturePath = TexturePath;
        DuplicatedComponent->LocalAABB = LocalAABB;

        // 페이드 관련 속성들
        DuplicatedComponent->FadeInStartDelay = FadeInStartDelay;
        DuplicatedComponent->FadeInDuration = FadeInDuration;
        DuplicatedComponent->FadeStartDelay = FadeStartDelay;
        DuplicatedComponent->FadeDuration = FadeDuration;
        DuplicatedComponent->FadeScreenSize = FadeScreenSize;

        // 메쉬 (공유 리소스)
        DuplicatedComponent->DecalBoxMesh = DecalBoxMesh;

        // 머티리얼 재설정
        if (!TexturePath.empty())
        {
            DuplicatedComponent->SetDecalTexture(TexturePath);
        }

        // 자식 컴포넌트 복제
        DuplicatedComponent->DuplicateSubObjects();
    }
    return DuplicatedComponent;
}

void UDecalComponent::DuplicateSubObjects()
{
    UPrimitiveComponent::DuplicateSubObjects();
    // DecalBoxMesh는 공유 리소스이므로 복사하지 않음
}

void UDecalComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // 0) 트랜스폼 직렬화/역직렬화는 상위(UPrimitiveComponent)에서 처리
    UPrimitiveComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        // TexturePath 로드
        if (!InOut.TexturePath.empty())
        {
            SetDecalTexture(InOut.TexturePath);
        }
    }
    else
    {
        // TexturePath 저장
        InOut.TexturePath = TexturePath;
    }
}

void UDecalComponent::RenderDetails()
{
	ImGui::Text("Decal Component Settings");

	// Decal Texture Setting
	ImGui::Separator();

	// Editor/Icon 폴더에서 동적으로 스프라이트 옵션 로드
	static TArray<FString> SpriteOptions;
	static bool bSpriteOptionsLoaded = false;
	static int currentSpriteIndex = 0; // 현재 선택된 스프라이트 인덱스

	if (!bSpriteOptionsLoaded)
	{
		// Editor/Icon 폴더에서 .dds 파일들을 찾아서 추가
		SpriteOptions = GetIconFiles();
		bSpriteOptionsLoaded = true;

		// 현재 텍스처와 일치하는 인덱스 찾기
		FString CurrentTexture = GetTexturePath();
		for (int i = 0; i < SpriteOptions.size(); ++i)
		{
			if (SpriteOptions[i] == CurrentTexture)
			{
				currentSpriteIndex = i;
				break;
			}
		}
	}

	// 스프라이트 선택 드롭다운 메뉴
	ImGui::Text("Sprite Texture:");
	FString currentDisplayName = (currentSpriteIndex >= 0 && currentSpriteIndex < SpriteOptions.size())
		? GetBaseNameNoExt(SpriteOptions[currentSpriteIndex])
		: "Select Sprite";

	if (ImGui::BeginCombo("##SpriteCombo", currentDisplayName.c_str()))
	{
		for (int i = 0; i < SpriteOptions.size(); ++i)
		{
			FString displayName = GetBaseNameNoExt(SpriteOptions[i]);
			bool isSelected = (currentSpriteIndex == i);

			if (ImGui::Selectable(displayName.c_str(), isSelected))
			{
				currentSpriteIndex = i;
				SetDecalTexture(SpriteOptions[i]);
			}

			// 현재 선택된 항목에 포커스 설정
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// 새로고침 버튼 (같은 줄에)
	ImGui::SameLine();
	if (ImGui::Button("Refresh"))
	{
		bSpriteOptionsLoaded = false; // 다음에 다시 로드하도록
		currentSpriteIndex = 0; // 인덱스 리셋
	}

	ImGui::Separator();

	int32 SortOrder = GetSortOrder();
	if (ImGui::DragInt("Sort Order", &SortOrder));
	{
		SetSortOrder(SortOrder);
	}

	ImGui::Separator();

	// Decal Fade In/Out
	float FadeScreenSize = GetFadeScreenSize();
	if (ImGui::DragFloat("Fade Screen Size", &FadeScreenSize, 0.01f, 0.0f, 100.0f))
	{
		SetFadeScreenSize(FadeScreenSize);
	}

	float FadeStartDelay = GetFadeStartDelay();
	if (ImGui::DragFloat("Fade Start Delay", &FadeStartDelay, 0.1f))
	{
		SetFadeStartDelay(FadeStartDelay);
	}

	float FadeDuration = GetFadeDuration();
	if (ImGui::DragFloat("Fade Duration", &FadeDuration, 0.1f))
	{
		SetFadeDuration(FadeDuration);
	}

	float FadeInStartDelay = GetFadeInStartDelay();
	if (ImGui::DragFloat("Fade In StartDelay", &FadeInStartDelay, 0.1f))
	{
		SetFadeInStartDelay(FadeInStartDelay);
	}

	float FadeInDuration = GetFadeInDuration();
	if (ImGui::DragFloat("Fade In Duration", &FadeInDuration, 0.1f))
	{
		SetFadeInDuration(FadeInDuration);
	}

	ImGui::Separator();

	// Decal UV Tiling
	FVector2D Tiling = GetUVTiling();
	if (ImGui::DragFloat2("UV Tiling", &Tiling.X, 0.1f, 1.0f, 10.0f))
	{
		SetUVTiling(Tiling);
	}
	FVector DecalSize = GetDecalSize();
	if (ImGui::DragFloat3("Decal Size", &DecalSize.X, 0.1f, 1.0f, 10.0f))
	{
		SetDecalSize(DecalSize);
	}
}
