// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "VitruvioActor.h"

#include "VitruvioComponent.h"

AVitruvioActor::AVitruvioActor()
{
	VitruvioComponent = CreateDefaultSubobject<UVitruvioComponent>(TEXT("VitruvioComponent"));
}
