#include "pch.h"
#include "ParticleModuleTypeDataBeam.h"

void UParticleModuleTypeDataBeam::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    auto SerializeVector = [](const bool bLoad, JSON& Handle, const char* Key, FVector& Value)
      {
          if (bLoad)
          {
              FJsonSerializer::ReadVector(Handle, Key, Value, Value, false);
          }
          else
          {
              JSON VecJson = JSON::Make(JSON::Class::Array);
              VecJson.append(Value.X); VecJson.append(Value.Y); VecJson.append(Value.Z);
              Handle[Key] = VecJson;
          }
      };

      if (bInIsLoading)
      {
          int32 BeamMethodInt = static_cast<int32>(BeamMethod);
          int32 BeamTaperMethodInt = static_cast<int32>(BeamTaperMethod);

          FJsonSerializer::ReadInt32(InOutHandle, "BeamMethod", BeamMethodInt, BeamMethodInt, false);
          FJsonSerializer::ReadInt32(InOutHandle, "BeamTaperMethod", BeamTaperMethodInt, BeamTaperMethodInt, false);
          FJsonSerializer::ReadInt32(InOutHandle, "TextureTile", TextureTile, TextureTile, false);
          FJsonSerializer::ReadFloat(InOutHandle, "TextureTileDistance", TextureTileDistance, TextureTileDistance, false);
          FJsonSerializer::ReadInt32(InOutHandle, "Sheets", Sheets, Sheets, false);
          FJsonSerializer::ReadInt32(InOutHandle, "MaxBeamCount", MaxBeamCount, MaxBeamCount, false);
          FJsonSerializer::ReadFloat(InOutHandle, "Speed", Speed, Speed, false);
          FJsonSerializer::ReadFloat(InOutHandle, "BaseWidth", BaseWidth, BaseWidth, false);
          FJsonSerializer::ReadInt32(InOutHandle, "InterpolationPoints", InterpolationPoints, InterpolationPoints, false);
          FJsonSerializer::ReadInt32(InOutHandle, "bAlwaysOn", bAlwaysOn, bAlwaysOn, false);
          FJsonSerializer::ReadInt32(InOutHandle, "UpVectorStepSize", UpVectorStepSize, UpVectorStepSize, false);

          BeamMethod = static_cast<EBeamMethod>(BeamMethodInt);
          BeamTaperMethod = static_cast<EBeamTaperMethod>(BeamTaperMethodInt);

          JSON DistanceJson = InOutHandle["Distance"];
          Distance.Serialize(true, DistanceJson);

          JSON TaperFactorJson = InOutHandle["TaperFactor"];
          TaperFactor.Serialize(true, TaperFactorJson);

          JSON TaperScaleJson = InOutHandle["TaperScale"];
          TaperScale.Serialize(true, TaperScaleJson);

          SerializeVector(true, InOutHandle, "SourcePosition", SourcePosition);
          SerializeVector(true, InOutHandle, "TargetPosition", TargetPosition);
          SerializeVector(true, InOutHandle, "SourceTangent", SourceTangent);
          SerializeVector(true, InOutHandle, "TargetTangent", TargetTangent);
      }
      else
      {
          InOutHandle["BeamMethod"] = static_cast<int32>(BeamMethod);
          InOutHandle["BeamTaperMethod"] = static_cast<int32>(BeamTaperMethod);
          InOutHandle["TextureTile"] = TextureTile;
          InOutHandle["TextureTileDistance"] = TextureTileDistance;
          InOutHandle["Sheets"] = Sheets;
          InOutHandle["MaxBeamCount"] = MaxBeamCount;
          InOutHandle["Speed"] = Speed;
          InOutHandle["BaseWidth"] = BaseWidth;
          InOutHandle["InterpolationPoints"] = InterpolationPoints;
          InOutHandle["bAlwaysOn"] = bAlwaysOn;
          InOutHandle["UpVectorStepSize"] = UpVectorStepSize;

          JSON DistanceJson = JSON::Make(JSON::Class::Object);
          Distance.Serialize(false, DistanceJson);
          InOutHandle["Distance"] = DistanceJson;

          JSON TaperFactorJson = JSON::Make(JSON::Class::Object);
          TaperFactor.Serialize(false, TaperFactorJson);
          InOutHandle["TaperFactor"] = TaperFactorJson;

          JSON TaperScaleJson = JSON::Make(JSON::Class::Object);
          TaperScale.Serialize(false, TaperScaleJson);
          InOutHandle["TaperScale"] = TaperScaleJson;

          SerializeVector(false, InOutHandle, "SourcePosition", SourcePosition);
          SerializeVector(false, InOutHandle, "TargetPosition", TargetPosition);
          SerializeVector(false, InOutHandle, "SourceTangent", SourceTangent);
          SerializeVector(false, InOutHandle, "TargetTangent", TargetTangent);
      }
}