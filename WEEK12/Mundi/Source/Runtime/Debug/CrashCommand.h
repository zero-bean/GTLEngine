#pragma once
#include <UEContainer.h>

/**
 * @brief 메모리 오염을 통해 크래시 핸들러를 테스트하기 위한 유틸리티 클래스입니다.
 *
 * 1. 콘솔에서 "Crash" 명령어 입력
 * 2. 매 프레임마다 TickCorruption()이 호출되어 점진적으로 메모리 오염
 * 3. 엔진이 오염된 객체를 사용할 때 자연스럽게 크래시 발생
 * 4. 크래시 발생 시 미니덤프 파일 생성 및 콜스택 분석 가능
 */
class CCrashCommand
{
public:
	CCrashCommand() = default;
	~CCrashCommand() = default;

	/**
	 * 메모리 오염 모드를 활성화합니다.
	 */
	static void EnableCorruptionMode();

	/**
	 * 매 프레임마다 소량의 메모리를 오염시킵니다.
	 * 호출 위치: EditorEngine::Tick()
	 */
	static void TickCorruption();

private:
	static bool bCorruptionEnabled;    // 오염 모드 활성화 여부
	static uint32_t CorruptionSeed;    // 난수 생성 시드
	static int32 CorruptionCounter;    // 오염 프레임 카운터
};
