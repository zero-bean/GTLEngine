#include "pch.h"
#include "BillboardComponent.h"
#include "ResourceManager.h"
#include "VertexData.h"
#include "CameraActor.h"
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

UBillboardComponent::UBillboardComponent()
{
    SetRelativeLocation({ 0, 0, 1 });

    auto& ResourceManager = UResourceManager::GetInstance();

    // 빌보드용 메시 가져오기 (단일 쿼드)
    BillboardQuad = ResourceManager.Get<UTextQuad>("Billboard");

    // 머티리얼 생성 또는 가져오기
   /* if (auto* M = ResourceManager.Get<UMaterial>("Billboard"))
    {
        Material = M;
        SetMaterial("Billboard.hlsl");
    }
    else
    {
        Material = NewObject<UMaterial>();
        ResourceManager.Add<UMaterial>("Billboard", Material);
    }*/
    SetMaterial("Billboard.hlsl");//메테리얼 자동 매칭
}

UBillboardComponent::~UBillboardComponent()
{
}

void UBillboardComponent::SetTexture(const FString& InTexturePath)
{
    TexturePath = InTexturePath;
}

void UBillboardComponent::SetUVCoords(float U, float V, float UL, float VL)
{
    UCoord = U;
    VCoord = V;
    ULength = UL;
    VLength = VL;
}

void UBillboardComponent::SetSpriteColor(const FLinearColor& InColor)
{
	// 다르면 업데이트
	if (InColor != SpriteColor)
	{
		SpriteColor = InColor;
		bIsChangedColor = true;
	}
}

UObject* UBillboardComponent::Duplicate()
{
  
    UBillboardComponent* DuplicatedComponent = Cast<UBillboardComponent>(NewObject(GetClass()));
    if (DuplicatedComponent)
    {
        CopyCommonProperties(DuplicatedComponent);
		DuplicatedComponent->TexturePath = TexturePath;
        DuplicatedComponent->SetEditable(this->bEdiableWhenInherited);
        DuplicatedComponent->SetBillboardSize(this->BillboardSize);
        DuplicatedComponent->DuplicateSubObjects();
        
    }
    return DuplicatedComponent;
}

void UBillboardComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}

void UBillboardComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // 0) 트랜스폼 직렬화/역직렬화는 상위(UPrimitiveComponent)에서 처리
    UPrimitiveComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        // TexturePath 로드
        if (!InOut.TexturePath.empty())
        {
            SetTexture(InOut.TexturePath);
        }
    }
    else
    {
        // TexturePath 저장
        InOut.TexturePath = TexturePath;
    }
}

void UBillboardComponent::CreateBillboardVertices()
{
    TArray<FBillboardVertexInfo_GPU> vertices;

    // 단일 쿼드의 4개 정점 생성 (카메라를 향하는 평면)
    // 중심이 (0,0,0)이고 크기가 BillboardWidth x BillboardHeight인 사각형
    float half = BillboardSize * 0.5f;

    FBillboardVertexInfo_GPU Info;

    // 정점 0: 좌상단 (-halfW, +halfH)
    Info.Position[0] = -half;
    Info.Position[1] = half;
    Info.Position[2] = 0.0f;
    Info.CharSize[0] = BillboardSize;
    Info.CharSize[1] = BillboardSize;
    Info.UVRect[0] = UCoord;   // u start
    Info.UVRect[1] = VCoord;   // v start
    Info.UVRect[2] = ULength;  // u length (UL)
    Info.UVRect[3] = VLength;  // v length (VL)
    vertices.push_back(Info);

    // 정점 1: 우상단 (+halfW, +halfH)
    Info.Position[0] = half;
    Info.Position[1] = half;
    Info.Position[2] = 0.0f;
    vertices.push_back(Info);

    // 정점 2: 좌하단 (-halfW, -halfH)
    Info.Position[0] = -half;
    Info.Position[1] = -half;
    Info.Position[2] = 0.0f;
    vertices.push_back(Info);

    // 정점 3: 우하단 (+halfW, -halfH)
    Info.Position[0] = half;
    Info.Position[1] = -half;
    Info.Position[2] = 0.0f;
    vertices.push_back(Info);

    // 동적 버텍스 버퍼 업데이트
    UResourceManager::GetInstance().UpdateDynamicVertexBuffer("Billboard", vertices);
}

void UBillboardComponent::Render(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj, const EEngineShowFlags ShowFlags)
{
    if (HasShowFlag(ShowFlags, EEngineShowFlags::SF_BillboardText) == false)
    {
        return;
    }
    // 텍스처 로드
    Material->Load(TexturePath, Renderer->GetRHIDevice()->GetDevice());

    // 카메라 정보 가져오기
    ACameraActor* CameraActor = GetOwner()->GetWorld()->GetCameraActor();
    FVector CamRight = CameraActor->GetActorRight();
    FVector CamUp = CameraActor->GetActorUp();

    // 빌보드 위치 설정
    FVector BillboardPos = GetWorldLocation();
    if (this->IsEditable())
    {
        FVector BillboardScale = GetRelativeScale();
        float Scale = BillboardScale.X > BillboardScale.Y ? BillboardScale.X : BillboardScale.Y;
        SetBillboardSize(Scale);
    }
    // 상수 버퍼 업데이트
    ////UUID만 필요하지만 기존 버퍼와 함수 재사용하기 위해서 모델버퍼 받아옴
    Renderer->UpdateSetCBuffer(ModelBufferType(FMatrix(), this->InternalIndex));
    //Renderer->UpdateSetCBuffer(ViewProjBufferType( FMatrix(), FMatrix()));
    Renderer->UpdateSetCBuffer(BillboardBufferType(BillboardPos,0, View.InverseAffine()));
	
	if (bIsChangedColor)
	{
		// TODO 색상용 상수버퍼 추가 및 업데이트
		// 셰이더 내부에서 조명용 빌보드인지 검사하는데 사용
		SpriteColor.A = SpriteColor.A + 1.0f;
		Renderer->UpdateSetCBuffer(ColorBufferType{FVector4
			{SpriteColor.R, SpriteColor.G, SpriteColor.B, SpriteColor.A}});				
		bIsChangedColor = false;
	}
	
	
    Renderer->OMSetDepthStencilState(EComparisonFunc::Disable);
    
    Renderer->OMSetBlendState(false);
    // 셰이더 준비
    Renderer->PrepareShader(Material->GetShader());

    // 빌보드 정점 생성 및 버퍼 업데이트
    CreateBillboardVertices();

    // 렌더링
    
    Renderer->RSSetState(EViewModeIndex::VMI_Unlit);
    Renderer->DrawIndexedPrimitiveComponent(this, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void UBillboardComponent::RenderDetails()
{
	// Billboard Component가 선택된 경우 Sprite UI
	ImGui::Separator();
	ImGui::Text("Billboard Component Settings");

	// Sprite 텍스처 경로 표시 및 변경
	FString CurrentTexture = GetTexturePath();
	ImGui::Text("Current Sprite: %s", CurrentTexture.c_str());

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
		FString currentTexturePath = GetTexturePath();
		for (int i = 0; i < SpriteOptions.size(); ++i)
		{
			if (SpriteOptions[i] == currentTexturePath)
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
				SetTexture(SpriteOptions[i]);
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

	ImGui::Spacing();

	// Screen Size Scaled 체크박스
	// bool bIsScreenSizeScaled = IsScreenSizeScaled();
	// if (ImGui::Checkbox("Is Screen Size Scaled", &bIsScreenSizeScaled))
	// {
	// 	SetScreenSizeScaled(bIsScreenSizeScaled);
	// }

	// Screen Size (Is Screen Size Scaled가 true일 때만 활성화)
	if (false) // (bIsScreenSizeScaled)
	{
		float screenSize = GetScreenSize();
		if (ImGui::DragFloat("Screen Size", &screenSize, 0.0001f, 0.0001f, 0.1f, "%.4f"))
		{
			SetScreenSize(screenSize);
		}
	}
	//else
	//{
	//	// Billboard Size (Is Screen Size Scaled가 false일 때)
	//	float billboardWidth = GetBillboardWidth();
	//	float billboardHeight = GetBillboardHeight();
	//
	//	if (ImGui::DragFloat("Width", &billboardWidth, 0.1f, 0.1f, 100.0f))
	//	{
	//		SetBillboardSize(billboardWidth, billboardHeight);
	//	}
	//
	//	if (ImGui::DragFloat("Height", &billboardHeight, 0.1f, 0.1f, 100.0f))
	//	{
	//		SetBillboardSize(billboardWidth, billboardHeight);
	//	}
	//}

	ImGui::Spacing();

	// UV 좌표 설정
	ImGui::Text("UV Coordinates");

	float u = GetU();
	float v = GetV();
	float ul = GetUL();
	float vl = GetVL();

	bool uvChanged = false;

	if (ImGui::DragFloat("U", &u, 0.01f))
		uvChanged = true;

	if (ImGui::DragFloat("V", &v, 0.01f))
		uvChanged = true;

	if (ImGui::DragFloat("UL", &ul, 0.01f))
		uvChanged = true;

	if (ImGui::DragFloat("VL", &vl, 0.01f))
		uvChanged = true;

	if (uvChanged)
	{
		SetUVCoords(u, v, ul, vl);
	}
}
