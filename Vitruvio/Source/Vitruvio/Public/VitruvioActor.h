// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "RuleAttributes.h"
#include "RulePackage.h"
#include "VitruvioModule.h"

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/Material.h"
#include "VitruvioActor.generated.h"

UCLASS()
class VITRUVIO_API AVitruvioActor : public AStaticMeshActor
{
	GENERATED_BODY()

	TAtomic<bool> Initialized = false;
	TAtomic<bool> AttributesReady = false;

	UPROPERTY()
	bool bValidRandomSeed = false;

	bool bNeedsRegenerate = false;
	bool bIsGenerating = false;

public:
	AVitruvioActor();

	/** CityEngine Rule Package used for generation. */
	UPROPERTY(EditAnywhere, DisplayName = "Rule Package", Category = "Vitruvio")
	URulePackage* Rpk;

	/** Random seed used for generation. */
	UPROPERTY(EditAnywhere, DisplayName = "Random Seed", Category = "Vitruvio")
	int32 RandomSeed;

	/** Automatically generate after changing attributes or properties. */
	UPROPERTY(EditAnywhere, DisplayName = "Generate Automatically", Category = "Vitruvio")
	bool GenerateAutomatically = true;

	/** Automatically hide initial shape (i.e. this actor's static mesh) after generation. */
	UPROPERTY(EditAnywhere, DisplayName = "Hide after Generation", Category = "Vitruvio")
	bool HideAfterGeneration = false;

	/** Rule attributes used for generation. */
	UPROPERTY(EditAnywhere, DisplayName = "Attributes", Category = "Vitruvio")
	TMap<FString, URuleAttribute*> Attributes;

	/** Default parent material for opaque geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Opaque Parent", Category = "Vitruvio Default Materials")
	UMaterial* OpaqueParent;

	/** Default parent material for masked geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Masked Parent", Category = "Vitruvio Default Materials")
	UMaterial* MaskedParent;

	/** Default parent material for translucent geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Translucent Parent", Category = "Vitruvio Default Materials")
	UMaterial* TranslucentParent;

	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void Generate();

protected:
	void BeginPlay() override;

public:
	void Tick(float DeltaTime) override;

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	void LoadDefaultAttributes(UStaticMesh* InitialShape);
};
