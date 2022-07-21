/* Copyright 2022 Esri
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
#include "RulePackage.h"
#include "VitruvioActor.h"

#include "VitruvioBlueprintLibrary.generated.h"

UCLASS()
class UVitruvioBlueprintLibrary : public UBlueprintFunctionLibrary

{
	GENERATED_BODY()

public:
	/**
	 * Returns all Actors attached to the given Actor which are viable to be converted to VitruvioActors.
	 *
	 * @param Root the root actor whose children are checked if they are viable Vitruvio Actors.
	 * @return all Actors attached to the given Actor which are viable to be converted to VitruvioActors.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	static VITRUVIO_API TArray<AActor*> GetViableVitruvioActorsInHierarchy(AActor* Root);

	/**
	 * Returns whether the given Actor can be converted to a VitruvioActor (see also ConvertToVitruvioActor). Converting an Actor to a VitruvioActor
	 * is only possible if the Actor has a valid initial shape component attached (eg. a StaticMeshComponent) and does not already
	 * have a VitruvioComponent attached.
	 *
	 * @param Actor the Actor to test
	 * @return whether the given Actor can be converted to a VitruvioActor
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitruvio")
	static VITRUVIO_API bool CanConvertToVitruvioActor(AActor* Actor);
};