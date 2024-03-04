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

#include "VitruvioBlueprintLibrary.h"

#include "Engine/StaticMeshActor.h"
#include "VitruvioActor.h"
#include "VitruvioBatchActor.h"

TArray<AActor*> UVitruvioBlueprintLibrary::GetVitruvioActorsInHierarchy(AActor* Root)
{
	if (!Root)
	{
		return {};
	}

	TArray<AActor*> VitruvioActors;
	if (Cast<AVitruvioActor>(Root) || Root->FindComponentByClass<UVitruvioComponent>())
	{
		VitruvioActors.Add(Root);
	}

	TArray<AActor*> ChildActors;
	Root->GetAttachedActors(ChildActors);
	for (AActor* Child : ChildActors)
	{
		VitruvioActors.Append(GetVitruvioActorsInHierarchy(Child));
	}

	return VitruvioActors;
}

TArray<AActor*> UVitruvioBlueprintLibrary::GetInitialShapesInHierarchy(AActor* Root)
{
	if (!Root)
	{
		return {};
	}

	TArray<AActor*> ViableActors;
	if (CanConvertToVitruvioActor(Root))
	{
		ViableActors.Add(Root);
	}

	// If the actor has a VitruvioComponent attached we do not further check its children.
	if (Root->FindComponentByClass<UVitruvioComponent>() == nullptr)
	{
		TArray<AActor*> ChildActors;
		Root->GetAttachedActors(ChildActors);

		for (AActor* Child : ChildActors)
		{
			ViableActors.Append(GetInitialShapesInHierarchy(Child));
		}
	}

	return ViableActors;
}

bool UVitruvioBlueprintLibrary::CanConvertToVitruvioActor(AActor* Actor)
{
	if (!Actor || Cast<AVitruvioActor>(Actor) || Cast<AVitruvioBatchActor>(Actor) || Actor->GetComponentByClass(UVitruvioComponent::StaticClass()))
	{
		return false;
	}

	for (const auto& InitialShapeClasses : UVitruvioComponent::GetInitialShapesClasses())
	{
		const UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(Actor))
		{
			return true;
		}
	}

	return false;
}
