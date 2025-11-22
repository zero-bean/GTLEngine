#pragma once
#include "Object.h"

class UParticleEmitter;

class UParticleSystem : public UObject
{

public:
	TArray<UParticleEmitter*> Emitters;
};