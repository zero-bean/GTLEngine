#include "pch.h"
#include "AnimAsset.h"
#include "JsonSerializer.h"
#include "PathUtils.h"

IMPLEMENT_CLASS(UAnimAsset)

namespace
{
    FString ExtractAssetNameFromPath(const FString& InPath)
    {
        if (InPath.empty()) { return FString(); }

        try
        {
            const FWideString WidePath = UTF8ToWide(InPath);
            std::filesystem::path FsPath(WidePath);
            return WideToUTF8(FsPath.stem().wstring());
        }
        catch (const std::exception&)
        {
            return FString();
        }
    }
}

void UAnimAsset::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // AssetName 로드
        FString LoadedAssetName;
        if (FJsonSerializer::ReadString(InOutHandle, "AssetName", LoadedAssetName, "", false))
        {
            AssetName = LoadedAssetName;
        }

        // FilePath 로드 (UResourceBase에서 상속)
        FString LoadedFilePath;
        if (FJsonSerializer::ReadString(InOutHandle, "FilePath", LoadedFilePath, "", false))
        {
            SetFilePath(NormalizePath(LoadedFilePath));

            // AssetName이 비어있을 때만 자동 생성
            if (AssetName.empty())
            {
                AssetName = ExtractAssetNameFromPath(FilePath);
            }
        }

        // SkeletonName 로드
        FString LoadedSkeletonName;
        if (FJsonSerializer::ReadString(InOutHandle, "SkeletonName", LoadedSkeletonName, "", false))
        {
            SkeletonName = LoadedSkeletonName;
        }
    }
    else
    {
        InOutHandle["FilePath"] = FilePath.c_str();
        InOutHandle["AssetName"] = AssetName.c_str();
        InOutHandle["SkeletonName"] = SkeletonName.c_str();
    }
}