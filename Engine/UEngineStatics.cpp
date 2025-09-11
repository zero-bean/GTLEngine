#include "stdafx.h"
#include "UEngineStatics.h"

uint32 UEngineStatics::NextUUID = 1;
uint32 UEngineStatics::TotalAllocationBytes = 0;
uint32 UEngineStatics::TotalAllocationCount = 0;
bool UEngineStatics::isEnabled = true;