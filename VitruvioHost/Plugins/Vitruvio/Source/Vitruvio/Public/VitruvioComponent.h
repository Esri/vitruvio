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
#include "GenerateCompletedCallbackProxy.h"
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
	FReportArray Reports;
};

struct FAttributesEvaluationQueueItem
{
	FAttributeMapPtr AttributeMap;
	bool bForceRegenerate;
	UGenerateCompletedCallbackProxy* CallbackProxy;
};

struct FGenerateQueueItem
{
	FGenerateResultDescription GenerateResultDescription;
	UGenerateCompletedCallbackProxy* CallbackProxy;
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

	/**
	 * Generates a model using the current Rule Package and initial shape. If the attributes are not yet available, they will first be evaluated. If
	 * no Initial Shape or Rule Package is set, this method will do nothing.
	 */
	void Generate(UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Sets the given Rule Package. This will reevaluate the attributes and if GenerateAutomatically is set to true, also regenerates the model.
	 */
	void SetRpk(URulePackage* RulePackage, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/** Returns true if the component has valid input data (initial shape and Rule Package). */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool HasValidInputData() const;

	/** Returns true if the component is ready to generate, meaning it HasValidInputData and the attributes are loaded. */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool IsReadyToGenerate() const;

	/**
	 * Sets the string attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param Name The name of the attribute to set.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new attribute if the no attribute is found with the given Name.
	 * @param CallbackProxy The callback proxy used to register for completion events.
	 */
	void SetStringAttribute(const FString& Name, const FString& Value, bool bAddIfNonExisting = false,
							UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Access the string attribute with the given Name. The OutValue is default initialized if no attribute with the given Name is found.
	 *
	 * @param Name The name of the string attribute.
	 * @param OutValue Set to the attributes value if it exists or to an empty String otherwise.
	 * @returns True if the String attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetStringAttribute(const FString& Name, FString& OutValue) const;

	/**
	 * Sets the string array attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @param CallbackProxy The optional callback proxy used for generate completed notifications.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	void SetStringArrayAttribute(const FString& Name, const TArray<FString>& Values, bool bAddIfNonExisting = false,
								 UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Access the string array attribute with the given Name. The OutValue is default initialized if no attribute with the given Name is found.
	 *
	 * @param Name The name of the string attribute.
	 * @param OutValue Set to the array attribute value if it exists or an empty array otherwise.
	 * @returns true if the string array attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetStringArrayAttribute(const FString& Name, TArray<FString>& OutValue) const;

	/**
	 * Sets the bool attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @param CallbackProxy The optional callback proxy used for generate completed notifications.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	void SetBoolAttribute(const FString& Name, bool Value, bool bAddIfNonExisting = false, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Access the bool attribute with the given Name. The OutValue is default initialized if no attribute with the given Name is found.
	 *
	 * @param Name The name of the bool attribute.
	 * @param OutValue Set to the attributes value if it exists or to false otherwise.
	 * @returns true if the bool attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetBoolAttribute(const FString& Name, bool& OutValue) const;

	/**
	 * Sets the bool array attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @param CallbackProxy The optional callback proxy used for generate completed notifications.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	void SetBoolArrayAttribute(const FString& Name, const TArray<bool>& Values, bool bAddIfNonExisting = false,
							   UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Access the bool array attribute with the given Name. The OutValue is default initialized if no attribute with the given Name is found.
	 *
	 * @param Name The name of the bool attribute.
	 * @param OutValue Set to the array attribute value if it exists or an empty array otherwise.
	 * @returns true if the bool array attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetBoolArrayAttribute(const FString& Name, TArray<bool>& OutValue) const;

	/**
	 * Sets the float attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @param CallbackProxy The optional callback proxy used for generate completed notifications.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	void SetFloatAttribute(const FString& Name, double Value, bool bAddIfNonExisting = false,
						   UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Access the float attribute with the given Name. The OutValue is default initialized if no attribute with the given Name is found.
	 *
	 * @param Name The name of the float attribute.
	 * @param OutValue Set to the attributes value if it exists or to 0.0f otherwise.
	 * @returns true if the float attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetFloatAttribute(const FString& Name, double& OutValue) const;

	/**
	 * Sets the float array attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @param CallbackProxy The optional callback proxy used for generate completed notifications.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	void SetFloatArrayAttribute(const FString& Name, const TArray<double>& Values, bool bAddIfNonExisting = false,
								UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Access the float array attribute with the given Name. The OutValue is default initialized if no attribute with the given Name is found.
	 *
	 * @param Name The name of the float attribute.
	 * @param OutValue Set to the array attribute value if it exists or an empty array otherwise.
	 * @returns true if the float array attribute with the given Name exists or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	bool GetFloatArrayAttribute(const FString& Name, TArray<double>& OutValue) const;

	/**
	 * Sets the given scalar attributes. If bAddIfNonExisting is set to false and a given key from the NewAttributes is not found in the current
	 * attributes, the key-value pair will be ignored. If bAddIfNonExisting is set to true, new attributes will be added in case they are not found in
	 * the current attributes. Regenerates the model if GenerateAutomatically is set to true. The type of the attribute will be deduced from its
	 * string representation: <br> "1.0" for the float 1.0 <br> "hello" for the string "hello" and <br> "true" for the bool true <br>
	 *
	 * @param NewAttributes The attributes to be sets
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @param CallbackProxy The optional callback proxy used for generate completed notifications.
	 */
	void SetAttributes(const TMap<FString, FString>& NewAttributes, bool bAddIfNonExisting = false,
					   UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Sets the given static mesh as initial shape. Regenerates the model if generate automatically is set to true.
	 *
	 * @param StaticMesh the new initial shape static mesh.
	 * @param CallbackProxy The optional callback proxy used for generate completed notifications.
	 */
	void SetMeshInitialShape(UStaticMesh* StaticMesh, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/**
	 * Sets the given spline points as initial shape. Regenerates the model if generate automatically is set to true.
	 *
	 * @param SplinePoints the new initial shape spline points.
	 * @param CallbackProxy The optional callback proxy used for generate completed notifications.
	 */
	void SetSplineInitialShape(const TArray<FSplinePoint>& SplinePoints, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	/** Returns the attributes used for generation. */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	const TMap<FString, URuleAttribute*>& GetAttributes() const;

	/** Returns the rule package used for generation. */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	URulePackage* GetRpk() const;

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
	void EvaluateRuleAttributes(bool ForceRegenerate = false, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGenerateCompletedDelegate);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttributesEvaluatedDelegate);

	/** Called after attributes have been evaluated. */
	UPROPERTY(BlueprintAssignable, Category = "Vitruvio")
	FOnAttributesEvaluatedDelegate OnAttributesEvaluated;

	/** Called after a model generation has completed. */
	UPROPERTY(BlueprintAssignable, Category = "Vitruvio")
	FGenerateCompletedDelegate OnGenerateCompleted;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHierarchyChanged, UVitruvioComponent*);
	static FOnHierarchyChanged OnHierarchyChanged;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAttributesChanged, UObject*, struct FPropertyChangedEvent&);
	static FOnAttributesChanged OnAttributesChanged;

	virtual void PostLoad() override;

	virtual void OnComponentCreated() override;

	void LoadInitialShape();

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGenerateCompleted, UVitruvioComponent*, VitruvioComponent);
	UPROPERTY(BlueprintAssignable)
	FOnGenerateCompleted OnGenerateCompleted;

#if WITH_EDITOR
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
	FReportArray Reports;

	bool bInGenerateCallback = false;

	TQueue<FGenerateQueueItem> GenerateQueue;
	TQueue<FAttributesEvaluationQueueItem> AttributesEvaluationQueue;

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