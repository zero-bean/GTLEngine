#include "pch.h"
#include "Core/Public/PlatformTime.h"

double FWindowsPlatformTime::GSecondsPerCycle = 0.0;
bool   FWindowsPlatformTime::bInitialized = false;
