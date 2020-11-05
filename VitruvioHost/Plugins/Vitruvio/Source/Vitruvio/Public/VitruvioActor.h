// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "VitruvioComponent.h"
#include "VitruvioActor.generated.h"

UCLASS()
class VITRUVIO_API AVitruvioActor : public AActor
{
	GENERATED_BODY()

public:
	AVitruvioActor();

	UPROPERTY(VisibleAnywhere, Category = "Vitruvio")
	UVitruvioComponent* VitruvioComponent;

	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	void Initialize();

private:
	bool bInitialized = false;
};
