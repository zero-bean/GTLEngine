#include "pch.h"
#include "ShadowConfiguration.h"

FShadowConfiguration FShadowConfiguration::FromQuality(EShadowQuality InQuality)
{
	FShadowConfiguration Config;
	Config.Quality = InQuality;

	switch (InQuality)
	{
	case EShadowQuality::Low:
		Config.ShadowMapResolution = 512;
		Config.MaxShadowCastingLights = 4;
		Config.DefaultShadowBias = 0.01f;
		Config.DefaultShadowSlopeBias = 0.0f;
		Config.bEnablePCF = true;
		Config.PCFSamples = 1;
		break;

	case EShadowQuality::Medium:
		Config.ShadowMapResolution = 1024;
		Config.MaxShadowCastingLights = 10;
		Config.DefaultShadowBias = 0.005f;
		Config.DefaultShadowSlopeBias = 0.0f;
		Config.bEnablePCF = true;
		Config.PCFSamples = 4;
		break;

	case EShadowQuality::High:
		Config.ShadowMapResolution = 2048;
		Config.MaxShadowCastingLights = 16;
		Config.DefaultShadowBias = 0.003f;
		Config.DefaultShadowSlopeBias = 0.0f;
		Config.bEnablePCF = true;
		Config.PCFSamples = 9;
		break;

	case EShadowQuality::Ultra:
		Config.ShadowMapResolution = 4096;
		Config.MaxShadowCastingLights = 32;
		Config.DefaultShadowBias = 0.002f;
		Config.DefaultShadowSlopeBias = 0.0f;
		Config.bEnablePCF = true;
		Config.PCFSamples = 16;
		break;

	case EShadowQuality::Custom:
	default:
		// 현재 값 유지
		break;
	}

	return Config;
}

FShadowConfiguration FShadowConfiguration::GetPlatformDefault()
{
	// 현재는 Medium 품질을 기본값으로 반환
	// 향후 플랫폼별로 차별화 가능 (PC: High, Console: Medium, Mobile: Low)
	return FromQuality(EShadowQuality::Medium);
}

bool FShadowConfiguration::IsValid() const
{
	// Shadow map 해상도는 2의 제곱수이고 적절한 범위 내에 있어야 함
	if (ShadowMapResolution < 256 || ShadowMapResolution > 8192)
		return false;

	// 2의 제곱수 확인
	if ((ShadowMapResolution & (ShadowMapResolution - 1)) != 0)
		return false;

	// 최대 라이트 수는 적절한 범위 내
	if (MaxShadowCastingLights < 1 || MaxShadowCastingLights > 64)
		return false;

	// Bias는 음수가 아니어야 함
	if (DefaultShadowBias < 0.0f || DefaultShadowSlopeBias < 0.0f)
		return false;

	// PCF 샘플 수는 유효한 값이어야 함
	if (PCFSamples != 1 && PCFSamples != 4 && PCFSamples != 9 && PCFSamples != 16)
		return false;

	return true;
}
