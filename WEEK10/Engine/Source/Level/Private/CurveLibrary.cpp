#include "pch.h"
#include "Level/Public/CurveLibrary.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(UCurveLibrary, UObject)

// Define protected curve names
const TArray<FString> UCurveLibrary::ProtectedCurveNames = {
	"Linear",
	"EaseIn",
	"EaseOut",
	"EaseInOut",
	"EaseInBack",
	"EaseOutBack"
};

UCurveLibrary::UCurveLibrary()
{
}

UCurveLibrary::~UCurveLibrary() = default;

void UCurveLibrary::InitializeDefaults()
{
	// Add default preset curves
	AddOrUpdateCurve("Linear", FCurve::Linear());
	AddOrUpdateCurve("EaseIn", FCurve::EaseIn());
	AddOrUpdateCurve("EaseOut", FCurve::EaseOut());
	AddOrUpdateCurve("EaseInOut", FCurve::EaseInOut());
	AddOrUpdateCurve("EaseInBack", FCurve::EaseInBack());
	AddOrUpdateCurve("EaseOutBack", FCurve::EaseOutBack());
}

FCurve* UCurveLibrary::GetCurve(const FString& Name)
{
	auto It = Curves.Find(Name);
	return It ? &(*It) : nullptr;
}

const FCurve* UCurveLibrary::GetCurve(const FString& Name) const
{
	auto It = Curves.Find(Name);
	return It ? &(*It) : nullptr;
}

bool UCurveLibrary::AddOrUpdateCurve(const FString& Name, const FCurve& Curve)
{
	if (!IsValidCurveName(Name))
	{
		UE_LOG_ERROR("CurveLibrary: Invalid curve name '%s'", Name.c_str());
		return false;
	}

	Curves[Name] = Curve;
	return true;
}

bool UCurveLibrary::RemoveCurve(const FString& Name)
{
	if (IsProtectedCurve(Name))
	{
		UE_LOG_ERROR("CurveLibrary: Cannot remove protected curve '%s'", Name.c_str());
		return false;
	}

	return Curves.Remove(Name) > 0;
}

bool UCurveLibrary::RenameCurve(const FString& OldName, const FString& NewName)
{
	if (IsProtectedCurve(OldName))
	{
		UE_LOG_ERROR("CurveLibrary: Cannot rename protected curve '%s'", OldName.c_str());
		return false;
	}

	if (!IsValidCurveName(NewName))
	{
		UE_LOG_ERROR("CurveLibrary: Invalid new curve name '%s'", NewName.c_str());
		return false;
	}

	if (OldName == NewName)
	{
		return true; // No change needed
	}

	auto It = Curves.Find(OldName);
	if (!It)
	{
		UE_LOG_ERROR("CurveLibrary: Curve '%s' not found for renaming", OldName.c_str());
		return false;
	}

	if (Curves.Find(NewName))
	{
		UE_LOG_ERROR("CurveLibrary: Curve '%s' already exists", NewName.c_str());
		return false;
	}

	// Copy and remove old
	FCurve CurveCopy = *It;
	Curves.Remove(OldName);
	Curves[NewName] = CurveCopy;

	return true;
}

bool UCurveLibrary::HasCurve(const FString& Name) const
{
	return Curves.Find(Name) != nullptr;
}

bool UCurveLibrary::IsProtectedCurve(const FString& Name) const
{
	for (const FString& ProtectedName : ProtectedCurveNames)
	{
		if (ProtectedName == Name)
		{
			return true;
		}
	}
	return false;
}

TArray<FString> UCurveLibrary::GetCurveNames() const
{
	TArray<FString> Names;
	Names.Reserve(Curves.Num());

	for (const auto& Pair : Curves)
	{
		Names.Add(Pair.first);
	}

	return Names;
}

void UCurveLibrary::Clear()
{
	Curves.Empty();
}

void UCurveLibrary::Serialize(bool bLoading, JSON& InOutHandle)
{
	if (bLoading)
	{
		// Load curves from JSON
		Clear();

		JSON CurvesJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "Curves", CurvesJson))
		{
			for (auto& Pair : CurvesJson.ObjectRange())
			{
				const FString& CurveName = Pair.first;
				JSON& CurveDataJson = Pair.second;

				FCurve Curve;

				// Read control points
				FJsonSerializer::ReadFloat(CurveDataJson, "CP1_X", Curve.ControlPoint1X);
				FJsonSerializer::ReadFloat(CurveDataJson, "CP1_Y", Curve.ControlPoint1Y);
				FJsonSerializer::ReadFloat(CurveDataJson, "CP2_X", Curve.ControlPoint2X);
				FJsonSerializer::ReadFloat(CurveDataJson, "CP2_Y", Curve.ControlPoint2Y);

				// Read curve name (optional)
				FString CurveNameStr;
				if (FJsonSerializer::ReadString(CurveDataJson, "Name", CurveNameStr))
				{
					Curve.CurveName = CurveNameStr;
				}
				else
				{
					Curve.CurveName = CurveName; // Use key as name
				}

				Curves[CurveName] = Curve;
			}

			UE_LOG("CurveLibrary: Loaded %d curves", Curves.Num());
		}

		// Initialize defaults if no curves loaded
		if (Curves.Num() == 0)
		{
			InitializeDefaults();
		}
	}
	else
	{
		// Save curves to JSON
		JSON CurvesJson = JSON::Make(JSON::Class::Object);

		for (const auto& Pair : Curves)
		{
			const FString& CurveName = Pair.first;
			const FCurve& Curve = Pair.second;

			JSON CurveDataJson = JSON::Make(JSON::Class::Object);

			// Direct JSON assignment (no Write functions available)
			CurveDataJson["CP1_X"] = Curve.ControlPoint1X;
			CurveDataJson["CP1_Y"] = Curve.ControlPoint1Y;
			CurveDataJson["CP2_X"] = Curve.ControlPoint2X;
			CurveDataJson["CP2_Y"] = Curve.ControlPoint2Y;
			CurveDataJson["Name"] = Curve.CurveName;

			CurvesJson[CurveName] = CurveDataJson;
		}

		InOutHandle["Curves"] = CurvesJson;
	}
}

bool UCurveLibrary::IsValidCurveName(const FString& Name) const
{
	if (Name.empty())
	{
		return false;
	}

	// Check for invalid characters (optional - can be stricter)
	// For now, just check it's not empty
	return true;
}

UObject* UCurveLibrary::Duplicate()
{
	UCurveLibrary* DuplicatedLibrary = Cast<UCurveLibrary>(Super::Duplicate());

	// Deep copy all curves
	DuplicatedLibrary->Curves.Empty();
	for (const auto& Pair : Curves)
	{
		DuplicatedLibrary->Curves[Pair.first] = Pair.second; // FCurve is a struct, copied by value
	}

	return DuplicatedLibrary;
}
