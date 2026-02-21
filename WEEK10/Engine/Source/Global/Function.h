#pragma once

using std::string;
using std::wstring;

// ============================================================================
// Math Utility Functions
// ============================================================================

/**
 * @brief 선형 보간 (Linear Interpolation)
 * @tparam T 보간할 타입 (float, FVector, FQuaternion 등)
 * @param A 시작 값
 * @param B 끝 값
 * @param Alpha 보간 비율 [0, 1]
 * @return 보간된 값
 */
template <typename T>
T Lerp(const T& A, const T& B, float Alpha)
{
	return A * (1.0f - Alpha) + B * Alpha;
}

/** @brief FMath::FInterpTo (언리얼 엔진의 함수와 유사하게 구현)
 * Current 값을 Target 값으로 DeltaTime 동안 InterpSpeed의 속도로 부드럽게 이동
 */
template <typename T>
T InterpTo(const T& Current, const T& Target, float DeltaTime, float InterpSpeed)
{
	// 속도가 0 이하면 즉시 목표값 반환
	if (InterpSpeed <= 0.0f) { return Target; }

	// 목표까지의 거리
	const T Dist = Target - Current;

	// 프레임 속도와 관계없이 일정한 감속(ease-out)을 보장하는 계산
	const float Alpha = 1.0f - std::exp(-InterpSpeed * DeltaTime);

	// Current에서 Target 방향으로 Alpha * Dist 만큼 이동
	return Lerp(Current, Target, Alpha);
}

/**
 * @brief 값을 min과 max 사이로 제한
 * @tparam T 제한할 타입
 * @param Value 제한할 값
 * @param Min 최소값
 * @param Max 최대값
 * @return 제한된 값
 */
template <typename T>
T Clamp(const T& Value, const T& Min, const T& Max)
{
	return (Value < Min) ? Min : (Value > Max) ? Max : Value;
}

// ============================================================================
// Memory & COM Utilities
// ============================================================================

template <typename T>
static void SafeDelete(T& InDynamicObject)
{
	delete InDynamicObject;
	InDynamicObject = nullptr;
}

template <typename T>
void SafeRelease(T*& ComObj)
{
	if (ComObj != nullptr)
	{
		ComObj->Release();
		ComObj = nullptr;
	}
}

/**
 * @brief wstring을 멀티바이트 FString으로 변환합니다.
 * @param InString 변환할 FString
 * @return 변환된 wstring
 */
static FString WideStringToString(const wstring& InString)
{
	int32 ByteNumber = WideCharToMultiByte(CP_UTF8, 0,
	                                     InString.c_str(), -1, nullptr, 0, nullptr, nullptr);

	FString OutString(ByteNumber, 0);

	WideCharToMultiByte(CP_UTF8, 0,
	                    InString.c_str(), -1, OutString.data(), ByteNumber, nullptr, nullptr);

	return OutString;
}

/**
 * @brief 멀티바이트 UTF-8 FString을 와이드 스트링(wstring)으로 변환합니다.
 * @param InString 변환할 FString (UTF-8)
 * @return 변환된 wstring
 */
static wstring StringToWideString(const FString& InString)
{
	// 필요한 와이드 문자의 개수를 계산
	int32 WideCharNumber = MultiByteToWideChar(CP_UTF8, 0,
										  InString.c_str(), -1, nullptr, 0);

	// 계산된 크기만큼 wstring 버퍼를 할당
	wstring OutString(WideCharNumber, 0);

	// 실제 변환을 수행
	MultiByteToWideChar(CP_UTF8, 0,
						InString.c_str(), -1, OutString.data(), WideCharNumber);

	// null 문자 제거
	OutString.resize(WideCharNumber - 1);

	return OutString;
}

/**
 * @brief CP949 (ANSI) 타입의 문자열을 UTF-8 타입으로 변환하는 함수
 * CP949 To UTF-8 직변환 함수가 제공되지 않아 UTF-16을 브릿지로 사용한다
 * @param InANSIString ANSI 문자열
 * @return UTF-8 문자열
 */
static string ConvertCP949ToUTF8(const char* InANSIString)
{
	if (InANSIString == nullptr || InANSIString[0] == '\0')
	{
		return "";
	}

	// CP949 -> UTF-8 변환
	int WideCharacterSize = MultiByteToWideChar(CP_ACP, 0, InANSIString, -1, nullptr, 0);
	if (WideCharacterSize == 0)
	{
		return "";
	}

	wstring WideString(WideCharacterSize, 0);
	MultiByteToWideChar(CP_ACP, 0, InANSIString, -1, WideString.data(), WideCharacterSize);

	// UTF-16 -> UTF-8 변환
	int UTF8Size = WideCharToMultiByte(CP_UTF8, 0, WideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (UTF8Size == 0)
	{
		return "";
	}

	string UTF8String(UTF8Size, 0);
	WideCharToMultiByte(CP_UTF8, 0, WideString.c_str(), -1, UTF8String.data(), UTF8Size, nullptr, nullptr);

	if (!UTF8String.empty() && UTF8String.back() == '\0')
	{
		UTF8String.pop_back();
	}

	return UTF8String;
}

/**
 * @brief 경로 문자열의 디렉토리 구분자를 모두 '/'로 통일합니다.
 * @details Windows/Unix 간 경로 구분자 차이로 인한 중복 리소스 로드를 방지합니다.
 *          예: "Data\\Textures\\image.png" -> "Data/Textures/image.png"
 * @param InPath 정규화할 경로 문자열
 * @return 정규화된 경로 문자열 ('/' 구분자 사용)
 */
inline std::string NormalizePath(const std::string& InPath)
{
	std::string Result = InPath;
	std::replace(Result.begin(), Result.end(), '\\', '/');
	return Result;
}

/**
 * @brief UTF-8 문자열을 UTF-16 와이드 문자열로 변환합니다.
 * @details 한글 경로 등 비ASCII 문자를 포함한 경로를 Windows API에서 사용할 수 있도록 변환합니다.
 * @param InUtf8Str UTF-8 인코딩된 입력 문자열
 * @return UTF-16 인코딩된 와이드 문자열 (변환 실패 시 빈 문자열)
 */
inline std::wstring UTF8ToWide(const std::string& InUtf8Str)
{
	if (InUtf8Str.empty())
	{
		return std::wstring();
	}

	int needed = ::MultiByteToWideChar(CP_UTF8, 0, InUtf8Str.c_str(), -1, nullptr, 0);
	if (needed <= 0)
	{
		// UTF-8 변환 실패 시 ANSI fallback
		needed = ::MultiByteToWideChar(CP_ACP, 0, InUtf8Str.c_str(), -1, nullptr, 0);
		if (needed <= 0)
		{
			return std::wstring();
		}

		std::wstring result(needed - 1, L'\0');
		::MultiByteToWideChar(CP_ACP, 0, InUtf8Str.c_str(), -1, result.data(), needed);
		return result;
	}

	std::wstring result(needed - 1, L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, InUtf8Str.c_str(), -1, result.data(), needed);
	return result;
}

/**
 * @brief UTF-16 와이드 문자열을 UTF-8 문자열로 변환합니다.
 * @details Windows API에서 반환된 와이드 문자열을 UTF-8로 변환하여 프로젝트 전역에서 사용합니다.
 * @param InWideStr UTF-16 인코딩된 입력 와이드 문자열
 * @return UTF-8 인코딩된 문자열 (변환 실패 시 빈 문자열)
 */
inline std::string WideToUTF8(const std::wstring& InWideStr)
{
	if (InWideStr.empty())
	{
		return std::string();
	}

	int size_needed = ::WideCharToMultiByte(
		CP_UTF8,
		0,
		InWideStr.c_str(),
		static_cast<int>(InWideStr.size()),
		nullptr,
		0,
		nullptr,
		nullptr
	);

	if (size_needed <= 0)
	{
		return std::string();
	}

	std::string result(size_needed, 0);
	::WideCharToMultiByte(
		CP_UTF8,
		0,
		InWideStr.c_str(),
		static_cast<int>(InWideStr.size()),
		&result[0],
		size_needed,
		nullptr,
		nullptr
	);

	return result;
}
