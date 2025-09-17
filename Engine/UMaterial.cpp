#include "stdafx.h"
#include "UMaterial.h"
#include "UClass.h"
#include "UTexture.h"

IMPLEMENT_UCLASS(UMaterial, UObject);

UMaterial::~UMaterial()
{
    /* Material은 빌려쓰기만 할 뿐, 자원 책임은 외부로 위임 */
    VertexShader = nullptr;
    PixelShader = nullptr;
    InputLayout = nullptr;
    Texture = nullptr;
}

void UMaterial::Apply(ID3D11DeviceContext* InContext)
{
    if (InContext == nullptr) { return; }
    if (InputLayout) { InContext->IASetInputLayout(InputLayout); }
    if (VertexShader) { InContext->VSSetShader(VertexShader, nullptr, 0); }
    if (PixelShader) { InContext->PSSetShader(PixelShader, nullptr, 0); }
    if (Texture) { Texture->Bind(InContext, 0); }
}
