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

#include "CityEngineBlueprintLibrary.h"

#include "CityEngineActor.h"
#include "CityEngineBatchActor.h"

TArray<AActor*> UCityEngineBlueprintLibrary::GetAttachedCityEngineActors(AActor* Root)
{
	if (!Root)
	{
		return {};
	}

	TArray<AActor*> CityEngineActors;
	if (Cast<ACityEngineActor>(Root) || Root->FindComponentByClass<UCityEngineComponent>())
	{
		CityEngineActors.Add(Root);
	}

	TArray<AActor*> ChildActors;
	Root->GetAttachedActors(ChildActors);
	for (AActor* Child : ChildActors)
	{
		CityEngineActors.Append(GetAttachedCityEngineActors(Child));
	}

	return CityEngineActors;
}

TArray<AActor*> UCityEngineBlueprintLibrary::GetAttachedInitialShapes(AActor* Root)
{
	if (!Root)
	{
		return {};
	}

	TArray<AActor*> ViableActors;
	if (CanConvertToCityEngineActor(Root))
	{
		ViableActors.Add(Root);
	}

	// If the actor has a VitruvioComponent attached we do not further check its children.
	if (Root->FindComponentByClass<UCityEngineComponent>() == nullptr)
	{
		TArray<AActor*> ChildActors;
		Root->GetAttachedActors(ChildActors);

		for (AActor* Child : ChildActors)
		{
			ViableActors.Append(GetAttachedInitialShapes(Child));
		}
	}

	return ViableActors;
}

bool UCityEngineBlueprintLibrary::CanConvertToCityEngineActor(AActor* Actor)
{
	if (!Actor || Cast<ACityEngineActor>(Actor) || Cast<ACityEngineBatchActor>(Actor) || Actor->GetComponentByClass(UCityEngineComponent::StaticClass()))
	{
		return false;
	}

	for (const auto& InitialShapeClasses : UCityEngineComponent::GetInitialShapesClasses())
	{
		const UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(Actor))
		{
			return true;
		}
	}

	return false;
}
