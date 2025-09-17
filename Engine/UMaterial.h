#pragma once
#include "UObject.h"

class UTexture;

class UMaterial : public UObject
{
    DECLARE_UCLASS(UMaterial, UObject)
public:
    UMaterial() = default;
    virtual ~UMaterial() override;

    void Apply(ID3D11DeviceContext* InContext);

    // Getter //
    ID3D11VertexShader* GetVS() const { return VertexShader; }
    ID3D11PixelShader* GetPS() const { return PixelShader; }
    ID3D11InputLayout* GetInputLayout() const { return InputLayout; }
    UTexture* GetTexture() const { return Texture; }
    // Setter //
    void SetTexture(UTexture* InTexture) { Texture = InTexture; }
    void SetShader(ID3D11VertexShader* InVertexShader, ID3D11PixelShader* InPixelShader, ID3D11InputLayout* InInputLayout)
    {
        VertexShader = InVertexShader;
        PixelShader = InPixelShader;
        InputLayout = InInputLayout;
    }

private:
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    UTexture* Texture = nullptr;
};

