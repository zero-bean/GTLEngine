#pragma once

#include "Archive.h"

class FDuplicateDataReader : public FArchive
{
public:
	virtual ~FDuplicateDataReader() = default;

	virtual bool IsLoading() const override
	{
		return true;
	}

	virtual void Serialize(void* V, size_t Length)
	{
		
	}

	virtual FArchive& operator<<(FName& Name) override
	{
		// ===================== UE Code ========================== //
		//FNameEntryId ComparisonIndex;
		//FNameEntryId DisplayIndex;
		//int32 Number;
		//ByteOrderSerialize(&ComparisonIndex, sizeof(ComparisonIndex));
		//ByteOrderSerialize(&DisplayIndex, sizeof(DisplayIndex));
		//ByteOrderSerialize(&Number, sizeof(Number));
		//// copy over the name with a name made from the name index and number
		//N = FName(ComparisonIndex, DisplayIndex, Number);
		// ===================== UE Code ========================== //

		return *this;
	}

	virtual FArchive& operator<<(UObject*& Object) override
	{
		UObject* SourceObject = Object;
		Serialize(&Object, sizeof(UObject*));

		// ===================== UE Code ========================== //
		//FDuplicatedObject ObjectInfo = SourceObject ? DuplicatedObjectAnnotation.GetAnnotation(SourceObject) : FDuplicatedObject();
		//if (!ObjectInfo.IsDefault())
		//{
		//	Object = ObjectInfo.DuplicatedObject.GetEvenIfUnreachable();
		//}
		//else
		//{
		//	Object = SourceObject;
		//}
		// ===================== UE Code ========================== //

		return *this;
	}

private:

};
