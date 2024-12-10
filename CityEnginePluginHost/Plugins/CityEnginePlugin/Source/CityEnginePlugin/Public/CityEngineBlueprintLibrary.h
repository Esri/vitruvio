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

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CityEngineBlueprintLibrary.generated.h"

UCLASS()
class UCityEngineBlueprintLibrary : public UBlueprintFunctionLibrary

{
	GENERATED_BODY()

public:
	/**
	 * Returns all Actors attached to the given root Actor which are viable initial shapes for CityEngine Actors.
	 *
	 * @param Root the root Actor whose children are checked if they are viable initial shapes.
	 * @return all Actors attached to the given root Actor which are viable initial shapes.
	 */
	UFUNCTION(BlueprintCallable, Category = "CityEngine")
	static CITYENGINEPLUGIN_API TArray<AActor*> GetAttachedInitialShapes(AActor* Root);

	/**
	 * Returns all Actors attached to the given root Actor which are CityEngineActors or contain a CityEngineComponent.
	 *
	 * @param Root the root Actor whose children are checked if they are CityEngineActors or contain a CityEngineComponent.
	 * @return all Actors attached to the given root Actor which are CityEngineActors or contain a CityEngineComponent.
	 */
	UFUNCTION(BlueprintCallable, Category = "CityEngine")
	static CITYENGINEPLUGIN_API TArray<AActor*> GetAttachedCityEngineActors(AActor* Root);

	/**
	 * Returns whether the given Actor can be converted to a CityEngineActor (see also ConvertToCityEngineActor). Converting an Actor to a CityEngineActor
	 * is only possible if the Actor has a valid initial shape component attached (e.g. a StaticMeshComponent) and does not already
	 * have a CityEngineComponent attached.
	 *
	 * @param Actor the Actor to test
	 * @return whether the given Actor can be converted to a CityEngineActor
	 */
	UFUNCTION(BlueprintCallable, Category = "CityEngine")
	static CITYENGINEPLUGIN_API bool CanConvertToCityEngineActor(AActor* Actor);
};