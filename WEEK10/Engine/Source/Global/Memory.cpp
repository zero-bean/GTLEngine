#include "pch.h"
#include "Global/Memory.h"

#include <new>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

using std::atomic;
using std::align_val_t;
using std::memory_order_relaxed;

atomic<uint32> TotalAllocationBytes = 0;
atomic<uint32> TotalAllocationCount = 0;

#ifdef _DEBUG
#include <unordered_map>
#include <mutex>

// 재진입 방지 플래그 (무한 재귀 방지)
thread_local bool GIsInsideMemoryAllocation = false;

// 정적 초기화 순서 문제 해결: 함수 내 정적 변수로 싱글톤 패턴
static std::unordered_map<void*, FStackTrace*>& GetStackTraceMap()
{
	static std::unordered_map<void*, FStackTrace*> Instance;
	return Instance;
}

static std::mutex& GetStackTraceMutex()
{
	static std::mutex Instance;
	return Instance;
}

static bool GSymbolsInitialized = false;

// 콜스택 캡처
static FStackTrace* CaptureStackTrace()
{
	FStackTrace* Trace = static_cast<FStackTrace*>(malloc(sizeof(FStackTrace)));
	if (!Trace)
	{
		return nullptr;
	}

	// CaptureStackBackTrace: 현재 콜스택 캡처 (2 프레임 스킵 = CaptureStackTrace + operator new)
	Trace->FrameCount = static_cast<uint16>(::CaptureStackBackTrace(2, MAX_STACK_FRAMES, Trace->Frames, nullptr));
	return Trace;
}

// 콜스택 출력
static void PrintStackTrace(const FStackTrace* Trace, void* Address, size_t Size)
{
	if (!Trace || !GSymbolsInitialized)
	{
		return;
	}

	char buffer[2048];
	sprintf_s(buffer, "Memory Leak: %zu bytes at 0x%p\n", Size, Address);
	OutputDebugStringA(buffer);

	HANDLE Process = GetCurrentProcess();
	char Buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	SYMBOL_INFO* Symbol = reinterpret_cast<SYMBOL_INFO*>(Buffer);
	Symbol->MaxNameLen = MAX_SYM_NAME;
	Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	IMAGEHLP_LINE64 Line = {};
	Line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	for (int i = 0; i < Trace->FrameCount; ++i)
	{
		DWORD64 Address64 = reinterpret_cast<DWORD64>(Trace->Frames[i]);

		if (SymFromAddr(Process, Address64, nullptr, Symbol))
		{
			DWORD Displacement = 0;
			if (SymGetLineFromAddr64(Process, Address64, &Displacement, &Line))
			{
				sprintf_s(buffer, "  %s(%d): %s\n", Line.FileName, Line.LineNumber, Symbol->Name);
				OutputDebugStringA(buffer);
			}
			else
			{
				sprintf_s(buffer, "  %s (no line info)\n", Symbol->Name);
				OutputDebugStringA(buffer);
			}
		}
		else
		{
			sprintf_s(buffer, "  0x%p (no symbol)\n", Trace->Frames[i]);
			OutputDebugStringA(buffer);
		}
	}
	OutputDebugStringA("\n");
}

void InitializeMemoryTracking()
{
	HANDLE Process = GetCurrentProcess();
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

	if (SymInitialize(Process, nullptr, TRUE))
	{
		GSymbolsInitialized = true;
	}
}

void ShutdownMemoryTracking()
{
	// 남은 메모리 릭 출력
	std::lock_guard<std::mutex> Lock(GetStackTraceMutex());
	auto& StackTraceMap = GetStackTraceMap();

	if (!StackTraceMap.empty())
	{
		printf("======================================\n");
		printf("Memory Leaks Detected: %zu allocations\n", StackTraceMap.size());
		printf("======================================\n\n");

		for (const auto& Pair : StackTraceMap)
		{
			void* Address = Pair.first;
			FStackTrace* Trace = Pair.second;

			AllocHeader* Header = static_cast<AllocHeader*>(Address) - 1;
			PrintStackTrace(Trace, Address, Header->size);

			free(Trace);
		}

		StackTraceMap.clear();
	}

	if (GSymbolsInitialized)
	{
		SymCleanup(GetCurrentProcess());
		GSymbolsInitialized = false;
	}
}
#endif

/**
 * @brief 전역 메모리 관리를 위한 메모리 할당자 오버로딩 함수
 * @param InSize 할당 size
 * @return 할당한 공간에서 할당 공간 정보를 저장한 헤더를 제외한 나머지 공간의 첫 메모리 주소
 */
void* operator new(size_t InSize)
{
	TotalAllocationCount.fetch_add(1, memory_order_relaxed);
	TotalAllocationBytes.fetch_add(static_cast<uint32>(InSize), memory_order_relaxed);

	AllocHeader* MemoryHeader = static_cast<AllocHeader*>(malloc(sizeof(AllocHeader) + InSize));
	MemoryHeader->size = InSize;
	MemoryHeader->bIsAligned = false;

#ifdef _DEBUG
	MemoryHeader->StackTrace = nullptr;

	// 재진입 방지: 내부 할당(unordered_map, mutex 등)은 추적하지 않음
	if (!GIsInsideMemoryAllocation)
	{
		GIsInsideMemoryAllocation = true;

		MemoryHeader->StackTrace = CaptureStackTrace();
		void* UserPtr = MemoryHeader + 1;
		{
			std::lock_guard<std::mutex> Lock(GetStackTraceMutex());
			GetStackTraceMap()[UserPtr] = MemoryHeader->StackTrace;
		}

		GIsInsideMemoryAllocation = false;
		return UserPtr;
	}
#endif

	return MemoryHeader + 1;
}

/**
 * @brief 오버로드된 함수로 생성 처리한 메모리 공간을 할당 해제하는 함수
 * @param InMemory 처음에 객체 할당용으로 제공된 메모리 주소
 */
void operator delete(void* InMemory) noexcept
{
	if (!InMemory)
	{
		return;
	}

	AllocHeader* MemoryHeader = static_cast<AllocHeader*>(InMemory) - 1;
	size_t MemoryAllocSize = MemoryHeader->size;

#ifdef _DEBUG
	// 콜스택 정보 제거
	if (!GIsInsideMemoryAllocation && MemoryHeader->StackTrace)
	{
		GIsInsideMemoryAllocation = true;

		{
			std::lock_guard<std::mutex> Lock(GetStackTraceMutex());
			auto& StackTraceMap = GetStackTraceMap();
			auto It = StackTraceMap.find(InMemory);
			if (It != StackTraceMap.end())
			{
				free(It->second);
				StackTraceMap.erase(It);
			}
		}

		GIsInsideMemoryAllocation = false;
	}
#endif

	uint32 CurrentCount = TotalAllocationCount.load(memory_order_relaxed);
	if (CurrentCount > 0)
	{
		TotalAllocationCount.fetch_sub(1, memory_order_relaxed);
	}
	else
	{
		assert(!u8"allocation 처리한 객체보다 더 많은 수를 해제할 수 없음");
	}

	uint32 CurrentBytes = TotalAllocationBytes.load(memory_order_relaxed);
	if (CurrentBytes >= MemoryAllocSize)
	{
		TotalAllocationBytes.fetch_sub(static_cast<uint32>(MemoryAllocSize), memory_order_relaxed);
	}
	else
	{
		assert(!u8"allocation 처리한 메모리보다 더 많은 양의 메모리를 해제할 수 없음");
	}

	if (MemoryHeader->bIsAligned)
	{
		// Aligned allocation: retrieve original pointer
		void** RawStoragePtr = reinterpret_cast<void**>(MemoryHeader) - 1;
		void* RawMemoryPtr = *RawStoragePtr;
		free(RawMemoryPtr);
	}
	else
	{
		free(MemoryHeader);
	}
}

/**
 * 배열에 대한 메모리 할당자 오버로딩 함수
 * @param InSize 할당 size
 * 세부 처리는 new 단일 객체 처리로 진행된다
 */
void* operator new[](size_t InSize)
{
	return ::operator new(InSize);
}

/**
 * 배열에 대한 메모리 할당 해제 오버로딩 함수
 * @param InMemory 제공된 메모리 주소
 * 세부 처리는 delete 단일 객체 처리로 진행된다
 */
void operator delete[](void* InMemory) noexcept
{
	::operator delete(InMemory);
}

// C++17에서 추가로 제공된 Align된 메모리에 대한 오버로딩 함수
// SIMD 타입이 추후 필요한 것으로 보고 미리 구현해 둠

void* operator new(size_t InSize, align_val_t InAlignment)
{
	size_t Alignment = static_cast<size_t>(InAlignment);

	TotalAllocationCount.fetch_add(1, memory_order_relaxed);
	TotalAllocationBytes.fetch_add(static_cast<uint32>(InSize), memory_order_relaxed);

	// 수동 정렬: [RawMemoryPtr ... void*(원본주소) ... AllocHeader ... padding ... AlignedDataPtr]
	size_t TotalSize = sizeof(AllocHeader) + sizeof(void*) + Alignment - 1 + InSize;
	void* RawMemoryPtr = malloc(TotalSize);
	if (!RawMemoryPtr)
	{
		throw std::bad_alloc();
	}

	// 정렬된 주소 계산
	uintptr_t RawAddr = reinterpret_cast<uintptr_t>(RawMemoryPtr);
	uintptr_t AlignedAddr = (RawAddr + sizeof(AllocHeader) + sizeof(void*) + Alignment - 1) & ~(Alignment - 1);
	void* AlignedDataPtr = reinterpret_cast<void*>(AlignedAddr);

	// 정렬된 데이터 바로 앞에 헤더 배치
	AllocHeader* MemoryHeader = static_cast<AllocHeader*>(AlignedDataPtr) - 1;
	MemoryHeader->size = InSize;
	MemoryHeader->bIsAligned = true;

	// 원본 포인터 저장 (free용)
	void** RawStoragePtr = reinterpret_cast<void**>(MemoryHeader) - 1;
	*RawStoragePtr = RawMemoryPtr;

#ifdef _DEBUG
	MemoryHeader->StackTrace = nullptr;

	// 재진입 방지
	if (!GIsInsideMemoryAllocation)
	{
		GIsInsideMemoryAllocation = true;

		MemoryHeader->StackTrace = CaptureStackTrace();
		{
			std::lock_guard<std::mutex> Lock(GetStackTraceMutex());
			GetStackTraceMap()[AlignedDataPtr] = MemoryHeader->StackTrace;
		}

		GIsInsideMemoryAllocation = false;
	}
#endif

	return AlignedDataPtr;
}

void operator delete(void* InMemory, align_val_t InAlignment) noexcept
{
	::operator delete(InMemory);
}
