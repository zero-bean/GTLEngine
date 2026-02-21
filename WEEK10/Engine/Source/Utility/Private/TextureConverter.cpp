/**
 * @file TextureConverter.cpp
 * @brief DirectXTex를 사용한 텍스처 변환 유틸리티 구현
 */

#include "pch.h"
#include "Manager/Path/Public/PathManager.h"
#include "Utility/Public/TextureConverter.h"

#ifdef USE_DDS_CACHE
#include <DirectXTex.h>
#endif

bool FTextureConverter::ConvertToDDS(
	const std::string& SourcePath,
	const std::string& OutputPath,
	DXGI_FORMAT Format)
{
#ifdef USE_DDS_CACHE
	using namespace DirectX;

	// 1. 원본 이미지 로드
	std::wstring WSourcePath = UTF8ToWide(SourcePath);
	std::filesystem::path SourceFile(WSourcePath);

	if (!std::filesystem::exists(SourceFile))
	{
		UE_LOG_ERROR("TextureConverter: Source file not found: %s", SourcePath.c_str());
		return false;
	}

	TexMetadata metadata;
	ScratchImage image;

	// 파일 확장자에 따라 로드
	std::wstring ext = SourceFile.extension().wstring();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

	HRESULT hr = E_FAIL;

	if (ext == L".dds")
	{
		// 이미 DDS 포맷이면 변환 불필요
		return true;
	}
	else if (ext == L".tga")
	{
		hr = LoadFromTGAFile(WSourcePath.c_str(), &metadata, image);
	}
	else if (ext == L".hdr")
	{
		hr = LoadFromHDRFile(WSourcePath.c_str(), &metadata, image);
	}
	else
	{
		// 일반적인 포맷(PNG, JPG, BMP 등)은 WIC 사용
		hr = LoadFromWICFile(WSourcePath.c_str(), WIC_FLAGS_NONE, &metadata, image);
	}

	if (FAILED(hr))
	{
		UE_LOG_ERROR("TextureConverter: Failed to load source image: %s (HRESULT: 0x%08X)",
		       SourcePath.c_str(), hr);
		return false;
	}

	// 2. 블록 압축 사용 시 4픽셀 정렬로 리사이즈
	if (IsCompressed(Format))
	{
		size_t width = metadata.width;
		size_t height = metadata.height;

		// 4의 배수로 올림
		size_t alignedWidth = (width + 3) & ~3;
		size_t alignedHeight = (height + 3) & ~3;

		if (width != alignedWidth || height != alignedHeight)
		{
			ScratchImage resized;
			hr = Resize(image.GetImages(), image.GetImageCount(), metadata,
			            alignedWidth, alignedHeight, TEX_FILTER_DEFAULT, resized);

			if (SUCCEEDED(hr))
			{
				image = std::move(resized);
				metadata = image.GetMetadata();
			}
		}
	}

	// 3. 필요 시 밉맵 생성
	ScratchImage mipChain;
	if (bShouldGenerateMipmaps && metadata.mipLevels == 1)
	{
		hr = GenerateMipMaps(image.GetImages(), image.GetImageCount(), metadata,
		                     TEX_FILTER_DEFAULT, 0, mipChain);
		if (SUCCEEDED(hr))
		{
			image = std::move(mipChain);
			metadata = image.GetMetadata();
		}
	}

	// 4. 대상 포맷으로 압축
	ScratchImage compressed;
	if (IsCompressed(Format))
	{
		// 빠른 멀티스레드 압축
		hr = Compress(image.GetImages(), image.GetImageCount(), metadata,
		              Format, TEX_COMPRESS_PARALLEL | TEX_COMPRESS_DITHER,
		              TEX_THRESHOLD_DEFAULT, compressed);

		if (FAILED(hr))
		{
			UE_LOG_ERROR("TextureConverter: Compression failed: %s (HRESULT: 0x%08X)",
			       SourcePath.c_str(), hr);
			return false;
		}
	}
	else
	{
		// 비압축 포맷은 그냥 변환
		compressed = std::move(image);
	}

	// 5. 출력 경로 결정
	std::string FinalOutputPath = OutputPath.empty() ? GetDDSCachePath(SourcePath) : OutputPath;
	EnsureCacheDirectoryExists(FinalOutputPath);

	// 6. DDS로 저장
	std::wstring WOutputPath = UTF8ToWide(FinalOutputPath);
	hr = SaveToDDSFile(compressed.GetImages(), compressed.GetImageCount(),
	                   compressed.GetMetadata(), DDS_FLAGS_NONE, WOutputPath.c_str());

	if (FAILED(hr))
	{
		UE_LOG_ERROR("TextureConverter: Failed to save DDS file: %s (HRESULT: 0x%08X)",
		       FinalOutputPath.c_str(), hr);
		return false;
	}

	UE_LOG_SUCCESS("TextureConverter: Converted %s -> %s", SourcePath.c_str(), FinalOutputPath.c_str());
	return true;
#else
	UE_LOG_WARNING("TextureConverter: USE_DDS_CACHE is not defined");
	return false;
#endif
}

bool FTextureConverter::ShouldRegenerateDDS(
	const std::string& SourcePath,
	const std::string& DDSPath)
{
	namespace fs = std::filesystem;

	// DDS 캐시 존재 여부 확인
	fs::path SourceFile(UTF8ToWide(SourcePath));
	fs::path DDSFile(UTF8ToWide(DDSPath));

	if (!fs::exists(DDSFile))
	{
		return true; // 캐시가 없으면 재생성
	}

	if (!fs::exists(SourceFile))
	{
		return false; // 원본이 없으면 기존 캐시 사용
	}

	// 타임스탬프 비교
	auto SourceTime = fs::last_write_time(SourceFile);
	auto DDSTime = fs::last_write_time(DDSFile);

	// 원본이 캐시보다 최신이면 재생성
	return SourceTime > DDSTime;
}

std::string FTextureConverter::GetDDSCachePath(const std::string& SourcePath)
{
	// 1. 원본 경로 정규화 (백슬래시 -> 슬래시)
	std::string NormalizedPath = NormalizePath(SourcePath);

	// 2. 이미 DDS 캐시 경로인지 확인 (TextureCache 포함 + .dds 확장자)
	if (NormalizedPath.find("TextureCache") != std::string::npos &&
	    NormalizedPath.find(".dds") != std::string::npos)
	{
		return NormalizedPath;
	}

	// 3. PathManager에서 TextureCache 경로 가져오기
	const auto& TextureCachePath = UPathManager::GetInstance().GetTextureCachePath();
	std::string CacheBaseDir = WideToUTF8(TextureCachePath.wstring());

	// 4. Asset/ 기준 상대 경로 추출
	std::string RelativePath = NormalizedPath;

	// Asset/ 접두사 제거
	size_t AssetPos = RelativePath.find("Asset/");
	if (AssetPos != std::string::npos)
	{
		RelativePath = RelativePath.substr(AssetPos + 6); // "Asset/" 길이만큼 제거
	}
	else
	{
		// Asset/ 없으면 파일명만 사용
		size_t LastSlash = RelativePath.find_last_of("/\\");
		if (LastSlash != std::string::npos)
		{
			RelativePath = RelativePath.substr(LastSlash + 1);
		}
	}

	// 5. 캐시 경로 생성: Data/TextureCache/{RelativePath}.dds
	std::string CachePath = CacheBaseDir + "/" + RelativePath + ".dds";

	return NormalizePath(CachePath);
}

bool FTextureConverter::IsSupportedFormat(const std::string& Extension)
{
	std::string ext = Extension;
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	static const std::unordered_set<std::string> SupportedFormats = {
		".png", ".jpg", ".jpeg", ".bmp", ".tga", ".hdr", ".dds", ".tif", ".tiff"
	};

	return SupportedFormats.find(ext) != SupportedFormats.end();
}

void FTextureConverter::SetGenerateMipmaps(bool bGenerateMips)
{
	bShouldGenerateMipmaps = bGenerateMips;
}

DXGI_FORMAT FTextureConverter::GetRecommendedFormat(bool bHasAlpha, bool bSRGB)
{
	// BC3 (DXT5): 알파 포함 텍스처용 - 빠른 압축, 좋은 품질
	// BC1 (DXT1): 불투명 텍스처용 - 가장 빠른 압축, 작은 크기
	// BC7: 사용 가능하지만 매우 느림 (BC3보다 10~20배)

	// sRGB 포맷: Diffuse/Albedo 텍스처용 (감마 보정)
	// Linear 포맷: Normal/Data 텍스처용 (데이터 그대로)

	if (bSRGB)
	{
		return bHasAlpha ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM_SRGB;
	}
	else
	{
		return bHasAlpha ? DXGI_FORMAT_BC3_UNORM : DXGI_FORMAT_BC1_UNORM;
	}
}

void FTextureConverter::EnsureCacheDirectoryExists(const std::string& CachePath)
{
	namespace fs = std::filesystem;

	fs::path Path(UTF8ToWide(CachePath));
	fs::path Directory = Path.parent_path();

	if (!Directory.empty() && !fs::exists(Directory))
	{
		std::error_code ec;
		fs::create_directories(Directory, ec);

		if (ec)
		{
			UE_LOG_ERROR("TextureConverter: Failed to create cache directory: %s",
			       WideToUTF8(Directory.wstring()).c_str());
		}
	}
}
