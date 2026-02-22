#include "pch.h"
#include "ParticleModuleTypeDataMesh.h"

void UParticleModuleTypeDataMesh::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON StaticMeshJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "StaticMesh", StaticMeshJson, nullptr, false))
		{
			FString StaticMeshPath = StaticMeshJson["StaticMeshPath"].ToString();

			StaticMesh = UResourceManager::GetInstance().Load<UStaticMesh>(StaticMeshPath);
		}
	}
	else
	{
		JSON StaticMeshJSON = JSON::Make(JSON::Class::Object);

		StaticMeshJSON["StaticMeshPath"] = StaticMesh ? StaticMesh->GetFilePath() : "";

		InOutHandle["StaticMesh"] = StaticMeshJSON;
	}
}
