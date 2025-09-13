#pragma once

// FName은 문자열을 효율적으로 비교하고 저장하기 위한 구조체
struct FName
{
    FName(const char* InString);
    FName(const FString& InString);
	
    int32 Compare(const FName& Other) const;
    bool operator==(const FName&) const;
	bool operator!=(const FName&) const;
	
	const FString& ToString() const;

    FName GetNone() const;

    int32 DisplayIndex; // 비교 시엔 ComparisonIndex 사용
    int32 ComparisonIndex; // 실제 문자열이 필요할 때만 DisplayIndex 사용
};

// FNamePool에 저장될 실제 이름 데이터
class FNameEntry
{
public:
    // 원본 문자열 (대소문자 구분)
    FString String;

    // 이 이름의 대소문자 미구분 버전이 처음 등록될 때의 DisplayIndex
    // 즉, 같은 ComparisonIndex를 공유하는 이름들 중 대표 DisplayIndex
    int32 ComparisonIndex;

    FNameEntry(const FString& InString, int32 InComparisonIndex);
};

// UNamePool은 FNameEntry 객체들을 관리하는 싱글톤 클래스
class UNamePool
{
public:
	// 싱글톤 인스턴스 접근
	static UNamePool& GetInstance();

    // 이름을 시스템에 검색 or 등록 후 인덱스를 받아오는 핵심 함수
	void FindOrAddEntry(const FString& Name, int32& OutDisplayIndex, int32& OutComparisonIndex);

	// DisplayIndex로 FNameEntry를 찾아 반환
	const FString& GetString(int32 DisplayIndex) const;

    // 디버그 목적 로깅
	void LogDebugState() const;

private:
    UNamePool() = default;

	TArray<FNameEntry> Entries; // FNameEntry 객체들을 저장하는 메인 배열
	TMap<FString, int32> DisplayMap; // Key: 원본 문자열 (대소문자 구분), Value: DisplayIndex
	TMap<FString, int32> ComparisonMap; // Key: 소문자 문자열 (대소문자 미구분), Value: 대표 ComparisonIndex
};
