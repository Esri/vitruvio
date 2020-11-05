// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "VitruvioActor.h"

#include "VitruvioComponent.h"

AVitruvioActor::AVitruvioActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	VitruvioComponent = CreateDefaultSubobject<UVitruvioComponent>(TEXT("VitruvioComponent"));
	PrimaryActorTick.bCanEverTick = true;
}

void AVitruvioActor::Tick(float DeltaSeconds)
{
	// We do the initialization late here because we need all Components loaded and copy/pasted
	// In PostCreated we can not differentiate between an Actor which has been copy pasted
	// (in which case we would not need to load the initial shape) or spawned normally
	Initialize();
}

bool AVitruvioActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AVitruvioActor::Initialize()
{
	if (!bInitialized)
	{
		VitruvioComponent->Initialize();
		bInitialized = true;
	}
}
