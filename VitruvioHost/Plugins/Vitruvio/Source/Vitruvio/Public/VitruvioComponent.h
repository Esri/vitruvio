// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "RuleAttributes.h"
#include "RulePackage.h"
#include "VitruvioModule.h"

#include "CoreMinimal.h"
#include "InitialShape.h"
#include "VitruvioTypes.h"

#include "VitruvioComponent.generated.h"

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

struct FLoadAttributes
{
	FAttributeMapPtr AttributeMap;
	bool bKeepOldAttributes;
	bool bForceRegenerate;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VITRUVIO_API UVitruvioComponent : public UActorComponent
{
	GENERATED_BODY()

	TAtomic<bool> LoadingAttributes = false;

	UPROPERTY()
	bool bValidRandomSeed = false;

	UPROPERTY()
	bool bAttributesReady = false;

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

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Vitruvio")
	UInitialShape* InitialShape = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void Generate(bool bLoadAttributes = false);

	virtual void PostLoad() override;

	virtual void OnComponentCreated() override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void OnPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	void SetInitialShapeType(const TSubclassOf<UInitialShape>& Type);
#endif

	static TArray<TSubclassOf<UInitialShape>> GetInitialShapesClasses();

private:
	TQueue<FGenerateResultDescription> GenerateQueue;
	TQueue<FLoadAttributes> LoadAttributesQueue;

	FGenerateResult::FTokenPtr GenerateToken;
	FAttributeMapResult::FTokenPtr LoadAttributesInvalidationToken;

	void CalculateRandomSeed();

	void LoadDefaultAttributes(bool KeepOldAttributeValues = false, bool ForceRegenerate = false);

	void NotifyAttributesChanged();

	void RemoveGeneratedMeshes();

	void ProcessGenerateQueue();
	void ProcessLoadAttributesQueue();

	FConvertedGenerateResult BuildResult(FGenerateResultDescription& GenerateResult,
										 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
										 TMap<FString, Vitruvio::FTextureData>& TextureCache);

#if WITH_EDITOR
	FDelegateHandle PropertyChangeDelegate;
#endif
};