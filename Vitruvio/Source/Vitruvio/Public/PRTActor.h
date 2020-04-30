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
	bool AttributesReady = false;
	
	TFuture<TMap<FString, URuleAttribute*>> AttributesFuture;

public:
	APRTActor();

	UPROPERTY(EditAnywhere, DisplayName = "Rule Package", Category="CGA Rules")
	URulePackage* Rpk;

	UPROPERTY(EditAnywhere, Category="CGA Rules")
	int32 RandomSeed;
	
	UPROPERTY(EditAnywhere)
	TMap<FString, URuleAttribute*> Attributes;

	UPROPERTY(EditAnywhere)
	UMaterial* OpaqueParent;

	UPROPERTY(EditAnywhere)
    UMaterial* MaskedParent;

	UPROPERTY(EditAnywhere)
	UMaterial* TranslucentParent;
	
	UPROPERTY(EditAnywhere)
	bool GenerateAutomatically = true;

	UFUNCTION(BlueprintCallable)
	void Regenerate();

protected:
	void BeginPlay() override;
	void LoadDefaultAttributes(UStaticMesh* InitialShape);

public:
	void Tick(float DeltaTime) override;
	
	
#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	bool ShouldTickIfViewportsOnly() const override;
#endif
};
