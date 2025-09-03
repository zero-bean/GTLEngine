#pragma once
#include "pch.h"

struct TextureSet {
public:
    ID3D11Resource* resource;           // 원본 리소스 보관
    ID3D11Texture2D* texture;          // 캐스팅된 텍스처
    ID3D11ShaderResourceView* srv;

    TextureSet() : resource(nullptr), texture(nullptr), srv(nullptr) {}

    ~TextureSet() {
        Release();
    }

    void Release() {
        if (srv) { srv->Release(); srv = nullptr; }
        if (texture) { texture->Release(); texture = nullptr; }
        if (resource) { resource->Release(); resource = nullptr; }
    }

    bool IsValid() const {
        return texture != nullptr && srv != nullptr;
    }
};

