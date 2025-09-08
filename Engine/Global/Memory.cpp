#include "pch.h"

UINT TotalAllocationBytes = 0;
UINT TotalAllocationCount = 0;

void* operator new(size_t size)
{
	TotalAllocationCount++;
	TotalAllocationBytes += size;
	AllocHeader* header = (AllocHeader*)malloc(sizeof(AllocHeader) + size);
	header->size = size;
	return (void*)(header + 1);
}

void operator delete(void* memory) noexcept
{
	if (!memory) return;

	AllocHeader* header = ((AllocHeader*)memory) - 1;
	size_t size = header->size;

	if (TotalAllocationCount > 0) TotalAllocationCount--;

	if (TotalAllocationBytes >= size) TotalAllocationBytes -= size;

	free(header);
}

