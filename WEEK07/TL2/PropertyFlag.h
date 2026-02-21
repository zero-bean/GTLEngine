#pragma once

enum class EPropertyFlag : uint64
{
    CPF_None                = 0,

    // Duplication behavior
    CPF_Instanced          = 1 << 0,  // 복제 시 참조된 객체도 Deep Copy
    CPF_DuplicateTransient = 1 << 1,  // 복제 시 이 프로퍼티를 복사하지 않음

    // Future expansion
    CPF_Transient          = 1 << 2,  // 저장되지 않음 (깊은 복사)
    CPF_Config             = 1 << 3,  // Config 파일에서 로드 (현재 사용 X)
    CPF_EditAnywhere       = 1 << 4,  // 에디터에서 편집 가능 (얕은 복사)
};

// Flag 비트연산
inline EPropertyFlag operator|(EPropertyFlag A, EPropertyFlag B)
{
    return static_cast<EPropertyFlag>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
}

inline EPropertyFlag operator&(EPropertyFlag A, EPropertyFlag B)
{
    return static_cast<EPropertyFlag>(static_cast<uint64_t>(A) & static_cast<uint64_t>(B));
}

inline EPropertyFlag operator~(EPropertyFlag A)
{
    return static_cast<EPropertyFlag>(~static_cast<uint64_t>(A));
}

inline EPropertyFlag& operator|=(EPropertyFlag& A, EPropertyFlag B)
{
    A = A | B;
    return A;
}

inline EPropertyFlag& operator&=(EPropertyFlag& A, EPropertyFlag B)
{
    A = A & B;
    return A;
}

inline bool HasFlag(EPropertyFlag Flags, EPropertyFlag Flag)
{
    return (static_cast<uint64_t>(Flags) & static_cast<uint64_t>(Flag)) != 0;
}
