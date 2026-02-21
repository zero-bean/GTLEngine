#pragma once
#include "Global/CoreTypes.h"

class UObject;
class UClass;

/**
 * @brief UClass Metadata System
 * Runtime에 컴파일 시에 제거되는 다양한 클래스 정보를 제공하기 위해 만들어진 클래스
 * @param ClassName 클래스 이름
 * @param SuperClass 부모 클래스
 * @param ClassSize 클래스 크기
 * @param Constructor 생성자 함수 포인터
 */
class UClass
{
public:
	// 생성자 함수 포인터 타입 정의
	typedef UObject* (*ClassConstructorType)();

	UClass(const FString& InName, UClass* InSuperClass, size_t InClassSize, ClassConstructorType InConstructor);

	static UClass* FindClass(const FString& InClassName);
	static void SignUpClass(UClass* InClass);
	static void PrintAllClasses();

	bool IsChildOf(const UClass* InClass) const;
	UObject* CreateDefaultObject() const;

 	// Getter
	const FString& GetClass() const { return ClassName; }
	UClass* GetSuperClass() const { return SuperClass; }
	size_t GetClassSize() const { return ClassSize; }

private:
	FString ClassName;
	UClass* SuperClass;
	size_t ClassSize;
	ClassConstructorType Constructor;

	// Class Registry
	static TArray<UClass*> AllClasses;
};

/**
 * @brief RTTI 매크로 시스템
 *
 * 언리얼엔진의 UCLASS(), DECLARE_CLASS, IMPLEMENT_CLASS와 유사한 매크로들 정의
 */
// UCLASS 매크로, 기능은 존재하지 않으며 외관 상의 유사성을 목표로 세팅
#define UCLASS()

// Friend Class 처리를 위한 Generated Body 매크로
#define GENERATED_BODY() \
public: \
    friend class UClass; \
    friend class UObject;

// 클래스 선언부에 사용하는 매크로
#define DECLARE_CLASS(ClassName, SuperClassName) \
public: \
    typedef ClassName ThisClass; \
    typedef SuperClassName Super; \
    static UClass* StaticClass(); \
    virtual UClass* GetClass() const; \
    static UObject* CreateDefaultObject##ClassName(); \
private: \
    static UClass* ClassPrivate;

// 클래스 구현부에 사용하는 매크로
#define IMPLEMENT_CLASS(ClassName, SuperClassName) \
    UClass* ClassName::ClassPrivate = nullptr; \
    UClass* ClassName::StaticClass() \
    { \
        if (!ClassPrivate) \
        { \
            ClassPrivate = new UClass( \
                FString(#ClassName), \
                SuperClassName::StaticClass(), \
                sizeof(ClassName), \
                &ClassName::CreateDefaultObject##ClassName \
            ); \
            UClass::SignUpClass(ClassPrivate); \
        } \
        return ClassPrivate; \
    } \
    UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    UObject* ClassName::CreateDefaultObject##ClassName() \
    { \
        return new ClassName(); \
    }

// UObject의 기본 매크로 (Base Class)
#define IMPLEMENT_CLASS_BASE(ClassName) \
    UClass* ClassName::ClassPrivate = nullptr; \
    UClass* ClassName::StaticClass() \
    { \
        if (!ClassPrivate) \
        { \
            ClassPrivate = new UClass( \
                FString(#ClassName), \
                nullptr, \
                sizeof(ClassName), \
                &ClassName::CreateDefaultObject##ClassName \
            ); \
            UClass::SignUpClass(ClassPrivate); \
        } \
        return ClassPrivate; \
    } \
    UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    UObject* ClassName::CreateDefaultObject##ClassName() \
    { \
        return new ClassName(); \
    }
