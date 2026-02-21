#pragma once

class FTextureManager
{
public:
    FTextureManager();
    ~FTextureManager();
    
    UTexture* LoadTexture(const FName& InFilePath, bool bSRGB = true);
    void LoadAllTexturesFromDirectory(const path& InDirectoryPath);
    const TMap<FName, UTexture*>& GetTextureCache() const;

private:
    ComPtr<ID3D11ShaderResourceView> CreateTextureFromFile(const path& InFilePath, bool bSRGB);
	
    TMap<FName, UTexture*> TextureCaches;
    ID3D11SamplerState* DefaultSampler; // 추후 샘플러 종류가 많아지면 매핑 형태로 캐싱 후 사용
};
