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

#include "GenerateCompletedCallbackProxy.h"

#include "VitruvioBlueprintLibrary.h"
#include "VitruvioComponent.h"

namespace
{
USceneComponent* CopyInitialShapeSceneComponent(AActor* OldActor, AActor* NewActor)
{
	for (const auto& InitialShapeClasses : UVitruvioComponent::GetInitialShapesClasses())
	{
		UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(OldActor))
		{
			return DefaultInitialShape->CopySceneComponent(OldActor, NewActor);
		}
	}
	return nullptr;
}
} // namespace

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetRpk(UVitruvioComponent* VitruvioComponent, URulePackage* RulePackage)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetRpk(RulePackage, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::Generate(UVitruvioComponent* VitruvioComponent)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->Generate();
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetFloatAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																					float Value, bool bAddIfNonExisting)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetFloatAttribute(Name, Value, bAddIfNonExisting, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetStringAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																					 const FString& Value, bool bAddIfNonExisting)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetStringAttribute(Name, Value, bAddIfNonExisting, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetBoolAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																				   bool Value, bool bAddIfNonExisting)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetBoolAttribute(Name, Value, bAddIfNonExisting, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetFloatArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						 const TArray<double>& Values, bool bAddIfNonExisting)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetFloatArrayAttribute(Name, Values, bAddIfNonExisting, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetStringArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						  const TArray<FString>& Values, bool bAddIfNonExisting)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetStringArrayAttribute(Name, Values, bAddIfNonExisting, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetBoolArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						const TArray<bool>& Values, bool bAddIfNonExisting)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetBoolArrayAttribute(Name, Values, bAddIfNonExisting, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetAttributes(UVitruvioComponent* VitruvioComponent,
																				const TMap<FString, FString>& NewAttributes, bool bAddIfNonExisting)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetAttributes(NewAttributes, bAddIfNonExisting, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetMeshInitialShape(UVitruvioComponent* VitruvioComponent, UStaticMesh* StaticMesh)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetMeshInitialShape(StaticMesh, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetSplineInitialShape(UVitruvioComponent* VitruvioComponent,
																						const TArray<FSplinePoint>& SplinePoints)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	VitruvioComponent->SetSplineInitialShape(SplinePoints, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::ConvertToVitruvioActor(const TArray<AActor*>& Actors,
																						 TArray<AVitruvioActor*>& OutVitruvioActors,
																						 URulePackage* Rpk, bool bGenerateModels)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	for (AActor* Actor : Actors)
	{
		AActor* OldAttachParent = Actor->GetAttachParentActor();
		if (UVitruvioBlueprintLibrary::CanConvertToVitruvioActor(Actor))
		{
			AVitruvioActor* VitruvioActor = Actor->GetWorld()->SpawnActor<AVitruvioActor>(Actor->GetActorLocation(), Actor->GetActorRotation());

			USceneComponent* NewInitialShapeSceneComponent = CopyInitialShapeSceneComponent(Actor, VitruvioActor);
			NewInitialShapeSceneComponent->SetWorldTransform(VitruvioActor->GetTransform());

			UVitruvioComponent* VitruvioComponent = VitruvioActor->VitruvioComponent;

			VitruvioActor->Initialize();

			VitruvioComponent->GenerateAutomatically = bGenerateModels;
			VitruvioComponent->SetRpk(Rpk, Proxy);

			if (OldAttachParent)
			{
				VitruvioActor->AttachToActor(OldAttachParent, FAttachmentTransformRules::KeepWorldTransform);
			}

			Actor->Destroy();

			// After constructing the VitruvioActor we always want to generate models automatically by default
			VitruvioComponent->GenerateAutomatically = true;

			OutVitruvioActors.Add(VitruvioActor);
		}
	}
	return Proxy;
}
