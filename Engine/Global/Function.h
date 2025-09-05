#pragma once

template<typename T>
static void SafeDelete(T& InDynamicObject)
{
	delete InDynamicObject;
	InDynamicObject = nullptr;
}
