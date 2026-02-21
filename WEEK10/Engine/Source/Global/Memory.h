#pragma once

#include <atomic>

extern std::atomic<uint32> TotalAllocationBytes;
extern std::atomic<uint32> TotalAllocationCount;

#ifdef _DEBUG
constexpr int MAX_STACK_FRAMES = 32;

struct FStackTrace
{
	void* Frames[MAX_STACK_FRAMES];
	uint16 FrameCount;
};
#endif

struct AllocHeader
{
	size_t size;
	bool bIsAligned;
#ifdef _DEBUG
	FStackTrace* StackTrace;
#endif
};

#ifdef _DEBUG
	void InitializeMemoryTracking();
	void ShutdownMemoryTracking();
#endif

