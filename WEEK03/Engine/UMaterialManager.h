#pragma once
#include "UEngineSubsystem.h"

class UMaterial;
class URenderer;

class UMaterialManager : public UEngineSubsystem
{
	DECLARE_UCLASS(UMaterialManager, UEngineSubsystem)
public:
    UMaterialManager();
    ~UMaterialManager();

    bool Initialize(URenderer* renderer);

    UMaterial* RetrieveMaterial(const FString& name);
    bool RegisterMaterial(const FString& name, UMaterial* material)
    {
        if (!material) { return false; }
        Materials[name] = material;
        return true;
    }

private:
    TMap<FString, UMaterial*> Materials;
};

