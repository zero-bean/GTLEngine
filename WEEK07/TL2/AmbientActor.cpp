#include"pch.h"
#include "AmbientActor.h"


AAmbientActor::AAmbientActor()
{
}

AAmbientActor::~AAmbientActor()
{
}

UObject* AAmbientActor::Duplicate()
{
	return Super_t::Duplicate();
}

void AAmbientActor::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();
}
