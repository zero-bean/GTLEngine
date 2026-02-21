#pragma once

/**
 * @brief Build Configuration Macros
 * 빌드 타입에 따라 에디터/UI 기능을 제어하는 매크로 정의
 *
 * 사용법:
 * - Visual Studio Configuration에서 STAND_ALONE=1 매크로를 정의하면 StandAlone 빌드
 * - 정의하지 않으면 기본 Editor 빌드
 */

// STAND_ALONE 매크로가 정의되지 않은 경우 기본값 0 (Editor 빌드)
#ifndef STAND_ALONE
	#define STAND_ALONE 0
#endif

// STAND_ALONE 빌드일 때 에디터 기능 비활성화
#if STAND_ALONE
	#define WITH_EDITOR 0
	#define WITH_UI 0
#else
	#define WITH_EDITOR 1
	#define WITH_UI 1
#endif
