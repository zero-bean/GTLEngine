#include "pch.h"
#include "ParticleModuleRequired.h"

UParticleModuleRequired::UParticleModuleRequired()
{
	bUpdate = false;
	bSpawn = false;
}

void UParticleModuleRequired::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// Material 직렬화 데이터 로드
		JSON MaterialJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "MaterialData", MaterialJson, nullptr, false))
		{
			// Shader 경로 로드
			FString ShaderPath = MaterialJson["ShaderPath"].ToString();

			// MaterialInfo 로드
			FString DiffuseTexture = MaterialJson["DiffuseTexture"].ToString();
			FString NormalTexture = MaterialJson["NormalTexture"].ToString();
			FString SpecularTexture = MaterialJson["SpecularTexture"].ToString();

			// Material 재구성
			if (!ShaderPath.empty())
			{
				UMaterial* NewMaterial = NewObject<UMaterial>();
				UShader* Shader = UResourceManager::GetInstance().Load<UShader>(ShaderPath);
				if (Shader)
				{
					NewMaterial->SetShader(Shader);
				}

				// MaterialInfo 설정
				FMaterialInfo MatInfo;
				MatInfo.MaterialName = "ParticleSpriteMaterial";
				MatInfo.DiffuseTextureFileName = DiffuseTexture;
				MatInfo.NormalTextureFileName = NormalTexture;
				MatInfo.SpecularTextureFileName = SpecularTexture;
				NewMaterial->SetMaterialInfo(MatInfo);

				// 텍스처 로드
				NewMaterial->ResolveTextures();

				Material = NewMaterial;
			}
		}

		// 다른 필드들은 키 체크 없이 로드
		if (InOutHandle.hasKey("EmitterDuration"))
			EmitterDuration = static_cast<float>(InOutHandle["EmitterDuration"].ToFloat());
		if (InOutHandle.hasKey("EmitterDelay"))
			EmitterDelay = static_cast<float>(InOutHandle["EmitterDelay"].ToFloat());
		if (InOutHandle.hasKey("EmitterLoops"))
			EmitterLoops = InOutHandle["EmitterLoops"].ToInt();
		if (InOutHandle.hasKey("bUseLocalSpace"))
			bUseLocalSpace = InOutHandle["bUseLocalSpace"].ToBool();
		if (InOutHandle.hasKey("MaxActiveParticles"))
			MaxActiveParticles = InOutHandle["MaxActiveParticles"].ToInt();
	}
	else
	{
		// Save - Material 전체 직렬화
		if (Material)
		{
			JSON MaterialJson = JSON::Make(JSON::Class::Object);

			// Shader 경로 저장
			UShader* Shader = Material->GetShader();
			MaterialJson["ShaderPath"] = Shader ? Shader->GetFilePath() : "";

			// MaterialInfo에서 텍스처 경로 저장
			const FMaterialInfo& MatInfo = Material->GetMaterialInfo();
			MaterialJson["DiffuseTexture"] = MatInfo.DiffuseTextureFileName;
			MaterialJson["NormalTexture"] = MatInfo.NormalTextureFileName;
			MaterialJson["SpecularTexture"] = MatInfo.SpecularTextureFileName;

			InOutHandle["MaterialData"] = MaterialJson;
		}
		else
		{
			// Material이 없으면 빈 객체 저장
			InOutHandle["MaterialData"] = JSON::Make(JSON::Class::Object);
		}

		InOutHandle["EmitterDuration"] = EmitterDuration;
		InOutHandle["EmitterDelay"] = EmitterDelay;
		InOutHandle["EmitterLoops"] = EmitterLoops;
		InOutHandle["bUseLocalSpace"] = bUseLocalSpace;
		InOutHandle["MaxActiveParticles"] = MaxActiveParticles;

	}
}
