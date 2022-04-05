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
	DECLARE_MULTICAST_DELEGATE(FGenerateCompletedDelegateCpp);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttributesEvaluatedDelegate);
	DECLARE_MULTICAST_DELEGATE(FOnAttributesEvaluatedDelegateCpp);

	/** Called after the attributes have been evaluated. Note that it is not guaranteed that this callback is ever called. */
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Attributes Evaluated"), Category = "Vitruvio")
	FOnAttributesEvaluatedDelegate OnAttributesEvaluated;
	FOnAttributesEvaluatedDelegateCpp OnAttributesEvaluatedCpp;

	/** Called after generate has completed. Note that it is not guaranteed that this callback is ever called. */
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Generate Completed"), Category = "Vitruvio")
	FGenerateCompletedDelegate OnGenerateCompleted;
	FGenerateCompletedDelegateCpp OnGenerateCompletedCpp;

	/**
	 * Sets the given Rule Package. This will reevaluate the attributes and if GenerateAutomatically is set to true, also regenerates the model.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetRpk(UVitruvioComponent* VitruvioComponent, URulePackage* RulePackage);

	/**
	 * Generates a model using the current Rule Package and initial shape. If the attributes are not yet available, they will first be evaluated. If
	 * no Initial Shape or Rule Package is set, this method will do nothing.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* Generate(UVitruvioComponent* VitruvioComponent);

	/**
	 * Sets the float attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new attribute if the no attribute is found with the given Name.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetFloatAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name, float Value,
															  bool bAddIfNonExisting = false);

	/**
	 * Sets the string attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute to set.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new attribute if the no attribute is found with the given Name.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetStringAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name, const FString& Value,
															   bool bAddIfNonExisting = false);

	/**
	 * Sets the bool attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bAddIfNonExisting Adds a new attribute if the no attribute is found with the given Name.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetBoolAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name, bool Value,
															 bool bAddIfNonExisting = false);

	/**
	 * Sets the float array attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new attribute if the no attribute is found with the given Name.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetFloatArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																   const TArray<double>& Values, bool bAddIfNonExisting = false);

	/**
	 * Sets a string array attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new attribute if the no attribute is found with the given Name.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetStringArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																	const TArray<FString>& Values, bool bAddIfNonExisting = false);

	/**
	 * Sets a bool array attribute with the given Name to the given value. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set.
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bAddIfNonExisting Adds a new attribute if the no attribute is found with the given Name.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetBoolArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																  const TArray<bool>& Values, bool bAddIfNonExisting = false);

	/**
	 * Sets the given scalar attributes. If bAddIfNonExisting is set to false and a given key from the NewAttributes is not found in the current
	 * attributes, the key-value pair will be ignored. If bAddIfNonExisting is set to true, new attributes will be added in case they are not found in
	 * the current attributes. Regenerates the model if GenerateAutomatically is set to true. The type of the attribute will be deduced from its
	 * string representation: <br> "1.0" for the float 1.0 <br> "hello" for the string "hello" and <br> "true" for the bool true <br>
	 *
	 * @param VitruvioComponent The VitruvioComponent where the attribute is set.
	 * @param NewAttributes The attributes to be set.
	 * @param bAddIfNonExisting Adds a new attribute if the no attribute is found with the given Name.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetAttributes(UVitruvioComponent* VitruvioComponent, const TMap<FString, FString>& NewAttributes,
														  bool bAddIfNonExisting = false);

	/**
	 * Sets the given static mesh as initial shape. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the initial shape is set.
	 * @param StaticMesh The new initial shape static mesh.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetMeshInitialShape(UVitruvioComponent* VitruvioComponent, UStaticMesh* StaticMesh);

	/**
	 * Sets the given spline points as initial shape. Regenerates the model if GenerateAutomatically is set to true.
	 *
	 * @param VitruvioComponent The VitruvioComponent where the initial shape is set.
	 * @param SplinePoints The new initial shape spline points.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* SetSplineInitialShape(UVitruvioComponent* VitruvioComponent, const TArray<FSplinePoint>& SplinePoints);

	/**
	 * Converts the given Actors to VitruvioActors and optionally assigns the given RulePackage. If an Actor can not be converted (see
	 * CanConvertToVitruvioActor) it will be ignored.
	 *
	 * @param WorldContextObject
	 * @param Actors The Actors to convert to VitruvioActors.
	 * @param OutVitruvioActors The converted VitruvioActors.
	 * @param Rpk The optional RulePackage.
	 * @param bGenerateModels Whether a model should be generated after the conversion. Only applicable if the RulePackage has been set.
	 * @return The converted VitruvioActors.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true, WorldContext = "WorldContextObject"), Category = "Vitruvio")
	static UGenerateCompletedCallbackProxy* ConvertToVitruvioActor(UObject* WorldContextObject, const TArray<AActor*>& Actors,
																   TArray<AVitruvioActor*>& OutVitruvioActors, URulePackage* Rpk = nullptr,
																   bool bGenerateModels = true);
};