// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "VitruvioActor.h"

#include "VitruvioComponent.h"

AVitruvioActor::AVitruvioActor()
{
	VitruvioComponent = CreateDefaultSubobject<UVitruvioComponent>(TEXT("VitruvioComponent"));
}
