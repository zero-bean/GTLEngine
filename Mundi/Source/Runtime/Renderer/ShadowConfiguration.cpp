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
		Config.MaxDirectionalLights = 1;
		Config.MaxSpotLights = 1;
		Config.MaxPointLights = 2;
		Config.MaxShadowCastingLights = 4;
		break;

	case EShadowQuality::Medium:
		Config.ShadowMapResolution = 1024;
		Config.MaxDirectionalLights = 1;
		Config.MaxSpotLights = 4;
		Config.MaxPointLights = 5;
		Config.MaxShadowCastingLights = 10;
		break;

	case EShadowQuality::High:
		Config.ShadowMapResolution = 2048;
		Config.MaxDirectionalLights = 1;
		Config.MaxSpotLights = 7;
		Config.MaxPointLights = 8;
		Config.MaxShadowCastingLights = 16;
		break;

	case EShadowQuality::Ultra:
		Config.ShadowMapResolution = 4096;
		Config.MaxDirectionalLights = 1;
		Config.MaxSpotLights = 15;
		Config.MaxPointLights = 16;
		Config.MaxShadowCastingLights = 32;
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
	return FromQuality(EShadowQuality::Custom);
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
	if (MaxShadowCastingLights < 1)
		return false;

	return true;
}
