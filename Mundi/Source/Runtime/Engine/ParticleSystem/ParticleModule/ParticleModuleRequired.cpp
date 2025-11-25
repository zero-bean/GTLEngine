#include "pch.h"
#include "ParticleModuleRequired.h"

void UParticleModuleRequired::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// Material은 경로만 저장/로드
		FString MaterialPath = InOutHandle["Material"].ToString();
		if (!MaterialPath.empty())
		{
			Material = UResourceManager::GetInstance().Load<UMaterial>(MaterialPath);
		}
		EmitterDuration = static_cast<float>(InOutHandle["EmitterDuration"].ToFloat());
		EmitterDelay = static_cast<float>(InOutHandle["EmitterDelay"].ToFloat());
		EmitterLoops = InOutHandle["EmitterLoops"].ToInt();
		bUseLocalSpace = InOutHandle["bUseLocalSpace"].ToBool();
	}
	else
	{
		// Save
		InOutHandle["Material"] = Material ? Material->GetFilePath() : "";
		InOutHandle["EmitterDuration"] = EmitterDuration;
		InOutHandle["EmitterDelay"] = EmitterDelay;
		InOutHandle["EmitterLoops"] = EmitterLoops;
		InOutHandle["bUseLocalSpace"] = bUseLocalSpace;
	}
}
