#pragma once
#include "ResourceBase.h"

enum class ELightShadingModel : uint32_t
{
	Phong = 0,
	BlinnPhong = 1,
	Lambert= 2,
	BRDF = 3,
	Gouraud = 4,
	Unlit = 5,
	Count,
};

enum class ENormalMapMode : uint8_t
{
	NoNormalMap = 0,
	HasNormalMap = 1,
	ENormalMapModeCount = 2,
};

class UShader : public UResourceBase
{
public:
	DECLARE_CLASS(UShader, UResourceBase)

	void Load(const FString& ShaderPath, ID3D11Device* InDevice);
	   
	ID3D11InputLayout* GetInputLayout() const { return InputLayout; }
	ID3D11VertexShader* GetVertexShader() const { return VertexShaders[(size_t)ActiveModel]; }
	ID3D11PixelShader* GetPixelShader() const
	{
		return PixelShaders[(size_t)ActiveModel][(size_t)ActiveNormalMode];
	}

	void SetActiveMode(ELightShadingModel Model) { ActiveModel = Model; }
	ELightShadingModel GetActiveMode() const { return ActiveModel; }

	void SetActiveNormalMode(ENormalMapMode InMode) { ActiveNormalMode = InMode; }

	static const D3D_SHADER_MACRO* GetMacros(ELightShadingModel Model);

protected:
	virtual ~UShader();

	ELightShadingModel ActiveModel = ELightShadingModel::BlinnPhong;
	ENormalMapMode ActiveNormalMode = ENormalMapMode::NoNormalMap;

	// Default macro table; actual selection is decided at load time.
	D3D_SHADER_MACRO DefinesRender[8] =
	{
		{ "LIGHTING_MODEL_PHONG",  "0" },
		{ "LIGHTING_MODEL_BLINN_PHONG",  "1" },
		{ "LIGHTING_MODEL_BRDF",  "0" },
		{ "LIGHTING_MODEL_LAMBERT",  "0" },
		{ "LIGHTING_MODEL_GOURAUD",  "0" },
		{ "LIGHTING_MODEL_UNLIT",  "0" },
		{ "USE_TILED_CULLING",  "1" },  // Enable tile-based light culling
		{ nullptr, nullptr }
	};
	 
private:
	ID3DBlob* VSBlobs[(size_t)ELightShadingModel::Count] = { nullptr };
	ID3DBlob* PSBlob = nullptr;

	ID3D11InputLayout* InputLayout = nullptr;
	ID3D11VertexShader* VertexShaders[(size_t)ELightShadingModel::Count] = { nullptr };
	ID3D11PixelShader* PixelShaders
		[(size_t)ELightShadingModel::Count]
		[(size_t)ENormalMapMode::ENormalMapModeCount] = { nullptr };

	void CreateInputLayout(ID3D11Device* Device, const FString& InShaderPath);
	void ReleaseResources();
};

struct FVertexPositionColor
{
	static const D3D11_INPUT_ELEMENT_DESC* GetLayout()
	{
		static const D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		return layout;
	}
	static uint32 GetLayoutCount() { return 2; }
};

struct FVertexPositionColorTexturNormal
{
	static const D3D11_INPUT_ELEMENT_DESC* GetLayout()
	{
		static const D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		return layout;
	}

	static uint32 GetLayoutCount() { return 4; }
};

struct FVertexPositionBillBoard
{
	static const D3D11_INPUT_ELEMENT_DESC* GetLayout()

	{

		static const D3D11_INPUT_ELEMENT_DESC layout[] = {

		{ "WORLDPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},

		{ "UVRECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},

		};

		return layout;

	}
	 

	static uint32 GetLayoutCount() { return 3; }

};
