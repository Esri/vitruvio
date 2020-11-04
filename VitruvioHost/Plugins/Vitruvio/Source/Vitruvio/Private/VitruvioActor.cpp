// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "VitruvioActor.h"

#include "VitruvioComponent.h"

AVitruvioActor::AVitruvioActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	VitruvioComponent = CreateDefaultSubobject<UVitruvioComponent>(TEXT("VitruvioComponent"));
}

void AVitruvioActor::PostEditImport()
{
	VitruvioComponent->Generate();
}
