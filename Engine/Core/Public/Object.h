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
	void SetOuter(UObject* InObject) { Outer = InObject; }

private:
	UINT ID;
	FString Name;
	UObject* Outer;
};
