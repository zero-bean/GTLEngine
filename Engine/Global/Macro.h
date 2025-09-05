#pragma once

#define DT UTimeManager::GetInstance().GetDeltaTime()

// 싱글톤 패턴 매크로화
// Declare - Implement를 쌍으로 사용해야 한다
#define DECLARE_SINGLETON(ClassName) \
public: \
static ClassName& GetInstance(); \
private: \
ClassName(); \
virtual ~ClassName(); \
ClassName(const ClassName&) = delete; \
ClassName& operator=(const ClassName&) = delete; \
ClassName(ClassName&&) = delete; \
ClassName& operator=(ClassName&&) = delete;

#define IMPLEMENT_SINGLETON(ClassName) \
ClassName& ClassName::GetInstance() \
{ \
static ClassName Instance; \
return Instance; \
}
