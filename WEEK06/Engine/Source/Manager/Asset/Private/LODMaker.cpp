#include "pch.h"
#include "Manager/Asset/Public/LODMaker.h"

#include <map>
#include <set>

bool FMeshSimplifier::LoadFromObj(const FString& InFilename)
{
	if (!FObjImporter::LoadObj(InFilename, &ObjInfo))
	{
		UE_LOG("OBJ 로딩 실패: %s", InFilename.data());
		return false;
	}

	SimplificationVertices.reserve(ObjInfo.VertexList.size());
	for (const auto& Vertex : ObjInfo.VertexList)
	{
		FSimplificationVertex SV;
		SV.Position = Vertex;
		SimplificationVertices.push_back(SV);
	}

	for (const auto& ObjectInfo : ObjInfo.ObjectInfoList)
	{
		for (size_t i = 0; i < ObjectInfo.VertexIndexList.size(); i += 3)
		{
			FTriangle Tri;
			for (int j = 0; j < 3; ++j)
			{
				Tri.Vertices[j].VertexIndex = static_cast<int>(ObjectInfo.VertexIndexList[i + j]);

				if (i + j < ObjectInfo.TexCoordIndexList.size() && !ObjectInfo.TexCoordIndexList.empty())
				{
					Tri.Vertices[j].TexCoordIndex = static_cast<int>(ObjectInfo.TexCoordIndexList[i + j]);
				}
				else
				{
					Tri.Vertices[j].TexCoordIndex = -1;
				}

				if (i + j < ObjectInfo.NormalIndexList.size() && !ObjectInfo.NormalIndexList.empty())
				{
					Tri.Vertices[j].NormalIndex = static_cast<int>(ObjectInfo.NormalIndexList[i + j]);
				}
				else
				{
					Tri.Vertices[j].NormalIndex = -1;
				}
			}
			Triangles.push_back(Tri);
		}
	}

	UE_LOG("%s OBJ 로드 완료: 정점 %llu개, 삼각형 %llu 개", InFilename.data(), SimplificationVertices.size(), Triangles.size());
	return true;
}

void FMeshSimplifier::Simplify(float InReductionRatio)
{
	uint32 OriginalTriangleCount = static_cast<uint32>(Triangles.size());
	uint32 TargetTriangleCount = static_cast<uint32>(OriginalTriangleCount * InReductionRatio);

	UE_LOG("단순화 시작...");
	UE_LOG("원본 삼각형 수: %d", OriginalTriangleCount);
	UE_LOG("감축 비율: %.2f (%.1f%%)", InReductionRatio, InReductionRatio * 100.0f);
	UE_LOG("목표 삼각형 수: %d", TargetTriangleCount);

	if (InReductionRatio >= 1.0f)
	{
		UE_LOG("감축 비율이 1.0 이상이므로 단순화를 건너뜁니다.");
		return;
	}

	if (InReductionRatio <= 0.0f)
	{
		UE_LOG("감축 비율이 0.0 이하이므로 단순화를 건너뜁니다.");
		return;
	}

	if (TargetTriangleCount >= OriginalTriangleCount)
	{
		UE_LOG("목표 삼각형 수가 원본보다 크거나 같아 단순화를 건너뜁니다.");
		return;
	}

	CalculateInitialQuadrics();
	TPriorityQueue PQ = CalculateAllEdgeCosts();

	uint32 ActiveTriangles = OriginalTriangleCount;
	while (ActiveTriangles > TargetTriangleCount && !PQ.empty())
	{
		FEdgeCollapsePair BestPair = PQ.top();
		PQ.pop();

		if (!SimplificationVertices[BestPair.V1Index].bIsActive || !SimplificationVertices[BestPair.V2Index].bIsActive)
		{
			continue;
		}
		ActiveTriangles -= CollapseEdge(BestPair.V1Index, BestPair.V2Index, BestPair.OptimalPosition);
	}

	float ActualRatio = static_cast<float>(ActiveTriangles) / static_cast<float>(OriginalTriangleCount);

	UE_LOG("단순화 완료.");
	UE_LOG("최종 삼각형 수: %d", ActiveTriangles);
	UE_LOG("실제 감축 비율: %.2f (%.1f%%)", ActualRatio, ActualRatio * 100.0f);
}

bool FMeshSimplifier::SaveToObj(const FString& InFilename, float InReductionRatio)
{
	TArray<FVector> FinalPositions;
	TArray<FVector2> FinalTexCoords;
	TArray<FVector> FinalNormals;

	std::map<uint32, uint32> OldVToNewV;
	std::map<int, uint32> OldVtToNewVt;
	std::map<int, uint32> OldVnToNewVn;

	for (uint32 i = 0; i < SimplificationVertices.size(); ++i)
	{
		if (SimplificationVertices[i].bIsActive)
		{
			OldVToNewV[i] = static_cast<uint32>(FinalPositions.size());
			FinalPositions.push_back(SimplificationVertices[i].Position);
		}
	}

	TArray<FMeshIndex> FinalIndices;
	for (const auto& Tri : Triangles)
	{
		if (Tri.bIsActive)
		{
			FMeshIndex NewTriIndices[3];
			bool bIsValidTri = true;

			for (int i = 0; i < 3; ++i)
			{
				uint32 OldVIndex = Tri.Vertices[i].VertexIndex;
				if (OldVToNewV.find(OldVIndex) == OldVToNewV.end())
				{
					bIsValidTri = false;
					break;
				}
				NewTriIndices[i].VertexIndex = OldVToNewV[OldVIndex];

				int OldVtIndex = Tri.Vertices[i].TexCoordIndex;
				if (OldVtIndex != -1)
				{
					if (OldVtToNewVt.find(OldVtIndex) == OldVtToNewVt.end())
					{
						OldVtToNewVt[OldVtIndex] = static_cast<uint32>(FinalTexCoords.size());
						FinalTexCoords.push_back(ObjInfo.TexCoordList[OldVtIndex]);
					}
					NewTriIndices[i].TexCoordIndex = OldVtToNewVt[OldVtIndex];
				}
				else { NewTriIndices[i].TexCoordIndex = -1; }

				int OldVnIndex = Tri.Vertices[i].NormalIndex;
				if (OldVnIndex != -1)
				{
					if (OldVnToNewVn.find(OldVnIndex) == OldVnToNewVn.end())
					{
						OldVnToNewVn[OldVnIndex] = static_cast<uint32>(FinalNormals.size());
						FinalNormals.push_back(ObjInfo.NormalList[OldVnIndex]);
					}
					NewTriIndices[i].NormalIndex = OldVnToNewVn[OldVnIndex];
				}
				else { NewTriIndices[i].NormalIndex = -1; }
			}

			if (bIsValidTri)
			{
				FinalIndices.push_back(NewTriIndices[0]);
				FinalIndices.push_back(NewTriIndices[1]);
				FinalIndices.push_back(NewTriIndices[2]);
			}
		}
	}

	namespace fs = std::filesystem;

	// 원본 파일 경로에서 LOD 파일 경로 생성
	fs::path OriginalPath(InFilename);
	FString OriginalStem = OriginalPath.stem().string();

	int RatioInt = static_cast<int>(InReductionRatio * 100);
	FString RatioStr = std::to_string(RatioInt);
	if (RatioStr.length() < 3)
	{
		RatioStr = std::string(3 - RatioStr.length(), '0') + RatioStr;
	}

	// LOD 폴더 경로 생성
	fs::path LodDirectory = OriginalPath.parent_path() / "LOD";
	if (!fs::exists(LodDirectory))
	{
		fs::create_directories(LodDirectory);
		UE_LOG("LOD 폴더 생성: %s", LodDirectory.string().data());
	}

	// LOD OBJ 파일 경로 생성 (예: A_lod_075.obj)
	fs::path LodObjPath = LodDirectory / (OriginalStem + "_lod_" + RatioStr + ".obj");

	// LOD MTL 파일명 및 경로 생성 (예: A_lod_075.mtl)
	FString LodMtlFilename = OriginalStem + "_lod_" + RatioStr + ".mtl";
	fs::path LodMtlPath = LodDirectory / LodMtlFilename;

	// 원본 MTL 파일 경로
	fs::path OriginalMtlPath = OriginalPath.parent_path() / (OriginalStem + ".mtl");

	// 원본 MTL 파일이 존재하면 복사하고 텍스처 파일들도 처리
	bool bHasMtl = false;
	if (fs::exists(OriginalMtlPath))
	{
		try
		{
			// MTL 파일 읽기 및 텍스처 파일 경로 추출
			std::ifstream MtlFile(OriginalMtlPath);
			std::vector<FString> TextureFiles;
			FString Line;

			while (std::getline(MtlFile, Line))
			{
				// 텍스처 관련 키워드들 검사
				if (Line.find("map_Kd ") == 0 || // Diffuse map
					Line.find("map_Ka ") == 0 || // Ambient map
					Line.find("map_Ks ") == 0 || // Specular map
					Line.find("map_Ns ") == 0 || // Specular highlight map
					Line.find("map_d ") == 0 || // Alpha map
					Line.find("map_bump ") == 0 || // Bump map
					Line.find("bump ") == 0 || // Bump map (alternate)
					Line.find("map_Disp ") == 0 || // Displacement map
					Line.find("disp ") == 0 || // Displacement map (alternate)
					Line.find("map_Kn ") == 0 || // Normal map
					Line.find("norm ") == 0) // Normal map (alternate)
				{
					// 키워드 다음의 파일명 추출
					size_t SpacePos = Line.find(' ');
					if (SpacePos != FString::npos)
					{
						FString TextureFile = Line.substr(SpacePos + 1);
						// 앞뒤 공백 제거
						TextureFile.erase(0, TextureFile.find_first_not_of(" \t\r\n"));
						TextureFile.erase(TextureFile.find_last_not_of(" \t\r\n") + 1);

						if (!TextureFile.empty())
						{
							TextureFiles.push_back(TextureFile);
						}
					}
				}
			}
			MtlFile.close();

			// MTL 파일 복사
			fs::copy_file(OriginalMtlPath, LodMtlPath, fs::copy_options::overwrite_existing);
			UE_LOG("MTL 파일 복사 완료: %s", LodMtlPath.string().data());
			bHasMtl = true;

			// 텍스처 파일들 복사
			for (const auto& TextureFile : TextureFiles)
			{
				fs::path OriginalTexturePath = OriginalPath.parent_path() / TextureFile;
				fs::path LodTexturePath = LodDirectory / TextureFile;

				if (fs::exists(OriginalTexturePath))
				{
					try
					{
						fs::copy_file(OriginalTexturePath, LodTexturePath, fs::copy_options::overwrite_existing);
						UE_LOG("텍스처 파일 복사 완료: %s", LodTexturePath.string().data());
					}
					catch (const fs::filesystem_error& e)
					{
						std::cerr << "텍스처 파일 복사 실패 (" << TextureFile << "): " << e.what() << std::endl;
					}
				}
				else
				{
					UE_LOG("텍스처 파일을 찾을 수 없습니다: %s", OriginalTexturePath.string().data());
				}
			}

			if (!TextureFiles.empty())
			{
				UE_LOG("총 %llu개의 텍스처 파일을 처리했습니다.", TextureFiles.size());
			}
		}
		catch (const fs::filesystem_error& e)
		{
			std::cerr << "MTL 파일 복사 실패: " << e.what() << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "MTL 파일 처리 중 오류: " << e.what() << std::endl;
		}
	}
	else
	{
		UE_LOG("MTL 파일을 찾을 수 없습니다: %s", OriginalMtlPath.string().data());;
	}

	// LOD OBJ 파일 생성
	std::ofstream File(LodObjPath);
	if (!File.is_open())
	{
		std::cerr << "파일 열기 실패: " << LodObjPath.string() << std::endl;
		return false;
	}

	// MTL 파일 참조 (OBJ 표준)
	if (bHasMtl)
	{
		File << "mtllib " << LodMtlFilename << '\n';
	}

	for (const auto& V : FinalPositions) { File << "v " << V.X << " " << V.Y << " " << V.Z << std::endl; }
	for (const auto& Vt : FinalTexCoords) { File << "vt " << Vt.X << " " << Vt.Y << std::endl; }
	for (const auto& Vn : FinalNormals) { File << "vn " << Vn.X << " " << Vn.Y << " " << Vn.Z << std::endl; }

	if (!ObjInfo.ObjectMaterialInfoList.empty())
	{
		File << "usemtl " << ObjInfo.ObjectMaterialInfoList[0].Name << std::endl;
	}

	for (size_t i = 0; i < FinalIndices.size(); i += 3)
	{
		File << "f";
		for (int j = 0; j < 3; ++j)
		{
			File << " " << FinalIndices[i + j].VertexIndex + 1;
			if (FinalIndices[i + j].TexCoordIndex != -1)
			{
				File << "/" << FinalIndices[i + j].TexCoordIndex + 1;
				if (FinalIndices[i + j].NormalIndex != -1) File << "/" << FinalIndices[i + j].NormalIndex + 1;
			}
			else if (FinalIndices[i + j].NormalIndex != -1)
			{
				File << "//" << FinalIndices[i + j].NormalIndex + 1;
			}
		}
		File << std::endl;
	}

	File.close();
	UE_LOG("단순화된 메시 저장 완료: %s", LodObjPath.string().data());
	return true;
}

// bool FMeshSimplifier::SaveToObj(const FString& InFilename)
// {
//     TArray<FVector> FinalPositions;
//     TArray<FVector2> FinalTexCoords;
//     TArray<FVector> FinalNormals;
//
//     std::map<uint32, uint32> OldVToNewV;
//     std::map<int, uint32> OldVtToNewVt;
//     std::map<int, uint32> OldVnToNewVn;
//
//     for (uint32 i = 0; i < SimplificationVertices.size(); ++i)
//     {
//         if (SimplificationVertices[i].bIsActive)
//         {
//             OldVToNewV[i] = static_cast<uint32>(FinalPositions.size());
//             FinalPositions.push_back(SimplificationVertices[i].Position);
//         }
//     }
//
//     TArray<FMeshIndex> FinalIndices;
//     for (const auto& Tri : Triangles)
//     {
//         if (Tri.bIsActive)
//         {
//             FMeshIndex NewTriIndices[3];
//             bool bIsValidTri = true;
//
//             for (int i = 0; i < 3; ++i)
//             {
//                 uint32 OldVIndex = Tri.Vertices[i].VertexIndex;
//                 if (OldVToNewV.find(OldVIndex) == OldVToNewV.end())
//                 {
//                     bIsValidTri = false; break;
//                 }
//                 NewTriIndices[i].VertexIndex = OldVToNewV[OldVIndex];
//
//                 int OldVtIndex = Tri.Vertices[i].TexCoordIndex;
//                 if (OldVtIndex != -1)
//                 {
//                     if (OldVtToNewVt.find(OldVtIndex) == OldVtToNewVt.end())
//                     {
//                         OldVtToNewVt[OldVtIndex] = static_cast<uint32>(FinalTexCoords.size());
//                         FinalTexCoords.push_back(ObjInfo.TexCoordList[OldVtIndex]);
//                     }
//                     NewTriIndices[i].TexCoordIndex = OldVtToNewVt[OldVtIndex];
//                 }
//                 else { NewTriIndices[i].TexCoordIndex = -1; }
//
//                 int OldVnIndex = Tri.Vertices[i].NormalIndex;
//                 if (OldVnIndex != -1)
//                 {
//                     if (OldVnToNewVn.find(OldVnIndex) == OldVnToNewVn.end())
//                     {
//                         OldVnToNewVn[OldVnIndex] = static_cast<uint32>(FinalNormals.size());
//                         FinalNormals.push_back(ObjInfo.NormalList[OldVnIndex]);
//                     }
//                     NewTriIndices[i].NormalIndex = OldVnToNewVn[OldVnIndex];
//                 }
//                 else { NewTriIndices[i].NormalIndex = -1; }
//             }
//
//             if (bIsValidTri)
//             {
//                 FinalIndices.push_back(NewTriIndices[0]);
//                 FinalIndices.push_back(NewTriIndices[1]);
//                 FinalIndices.push_back(NewTriIndices[2]);
//             }
//         }
//     }
//
//     // MTL 파일 복사 처리
//     namespace fs = std::filesystem;
//     fs::path OutputPath(InFilename);
//     fs::path OutputDir = OutputPath.parent_path();
//
//     // LOD MTL 파일명 자동 생성 (.obj → .mtl)
//     fs::path LodMtlPath = fs::path(InFilename).replace_extension(".mtl");
//     FString LodMtlFilename = LodMtlPath.filename().generic_string();
//
//     std::ofstream File(InFilename);
//     if (!File.is_open())
//     {
//         std::cerr << "파일 열기 실패: " << InFilename << std::endl;
//         return false;
//     }
//
//     // LOD MTL 파일 참조 (OBJ 표준)
//     if (!LodMtlFilename.empty())
//     {
//         File << "mtllib " << LodMtlFilename << '\n';
//     }
//
//     for (const auto& V : FinalPositions) { File << "v " << V.X << " " << V.Y << " " << V.Z << std::endl; }
//     for (const auto& Vt : FinalTexCoords) { File << "vt " << Vt.X << " " << Vt.Y << std::endl; }
//     for (const auto& Vn : FinalNormals) { File << "vn " << Vn.X << " " << Vn.Y << " " << Vn.Z << std::endl; }
//
//     if (!ObjInfo.ObjectMaterialInfoList.empty())
//     {
//         File << "usemtl " << ObjInfo.ObjectMaterialInfoList[0].Name << std::endl;
//     }
//
//     for (size_t i = 0; i < FinalIndices.size(); i += 3)
//     {
//         File << "f";
//         for (int j = 0; j < 3; ++j)
//         {
//             File << " " << FinalIndices[i + j].VertexIndex + 1;
//             if (FinalIndices[i + j].TexCoordIndex != -1)
//             {
//                 File << "/" << FinalIndices[i + j].TexCoordIndex + 1;
//                 if (FinalIndices[i + j].NormalIndex != -1) File << "/" << FinalIndices[i + j].NormalIndex + 1;
//             }
//             else if (FinalIndices[i + j].NormalIndex != -1)
//             {
//                 File << "//" << FinalIndices[i + j].NormalIndex + 1;
//             }
//         }
//         File << std::endl;
//     }
//
//     File.close();
//     std::cout << "단순화된 메시 저장 완료: " << InFilename << std::endl;
//     return true;
// }

void FMeshSimplifier::CalculateInitialQuadrics()
{
	for (const auto& Tri : Triangles)
	{
		const FVector& P1 = SimplificationVertices[Tri.Vertices[0].VertexIndex].Position;
		const FVector& P2 = SimplificationVertices[Tri.Vertices[1].VertexIndex].Position;
		const FVector& P3 = SimplificationVertices[Tri.Vertices[2].VertexIndex].Position;

		FVector Normal = (P2 - P1).Cross(P3 - P1);
		Normal.Normalize();
		float a = Normal.X, b = Normal.Y, c = Normal.Z;
		float d = -Normal.Dot(P1);

		FMatrix Kp;
		Kp.Data[0][0] = a * a;
		Kp.Data[0][1] = a * b;
		Kp.Data[0][2] = a * c;
		Kp.Data[0][3] = a * d;
		Kp.Data[1][0] = b * a;
		Kp.Data[1][1] = b * b;
		Kp.Data[1][2] = b * c;
		Kp.Data[1][3] = b * d;
		Kp.Data[2][0] = c * a;
		Kp.Data[2][1] = c * b;
		Kp.Data[2][2] = c * c;
		Kp.Data[2][3] = c * d;
		Kp.Data[3][0] = d * a;
		Kp.Data[3][1] = d * b;
		Kp.Data[3][2] = d * c;
		Kp.Data[3][3] = d * d;

		SimplificationVertices[Tri.Vertices[0].VertexIndex].Q += Kp;
		SimplificationVertices[Tri.Vertices[1].VertexIndex].Q += Kp;
		SimplificationVertices[Tri.Vertices[2].VertexIndex].Q += Kp;
	}
}

TPriorityQueue FMeshSimplifier::CalculateAllEdgeCosts()
{
	TPriorityQueue PQ;
	std::set<std::pair<uint32, uint32>> UniqueEdges;

	for (const auto& Tri : Triangles)
	{
		for (int i = 0; i < 3; ++i)
		{
			uint32 V1Index = Tri.Vertices[i].VertexIndex;
			uint32 V2Index = Tri.Vertices[(i + 1) % 3].VertexIndex;
			if (V1Index > V2Index) std::swap(V1Index, V2Index);

			if (UniqueEdges.find({V1Index, V2Index}) == UniqueEdges.end())
			{
				PQ.push(CalculateEdgeCost(V1Index, V2Index));
				UniqueEdges.insert({V1Index, V2Index});
			}
		}
	}
	return PQ;
}

FEdgeCollapsePair FMeshSimplifier::CalculateEdgeCost(uint32 InV1Index, uint32 InV2Index)
{
	const auto& V1 = SimplificationVertices[InV1Index];
	const auto& V2 = SimplificationVertices[InV2Index];
	FMatrix QSum = V1.Q + V2.Q;

	FMatrix QPrime = QSum;
	QPrime.Data[3][0] = 0.0f;
	QPrime.Data[3][1] = 0.0f;
	QPrime.Data[3][2] = 0.0f;
	QPrime.Data[3][3] = 1.0f;

	FVector OptimalPos;
	if (abs(QPrime.Determinant()) > std::numeric_limits<float>::epsilon())
	{
		FMatrix QInv = QPrime.Inverse();
		OptimalPos = FVector(QInv.Data[0][3], QInv.Data[1][3], QInv.Data[2][3]);
	}
	else
	{
		OptimalPos = (V1.Position + V2.Position) * 0.5f;
	}

	FVector4 P(OptimalPos.X, OptimalPos.Y, OptimalPos.Z, 1.0f);
	FVector4 QV;
	QV.X = QSum.Data[0][0] * P.X + QSum.Data[0][1] * P.Y + QSum.Data[0][2] * P.Z + QSum.Data[0][3] * P.W;
	QV.Y = QSum.Data[1][0] * P.X + QSum.Data[1][1] * P.Y + QSum.Data[1][2] * P.Z + QSum.Data[1][3] * P.W;
	QV.Z = QSum.Data[2][0] * P.X + QSum.Data[2][1] * P.Y + QSum.Data[2][2] * P.Z + QSum.Data[2][3] * P.W;
	QV.W = QSum.Data[3][0] * P.X + QSum.Data[3][1] * P.Y + QSum.Data[3][2] * P.Z + QSum.Data[3][3] * P.W;
	float Cost = P.Dot(QV);

	return {InV1Index, InV2Index, Cost, OptimalPos};
}

uint32 FMeshSimplifier::CollapseEdge(uint32 InUIndex, uint32 InVIndex, const FVector& InOptimalPosition)
{
	SimplificationVertices[InUIndex].bIsActive = false;
	SimplificationVertices[InVIndex].Position = InOptimalPosition;
	SimplificationVertices[InVIndex].Q += SimplificationVertices[InUIndex].Q;

	uint32 TrianglesRemoved = 0;

	for (auto& Tri : Triangles)
	{
		if (Tri.bIsActive)
		{
			bool bHasU = false, bHasV = false;
			for (int j = 0; j < 3; ++j)
			{
				if (Tri.Vertices[j].VertexIndex == InUIndex) bHasU = true;
				if (Tri.Vertices[j].VertexIndex == InVIndex) bHasV = true;
			}

			if (bHasU && bHasV)
			{
				Tri.bIsActive = false;
				TrianglesRemoved++;
			}
			else if (bHasU)
			{
				for (int j = 0; j < 3; ++j)
				{
					if (Tri.Vertices[j].VertexIndex == InUIndex)
					{
						Tri.Vertices[j].VertexIndex = InVIndex;
					}
				}
			}
		}
	}
	return TrianglesRemoved;
}
