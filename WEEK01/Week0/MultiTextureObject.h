#pragma once
#include "TextureSet.h"
#include "Object.h"
class MultiTextureObject : public Object
{
private:
	std::vector<TextureSet> textures;
	int currentTextureIndex = 0;

public:
    // 여러 텍스처셋을 한번에 설정
    void SetTextureSets(const std::vector<TextureSet>& sets) {
        // 기존 텍스처들 해제
        ClearTextureSets();

        // 새 텍스처들 설정
        textures = sets;
        for (auto& textureSet : textureSets) {
            if (textureSet.texture) textureSet.texture->AddRef();
            if (textureSet.srv) textureSet.srv->AddRef();
        }

        currentTextureIndex = 0; // 첫 번째 텍스처로 초기화
    }

    // 다음 텍스처로 순환
    void NextTexture() {
        if (!textures.empty()) {
            currentTextureIndex = (currentTextureIndex + 1) % textures.size();
        }
    }

    // 특정 인덱스의 텍스처로 변경
    void SetTextureIndex(int index) {
        if (index >= 0 && index < textures.size()) {
            currentTextureIndex = index;
        }
    }

    // 현재 텍스처 인덱스 반환
    int GetCurrentTextureIndex() const {
        return currentTextureIndex;
    }

    // 텍스처 개수 반환
    int GetTextureCount() const {
        return static_cast<int>(textures.size());
    }

    // Object의 BindTexture를 오버라이드하여 현재 인덱스 바인드
    void BindTexture(ID3D11DeviceContext* deviceContext, UINT slot = 0) const override {
        if (!textures.empty() && currentTextureIndex < textures.size()) {
            const auto& currentSet = textures[currentTextureIndex];
            if (deviceContext && currentSet.srv) {
                deviceContext->PSSetShaderResources(slot, 1, &currentSet.srv);
            }
        }
    }

    // 유효한 텍스처가 있는지 확인
    bool HasValidTexture() const override {
        return !textures.empty() &&
            currentTextureIndex < textures.size() &&
            textures[currentTextureIndex].IsValid();
    }

private:
    // 모든 텍스처들 해제
    void ClearTextureSets() {
        for (auto& textureSet : textures) {
            if (textureSet.srv) textureSet.srv->Release();
            if (textureSet.texture) textureSet.texture->Release();
        }
        textures.clear();
        currentTextureIndex = 0;
    }

public:
    virtual ~MultiTextureObject() {
        ClearTextureSets();
    }
};