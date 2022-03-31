/* Copyright 2021 Esri
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
	FString Name;
	TSharedPtr<FVitruvioMesh> InstanceMesh;
	TArray<UMaterialInstanceDynamic*> OverrideMaterials;
	TArray<FTransform> Transforms;
};

struct FConvertedGenerateResult
{
	TSharedPtr<FVitruvioMesh> ShapeMesh;
	TArray<FInstance> Instances;
	TMap<FString, FReport> Reports;
};

struct FAttributesEvaluation
{
	FAttributeMapPtr AttributeMap;
	bool bForceRegenerate;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VITRUVIO_API UVitruvioComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY()
	bool bValidRandomSeed = false;

	UPROPERTY()
	bool bAttributesReady = false;

	bool bIsGenerating = false;

	bool bNotifyAttributeChange = false;

public:
	UVitruvioComponent();

	/** Automatically generate after changing attributes or properties. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "Generate Automatically", Category = "Vitruvio")
	bool GenerateAutomatically = true;

	/** Automatically hide initial shape after generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "Hide Initial Shape after Generation", Category = "Vitruvio")
	bool HideAfterGeneration = false;

	/** Default parent material for opaque geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Opaque Parent", Category = "Vitruvio Default Materials")
	UMaterial* OpaqueParent;

	/** Default parent material for masked geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Masked Parent", Category = "Vitruvio Default Materials")
	UMaterial* MaskedParent;

	/** Default parent material for translucent geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Translucent Parent", Category = "Vitruvio Default Materials")
	UMaterial* TranslucentParent;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Vitruvio", Transient, DuplicateTransient, TextExportTransient)
	UInitialShape* InitialShape = nullptr;

	UPROPERTY(EditAnywhere, Category = "Vitruvio", meta = (DisplayName = "Generate Collision Mesh"))
	bool GenerateCollision = true;

	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void Generate();

	/** Returns true if the component has valid input data (initial shape and RPK). */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool HasValidInputData() const;

	/** Returns true if the component is ready to generate, meaning it HasValidInputData and the attributes are loaded. */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool IsReadyToGenerate() const;

	/** Sets the given Rpk and possibly invalidates already loaded attributes. */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void SetRpk(URulePackage* RulePackage);

	/**
	 * Sets string attributes used for generation.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param Name The name of the attribute to set.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void SetStringAttribute(const FString& Name, const FString& Value, bool bAddIfNonExisting = false);

	/**
	 * Access String attribute values used for generation.
	 *
	 * @param Name The name of the float attribute.
	 * @param OutValue Set to the attributes value if it exists or to an empty String otherwise.
	 * @returns True if the String attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetStringAttribute(const FString& Name, FString& OutValue) const;

	/**
	 * Sets bool attributes used for generation.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void SetBoolAttribute(const FString& Name, bool Value, bool bAddIfNonExisting = false);

	/**
	 * Access bool attribute values used for generation.
	 *
	 * @param Name The name of the bool attribute.
	 * @param OutValue Set to the attributes value if it exists or to false otherwise.
	 * @returns true if the float attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetBoolAttribute(const FString& Name, bool& OutValue) const;

	/**
	 * Sets float attributes used for generation.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void SetFloatAttribute(const FString& Name, float Value, bool bAddIfNonExisting = false);

	/**
	 * Access float attribute values used for generation.
	 *
	 * @param Name The name of the float attribute.
	 * @param OutValue Set to the attributes value if it exists or to 0.0f otherwise.
	 * @returns true if the float attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetFloatAttribute(const FString& Name, float& OutValue) const;

	/**
	 * Sets the given attributes. If a key does not exist in the current attribute map the key-value pair will be ignored.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 * The type of the attribute will be deduced from its string representation: <br>
	 * "1.0" for the float 1.0 <br>
	 * "hello" for the string "hello" and <br>
	 * "true" for the bool true <br>
	 *
	 * @param NewAttributes the attributes to be set
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void SetAttributes(const TMap<FString, FString>& NewAttributes, bool bAddIfNonExisting = false);

	/**
	 * Sets the given static mesh as initial shape. Regenerates the model if generate automatically is set to true.
	 *
	 * @param StaticMesh the new initial shape static mesh.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void SetMeshInitialShape(UStaticMesh* StaticMesh);

	/**
	 * Sets the given spline points as initial shape. Regenerates the model if generate automatically is set to true.
	 *
	 * @param SplinePoints the new initial shape spline points.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void SetSplineInitialShape(const TArray<FSplinePoint>& SplinePoints);

	/** Returns the attributes used for generation. */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	const TMap<FString, URuleAttribute*>& GetAttributes() const;

	/** Returns the rule package used for generation. */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	URulePackage* GetRpk() const;

	/** Returns the reports created during generation. */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	const TMap<FString, FReport>& GetReports() const;
		
	/**
	 * Sets the random seed used for generation.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	void SetRandomSeed(int32 NewRandomSeed);

	/* Initialize the VitruvioComponent. Only needs to be called if the Component is natively attached. */
	void Initialize();

	/* Removes the generated meshes from this VitruvioComponent. */
	void RemoveGeneratedMeshes();

	/**
	 * Evaluate rule attributes.
	 *
	 * @param ForceRegenerate Whether to force regenerate even if generate automatically is set to false
	 */
	void EvaluateRuleAttributes(bool ForceRegenerate = false);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHierarchyChanged, UVitruvioComponent*);
	static FOnHierarchyChanged OnHierarchyChanged;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAttributesChanged, UObject*, struct FPropertyChangedEvent&);
	static FOnAttributesChanged OnAttributesChanged;

	virtual void PostLoad() override;

	virtual void OnComponentCreated() override;

	void LoadInitialShape();

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void OnPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	void SetInitialShapeType(const TSubclassOf<UInitialShape>& Type);
#endif

	static TArray<TSubclassOf<UInitialShape>> GetInitialShapesClasses();

private:
	/** CityEngine Rule Package used for generation. */
	UPROPERTY(EditAnywhere, DisplayName = "Rule Package", Category = "Vitruvio", meta = (AllowPrivateAccess = "true"))
	URulePackage* Rpk;

	/** Rule attributes used for generation. */
	UPROPERTY(EditAnywhere, DisplayName = "Attributes", Category = "Vitruvio")
	TMap<FString, URuleAttribute*> Attributes;

	/** Random seed used for generation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Random Seed", Category = "Vitruvio", meta = (AllowPrivateAccess = "true"))
	int32 RandomSeed;

	/** CGA Reports from generation. */
	UPROPERTY(VisibleAnywhere, DisplayName = "Reports", Category = "Vitruvio")
	TMap<FString, FReport> Reports;

	TQueue<FGenerateResultDescription> GenerateQueue;
	TQueue<FAttributesEvaluation> AttributesEvaluationQueue;

	FGenerateResult::FTokenPtr GenerateToken;
	FAttributeMapResult::FTokenPtr EvalAttributesInvalidationToken;

	bool HasGeneratedMesh = false;

	void CalculateRandomSeed();

	void NotifyAttributesChanged();

	void ProcessGenerateQueue();
	void ProcessAttributesEvaluationQueue();

	FConvertedGenerateResult BuildResult(FGenerateResultDescription& GenerateResult,
										 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
										 TMap<FString, Vitruvio::FTextureData>& TextureCache);

#if WITH_EDITOR
	FDelegateHandle PropertyChangeDelegate;
#endif
};