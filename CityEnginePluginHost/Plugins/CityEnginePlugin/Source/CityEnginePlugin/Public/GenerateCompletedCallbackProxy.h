/* Copyright 2024 Esri
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
#include "CityEngineComponent.h"

#include "GenerateCompletedCallbackProxy.generated.h"

class ACityEngineActor;
class URulePackage;

struct FExecuteAfterCountdown
{
	FCriticalSection CountDownLock;
	int32 Count;
	TFunction<void()> Fun;

	FExecuteAfterCountdown(int32 Count, TFunction<void()> Fun) : Count(Count), Fun(Fun) {}

	FExecuteAfterCountdown(const FExecuteAfterCountdown& Other) : Count(Other.Count), Fun(Other.Fun) {}

	void operator()()
	{
		FScopeLock Lock(&CountDownLock);
		Count--;
		if (Count <= 0)
		{
			Fun();
		}
	}
};

UCLASS()
class CITYENGINEPLUGIN_API UGenerateCompletedCallbackProxy final : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGenerateCompletedDynDelegate);
	DECLARE_MULTICAST_DELEGATE(FGenerateCompletedDelegate);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttributesEvaluatedDynDelegate);
	DECLARE_MULTICAST_DELEGATE(FOnAttributesEvaluatedDelegate);

	/** Called after the attributes have been evaluated. Note that it is not guaranteed that this callback is ever called. */
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Attributes Evaluated"), Category = "CityEngine")
	FOnAttributesEvaluatedDynDelegate OnAttributesEvaluatedBlueprint;
	FOnAttributesEvaluatedDelegate OnAttributesEvaluated;

	/** Called after generate has completed. Note that it is not guaranteed that this callback is ever called. */
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Generate Completed"), Category = "CityEngine")
	FGenerateCompletedDynDelegate OnGenerateCompletedBlueprint;
	FGenerateCompletedDelegate OnGenerateCompleted;

	/**
	 * Sets the given Rule Package. This will reevaluate the attributes and if bGenerateModel is set to true, also generates the model.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetRpk(UCityEngineComponent* CityEngineComponent, URulePackage* RulePackage, bool bGenerateModel = true);

	/**
	 * Sets the random seed used for generation. This will reevaluate the attributes and if bGenerateModel is set to true, also generates the model.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetRandomSeed(UCityEngineComponent* CityEngineComponent, int32 NewRandomSeed, bool bGenerateModel = true);

	/**
	 * Generates a model using the current Rule Package and initial shape. If the attributes are not yet available, they will first be evaluated. If
	 * no Initial Shape or Rule Package is set, this method will do nothing.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* Generate(UCityEngineComponent* CityEngineComponent, FGenerateOptions GenerateOptions);

	/**
	 * Sets the float attribute with the given Name to the given value. Regenerates the model if bGenerateModel is set to true.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bGenerateModel Whether a model should be generated after the attribute has been set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetFloatAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name, float Value,
															  bool bGenerateModel = true);

	/**
	 * Sets the string attribute with the given Name to the given value. Regenerates the model if bGenerateModel is set to true.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the attribute is set
	 * @param Name The name of the attribute to set.
	 * @param Value The new value for the attribute.
	 * @param bGenerateModel Whether a model should be generated after the attribute has been set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetStringAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name, const FString& Value,
															   bool bGenerateModel = true);

	/**
	 * Sets the bool attribute with the given Name to the given value. Regenerates the model if bGenerateModel is set to true.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Value The new value for the attribute.
	 * @param bGenerateModel Whether a model should be generated after the attribute has been set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetBoolAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name, bool Value,
															 bool bGenerateModel = true);

	/**
	 * Sets the float array attribute with the given Name to the given value. Regenerates the model if bGenerateModel is set to true.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bGenerateModel Whether a model should be generated after the attribute has been set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetFloatArrayAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																   const TArray<double>& Values, bool bGenerateModel = true);

	/**
	 * Sets a string array attribute with the given Name to the given value. Regenerates the model if bGenerateModel is set to true.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the attribute is set
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bGenerateModel Whether a model should be generated after the attribute has been set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetStringArrayAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																	const TArray<FString>& Values, bool bGenerateModel = true);

	/**
	 * Sets a bool array attribute with the given Name to the given value. Regenerates the model if bGenerateModel is set to true.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the attribute is set.
	 * @param Name The name of the attribute.
	 * @param Values The new values for the attribute.
	 * @param bGenerateModel Whether a model should be generated after the attribute has been set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetBoolArrayAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																  const TArray<bool>& Values, bool bGenerateModel = true);

	/**
	 * Sets the given attributes. If a key from the NewAttributes is not found in the current attributes, the key-value pair will be ignored.
	 * Regenerates the model if bGenerateModel is set to true. Arrays are surrounded with [] and their values separated by commas
	 * eg: "[1.3,4.5,0]" for a float array with the values 1.3, 4.5 and 0.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the attribute is set.
	 * @param NewAttributes The attributes to be set.
	 * @param bGenerateModel Whether a model should be generated after the attributes have been set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetAttributes(UCityEngineComponent* CityEngineComponent, const TMap<FString, FString>& NewAttributes,
														  bool bGenerateModel = true);

	/**
	 * Sets the given static mesh as initial shape. Regenerates the model if bGenerateModel is set to true.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the initial shape is set.
	 * @param StaticMesh The new initial shape static mesh.
	 * @param bGenerateModel Whether a model should be generated after the initial shape has set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetMeshInitialShape(UCityEngineComponent* CityEngineComponent, UStaticMesh* StaticMesh,
																bool bGenerateModel = true);

	/**
	 * Sets the given spline points as initial shape. Regenerates the model if bGenerateModel is set to true.
	 *
	 * @param CityEngineComponent The CityEngineComponent where the initial shape is set.
	 * @param SplinePoints The new initial shape spline points.
	 * @param bGenerateModel Whether a model should be generated after the initial shape has set.
	 * @returns a callback proxy used to register for completion events.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* SetSplineInitialShape(UCityEngineComponent* CityEngineComponent, const TArray<FSplinePoint>& SplinePoints,
																  bool bGenerateModel = true);

	/**
	 * Converts the given Actors to CityEngineActors and optionally assigns the given RulePackage. If an Actor can not be converted (see
	 * CanConvertToVitruvioActor) it will be ignored.
	 *
	 * @param WorldContextObject
	 * @param Actors The Actors to convert to CityEngineActors.
	 * @param OutCityEngineActors The converted CityEngineActors.
	 * @param Rpk The optional RulePackage.
	 * @param bGenerateModels Whether a model should be generated after the conversion. Only applicable if the RulePackage has been set.
	 * @param bBatchGeneration Whether the newly created CityEngineActors should be batch-generated.
	 * @return The converted CityEngineActors.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true, WorldContext = "WorldContextObject"), Category = "CityEngine")
	static UGenerateCompletedCallbackProxy* ConvertToCityEngine(UObject* WorldContextObject, const TArray<AActor*>& Actors,
																   TArray<ACityEngineActor*>& OutCityEngineActors, URulePackage* Rpk = nullptr,
																   bool bGenerateModels = true, bool bBatchGeneration = false);
};