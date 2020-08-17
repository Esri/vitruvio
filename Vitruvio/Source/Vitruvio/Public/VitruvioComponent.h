// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "RuleAttributes.h"
#include "RulePackage.h"
#include "VitruvioModule.h"

#include "CoreMinimal.h"
#include "InitialShape.h"
#include "VitruvioTypes.h"

#if WITH_EDITOR
#include "IDetailGroup.h"
#endif

#include "VitruvioComponent.generated.h"

class FInitialShapeFactory;

struct FInstance
{
	UStaticMesh* Mesh;
	TArray<UMaterialInstanceDynamic*> OverrideMaterials;
	TArray<FTransform> Transforms;
};

struct FConvertedGenerateResult
{
	UStaticMesh* ShapeMesh;
	TArray<FInstance> Instances;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VITRUVIO_API UVitruvioComponent : public UActorComponent
{
	GENERATED_BODY()

	TAtomic<bool> Initialized = false;
	TAtomic<bool> AttributesReady = false;
	TAtomic<bool> LoadingAttributes = false;

	UPROPERTY()
	bool bValidRandomSeed = false;

	bool bNeedsRegenerate = false;
	bool bIsGenerating = false;

public:
	UVitruvioComponent();

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

	FInitialShapeFactory* InitialShapeFactory;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Vitruvio")
	UInitialShape* InitialShape;

	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void Generate();

	virtual void OnRegister() override;

	virtual void OnUnregister() override;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	static FInitialShapeFactory* FindFactory(UVitruvioComponent* VitruvioComponent);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void OnPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
#endif

private:
	TQueue<FGenerateResultDescription> GenerateQueue;

	void LoadDefaultAttributes(bool KeepOldAttributeValues = false);

	void NotifyAttributesChanged();

	void RemoveGeneratedMeshes();

	FConvertedGenerateResult BuildResult(FGenerateResultDescription& GenerateResult,
										 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache);

#if WITH_EDITOR
	FDelegateHandle PropertyChangeDelegate;
#endif
};

class FInitialShapeFactory
{
public:
	virtual ~FInitialShapeFactory() = default;

	virtual UInitialShape* CreateInitialShape(UVitruvioComponent* Component, UInitialShape* OldInitialShape) const = 0;
	virtual bool CanCreateFrom(UVitruvioComponent* Component) const = 0;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, FProperty* Property) = 0;
#endif
};
