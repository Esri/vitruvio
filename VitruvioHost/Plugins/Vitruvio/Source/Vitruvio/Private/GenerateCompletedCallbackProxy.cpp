/* Copyright 2023 Esri
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

#include "Algo/Count.h"
#include "Engine/World.h"
#include "VitruvioBlueprintLibrary.h"
#include "VitruvioComponent.h"

namespace
{
void CopyInitialShapeSceneComponent(AActor* OldActor, AActor* NewActor)
{
	for (const auto& InitialShapeClasses : UVitruvioComponent::GetInitialShapesClasses())
	{
		UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(OldActor))
		{
			DefaultInitialShape->CopySceneComponent(OldActor, NewActor);
		}
	}
}

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
} // namespace

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetRpk(UVitruvioComponent* VitruvioComponent, URulePackage* RulePackage,
																		 bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetRpk(RulePackage, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetRandomSeed(UVitruvioComponent* VitruvioComponent, int32 NewRandomSeed,
																				bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetRandomSeed(NewRandomSeed, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::Generate(UVitruvioComponent* VitruvioComponent)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->Generate(Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetFloatAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																					float Value, bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetFloatAttribute(Name, Value, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetStringAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																					 const FString& Value, bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetStringAttribute(Name, Value, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetBoolAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																				   bool Value, bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetBoolAttribute(Name, Value, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetFloatArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						 const TArray<double>& Values, bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetFloatArrayAttribute(Name, Values, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetStringArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						  const TArray<FString>& Values, bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetStringArrayAttribute(Name, Values, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetBoolArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						const TArray<bool>& Values, bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetBoolArrayAttribute(Name, Values, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetAttributes(UVitruvioComponent* VitruvioComponent,
																				const TMap<FString, FString>& NewAttributes, bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetAttributes(NewAttributes, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetMeshInitialShape(UVitruvioComponent* VitruvioComponent, UStaticMesh* StaticMesh,
																					  bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetMeshInitialShape(StaticMesh, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetSplineInitialShape(UVitruvioComponent* VitruvioComponent,
																						const TArray<FSplinePoint>& SplinePoints, bool bGenerateModel)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(VitruvioComponent);
	VitruvioComponent->SetSplineInitialShape(SplinePoints, bGenerateModel, Proxy);
	return Proxy;
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::ConvertToVitruvioActor(UObject* WorldContextObject, const TArray<AActor*>& Actors,
																						 TArray<AVitruvioActor*>& OutVitruvioActors,
																						 URulePackage* Rpk, bool bGenerateModels)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(WorldContextObject);
	const int32 TotalActors = Algo::CountIf(Actors, [](AActor* Actor) { return UVitruvioBlueprintLibrary::CanConvertToVitruvioActor(Actor); });

	UGenerateCompletedCallbackProxy* InternalProxy = NewObject<UGenerateCompletedCallbackProxy>();
	InternalProxy->RegisterWithGameInstance(WorldContextObject);
	InternalProxy->OnGenerateCompleted.AddLambda(FExecuteAfterCountdown(TotalActors, [Proxy]() {
		Proxy->OnGenerateCompletedBlueprint.Broadcast();
		Proxy->OnGenerateCompleted.Broadcast();
	}));
	InternalProxy->OnAttributesEvaluated.AddLambda(FExecuteAfterCountdown(TotalActors, [Proxy]() {
		Proxy->OnAttributesEvaluatedBlueprint.Broadcast();
		Proxy->OnAttributesEvaluated.Broadcast();
	}));

	for (AActor* Actor : Actors)
	{
		AActor* OldAttachParent = Actor->GetAttachParentActor();
		if (UVitruvioBlueprintLibrary::CanConvertToVitruvioActor(Actor))
		{
			AVitruvioActor* VitruvioActor = Actor->GetWorld()->SpawnActor<AVitruvioActor>(Actor->GetActorLocation(), Actor->GetActorRotation());

			CopyInitialShapeSceneComponent(Actor, VitruvioActor);

			UVitruvioComponent* VitruvioComponent = VitruvioActor->VitruvioComponent;
			VitruvioComponent->SetRpk(Rpk, bGenerateModels, InternalProxy);

			if (OldAttachParent)
			{
				VitruvioActor->AttachToActor(OldAttachParent, FAttachmentTransformRules::KeepWorldTransform);
			}

			Actor->Destroy();

			OutVitruvioActors.Add(VitruvioActor);
		}
	}
	return Proxy;
}
