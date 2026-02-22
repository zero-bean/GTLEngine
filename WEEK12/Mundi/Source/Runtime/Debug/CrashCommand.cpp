#include "pch.h"
#include <Windows.h>
#include "CrashCommand.h"
#include "CrashHandler.h"
#include "ObjectFactory.h"
#include <cstring>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Static 멤버 변수 정의
bool CCrashCommand::bCorruptionEnabled = false;
uint32_t CCrashCommand::CorruptionSeed = 0;
int32 CCrashCommand::CorruptionCounter = 0;


// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Private: 메모리 오염

/**
 * @brief GUObjectArray에서 무작위 객체들의 메모리를 오염시킵니다.
 * @param Total - 전체 객체 배열 크기
 * @param Seed - 난수 생성 시드
 * @param CorruptionAmount - 오염시킬 객체 개수
 *
 * 1. 유효한 객체들 중 랜덤하게 선택
 * 2. 선택된 객체의 메모리 일부를 쓰레기 값(0xDEADBEEF 등)으로 덮어씀
 * 3. GUObjectArray의 포인터 일부를 잘못된 주소로 변경
 *
 * @notion 이 함수는 크래시를 직접 유발하지 않고, 메모리만 오염시킵니다.
 *		   실제 크래시는 엔진이 오염된 객체를 사용할 때 자연스럽게 발생합니다.
 */
static void __declspec(noinline) CorruptMemory(int32 Total, uint32_t Seed, int32 CorruptionAmount)
{
	// 간단한 LCG(Linear Congruential Generator) 난수 생성기
	uint32_t RngState = Seed;
	auto SimpleRand = [&RngState]() -> uint32_t {
		RngState = RngState * 1103515245 + 12345;
		return (RngState / 65536) % 32768;
		};

	// Step 1: 유효한 객체 인덱스 수집
	int32* ValidIndices = (int32*)_alloca(Total * sizeof(int32));
	int32 ValidCount = 0;

	for (int32 i = 0; i < Total; ++i)
	{
		if (GUObjectArray[i] != nullptr)
		{
			ValidIndices[ValidCount++] = i;
		}
	}

	if (ValidCount == 0) { return; }

	// Step 2: 지정된 개수만큼 객체 메모리 오염
	for (int32 Count = 0; Count < CorruptionAmount && ValidCount > 0; ++Count)
	{
		// 랜덤 객체 선택
		int32 PickIndex = ValidIndices[SimpleRand() % ValidCount];
		UObject* Victim = GUObjectArray[PickIndex];
		if (!Victim) { continue; }

		// 객체 메모리를 난수로 덮어쓰기
		const size_t ObjectSize = sizeof(UObject);
		unsigned char* ObjBytes = reinterpret_cast<unsigned char*>(Victim);

		for (size_t i = 0; i < ObjectSize; i += sizeof(uint32_t))
		{
			uint32_t Garbage = SimpleRand();
			size_t BytesToCopy = (sizeof(uint32_t) < (ObjectSize - i)) ? sizeof(uint32_t) : (ObjectSize - i);
			std::memcpy(ObjBytes + i, &Garbage, BytesToCopy);
		}
	}

	// Step 3: GUObjectArray 포인터 일부를 잘못된 주소로 변경
	int32 MaxCorrupt = (Total / 50 < 5) ? (Total / 50) : 5;
	for (int i = 0; i < MaxCorrupt; ++i)
	{
		int32 CorruptIdx = SimpleRand() % Total;
		GUObjectArray[CorruptIdx] = reinterpret_cast<UObject*>(0xDEADBEEF);
	}
}


// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Public

/**
 * @brief 메모리 오염 모드를 활성화합니다.
 * @note  콘솔에서 "Crash" 명령어 입력
 * - TickCorruption()이 매 프레임마다 메모리를 오염시키기 시작
 * - 크래시 발생 시 미니덤프 생성 준비
 */
void CCrashCommand::EnableCorruptionMode()
{
	bCorruptionEnabled = true;
	CorruptionSeed = static_cast<uint32_t>(GetTickCount64());
	CorruptionCounter = 0;

	// 크래시 발생 시 덤프 파일 생성 준비
	FCrashHandler::SetNextDumpProfileDataSegsOnly();
}

/**
 * @brief 매 프레임마다 호출되어 점진적으로 메모리를 오염시킵니다.
 * @note 사용 위치: EditorEngine::Tick()
 * - 오염 모드가 활성화되어 있으면 매 프레임 1~3개의 객체를 오염
 * - 엔진이 정상적으로 동작하다가 오염된 객체를 사용할 때 크래시 발생
 */
void CCrashCommand::TickCorruption()
{
	if (!bCorruptionEnabled) { return; }

	int32 Total = GUObjectArray.Num();
	if (Total <= 0) { return; }

	// 매 프레임마다 1~3개의 객체만 추가로 오염됩니다.
	int32 CorruptionAmount = 1 + (CorruptionCounter % 3);
	CorruptMemory(Total, CorruptionSeed + CorruptionCounter, CorruptionAmount);

	CorruptionCounter++;
}