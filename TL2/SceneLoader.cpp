#include "pch.h"
#include "SceneLoader.h"

#include <algorithm>
#include <iomanip>

static bool ParsePerspectiveCamera(const JSON& Root, FPerspectiveCameraData& OutCam)
{
    if (!Root.hasKey("PerspectiveCamera"))
        return false;

    const JSON& Cam = Root.at("PerspectiveCamera");

    // 배열형 벡터(Location, Rotation) 파싱 (스칼라 실패 시 무시)
    auto readVec3 = [](JSON arr, FVector& outVec)
        {
            try
            {
                outVec = FVector(
                    (float)arr[0].ToFloat(),
                    (float)arr[1].ToFloat(),
                    (float)arr[2].ToFloat());
            }
            catch (...) {} // 실패 시 기본값 유지
        };

    if (Cam.hasKey("Location"))
        readVec3(Cam.at("Location"), OutCam.Location);
    if (Cam.hasKey("Rotation"))
        readVec3(Cam.at("Rotation"), OutCam.Rotation);

    // 스칼라 또는 [스칼라] 모두 허용
    auto readScalarFlexible = [](const JSON& parent, const char* key, float& outVal)
        {
            if (!parent.hasKey(key)) return;
            const JSON& node = parent.at(key);
            try
            {
                // 배열 형태 시도 (예: "FOV": [60.0])
                outVal = (float)node.at(0).ToFloat();
            }
            catch (...)
            {
                // 스칼라 (예: "FOV": 60.0)
                outVal = (float)node.ToFloat();
            }
        };

    readScalarFlexible(Cam, "FOV", OutCam.FOV);
    readScalarFlexible(Cam, "NearClip", OutCam.NearClip);
    readScalarFlexible(Cam, "FarClip", OutCam.FarClip);

    return true;
}

TArray<FPrimitiveData> FSceneLoader::Load(const FString& FileName, FPerspectiveCameraData* OutCameraData)
{
    std::ifstream file(FileName);
    if (!file.is_open())
    {
		UE_LOG("Scene load failed. Cannot open file: %s", FileName.c_str());
        return {};
    }

    std::stringstream Buffer;
    Buffer << file.rdbuf();
    std::string content = Buffer.str();

    try {
        JSON j = JSON::Load(content);

        // 카메라 먼저 파싱
        if (OutCameraData)
        {
            FPerspectiveCameraData Temp{};
            if (ParsePerspectiveCamera(j, Temp))
            {
                *OutCameraData = Temp;
            }
            else
            {
                // 카메라 블록이 없으면 값을 건드리지 않음
            }
        }

        return Parse(j);
    }
    catch (const std::exception& e) {
		UE_LOG("Scene load failed. JSON parse error: %s", e.what());
        return {};
    }
}

void FSceneLoader::Save(TArray<FPrimitiveData> InPrimitiveData, const FPerspectiveCameraData* InCameraData, const FString& SceneName)
{
    uint32 NextUUID = UObject::PeekNextUUID();

    namespace fs = std::filesystem;
    fs::path outPath(SceneName);
    if (!outPath.has_parent_path())
        outPath = fs::path("Scene") / outPath;
    if (outPath.extension().string() != ".Scene")
        outPath.replace_extension(".Scene");
    std::error_code ec;
    fs::create_directories(outPath.parent_path(), ec);

    auto NormalizePath = [](FString Path) -> FString
        {
            // 절대 경로를 프로젝트 기준 상대 경로로 변환
            fs::path absPath = fs::absolute(Path);
            fs::path currentPath = fs::current_path();

            std::error_code ec;
            fs::path relativePath = fs::relative(absPath, currentPath, ec);

            // 상대 경로 변환 실패 시 원본 경로 사용
            FString result = ec ? Path : relativePath.string();

            // 백슬래시를 슬래시로 변환 (크로스 플랫폼 호환)
            for (auto& ch : result)
            {
                if (ch == '\\') ch = '/';
            }
            return result;
        };

    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(6);

    auto writeVec3 = [&](const char* name, const FVector& v, int indent)
        {
            std::string tabs(indent, ' ');
            oss << tabs << "\"" << name << "\" : [" << v.X << ", " << v.Y << ", " << v.Z << "]";
        };

    oss << "{\n";
    oss << "  \"Version\" : 1,\n";
    oss << "  \"NextUUID\" : " << NextUUID;

    bool bHasCamera = (InCameraData != nullptr);

    if (bHasCamera)
    {
        oss << ",\n";
        oss << "  \"PerspectiveCamera\" : {\n";
        // 순서: FOV, FarClip, Location, NearClip, Rotation (FOV/Clip들은 단일 요소 배열)
        oss << "    \"FOV\" : [" << InCameraData->FOV << "],\n";
        oss << "    \"FarClip\" : [" << InCameraData->FarClip << "],\n";
        writeVec3("Location", InCameraData->Location, 4); oss << ",\n";
        oss << "    \"NearClip\" : [" << InCameraData->NearClip << "],\n";
        writeVec3("Rotation", InCameraData->Rotation, 4); oss << "\n";
        oss << "  }";
    }

    // Primitives 블록
    oss << (bHasCamera ? ",\n" : ",\n"); // 카메라 없더라도 컴마 후 줄바꿈
    oss << "  \"Primitives\" : {\n";
    for (size_t i = 0; i < InPrimitiveData.size(); ++i)
    {
        const FPrimitiveData& Data = InPrimitiveData[i];
        oss << "    \"" << Data.UUID << "\" : {\n";
        // 순서: Location, ObjStaticMeshAsset, Rotation, Scale, Type
        writeVec3("Location", Data.Location, 6); oss << ",\n";

        FString AssetPath = NormalizePath(Data.ObjStaticMeshAsset);
        oss << "      \"ObjStaticMeshAsset\" : " << "\"" << AssetPath << "\",\n";

        writeVec3("Rotation", Data.Rotation, 6); oss << ",\n";
        writeVec3("Scale", Data.Scale, 6); oss << ",\n";
        oss << "      \"Type\" : " << "\"" << Data.Type << "\"\n";
        oss << "    }" << (i + 1 < InPrimitiveData.size() ? "," : "") << "\n";
    }
    oss << "  }\n";
    oss << "}\n";

    const std::string finalPath = outPath.make_preferred().string();
    std::ofstream OutFile(finalPath.c_str(), std::ios::out | std::ios::trunc);
    if (OutFile.is_open())
    {
        OutFile << oss.str();
        OutFile.close();
    }
    else
    {
        UE_LOG("Scene save failed. Cannot open file: %s", finalPath.c_str());
    }
}

// ========================================
// Version 2 API Implementation
// ========================================

void FSceneLoader::SaveV2(const FSceneData& SceneData, const FString& SceneName)
{
    namespace fs = std::filesystem;
    fs::path outPath(SceneName);
    if (!outPath.has_parent_path())
        outPath = fs::path("Scene") / outPath;
    if (outPath.extension().string() != ".Scene")
        outPath.replace_extension(".Scene");
    std::error_code ec;
    fs::create_directories(outPath.parent_path(), ec);

    auto NormalizePath = [](FString Path) -> FString
    {
        // 절대 경로를 프로젝트 기준 상대 경로로 변환
        fs::path absPath = fs::absolute(Path);
        fs::path currentPath = fs::current_path();

        std::error_code ec;
        fs::path relativePath = fs::relative(absPath, currentPath, ec);

        // 상대 경로 변환 실패 시 원본 경로 사용
        FString result = ec ? Path : relativePath.string();

        // 백슬래시를 슬래시로 변환 (크로스 플랫폼 호환)
        for (auto& ch : result)
        {
            if (ch == '\\') ch = '/';
        }
        return result;
    };

    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(6);

    auto writeVec3 = [&](const char* name, const FVector& v, int indent)
    {
        std::string tabs(indent, ' ');
        oss << tabs << "\"" << name << "\" : [" << v.X << ", " << v.Y << ", " << v.Z << "]";
    };

    // Root
    oss << "{\n";
    oss << "  \"Version\" : " << SceneData.Version << ",\n";
    oss << "  \"NextUUID\" : " << SceneData.NextUUID << ",\n";

    // Camera
    oss << "  \"PerspectiveCamera\" : {\n";
    oss << "    \"FOV\" : [" << SceneData.Camera.FOV << "],\n";
    oss << "    \"FarClip\" : [" << SceneData.Camera.FarClip << "],\n";
    writeVec3("Location", SceneData.Camera.Location, 4); oss << ",\n";
    oss << "    \"NearClip\" : [" << SceneData.Camera.NearClip << "],\n";
    writeVec3("Rotation", SceneData.Camera.Rotation, 4); oss << "\n";
    oss << "  },\n";

    // Actors
    oss << "  \"Actors\" : [\n";
    for (size_t i = 0; i < SceneData.Actors.size(); ++i)
    {
        const FActorData& Actor = SceneData.Actors[i];
        oss << "    {\n";
        oss << "      \"UUID\" : " << Actor.UUID << ",\n";
        oss << "      \"Type\" : \"" << Actor.Type << "\",\n";
        oss << "      \"Name\" : \"" << Actor.Name << "\",\n";
        oss << "      \"RootComponentUUID\" : " << Actor.RootComponentUUID << "\n";
        oss << "    }" << (i + 1 < SceneData.Actors.size() ? "," : "") << "\n";
    }
    oss << "  ],\n";

    // Components
    oss << "  \"Components\" : [\n";
    for (size_t i = 0; i < SceneData.Components.size(); ++i)
    {
        const FComponentData& Comp = SceneData.Components[i];
        oss << "    {\n";
        oss << "      \"UUID\" : " << Comp.UUID << ",\n";
        oss << "      \"OwnerActorUUID\" : " << Comp.OwnerActorUUID << ",\n";
        oss << "      \"ParentComponentUUID\" : " << Comp.ParentComponentUUID << ",\n";
        oss << "      \"Type\" : \"" << Comp.Type << "\",\n";
        writeVec3("RelativeLocation", Comp.RelativeLocation, 6); oss << ",\n";
        writeVec3("RelativeRotation", Comp.RelativeRotation, 6); oss << ",\n";
        writeVec3("RelativeScale", Comp.RelativeScale, 6);

        // Type별 속성
        bool bHasTypeSpecificData = false;

        // StaticMeshComponent
        if (Comp.Type.find("StaticMeshComponent") != std::string::npos && !Comp.StaticMesh.empty())
        {
            if (!bHasTypeSpecificData) { oss << ",\n"; bHasTypeSpecificData = true; }
            FString AssetPath = NormalizePath(Comp.StaticMesh);
            oss << "      \"StaticMesh\" : \"" << AssetPath << "\"";

            if (!Comp.Materials.empty())
            {
                oss << ",\n";
                oss << "      \"Materials\" : [";
                for (size_t m = 0; m < Comp.Materials.size(); ++m)
                {
                    oss << "\"" << Comp.Materials[m] << "\"";
                    if (m + 1 < Comp.Materials.size()) oss << ", ";
                }
                oss << "]";

                // Normal Map Texture
                if (!Comp.MaterialNormalMapOverrides.empty() && Comp.MaterialNormalMapOverrides.size() == Comp.Materials.size())
                {
                    oss << ",\n";
                    oss << "      \"MaterialNormalMapOverrides\" : [";
                    for (size_t m = 0; m < Comp.MaterialNormalMapOverrides.size(); ++m)
                    {
                        oss << "\"" << Comp.MaterialNormalMapOverrides[m] << "\"";
                        if (m + 1 < Comp.MaterialNormalMapOverrides.size()) oss << ", ";
                    }
                    oss << "]";
                }
            }
        }

        // DecalComponent, BillboardComponent, DecalSpotLightComponent
        if ((Comp.Type.find("DecalComponent") != std::string::npos ||
             Comp.Type.find("BillboardComponent") != std::string::npos ||
             Comp.Type.find("DecalSpotLightComponent") != std::string::npos) &&
            !Comp.TexturePath.empty())
        {
            if (!bHasTypeSpecificData) { oss << ",\n"; bHasTypeSpecificData = true; }
            else { oss << ",\n"; }
            FString TexturePath = NormalizePath(Comp.TexturePath);
            oss << "      \"TexturePath\" : \"" << TexturePath << "\"";
        }
        if (Comp.Type.find("PointLightComponent") != std::string::npos)
        {
            if (!bHasTypeSpecificData) { oss << ",\n"; bHasTypeSpecificData = true; }
            else { oss << ",\n"; }

            const FLightComponentProperty& LP = Comp.LightProperty;
            const FPointLightProperty& FB = Comp.PointLightProperty; // FComponentData 내부에 있다고 가정

            oss << "      \"PointLightData\" : {\n";
            oss << "        \"Intensity\" : " << LP.Intensity << ",\n";
            oss << "        \"Temperature\" : " << LP.Temperature << ",\n";            
            oss << "        \"Radius\" : " << FB.Radius << ",\n";
            oss << "        \"RadiusFallOff\" : " << FB.RadiusFallOff << ",\n";
            oss << "        \"Segments\" : " << FB.Segments << ",\n";
            oss << "        \"FinalColor\" : [" << LP.FinalColor.R << ", " << LP.FinalColor.G << ", " << LP.FinalColor.B << ", " << LP.FinalColor.A << "],\n";
            oss << "        \"TintColor\" : [" << LP.TintColor.R << ", " << LP.TintColor.G << ", " << LP.TintColor.B << ", " << LP.TintColor.A << "],\n";
            oss << "        \"TempColor\" : [" << LP.TempColor.R << ", " << LP.TempColor.G << ", " << LP.TempColor.B << ", " << LP.TempColor.A << "],\n";
            oss << "        \"EnableDebugLine\" : " << (LP.bEnableDebugLine ? "true" : "false") << "\n";
            oss << "      }";
        }
        if (Comp.Type.find("SpotLightComponent") != std::string::npos)
        {
            if (!bHasTypeSpecificData) { oss << ",\n"; bHasTypeSpecificData = true; }
            else { oss << ",\n"; }

            const FLightComponentProperty& LP = Comp.LightProperty;
            const FSpotLightProperty& FB = Comp.SpotLightProperty; // FComponentData 내부에 있다고 가정

            oss << "      \"SpotLightData\" : {\n";
            oss << "        \"Intensity\" : " << LP.Intensity << ",\n";
            oss << "        \"Temperature\" : " << LP.Temperature << ",\n";            
            oss << "        \"InnnerConeAngle\" : " << FB.InnnerConeAngle << ",\n";
            oss << "        \"OuterConeAngle\" : " << FB.OuterConeAngle << ",\n";
            oss << "        \"InAndOutSmooth\" : " << FB.InAndOutSmooth << ",\n";
            oss << "        \"Segments\" : " << FB.CircleSegments << ",\n";
            oss << "        \"Direction\" : [" << FB.Direction.X << ", " << FB.Direction.Y << ", " << FB.Direction.Z << ", "<< FB.Direction.W << "],\n";
            oss << "        \"FinalColor\" : [" << LP.FinalColor.R << ", " << LP.FinalColor.G << ", " << LP.FinalColor.B << ", " << LP.FinalColor.A << "],\n";
            oss << "        \"TintColor\" : [" << LP.TintColor.R << ", " << LP.TintColor.G << ", " << LP.TintColor.B << ", " << LP.TintColor.A << "],\n";
            oss << "        \"TempColor\" : [" << LP.TempColor.R << ", " << LP.TempColor.G << ", " << LP.TempColor.B << ", " << LP.TempColor.A << "],\n";
            oss << "        \"EnableDebugLine\" : " << (LP.bEnableDebugLine ? "true" : "false") << "\n";
            oss << "      }";
        }
        if (Comp.Type.find("DirectionalLightComponent") != std::string::npos)
        {
            if (!bHasTypeSpecificData) { oss << ",\n"; bHasTypeSpecificData = true; }
            else { oss << ",\n"; }

            const FLightComponentProperty& LP = Comp.LightProperty;
            const FDirectionalLightProperty& FB = Comp.DirectionalLightProperty; // FComponentData 내부에 있다고 가정

            oss << "      \"DirectionalLightData\" : {\n";
            oss << "        \"Intensity\" : " << LP.Intensity << ",\n";
            oss << "        \"Temperature\" : " << LP.Temperature << ",\n";
            oss << "        \"Direction\" : [" << FB.Direction.X << ", " << FB.Direction.Y << ", " << FB.Direction.Z << "],\n";
            oss << "        \"EnableSpecular\" : " << FB.bEnableSpecular << ",\n";
            oss << "        \"FinalColor\" : [" << LP.FinalColor.R << ", " << LP.FinalColor.G << ", " << LP.FinalColor.B << ", " << LP.FinalColor.A << "],\n";
            oss << "        \"TintColor\" : [" << LP.TintColor.R << ", " << LP.TintColor.G << ", " << LP.TintColor.B << ", " << LP.TintColor.A << "],\n";
            oss << "        \"TempColor\" : [" << LP.TempColor.R << ", " << LP.TempColor.G << ", " << LP.TempColor.B << ", " << LP.TempColor.A << "],\n";
            oss << "        \"EnableDebugLine\" : " << (LP.bEnableDebugLine ? "true" : "false") << "\n";
            oss << "      }";
        }
        if (Comp.Type.find("ProjectileMovementComponent") != std::string::npos)
        {
            if (!bHasTypeSpecificData) { oss << ",\n"; bHasTypeSpecificData = true; }
            else { oss << ",\n"; }

            const FProjectileMovementProperty& PM = Comp.ProjectileMovementProperty;

            oss << "      \"ProjectileMovementData\" : {\n";
            oss << "        \"InitialSpeed\" : " << PM.InitialSpeed << ",\n";
            oss << "        \"MaxSpeed\" : " << PM.MaxSpeed << ",\n";
            oss << "        \"GravityScale\" : " << PM.GravityScale << "\n";
            oss << "      }";
        }

        // RotationMovementComponent
        if (Comp.Type.find("RotationMovementComponent") != std::string::npos)
        {
            if (!bHasTypeSpecificData) { oss << ",\n"; bHasTypeSpecificData = true; }
            else { oss << ",\n"; }

            const FRotationMovementProperty& RM = Comp.RotationMovementProperty;

            oss << "      \"RotationMovementData\" : {\n";
            oss << "        \"RotationRate\" : [" << RM.RotationRate.X << ", " << RM.RotationRate.Y << ", " << RM.RotationRate.Z << "],\n";
            oss << "        \"PivotTranslation\" : [" << RM.PivotTranslation.X << ", " << RM.PivotTranslation.Y << ", " << RM.PivotTranslation.Z << "],\n";
            oss << "        \"bRotationInLocalSpace\" : " << (RM.bRotationInLocalSpace ? "true" : "false") << "\n";
            oss << "      }";
        }

        oss << "\n";
        oss << "    }" << (i + 1 < SceneData.Components.size() ? "," : "") << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";

    const std::string finalPath = outPath.make_preferred().string();
    std::ofstream OutFile(finalPath.c_str(), std::ios::out | std::ios::trunc);
    if (OutFile.is_open())
    {
        OutFile << oss.str();
        OutFile.close();
    }
    else
    {
        UE_LOG("Scene save failed. Cannot open file: %s", finalPath.c_str());
    }
}

FSceneData FSceneLoader::LoadV2(const FString& FileName)
{
    FSceneData Result;

    std::ifstream file(FileName);
    if (!file.is_open())
    {
        UE_LOG("Scene load failed. Cannot open file: %s", FileName.c_str());
        return Result;
    }

    std::stringstream Buffer;
    Buffer << file.rdbuf();
    std::string content = Buffer.str();

    try {
        JSON j = JSON::Load(content);
        Result = ParseV2(j);
    }
    catch (const std::exception& e) {
        UE_LOG("Scene load failed. JSON parse error: %s", e.what());
    }

    return Result;
}

FSceneData FSceneLoader::ParseV2(const JSON& Json)
{
    FSceneData Data;

    // Version
    if (Json.hasKey("Version"))
        Data.Version = static_cast<uint32>(Json.at("Version").ToInt());

    // NextUUID
    if (Json.hasKey("NextUUID"))
        Data.NextUUID = static_cast<uint32>(Json.at("NextUUID").ToInt());

    // Camera
    if (Json.hasKey("PerspectiveCamera"))
    {
        ParsePerspectiveCamera(Json, Data.Camera);
    }

    // Actors
    if (Json.hasKey("Actors"))
    {
        const JSON& ActorsJson = Json.at("Actors");
        for (size_t i = 0; i < ActorsJson.size(); ++i)
        {
            const JSON& ActorJson = ActorsJson.at(i);
            FActorData Actor;

            if (ActorJson.hasKey("UUID"))
                Actor.UUID = static_cast<uint32>(ActorJson.at("UUID").ToInt());
            if (ActorJson.hasKey("Type"))
                Actor.Type = ActorJson.at("Type").ToString();
            if (ActorJson.hasKey("Name"))
                Actor.Name = ActorJson.at("Name").ToString();
            if (ActorJson.hasKey("RootComponentUUID"))
                Actor.RootComponentUUID = static_cast<uint32>(ActorJson.at("RootComponentUUID").ToInt());

            Data.Actors.push_back(Actor);
        }
    }

    // Components
    if (Json.hasKey("Components"))
    {
        const JSON& CompsJson = Json.at("Components");
        for (size_t i = 0; i < CompsJson.size(); ++i)
        {
            const JSON& CompJson = CompsJson.at(i);
            FComponentData Comp;

            if (CompJson.hasKey("UUID"))
                Comp.UUID = static_cast<uint32>(CompJson.at("UUID").ToInt());
            if (CompJson.hasKey("OwnerActorUUID"))
                Comp.OwnerActorUUID = static_cast<uint32>(CompJson.at("OwnerActorUUID").ToInt());
            if (CompJson.hasKey("ParentComponentUUID"))
                Comp.ParentComponentUUID = static_cast<uint32>(CompJson.at("ParentComponentUUID").ToInt());
            if (CompJson.hasKey("Type"))
                Comp.Type = CompJson.at("Type").ToString();

            // Transform
            if (CompJson.hasKey("RelativeLocation"))
            {
                auto loc = CompJson.at("RelativeLocation");
                Comp.RelativeLocation = FVector(
                    (float)loc[0].ToFloat(),
                    (float)loc[1].ToFloat(),
                    (float)loc[2].ToFloat()
                );
            }

            if (CompJson.hasKey("RelativeRotation"))
            {
                auto rot = CompJson.at("RelativeRotation");
                Comp.RelativeRotation = FVector(
                    (float)rot[0].ToFloat(),
                    (float)rot[1].ToFloat(),
                    (float)rot[2].ToFloat()
                );
            }

            if (CompJson.hasKey("RelativeScale"))
            {
                auto scale = CompJson.at("RelativeScale");
                Comp.RelativeScale = FVector(
                    (float)scale[0].ToFloat(),
                    (float)scale[1].ToFloat(),
                    (float)scale[2].ToFloat()
                );
            }

            // Type별 속성
            if (CompJson.hasKey("StaticMesh"))
                Comp.StaticMesh = CompJson.at("StaticMesh").ToString();

            if (CompJson.hasKey("Materials"))
            {
                const JSON& matsJson = CompJson.at("Materials");
                for (size_t m = 0; m < matsJson.size(); ++m)
                {
                    Comp.Materials.push_back(matsJson.at(m).ToString());
                }
            }

            // Normal Map Texture
            if (CompJson.hasKey("MaterialNormalMapOverrides"))
            {
                const JSON& normsJson = CompJson.at("MaterialNormalMapOverrides");
                for (size_t m = 0; m < normsJson.size(); ++m)
                {
                    Comp.MaterialNormalMapOverrides.push_back(normsJson.at(m).ToString());
                }
            }

            if (CompJson.hasKey("TexturePath"))
                Comp.TexturePath = CompJson.at("TexturePath").ToString();

            // 🔥 PointLightComponent (FPointLightProperty)
            if (Comp.Type.find("PointLightComponent") != std::string::npos &&
                CompJson.hasKey("PointLightData"))
            {
                const JSON& PointDataJson = CompJson.at("PointLightData");

                if (PointDataJson.hasKey("Intensity"))
                    Comp.LightProperty.Intensity = (float)PointDataJson.at("Intensity").ToFloat();

                if (PointDataJson.hasKey("Temperature"))
                    Comp.LightProperty.Temperature = (float)PointDataJson.at("Temperature").ToFloat();

                if (PointDataJson.hasKey("EnableDebugLine"))
                    Comp.LightProperty.bEnableDebugLine = PointDataJson.at("EnableDebugLine").ToBool();

                if (PointDataJson.hasKey("FinalColor"))
                {
                    auto FinalColorJson = PointDataJson.at("FinalColor");
                    if (FinalColorJson.size() >= 4)
                    {
                        Comp.LightProperty.FinalColor = FLinearColor(
                            (float)FinalColorJson[0].ToFloat(),
                            (float)FinalColorJson[1].ToFloat(),
                            (float)FinalColorJson[2].ToFloat(),
                            (float)FinalColorJson[3].ToFloat()
                        );
                    }
                }

                if (PointDataJson.hasKey("TintColor"))
                {
                    auto TintColorJson = PointDataJson.at("TintColor");
                    if (TintColorJson.size() >= 4)
                    {
                        Comp.LightProperty.TintColor = FLinearColor(
                            (float)TintColorJson[0].ToFloat(),
                            (float)TintColorJson[1].ToFloat(),
                            (float)TintColorJson[2].ToFloat(),
                            (float)TintColorJson[3].ToFloat()
                        );
                    }
                }

                if (PointDataJson.hasKey("TempColor"))
                {
                    auto TempColorJson = PointDataJson.at("TempColor");
                    if (TempColorJson.size() >= 4)
                    {
                        Comp.LightProperty.FinalColor = FLinearColor(
                            (float)TempColorJson[0].ToFloat(),
                            (float)TempColorJson[1].ToFloat(),
                            (float)TempColorJson[2].ToFloat(),
                            (float)TempColorJson[3].ToFloat()
                        );
                    }
                }

                if (PointDataJson.hasKey("Radius"))
                    Comp.PointLightProperty.Radius = (float)PointDataJson.at("Radius").ToFloat();

                if (PointDataJson.hasKey("RadiusFallOff"))
                    Comp.PointLightProperty.RadiusFallOff = (float)PointDataJson.at("RadiusFallOff").ToFloat();

                if (PointDataJson.hasKey("RadiusFallOff"))
                    Comp.PointLightProperty.RadiusFallOff = (float)PointDataJson.at("RadiusFallOff").ToFloat();

                if (PointDataJson.hasKey("Segments"))
                    Comp.PointLightProperty.Segments = (int)PointDataJson.at("Segments").ToInt();
            }

            // 🔥 PointLightComponent (FPointLightProperty)
            if (Comp.Type.find("DirectionalLightComponent") != std::string::npos &&
                CompJson.hasKey("DirectionalLightData"))
            {
                const JSON& DataJson = CompJson.at("DirectionalLightData");

                if (DataJson.hasKey("Intensity"))
                    Comp.LightProperty.Intensity = (float)DataJson.at("Intensity").ToFloat();

                if (DataJson.hasKey("Temperature"))
                    Comp.LightProperty.Temperature = (float)DataJson.at("Temperature").ToFloat();

                if (DataJson.hasKey("EnableDebugLine"))
                    Comp.LightProperty.bEnableDebugLine = DataJson.at("EnableDebugLine").ToBool();

                if (DataJson.hasKey("FinalColor"))
                {
                    auto ColorJson = DataJson.at("FinalColor");
                    if (ColorJson.size() >= 4)
                    {
                        Comp.LightProperty.FinalColor = FLinearColor(
                            (float)ColorJson[0].ToFloat(), (float)ColorJson[1].ToFloat(),
                            (float)ColorJson[2].ToFloat(), (float)ColorJson[3].ToFloat());
                    }
                }

                if (DataJson.hasKey("TintColor"))
                {
                    auto ColorJson = DataJson.at("TintColor");
                    if (ColorJson.size() >= 4)
                    {
                        Comp.LightProperty.TintColor = FLinearColor(
                            (float)ColorJson[0].ToFloat(), (float)ColorJson[1].ToFloat(),
                            (float)ColorJson[2].ToFloat(), (float)ColorJson[3].ToFloat());
                    }
                }

                if (DataJson.hasKey("TempColor"))
                {
                    auto ColorJson = DataJson.at("TempColor");
                    if (ColorJson.size() >= 4)
                    {
                        Comp.LightProperty.TempColor = FLinearColor(
                            (float)ColorJson[0].ToFloat(), (float)ColorJson[1].ToFloat(),
                            (float)ColorJson[2].ToFloat(), (float)ColorJson[3].ToFloat());
                    }
                }

                if (DataJson.hasKey("EnableSpecular"))
                    Comp.DirectionalLightProperty.bEnableSpecular = DataJson.at("EnableSpecular").ToInt();

                if (DataJson.hasKey("Direction"))
                {
                    auto DirectionJson = DataJson.at("Direction");
                    if (DirectionJson.size() >= 3)
                    {
                        Comp.DirectionalLightProperty.Direction = FVector(
                            (float)DirectionJson[0].ToFloat(),
                            (float)DirectionJson[1].ToFloat(),
                            (float)DirectionJson[2].ToFloat()
                        );
                    }
                }                
            }

            if (Comp.Type.find("SpotLightComponent") != std::string::npos &&
                CompJson.hasKey("SpotLightData"))
            {
                const JSON& PointDataJson = CompJson.at("SpotLightData");

                if (PointDataJson.hasKey("Intensity"))
                    Comp.LightProperty.Intensity = (float)PointDataJson.at("Intensity").ToFloat();

                if (PointDataJson.hasKey("Temperature"))
                    Comp.LightProperty.Temperature = (float)PointDataJson.at("Temperature").ToFloat();

                if (PointDataJson.hasKey("EnableDebugLine"))
                    Comp.LightProperty.bEnableDebugLine = PointDataJson.at("EnableDebugLine").ToBool();

                if (PointDataJson.hasKey("FinalColor"))
                {
                    auto FinalColorJson = PointDataJson.at("FinalColor");
                    if (FinalColorJson.size() >= 4)
                    {
                        Comp.LightProperty.FinalColor = FLinearColor(
                            (float)FinalColorJson[0].ToFloat(),
                            (float)FinalColorJson[1].ToFloat(),
                            (float)FinalColorJson[2].ToFloat(),
                            (float)FinalColorJson[3].ToFloat()
                        );
                    }
                }

                if (PointDataJson.hasKey("TintColor"))
                {
                    auto TintColorJson = PointDataJson.at("TintColor");
                    if (TintColorJson.size() >= 4)
                    {
                        Comp.LightProperty.TintColor = FLinearColor(
                            (float)TintColorJson[0].ToFloat(),
                            (float)TintColorJson[1].ToFloat(),
                            (float)TintColorJson[2].ToFloat(),
                            (float)TintColorJson[3].ToFloat()
                        );
                    }
                }

                if (PointDataJson.hasKey("TempColor"))
                {
                    auto TempColorJson = PointDataJson.at("TempColor");
                    if (TempColorJson.size() >= 4)
                    {
                        Comp.LightProperty.FinalColor = FLinearColor(
                            (float)TempColorJson[0].ToFloat(),
                            (float)TempColorJson[1].ToFloat(),
                            (float)TempColorJson[2].ToFloat(),
                            (float)TempColorJson[3].ToFloat()
                        );
                    }
                }

                if (PointDataJson.hasKey("InnnerConeAngle"))
                    Comp.SpotLightProperty.InnnerConeAngle = (float)PointDataJson.at("InnnerConeAngle").ToFloat();

                if (PointDataJson.hasKey("OuterConeAngle"))
                    Comp.SpotLightProperty.OuterConeAngle = (float)PointDataJson.at("OuterConeAngle").ToFloat();

                if (PointDataJson.hasKey("InAndOutSmooth"))
                    Comp.SpotLightProperty.InAndOutSmooth = (float)PointDataJson.at("InAndOutSmooth").ToFloat();

                if (PointDataJson.hasKey("Segments"))
                    Comp.SpotLightProperty.CircleSegments = (int)PointDataJson.at("Segments").ToInt();
            }
            
            if (Comp.Type.find("ProjectileMovementComponent") != std::string::npos &&
                CompJson.hasKey("ProjectileMovementData"))
            {
                const JSON& PMJson = CompJson.at("ProjectileMovementData");
                if (PMJson.hasKey("InitialSpeed"))
                    Comp.ProjectileMovementProperty.InitialSpeed = (float)PMJson.at("InitialSpeed").ToFloat();
                if (PMJson.hasKey("MaxSpeed"))
                    Comp.ProjectileMovementProperty.MaxSpeed = (float)PMJson.at("MaxSpeed").ToFloat();
                if (PMJson.hasKey("GravityScale"))
                    Comp.ProjectileMovementProperty.GravityScale = (float)PMJson.at("GravityScale").ToFloat();
            }

            // RotationMovementComponent
            if (Comp.Type.find("RotationMovementComponent") != std::string::npos &&
                CompJson.hasKey("RotationMovementData"))
            {
                const JSON& RMJson = CompJson.at("RotationMovementData");

                if (RMJson.hasKey("RotationRate"))
                {
                    auto arr = RMJson.at("RotationRate");
                    Comp.RotationMovementProperty.RotationRate = FVector(
                        (float)arr[0].ToFloat(),
                        (float)arr[1].ToFloat(),
                        (float)arr[2].ToFloat()
                    );
                }
                if (RMJson.hasKey("PivotTranslation"))
                {
                    auto arr = RMJson.at("PivotTranslation");
                    Comp.RotationMovementProperty.PivotTranslation = FVector(
                        (float)arr[0].ToFloat(),
                        (float)arr[1].ToFloat(),
                        (float)arr[2].ToFloat()
                    );
                }
                if (RMJson.hasKey("bRotationInLocalSpace"))
                    Comp.RotationMovementProperty.bRotationInLocalSpace = RMJson.at("bRotationInLocalSpace").ToBool();
            }
            Data.Components.push_back(Comp);
        }
    }

    return Data;
}

// ─────────────────────────────────────────────
// NextUUID 메타만 읽어오는 간단한 헬퍼
// 저장 포맷상 "NextUUID"는 "마지막으로 사용된 UUID"이므로,
// 호출 측에서 +1 해서 SetNextUUID 해야 함
// ─────────────────────────────────────────────
bool FSceneLoader::TryReadNextUUID(const FString& FilePath, uint32& OutNextUUID)
{
    std::ifstream file(FilePath);
    if (!file.is_open())
    {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    try
    {
        JSON j = JSON::Load(content);
        if (j.hasKey("NextUUID"))
        {
            // 정수 파서가 없으면 ToFloat로 받아서 캐스팅
			OutNextUUID = static_cast<uint32>(j.at("NextUUID").ToInt());
            return true;
        }
    }
    catch (...)
    {
        // 무시하고 false 반환
    }
    return false;
}

TArray<FPrimitiveData> FSceneLoader::Parse(const JSON& Json)
{
    TArray<FPrimitiveData> Primitives;

    if (!Json.hasKey("Primitives"))
    {
        std::cerr << "Primitives 섹션이 존재하지 않습니다." << std::endl;
        return Primitives;
    }

    auto PrimitivesJson = Json.at("Primitives");
    for (auto& kv : PrimitivesJson.ObjectRange())
    {
        // kv.first: 키(문자열), kv.second: 값(JSON 객체)
        const std::string& key = kv.first;
        const JSON& value = kv.second;

        FPrimitiveData data;

        // 키를 UUID로 파싱 (숫자가 아니면 0 유지)
        try
        {
            // 공백 제거 후 파싱
            std::string trimmed = key;
            trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
            data.UUID = static_cast<uint32>(std::stoul(trimmed));
        }
        catch (...)
        {
            data.UUID = 0; // 레거시 호환: 숫자 키가 아니면 0
        }

        auto loc = value.at("Location");
        data.Location = FVector(
            (float)loc[0].ToFloat(),
            (float)loc[1].ToFloat(),
            (float)loc[2].ToFloat()
        );

        auto rot = value.at("Rotation");
        data.Rotation = FVector(
            (float)rot[0].ToFloat(),
            (float)rot[1].ToFloat(),
            (float)rot[2].ToFloat()
        );

        auto scale = value.at("Scale");
        data.Scale = FVector(
            (float)scale[0].ToFloat(),
            (float)scale[1].ToFloat(),
            (float)scale[2].ToFloat()
        );

        if (value.hasKey("ObjStaticMeshAsset"))
        {
            data.ObjStaticMeshAsset = value.at("ObjStaticMeshAsset").ToString();
        }
        else
        {
            data.ObjStaticMeshAsset = "";
        }

        data.Type = value.at("Type").ToString();

        Primitives.push_back(data);
    }

    return Primitives;
}
