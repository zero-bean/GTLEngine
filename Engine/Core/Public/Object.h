#pragma once

class UObject
{
public:
	// Special Member Function
    UObject();

	UObject* Outer = nullptr;
	FString Name;
    virtual ~UObject();

private:
	UINT UUID = -1;
	UINT InternalIndex = -1;
};

extern TArray<UObject*> GUObjectArray;
