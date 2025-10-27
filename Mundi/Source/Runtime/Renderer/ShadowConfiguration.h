#pragma once

// Shadow 품질 프리셋 레벨
enum class EShadowQuality : uint8
{
	Low,        // 512x512, 4 lights, bias 0.01
	Medium,     // 1024x1024, 10 lights, bias 0.005 (현재 기본값)
	High,       // 2048x2048, 16 lights, bias 0.003
	Ultra,      // 4096x4096, 32 lights, bias 0.002
	Custom      // 사용자 정의
};

// Shadow 시스템 설정
// 모든 shadow 관련 설정을 중앙화하여 magic number 제거
struct FShadowConfiguration
{
	EShadowQuality Quality = EShadowQuality::Medium;

	uint32 ShadowMapResolution = 1024;

	// 광원 타입별 최대 쉐도우 캐스팅 라이트 수
	uint32 MaxDirectionalLights = 1;
	uint32 MaxSpotLights = 9;
	uint32 MaxPointLights = 5;

	// 전체 쉐도우 캐스팅 라이트 수 (통계/검증용)
	uint32 MaxShadowCastingLights = 10;

	float DefaultShadowBias = 0.005f;
	float DefaultShadowSlopeBias = 0.0f;
	bool bEnablePCF = true;
	uint32 PCFSamples = 4;  // 1, 4, 9, 16

	// 품질 프리셋으로부터 설정 로드
	static FShadowConfiguration FromQuality(EShadowQuality InQuality);

	// 현재 플랫폼의 기본 설정 가져오기
	static FShadowConfiguration GetPlatformDefault();

	// 설정 값 유효성 검증
	bool IsValid() const;
};
