#include "pch.h"
#include "Core/Public/Object.h"

static UINT NextID = 0;

UObject::UObject()
{
    ++NextID;
    ID = NextID;
}

UObject::~UObject()
{
}
