#pragma once
#include "pch.h"
#include "ICollide.h"
#include "TextureSet.h"

class Renderer;

class Object :public ICollide
{
public:
	Object() {};
    virtual ~Object() {
        // 가상 소멸자에서 리소스 정리
        if (ShaderResourceView) {
            ShaderResourceView->Release();
            ShaderResourceView = nullptr;
        }
        if (Texture) {
            Texture->Release();
            Texture = nullptr;
        }
        if (ConstantBuffer) {
            ConstantBuffer->Release();
            ConstantBuffer = nullptr;
        }
        if (VertexBuffer) {
            VertexBuffer->Release();
            VertexBuffer = nullptr;
        }
    }

public:
	virtual void Initialize(Renderer& renderer) = 0;
	virtual void Update(Renderer& renderer) = 0;
	virtual void Render(Renderer& renderer) = 0;
	virtual void Release() = 0;
    void SetTextureSet(const TextureSet& textureSet)
    {
        // 기존 리소스 해제
        if (ShaderResourceView) {
            ShaderResourceView->Release();
            ShaderResourceView = nullptr;
        }
        if (Texture) {
            Texture->Release();
            Texture = nullptr;
        }

        // 새 리소스 설정 및 참조 카운트 증가
        if (textureSet.IsValid()) {
            Texture = textureSet.texture;
            ShaderResourceView = textureSet.srv;

            if (Texture) Texture->AddRef();
            if (ShaderResourceView) ShaderResourceView->AddRef();
        }
    }

    // BindTexture 메서드 구현
    void BindTexture(ID3D11DeviceContext* deviceContext, UINT slot = 0) const
    {
        if (deviceContext && ShaderResourceView) {
            // 픽셀 셰이더에 셰이더 리소스 뷰를 바인딩
            deviceContext->PSSetShaderResources(slot, 1, &ShaderResourceView);
        }
    }

    // 여러 텍스처 슬롯에 바인딩하는 메서드
    void BindTextureToSlot(ID3D11DeviceContext* deviceContext, UINT slot) const
    {
        BindTexture(deviceContext, slot);
    }

    // 텍스처가 유효한지 확인하는 헬퍼 메서드
    bool HasValidTexture() const
    {
        return (Texture != nullptr && ShaderResourceView != nullptr);
    }

protected:
	ID3D11Buffer* VertexBuffer{};
	ID3D11Buffer* ConstantBuffer{};
	ID3D11Texture2D* Texture{};
	ID3D11ShaderResourceView* ShaderResourceView{};

	FVector3 WorldPosition{};

	float Scale{};
	int  NumVertices{};
};

