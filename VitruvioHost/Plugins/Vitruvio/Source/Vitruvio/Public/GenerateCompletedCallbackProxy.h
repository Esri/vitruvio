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

#include "Components/SplineComponent.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "GenerateCompletedCallbackProxy.generated.h"

class AVitruvioActor;
class URulePackage;
class UVitruvioComponent;

UCLASS()
class VITRUVIO_API UGenerateCompletedCallbackProxy final : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGenerateCompletedDelegate);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttributesEvaluatedDelegate);

	/** Called after the attributes have been evaluated. Note that it is not guaranteed that this callback is ever called. */
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Attributes Evaluated"), Category = "Vitruvio")
	FOnAttributesEvaluatedDelegate OnAttributesEvaluated;

	/** Called after generate has completed. Note that it is not guaranteed that this callback is ever called. */
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Generate Completed"), Category = "Vitruvio")
	FGenerateCompletedDelegate OnGenerateCompleted;

	/**
	 * Sets the given Rpk and possibly invalidates already loaded attributes. This will trigger a reevaluation of the attributes and if
	 * GenerateAutomatically is set to true also regenerates the the model.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetRpk(UVitruvioComponent* VitruvioComponent, URulePackage* RulePackage);

	/**
	 * Generates a model using the current RPK and initial shapes. If attributes are not loaded yet they will first be evaluated. If no Initial Shape
	 * or RPK is set this method will do nothing.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* Generate(UVitruvioComponent* VitruvioComponent);

	/**
	 * Sets float attributes used for generation.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetFloatAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name, float Value,
															  bool bAddIfNonExisting = false);

	/**
	 * Sets string attributes used for generation.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute to set.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetStringAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name, const FString& Value,
															   bool bAddIfNonExisting = false);

	/**
	 * Sets bool attributes used for generation.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetBoolAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name, bool Value,
															 bool bAddIfNonExisting = false);

	/**
	 * Sets the float array attribute with the given name.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetFloatArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																   const TArray<double>& Values, bool bAddIfNonExisting = false);

	/**
	 * Sets the string array attribute with the given name.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetStringArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																	const TArray<FString>& Values, bool bAddIfNonExisting = false);

	/**
	 * Sets the bool array attribute with the given name.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 * @returns true if the attribute has been set to the new value or false otherwise.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetBoolArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																  const TArray<bool>& Values, bool bAddIfNonExisting = false);

	/**
	 * Sets the given attributes. If a key does not exist in the current attribute map the key-value pair will be ignored.
	 * If GenerateAutomatically is set to true this will automatically trigger a regeneration.
	 * The type of the attribute will be deduced from its string representation: <br>
	 * "1.0" for the float 1.0 <br>
	 * "hello" for the string "hello" and <br>
	 * "true" for the bool true <br>
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param NewAttributes the attributes to be set
	 * @param bAddIfNonExisting Adds a new Attribute if the no Attribute is found with the given Name.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetAttributes(UVitruvioComponent* VitruvioComponent, const TMap<FString, FString>& NewAttributes,
														  bool bAddIfNonExisting = false);

	/**
	 * Sets the given static mesh as initial shape. Regenerates the model if generate automatically is set to true.
	 *
	 * @param StaticMesh the new initial shape static mesh.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetMeshInitialShape(UVitruvioComponent* VitruvioComponent, UStaticMesh* StaticMesh);

	/**
	 * Sets the given spline points as initial shape. Regenerates the model if generate automatically is set to true.
	 *
	 * @param SplinePoints the new initial shape spline points.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetSplineInitialShape(UVitruvioComponent* VitruvioComponent, const TArray<FSplinePoint>& SplinePoints);

	/**
	 * Converts the given Actors to VitruvioActors and optionally assigns the given RulePackage. If an Actor can not be converted (according to
	 * CanConvertToVitruvioActor) it will be ignored.
	 *
	 * @param Actors the Actors to convert to VitruvioActors
	 * @param OutVitruvioActors the converted VitruvioActors
	 * @param Rpk the optional RulePackage
	 * @param bGenerateModels Whether a model should be generated after the conversion. Only applicable if the RulePackage has been set.
	 * @return the converted VitruvioActors
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* ConvertToVitruvioActor(const TArray<AActor*>& Actors, TArray<AVitruvioActor*>& OutVitruvioActors,
																   URulePackage* Rpk = nullptr, bool bGenerateModels = true);
};