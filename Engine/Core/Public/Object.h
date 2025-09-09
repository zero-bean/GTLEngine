#pragma once

class UObject
{
public:
	// Special Member Function
	UObject();
	explicit UObject(const FString& InString);
	virtual ~UObject() = default;

	// Getter & Setter
	const FString& GetName() const { return Name; }
	const UObject* GetOuter() const { return Outer; }

	void SetName(const FString& InName) { Name = InName; }
	void SetOuter(UObject* InObject);

	void AddMemoryUsage(uint64 Bytes, uint32 Count = 1);
	void RemoveMemoryUsage(uint64 Bytes, uint32 Count = 1);

	uint64 GetAllocatedBytes() const { return AllocatedBytes; }
	uint32 GetAllocatedCount() const { return AllocatedCounts; }

private:
	uint32 UUID = -1;
	uint32 InternalIndex = -1;
	FString Name;
	UObject* Outer;

	uint32 AllocatedCounts = 0;
	uint64 AllocatedBytes = 0;
};

extern TArray<UObject*> GUObjectArray;
