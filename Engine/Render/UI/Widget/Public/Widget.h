#pragma once
#include "Core/Public/Object.h"

/**
 * @brief UI의 기능 단위인 위젯 클래스의 Interface Class
 * 위젯이라면 필수적인 공통 인터페이스와 기능을 제공
 */
class UWidget : public UObject
{
public:
	// Essential Role
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void RenderWidget() = 0;

	// Special Member Function
	UWidget() = default;
	UWidget(const FString& InName);
	~UWidget() override = default;
};
