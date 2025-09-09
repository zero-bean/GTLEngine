#include "pch.h"
#include "Utility/Public/LevelSerializer.h"

#include "json.hpp"
#include "Utility/Public/Metadata.h"

using json::JSON;

/**
 * @brief FVector를 JSON으로 변환
 */
JSON FLevelSerializer::VectorToJson(const FVector& InVector)
{
	JSON VectorArray = JSON::Make(JSON::Class::Array);
	VectorArray.append(InVector.X, InVector.Y, InVector.Z);
	return VectorArray;
}

/**
 * @brief JSON을 FVector로 변환
 * 적절하지 않은 JSON을 입력 받았을 경우 혹은 파싱에서 오류가 발생한 경우에 Zero Vector를 반환한다
 * 따로 로드에서 발생하는 에러는 일단 핸들링하지 않음
 */
FVector FLevelSerializer::JsonToVector(const JSON& InJsonData)
{
	if (InJsonData.JSONType() != JSON::Class::Array || InJsonData.size() != 3)
	{
		return {0.0f, 0.0f, 0.0f};
	}

	try
	{
		return {
			static_cast<float>(InJsonData.at(0).ToFloat()),
			static_cast<float>(InJsonData.at(1).ToFloat()),
			static_cast<float>(InJsonData.at(2).ToFloat())
		};
	}
	catch (const exception&)
	{
		return {0.0f, 0.0f, 0.0f};
	}
}

/**
 * @brief EPrimitiveType을 문자열로 변환
 */
FString FLevelSerializer::PrimitiveTypeToWideString(EPrimitiveType InType)
{
	switch (InType)
	{
	case EPrimitiveType::Sphere:
		return "Sphere";
	case EPrimitiveType::Cube:
		return "Cube";
	case EPrimitiveType::Triangle:
		return "Triangle";
	default:
		return "Unknown";
	}
}

/**
 * @brief 문자열을 EPrimitiveType으로 변환
 */
EPrimitiveType FLevelSerializer::StringToPrimitiveType(const FString& InTypeString)
{
	if (InTypeString == "Sphere")
	{
		return EPrimitiveType::Sphere;
	}
	if (InTypeString == "Cube")
	{
		return EPrimitiveType::Cube;
	}
	if (InTypeString == "Triangle")
	{
		return EPrimitiveType::Triangle;
	}

	return EPrimitiveType::None;
}

/**
 * @brief FPrimitiveMetadata를 JSON으로 변환
 */
JSON FLevelSerializer::PrimitiveMetadataToJson(const FPrimitiveMetadata& InPrimitive)
{
	JSON PrimitiveJson;
	PrimitiveJson["Location"] = VectorToJson(InPrimitive.Location);
	PrimitiveJson["Rotation"] = VectorToJson(InPrimitive.Rotation);
	PrimitiveJson["Scale"] = VectorToJson(InPrimitive.Scale);
	PrimitiveJson["Type"] = PrimitiveTypeToWideString(InPrimitive.Type);
	return PrimitiveJson;
}

/**
 * @brief JSON을 FPrimitiveData로 변환
 */
FPrimitiveMetadata FLevelSerializer::JsonToPrimitive(const JSON& InJsonData, uint32 InID)
{
	FPrimitiveMetadata PrimitiveMeta;
	PrimitiveMeta.ID = InID;

	try
	{
		if (InJsonData.JSONType() == JSON::Class::Object)
		{
			const JSON& LocationJson = InJsonData.at("Location");
			const JSON& RotationJson = InJsonData.at("Rotation");
			const JSON& ScaleJson = InJsonData.at("Scale");
			const JSON& TypeJson = InJsonData.at("Type");
			PrimitiveMeta.Location = JsonToVector(LocationJson);
			PrimitiveMeta.Rotation = JsonToVector(RotationJson);
			PrimitiveMeta.Scale = JsonToVector(ScaleJson);
			PrimitiveMeta.Type = StringToPrimitiveType(TypeJson.ToString());

			cout << "[JsonToPrimitive] ID " << InID << ": Scale = ("
				<< PrimitiveMeta.Scale.X << ", "
				<< PrimitiveMeta.Scale.Y << ", "
				<< PrimitiveMeta.Scale.Z << ")" << "\n";
		}
	}
	catch (const exception&)
	{
		// JSON 파싱 실패 시 기본값 유지
	}

	return PrimitiveMeta;
}

/**
 * @brief FLevelMetadata를 JSON으로 변환
 */
JSON FLevelSerializer::LevelToJson(const FLevelMetadata& InLevelData)
{
	JSON LevelJson;
	LevelJson["Version"] = InLevelData.Version;
	LevelJson["NextUUID"] = InLevelData.NextUUID;

	JSON PrimitivesJson;
	for (const auto& [ID, Primitive] : InLevelData.Primitives)
	{
		PrimitivesJson[to_string(ID)] = PrimitiveMetadataToJson(Primitive);
	}
	LevelJson["Primitives"] = PrimitivesJson;

	return LevelJson;
}

/**
 * @brief JSON을 FLevelData로 변환
 */
FLevelMetadata FLevelSerializer::JsonToLevel(JSON& InJsonData)
{
	FLevelMetadata LevelData;

	try
	{
		// 버전 정보 파싱
		if (InJsonData.hasKey("Version"))
		{
			LevelData.Version = InJsonData.at("Version").ToInt();
		}

		// NextUUID 파싱
		if (InJsonData.hasKey("NextUUID"))
		{
			LevelData.NextUUID = InJsonData["NextUUID"].ToInt();
		}

		// Primitives 파싱
		if (InJsonData.hasKey("Primitives") &&
			InJsonData["Primitives"].JSONType() == JSON::Class::Object)
		{
			const auto& PrimitivesJson = InJsonData["Primitives"];
			for (const auto& InPair : PrimitivesJson.ObjectRange())
			{
				try
				{
					uint32 ID = stoul(InPair.first);
					FPrimitiveMetadata Primitive = JsonToPrimitive(InPair.second, ID);
					LevelData.Primitives[ID] = Primitive;
				}
				catch (const exception&)
				{
					cout << "[JSON PARSER] Failed To Load Primitive From JSON" << "\n";
					continue;
				}
			}
		}
	}
	catch (const exception&)
	{
		cout << "[JSON PARSER] Failed To Load Level From JSON" << "\n";
	}

	return LevelData;
}

/**
 * @brief 레벨 데이터를 파일에 저장
 */
bool FLevelSerializer::SaveLevelToFile(const FLevelMetadata& InLevelData, const FString& InFilePath)
{
	try
	{
		JSON LevelJson = LevelToJson(InLevelData);

		ofstream File(InFilePath);
		if (!File.is_open())
		{
			return false;
		}

		File << setw(2) << LevelJson << "\n";
		File.close();

		return true;
	}
	catch (const exception&)
	{
		return false;
	}
}

/**
 * @brief 파일에서 레벨 데이터 로드
 */
bool FLevelSerializer::LoadLevelFromFile(FLevelMetadata& OutLevelData, const FString& InFilePath)
{
	try
	{
		ifstream File(InFilePath);
		if (!File.is_open())
		{
			return false;
		}

		// 파일 전체 내용을 한 번에 읽도록 처리
		File.seekg(0, std::ios::end);
		size_t FileSize = File.tellg();

		File.seekg(0, std::ios::beg);

		FString FileContent;
		FileContent.resize(FileSize);
		File.read(FileContent.data(), FileSize);
		File.close();

		cout << "[LevelSerializer] File Content Length: " << FileContent.length() << "\n";

		JSON JsonData = JSON::Load(FileContent);
		OutLevelData = JsonToLevel(JsonData);

		return true;
	}
	catch (const exception&)
	{
		return false;
	}
}

/**
 * @brief JSON 문자열을 예쁘게 포맷팅
 */
FString FLevelSerializer::FormatJsonString(const JSON& JsonData, int Indent)
{
	return JsonData.dump(Indent);
}

/**
 * @brief 레벨 데이터 유효성 검사
 */
bool FLevelSerializer::ValidateLevelData(const FLevelMetadata& InLevelData, FString& OutErrorMessage)
{
	OutErrorMessage.clear();

	// 버전 체크
	if (InLevelData.Version == 0)
	{
		OutErrorMessage = "Invalid Version: Version Must Be Greater Than 0";
		return false;
	}

	// NextUUID 체크
	uint32 MaxUsedID = 0;
	for (const auto& [ID, Primitive] : InLevelData.Primitives)
	{
		MaxUsedID = max(MaxUsedID, ID);

		// ID 일관성 체크
		if (Primitive.ID != ID)
		{
			OutErrorMessage = "ID Mismatch: Primitive ID (" + to_string(Primitive.ID) +
				") Doesn't Match Map Key (" + to_string(ID) + ")";
			return false;
		}

		// 타입 체크
		if (Primitive.Type == EPrimitiveType::None)
		{
			OutErrorMessage = "Invalid Primitive Type For ID " + to_string(ID);
			return false;
		}

		// 스케일 체크 (0이면 안됨)
		if (Primitive.Scale.X == 0.0f || Primitive.Scale.Y == 0.0f || Primitive.Scale.Z == 0.0f)
		{
			OutErrorMessage = "Invalid Scale For Primitive ID " + to_string(ID) +
				": Scale Components Must Be Non-Zero";
			return false;
		}
	}

	// NextUUID가 사용된 ID보다 큰지 체크
	if (!InLevelData.Primitives.empty() && InLevelData.NextUUID <= MaxUsedID)
	{
		OutErrorMessage = "Invalid NextUUID: Must Be Greater Than The Highest Used ID (" +
			to_string(MaxUsedID) + ")";
		return false;
	}

	return true;
}

/**
 * @brief 두 레벨 데이터 병합 (중복 ID는 덮어 씀)
 * XXX(KHJ): 현재는 필요하지 않은 상황인데 필요한 경우가 존재할지 고민 후 제거해도 될 듯
 */
FLevelMetadata FLevelSerializer::MergeLevelData(const FLevelMetadata& InBaseLevel,
                                                const FLevelMetadata& InMergeLevel)
{
	FLevelMetadata ResultLevel = InBaseLevel;

	// 버전은 더 높은 것으로
	ResultLevel.Version = max(InBaseLevel.Version, InMergeLevel.Version);

	// MergeLevel의 프리미티브들을 추가 / 덮어쓰기
	for (const auto& [ID, Primitive] : InMergeLevel.Primitives)
	{
		ResultLevel.Primitives[ID] = Primitive;
	}

	// NextUUID 업데이트
	ResultLevel.NextUUID = max(InBaseLevel.NextUUID, InMergeLevel.NextUUID);

	return ResultLevel;
}

/**
 * @brief 특정 타입의 프리미티브들만 필터링
 */
TArray<FPrimitiveMetadata> FLevelSerializer::FilterPrimitivesByType(const FLevelMetadata& InLevelData,
                                                                    EPrimitiveType InType)
{
	TArray<FPrimitiveMetadata> FilteredPrimitives;

	for (const auto& [ID, Primitive] : InLevelData.Primitives)
	{
		if (Primitive.Type == InType)
		{
			FilteredPrimitives.push_back(Primitive);
		}
	}

	return FilteredPrimitives;
}

/**
 * @brief 레벨 데이터 통계 정보 생성
 */
FLevelSerializer::FLevelStats FLevelSerializer::GenerateLevelStats(const FLevelMetadata& InLevelData)
{
	FLevelStats Stats;

	if (InLevelData.Primitives.empty())
	{
		return Stats;
	}

	Stats.TotalPrimitives = static_cast<uint32>(InLevelData.Primitives.size());

	// 바운딩 박스 초기화
	// bool bFirstPrimitive = true;

	for (const auto& [ID, Primitive] : InLevelData.Primitives)
	{
		// 타입별 카운트
		Stats.PrimitiveCountByType[Primitive.Type]++;

		// 바운딩 박스 계산
		// if (bFirstPrimitive)
		// {
		// 	Stats.BoundingBoxMin = Primitive.Location;
		// 	Stats.BoundingBoxMax = Primitive.Location;
		// 	bFirstPrimitive = false;
		// }
		// else
		// {
		// 	Stats.BoundingBoxMin.X = min(Stats.BoundingBoxMin.X, Primitive.Location.X);
		// 	Stats.BoundingBoxMin.Y = min(Stats.BoundingBoxMin.Y, Primitive.Location.Y);
		// 	Stats.BoundingBoxMin.Z = min(Stats.BoundingBoxMin.Z, Primitive.Location.Z);
		//
		// 	Stats.BoundingBoxMax.X = max(Stats.BoundingBoxMax.X, Primitive.Location.X);
		// 	Stats.BoundingBoxMax.Y = max(Stats.BoundingBoxMax.Y, Primitive.Location.Y);
		// 	Stats.BoundingBoxMax.Z = max(Stats.BoundingBoxMax.Z, Primitive.Location.Z);
		// }
	}

	return Stats;
}

/**
 * @brief 디버그용 레벨 정보 출력
 */
void FLevelSerializer::PrintLevelInfo(const FLevelMetadata& InLevelData)
{
	cout << "=== Level Information ===" << "\n";
	cout << "Version: " << InLevelData.Version << "\n";
	cout << "NextUUID: " << InLevelData.NextUUID << "\n";
	cout << "Total Primitives: " << InLevelData.Primitives.size() << "\n";

	if (!InLevelData.Primitives.empty())
	{
		FLevelStats Stats = GenerateLevelStats(InLevelData);

		cout << "\n--- Primitive Count by Type ---" << "\n";
		for (const auto& [Type, Count] : Stats.PrimitiveCountByType)
		{
			cout << PrimitiveTypeToWideString(Type) << ": " << Count << "\n";
		}

		// cout << "\n--- Bounding Box ---" << "\n";
		// cout << "Min: (" << Stats.BoundingBoxMin.X << ", " << Stats.BoundingBoxMin.Y
		// 	<< ", " << Stats.BoundingBoxMin.Z << ")" << "\n";
		// cout << "Max: (" << Stats.BoundingBoxMax.X << ", " << Stats.BoundingBoxMax.Y
		// 	<< ", " << Stats.BoundingBoxMax.Z << ")" << "\n";

		cout << "\n--- Detailed Primitive List ---" << "\n";
		for (const auto& [ID, Primitive] : InLevelData.Primitives)
		{
			cout << "ID " << ID << " [" << PrimitiveTypeToWideString(Primitive.Type) << "]" << "\n";
			cout << "  Location: (" << Primitive.Location.X << ", " << Primitive.Location.Y
				<< ", " << Primitive.Location.Z << ")" << "\n";
			cout << "  Rotation: (" << Primitive.Rotation.X << ", " << Primitive.Rotation.Y
				<< ", " << Primitive.Rotation.Z << ")" << "\n";
			cout << "  Scale: (" << Primitive.Scale.X << ", " << Primitive.Scale.Y
				<< ", " << Primitive.Scale.Z << ")" << "\n";
		}
	}

	cout << "=========================" << "\n";
}

/**
 * @brief JSON 파싱 오류 처리 함수
 */
bool FLevelSerializer::HandleJsonError(const exception& InException, const FString& InContext,
                                       FString& OutErrorMessage)
{
	OutErrorMessage = InContext + ": " + InException.what();
	return false;
}
