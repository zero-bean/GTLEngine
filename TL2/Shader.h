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
    // Recompile this shader with macros for a specific shading model (hot-reload)
    void ReloadForShadingModel(ELightShadingModel Model, ID3D11Device* InDevice);
	   
	ID3D11InputLayout* GetInputLayout() const { return InputLayout; }
	ID3D11VertexShader* GetVertexShader() const { return VertexShaders; }
	ID3D11PixelShader* GetPixelShader() const { return PixelShaders; }

	void SetActiveMode(ELightShadingModel Model) { ActiveModel = Model; }
	ELightShadingModel GetActiveMode() const { return ActiveModel; }

	void SetActiveNormalMode(ENormalMapMode InMode);

    static const D3D_SHADER_MACRO* GetMacros(ELightShadingModel Model);

protected:
	virtual ~UShader();

	ID3D11Device* Device = nullptr;

	ELightShadingModel ActiveModel = ELightShadingModel::BlinnPhong;
	ENormalMapMode ActiveNormalMode = ENormalMapMode::NoNormalMap;
 
private:
	ID3DBlob* VSBlobs = nullptr; //  [(size_t)ELightShadingModel::Count] = { nullptr };
	ID3DBlob* PSBlob = nullptr;

	ID3D11InputLayout* InputLayout = nullptr;
	ID3D11VertexShader* VertexShaders = nullptr;
	ID3D11PixelShader* PixelShaders  = nullptr;


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
