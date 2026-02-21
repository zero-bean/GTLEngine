#pragma once

class URenderer;

/// @brief 폰트 아틀라스를 사용한 텍스트 렌더링 클래스
/// 동적 정점 버퍼를 사용하여 텍스트를 효율적으로 렌더링합니다.
class UFontRenderer
{
public:
    /// @brief 폰트 정점 구조체 - 위치, UV, 문자 인덱스 포함
    struct FFontVertex
    {
        FVector Position;        // 3D 월드 좌표
        FVector2 TexCoord;      // 쿼드 내 UV 좌표 (0~1)
        uint32 CharIndex;       // ASCII 문자 코드
    };

    /// @brief 폰트 데이터 상수 버퍼 구조체 (HLSL FontDataBuffer와 일치)
    struct FFontConstantBuffer
    {
        FVector2 AtlasSize = FVector2(512.0f, 512.0f);      // 아틀라스 전체 크기
        FVector2 GlyphSize = FVector2(16.0f, 16.0f);         // 한 글자 크기
        FVector2 GridSize = FVector2(32.0f, 32.0f);          // 아틀라스 그리드 (32x32 글자)
        FVector2 Padding = FVector2(0.0f, 0.0f);             // 여백
    };

    // 생성자와 소멸자
    UFontRenderer();
    ~UFontRenderer();

    /// @brief 폰트 렌더러 초기화 - 리소스 로드 및 동적 정점 버퍼 생성
    bool Initialize();

    /// @brief 리소스 해제
    void Release();

    /// @brief 임의의 텍스트를 화면에 렌더링
    /// @param Text 렌더링할 텍스트 문자열
    /// @param WorldMatrix 월드 변환 행렬
    /// @param ViewProjectionConstants 뷰-프로젝션 상수 데이터
    /// @param CenterY 중앙 Y 좌표 (모델 좌표계)
    /// @param StartZ 시작 Z 좌표 (모델 좌표계)
    /// @param CharWidth 문자 너비
    /// @param CharHeight 문자 높이
    /// @param bEnableDepthTest 깊이 테스트 활성화 여부
    void RenderText(const char* Text, const FMatrix& WorldMatrix, const FViewProjConstants& ViewProjectionConstants,
                    float CenterY = 0.0f, float StartZ = -2.5f, float CharWidth = 1.0f, float CharHeight = 2.0f, bool bEnableDepthTest = false);

private:
    /// @brief 텍스트 렌더링을 위한 동적 정점 버퍼 생성
    bool CreateDynamicVertexBuffer();

    /// @brief 폰트 텍스처 로드
    bool LoadFontTexture();

    // 렌더링 리소스 (셰이더, 레이아웃 등은 Renderer에서 관리)
    ID3D11VertexShader* FontVertexShader = nullptr;
    ID3D11PixelShader* FontPixelShader = nullptr;
    ID3D11InputLayout* FontInputLayout = nullptr;
    ID3D11SamplerState* FontSampler = nullptr;

    /// @brief 폰트 아틀라스 텍스처 리소스
    ID3D11ShaderResourceView* FontAtlasTexture = nullptr;

    /// @brief 텍스트 렌더링을 위한 동적 정점 버퍼
    ID3D11Buffer* DynamicVertexBuffer = nullptr;

    /// @brief 폰트 데이터용 상수 버퍼
    ID3D11Buffer* FontDataConstantBuffer = nullptr;

    /// @brief 폰트 데이터
    FFontConstantBuffer ConstantBufferData;

    /// @brief 동적 정점 버퍼가 수용할 수 있는 최대 정점 개수
    static constexpr uint32 MAX_FONT_VERTICES = 4096;
};