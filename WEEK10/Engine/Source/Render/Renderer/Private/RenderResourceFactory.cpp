#include "pch.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/Renderer/Public/Renderer.h"

void FRenderResourceFactory::CreateStructuredShaderResourceView(ID3D11Buffer* Buffer, ID3D11ShaderResourceView** OutSRV)
{
	D3D11_BUFFER_DESC BufDesc = {};
	Buffer->GetDesc(&BufDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC Desc = {};
	Desc.Format = DXGI_FORMAT_UNKNOWN; // StructuredBuffer는 Format 없음
	Desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	Desc.Buffer.NumElements = BufDesc.ByteWidth / BufDesc.StructureByteStride;
	HRESULT hr = URenderer::GetInstance().GetDevice()->CreateShaderResourceView(Buffer, &Desc, OutSRV);

}
void FRenderResourceFactory::CreateUnorderedAccessView(ID3D11Buffer* Buffer, ID3D11UnorderedAccessView** OutUAV)
{
	D3D11_BUFFER_DESC BufDesc = {};
	Buffer->GetDesc(&BufDesc);

	D3D11_UNORDERED_ACCESS_VIEW_DESC Desc = {};
	Desc.Format = DXGI_FORMAT_UNKNOWN;
	Desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	Desc.Buffer.NumElements = BufDesc.ByteWidth / BufDesc.StructureByteStride;
	URenderer::GetInstance().GetDevice()->CreateUnorderedAccessView(Buffer, &Desc, OutUAV);
}

void FRenderResourceFactory::CreateVertexShaderAndInputLayout(const wstring& InFilePath,
                                                              const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescs, ID3D11VertexShader** OutVertexShader, ID3D11InputLayout** OutInputLayout)
{
	const char* EntryPoint = "mainVS";
	const char* ShaderModel = "vs_5_0";

	// CSO 파일 경로 생성
	wstring CSOPath = GetCompiledShaderPath(InFilePath, EntryPoint, "vs");

	ID3DBlob* VertexShaderBlob = nullptr;

	// 캐시 확인 및 로드/컴파일
	if (IsShaderUpToDate(InFilePath, CSOPath))
	{
		// 캐시된 CSO 로드
		VertexShaderBlob = LoadPrecompiledShader(CSOPath);
	}

	// 캐시 미스 또는 로드 실패 시 컴파일
	if (!VertexShaderBlob)
	{
		if (!CompileAndSaveShader(InFilePath, CSOPath, EntryPoint, ShaderModel, nullptr, 0, &VertexShaderBlob))
		{
			return;
		}
	}

	// VertexShader 생성
	URenderer::GetInstance().GetDevice()->CreateVertexShader(VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), nullptr, OutVertexShader);

	// InputLayout 생성
	if (InInputLayoutDescs.Num() > 0)
	{
		URenderer::GetInstance().GetDevice()->CreateInputLayout(InInputLayoutDescs.GetData(), static_cast<uint32>(InInputLayoutDescs.Num()), VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), OutInputLayout);
	}

	SafeRelease(VertexShaderBlob);
}

void FRenderResourceFactory::CreateVertexShaderAndInputLayout(const wstring& InFilePath,
                                                              const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescs, ID3D11VertexShader** OutVertexShader, ID3D11InputLayout** OutInputLayout,
                                                              const char* InEntryPoint, const D3D_SHADER_MACRO* InMacros)
{
	const char* ShaderModel = "vs_5_0";
	UINT Flag = 0;
#ifdef _DEBUG
	Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// CSO 파일 경로 생성
	wstring CSOPath = GetCompiledShaderPath(InFilePath, InEntryPoint, "vs");

	ID3DBlob* VertexShaderBlob = nullptr;

	// 캐시 확인 및 로드/컴파일
	if (IsShaderUpToDate(InFilePath, CSOPath))
	{
		// 캐시된 CSO 로드
		VertexShaderBlob = LoadPrecompiledShader(CSOPath);
	}

	// 캐시 미스 또는 로드 실패 시 컴파일
	if (!VertexShaderBlob)
	{
		if (!CompileAndSaveShader(InFilePath, CSOPath, InEntryPoint, ShaderModel, InMacros, Flag, &VertexShaderBlob))
		{
			return;
		}
	}

	// VertexShader 생성
	URenderer::GetInstance().GetDevice()->CreateVertexShader(VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), nullptr, OutVertexShader);

	// InputLayout 생성
	if (InInputLayoutDescs.Num() > 0)
	{
		URenderer::GetInstance().GetDevice()->CreateInputLayout(InInputLayoutDescs.GetData(), static_cast<uint32>(InInputLayoutDescs.Num()), VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), OutInputLayout);
	}

	SafeRelease(VertexShaderBlob);
}

ID3D11Buffer* FRenderResourceFactory::CreateVertexBuffer(FNormalVertex* InVertices, uint32 InByteWidth, bool bCpuAccess)
{
	D3D11_BUFFER_DESC Desc = { InByteWidth, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	if (bCpuAccess)
	{
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	D3D11_SUBRESOURCE_DATA InitData = { InVertices, 0, 0 };
	ID3D11Buffer* VertexBuffer = nullptr;
	URenderer::GetInstance().GetDevice()->CreateBuffer(&Desc, &InitData, &VertexBuffer);
	return VertexBuffer;
}

ID3D11Buffer* FRenderResourceFactory::CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess)
{
	D3D11_BUFFER_DESC Desc = { InByteWidth, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	if (bCpuAccess)
	{
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	D3D11_SUBRESOURCE_DATA InitData = { InVertices, 0, 0 };
	ID3D11Buffer* VertexBuffer = nullptr;
	URenderer::GetInstance().GetDevice()->CreateBuffer(&Desc, &InitData, &VertexBuffer);
	return VertexBuffer;
}

ID3D11Buffer* FRenderResourceFactory::CreateIndexBuffer(const void* InIndices, uint32 InByteWidth)
{
	D3D11_BUFFER_DESC Desc = { InByteWidth, D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA InitData = { InIndices, 0, 0 };
	ID3D11Buffer* IndexBuffer = nullptr;
	URenderer::GetInstance().GetDevice()->CreateBuffer(&Desc, &InitData, &IndexBuffer);
	return IndexBuffer;
}

void FRenderResourceFactory::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader)
{
	const char* EntryPoint = "mainPS";
	const char* ShaderModel = "ps_5_0";
	UINT Flag = 0;
#ifdef _DEBUG
	Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// CSO 파일 경로 생성
	wstring CSOPath = GetCompiledShaderPath(InFilePath, EntryPoint, "ps");

	ID3DBlob* PixelShaderBlob = nullptr;

	// 캐시 확인 및 로드/컴파일
	if (IsShaderUpToDate(InFilePath, CSOPath))
	{
		// 캐시된 CSO 로드
		PixelShaderBlob = LoadPrecompiledShader(CSOPath);
	}

	// 캐시 미스 또는 로드 실패 시 컴파일
	if (!PixelShaderBlob)
	{
		if (!CompileAndSaveShader(InFilePath, CSOPath, EntryPoint, ShaderModel, nullptr, Flag, &PixelShaderBlob))
		{
			return;
		}
	}

	// PixelShader 생성
	URenderer::GetInstance().GetDevice()->CreatePixelShader(PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(), nullptr, OutPixelShader);

	SafeRelease(PixelShaderBlob);
}

void FRenderResourceFactory::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader,
												const char* InEntryPoint, const D3D_SHADER_MACRO* InMacros)
{
	ID3DBlob* PixelShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;
	UINT Flag = 0;
#ifdef _DEBUG
	Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT Result = D3DCompileFromFile(InFilePath.data(), InMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, InEntryPoint, "ps_5_0", Flag, 0, &PixelShaderBlob, &ErrorBlob);
	if (FAILED(Result))
	{
		if (ErrorBlob) { OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer())); SafeRelease(ErrorBlob); }
		SafeRelease(PixelShaderBlob);
		return;
	}

	URenderer::GetInstance().GetDevice()->CreatePixelShader(PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(), nullptr, OutPixelShader);
	SafeRelease(PixelShaderBlob);
	SafeRelease(ErrorBlob);
}

void FRenderResourceFactory::CreateComputeShader(const wstring& InFilePath, ID3D11ComputeShader** OutComputeShader,
	const char* InEntryPoint, const D3D_SHADER_MACRO* InMacros)
{
	const char* ShaderModel = "cs_5_0";
	UINT Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#ifdef _DEBUG
	Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// CSO 파일 경로 생성
	wstring CSOPath = GetCompiledShaderPath(InFilePath, InEntryPoint, "cs");

	ID3DBlob* ShaderBlob = nullptr;

	// 캐시 확인 및 로드/컴파일
	if (IsShaderUpToDate(InFilePath, CSOPath))
	{
		// 캐시된 CSO 로드
		ShaderBlob = LoadPrecompiledShader(CSOPath);
	}

	// 캐시 미스 또는 로드 실패 시 컴파일
	if (!ShaderBlob)
	{
		if (!CompileAndSaveShader(InFilePath, CSOPath, InEntryPoint, ShaderModel, InMacros, Flag, &ShaderBlob))
		{
			return;
		}
	}

	// ComputeShader 생성
	URenderer::GetInstance().GetDevice()->CreateComputeShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, OutComputeShader);

	SafeRelease(ShaderBlob);
}

ID3D11SamplerState* FRenderResourceFactory::CreateSamplerState(D3D11_FILTER InFilter, D3D11_TEXTURE_ADDRESS_MODE InAddressMode)
{
	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = InFilter;
	SamplerDesc.AddressU = InAddressMode;
	SamplerDesc.AddressV = InAddressMode;
	SamplerDesc.AddressW = InAddressMode;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* SamplerState = nullptr;
	if (FAILED(URenderer::GetInstance().GetDevice()->CreateSamplerState(&SamplerDesc, &SamplerState)))
	{
		UE_LOG_ERROR("Renderer: 샘플러 스테이트 생성 실패");
		return nullptr;
	}
	return SamplerState;
}

ID3D11SamplerState* FRenderResourceFactory::CreateFXAASamplerState()
{
	// FXAA샘플러 상태 생성
	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* FXAASamplerState = nullptr;
	if (FAILED(URenderer::GetInstance().GetDevice()->CreateSamplerState(&SamplerDesc, &FXAASamplerState)))
	{
		UE_LOG_ERROR("Renderer: 샘플러 스테이트 생성 실패");
		return nullptr;
	}
	return FXAASamplerState;
}

ID3D11RasterizerState* FRenderResourceFactory::GetRasterizerState(const FRenderState& InRenderState)
{
	const FRasterKey Key{ ToD3D11(InRenderState.FillMode), ToD3D11(InRenderState.CullMode) };
	if (auto* FoundValuePtr = RasterCache.Find(Key))
	{
		return *FoundValuePtr;
	}

	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = Key.FillMode;
	RasterizerDesc.CullMode = Key.CullMode;
	RasterizerDesc.FrontCounterClockwise = TRUE;
	RasterizerDesc.DepthClipEnable = TRUE;

	ID3D11RasterizerState* RasterizerState = nullptr;
	if (FAILED(URenderer::GetInstance().GetDevice()->CreateRasterizerState(&RasterizerDesc, &RasterizerState)))
	{
		return nullptr;
	}

	RasterCache.Emplace(Key, RasterizerState);
	return RasterizerState;
}

void FRenderResourceFactory::ReleaseRasterizerState()
{
	for (auto& Cache : RasterCache)
	{
		SafeRelease(Cache.second);
	}
	RasterCache.Empty();
}

D3D11_CULL_MODE FRenderResourceFactory::ToD3D11(ECullMode InCull)
{
	switch (InCull)
	{
	case ECullMode::Back: return D3D11_CULL_BACK;
	case ECullMode::Front: return D3D11_CULL_FRONT;
	case ECullMode::None: return D3D11_CULL_NONE;
	default: return D3D11_CULL_BACK;
	}
}

D3D11_FILL_MODE FRenderResourceFactory::ToD3D11(EFillMode InFill)
{
	switch (InFill)
	{
	case EFillMode::Solid: return D3D11_FILL_SOLID;
	case EFillMode::WireFrame: return D3D11_FILL_WIREFRAME;
	default: return D3D11_FILL_SOLID;
	}
}

TMap<FRenderResourceFactory::FRasterKey, ID3D11RasterizerState*, FRenderResourceFactory::FRasterKeyHasher> FRenderResourceFactory::RasterCache;

// ============================================================================
// Shader Caching System Implementation
// ============================================================================

/**
 * @brief HLSL 파일 경로와 엔트리 포인트로부터 CSO 파일 경로 생성
 * @param InHLSLPath 원본 HLSL 파일 경로 (예: "Asset/Shader/MyShader.hlsl")
 * @param InEntryPoint 엔트리 포인트 이름 (예: "mainVS")
 * @param InShaderType Shader 타입 (예: "vs", "ps", "cs")
 * @return CSO 파일 경로 (예: "Asset/Shader/Compiled/MyShader_mainVS.cso")
 */
wstring FRenderResourceFactory::GetCompiledShaderPath(const wstring& InHLSLPath, const char* InEntryPoint, const char* InShaderType)
{
	path HLSLPath(InHLSLPath);
	path CompiledDir = HLSLPath.parent_path() / L"Compiled";

	// 파일명에서 확장자 제거
	wstring FileNameWithoutExt = HLSLPath.stem().wstring();

	// CSO 파일명 생성: FileName_EntryPoint.cso
	wstring CSOFileName = FileNameWithoutExt + L"_" + wstring(InEntryPoint, InEntryPoint + strlen(InEntryPoint)) + L".cso";

	return (CompiledDir / CSOFileName).wstring();
}

/**
 * @brief Compiled 디렉토리가 존재하지 않으면 생성
 * @param InCompiledPath CSO 파일 경로
 */
void FRenderResourceFactory::EnsureCompiledDirectoryExists(const wstring& InCompiledPath)
{
	path CSOPath(InCompiledPath);
	path CompiledDir = CSOPath.parent_path();

	if (!exists(CompiledDir))
	{
		try
		{
			create_directories(CompiledDir);
			UE_LOG_INFO("ShaderCache: Created directory '%ls'", CompiledDir.c_str());
		}
		catch (const filesystem::filesystem_error& Error)
		{
			UE_LOG_ERROR("ShaderCache: Failed to create directory '%ls': %s", CompiledDir.c_str(), Error.what());
		}
	}
}

/**
 * @brief CSO 파일이 HLSL 파일보다 최신인지 확인 (수정 시간 비교)
 * @param InHLSLPath 원본 HLSL 파일 경로
 * @param InCSOPath 컴파일된 CSO 파일 경로
 * @return true: CSO가 최신, false: HLSL이 최신 (재컴파일 필요)
 */
bool FRenderResourceFactory::IsShaderUpToDate(const wstring& InHLSLPath, const wstring& InCSOPath)
{
	try
	{
		if (!exists(InCSOPath))
		{
			return false; // CSO 파일이 없으면 컴파일 필요
		}

		auto HLSLTime = filesystem::last_write_time(InHLSLPath);
		auto CSOTime = filesystem::last_write_time(InCSOPath);

		// CSO가 HLSL보다 나중에 수정되었으면 최신 상태
		return CSOTime >= HLSLTime;
	}
	catch (const filesystem::filesystem_error& e)
	{
		UE_LOG_ERROR("ShaderCache: Failed to check file timestamps: %s", e.what());
		return false; // 에러 발생 시 안전하게 재컴파일
	}
}

/**
 * @brief 이미 컴파일된 CSO 파일을 로드
 * @param InCSOPath CSO 파일 경로
 * @return ID3DBlob 포인터 (실패 시 nullptr)
 */
ID3DBlob* FRenderResourceFactory::LoadPrecompiledShader(const wstring& InCSOPath)
{
	ID3DBlob* ShaderBlob = nullptr;

	HRESULT Result = D3DReadFileToBlob(InCSOPath.c_str(), &ShaderBlob);
	if (FAILED(Result))
	{
		UE_LOG_ERROR("ShaderCache: Failed to load precompiled shader '%ls'", InCSOPath.c_str());
		return nullptr;
	}

	UE_LOG_SUCCESS("ShaderCache: Loaded cached shader '%ls'", InCSOPath.c_str());
	return ShaderBlob;
}

/**
 * @brief HLSL 파일을 컴파일하고 CSO 파일로 저장
 * @param InHLSLPath 원본 HLSL 파일 경로
 * @param InCSOPath 저장할 CSO 파일 경로
 * @param InEntryPoint 엔트리 포인트 이름
 * @param InShaderModel Shader 모델 (예: "vs_5_0", "ps_5_0")
 * @param InMacros Shader 매크로 정의 (nullptr 가능)
 * @param InFlags 컴파일 플래그
 * @param OutBlob 컴파일된 Blob 출력 (호출자가 Release 필요)
 * @return 성공 여부
 */
bool FRenderResourceFactory::CompileAndSaveShader(const wstring& InHLSLPath, const wstring& InCSOPath,
	const char* InEntryPoint, const char* InShaderModel, const D3D_SHADER_MACRO* InMacros, UINT InFlags, ID3DBlob** OutBlob)
{
	ID3DBlob* ErrorBlob = nullptr;

	HRESULT Result = D3DCompileFromFile(InHLSLPath.c_str(), InMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		InEntryPoint, InShaderModel, InFlags, 0, OutBlob, &ErrorBlob);

	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			UE_LOG_ERROR("ShaderCache: Compilation failed for '%ls': %s", InHLSLPath.c_str(), static_cast<char*>(ErrorBlob->GetBufferPointer()));
			SafeRelease(ErrorBlob);
		}
		return false;
	}

	SafeRelease(ErrorBlob);

	// Compiled 디렉토리 생성
	EnsureCompiledDirectoryExists(InCSOPath);

	// CSO 파일로 저장
	Result = D3DWriteBlobToFile(*OutBlob, InCSOPath.c_str(), TRUE);
	if (FAILED(Result))
	{
		UE_LOG_ERROR("ShaderCache: Failed to save compiled shader to '%ls'", InCSOPath.c_str());
		return false;
	}

	UE_LOG_SUCCESS("ShaderCache: Compiled and saved shader '%ls' -> '%ls'", InHLSLPath.c_str(), InCSOPath.c_str());
	return true;
}
