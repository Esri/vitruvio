// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "VitruvioComponent.h"
#include "VitruvioActor.generated.h"

UCLASS()
class VITRUVIO_API AVitruvioActor : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	AVitruvioActor();

	UPROPERTY(VisibleAnywhere)
	UVitruvioComponent* VitruvioComponent;
};
