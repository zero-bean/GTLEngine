#pragma once
#include "Core/Public/Delegate.h"

/**
 * @brief Lua 스크립트에 Delegate를 노출하는 객체를 위한 인터페이스
 *
 * 사용 예시:
 * @code
 * class AActor : public UObject, public IDelegateProvider
 * {
 * public:
 *     TArray<FDelegateInfoBase*> GetDelegates() const override
 *     {
 *         TArray<FDelegateInfoBase*> Result;
 *         Result.Add(MakeDelegateInfo("OnActorBeginOverlap", &OnActorBeginOverlap));
 *         Result.Add(MakeDelegateInfo("OnActorEndOverlap", &OnActorEndOverlap));
 *         return Result;
 *     }
 * };
 * @endcode
 */
class IDelegateProvider
{
public:
    virtual ~IDelegateProvider() = default;

    /**
     * @brief 이 객체가 소유한 Lua 노출용 Delegate 목록 반환
     * @return Lua에 노출할 Delegate 정보 배열
     */
    virtual TArray<FDelegateInfoBase*> GetDelegates() const = 0;
};
