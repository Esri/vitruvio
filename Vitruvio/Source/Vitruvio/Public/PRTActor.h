// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "RuleAttributes.h"
#include "RulePackage.h"
#include "Materials/Material.h"
#include "PRTActor.generated.h"

UCLASS()
class VITRUVIO_API APRTActor : public AStaticMeshActor
{
	GENERATED_BODY()

	bool Initialized = false;

	bool Regenerated = false;

public:
	APRTActor();

	UPROPERTY(EditAnywhere, DisplayName = "Rule Package")
	URulePackage* Rpk;

	UPROPERTY(EditAnywhere)
	TMap<FString, URuleAttribute*> GenerateAttributes;

	UPROPERTY(EditAnywhere)
	UMaterial* OpaqueParent;

	UPROPERTY(EditAnywhere)
	bool GenerateAutomatically = true;

	UFUNCTION(BlueprintCallable)
	void Regenerate();

protected:
	void BeginPlay() override;

public:
	void Tick(float DeltaTime) override;

#ifdef WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	bool ShouldTickIfViewportsOnly() const override;
#endif
};
