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

		break;
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

template <typename TFun>
UGenerateCompletedCallbackProxy* ExecuteIfComponentValid(const FString& FunctionName, UVitruvioComponent* VitruvioComponent, TFun&& Function)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	if (VitruvioComponent)
	{
		Proxy->RegisterWithGameInstance(VitruvioComponent);
		Function(Proxy, VitruvioComponent);
	}
	else
	{
		UE_LOG(LogVitruvioComponent, Error, TEXT("Cannot execute \"%s\" without valid VitruvioComponent argument."), *FunctionName)
	}

	return Proxy;
}

} // namespace

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetRpk(UVitruvioComponent* VitruvioComponent, URulePackage* RulePackage,
																		 bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetRpk"), VitruvioComponent, [RulePackage, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetRpk(RulePackage, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetRandomSeed(UVitruvioComponent* VitruvioComponent, int32 NewRandomSeed,
																				bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetRandomSeed"), VitruvioComponent, [NewRandomSeed, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetRandomSeed(NewRandomSeed, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::Generate(UVitruvioComponent* VitruvioComponent, const FGenerateOptions& GenerateOptions)
{
	return ExecuteIfComponentValid(TEXT("Generate"), VitruvioComponent, [GenerateOptions](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->Generate(Proxy, GenerateOptions);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetFloatAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																					float Value, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetFloatAttribute"), VitruvioComponent, [&Name, &Value, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetFloatAttribute(Name, Value, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetStringAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																					 const FString& Value, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetStringAttribute"), VitruvioComponent, [&Name, &Value, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetStringAttribute(Name, Value, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetBoolAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																				   bool Value, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetBoolAttribute"), VitruvioComponent, [&Name, &Value, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetBoolAttribute(Name, Value, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetFloatArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						 const TArray<double>& Values, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetFloatArrayAttribute"), VitruvioComponent, [&Name, &Values, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetFloatArrayAttribute(Name, Values, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetStringArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						  const TArray<FString>& Values, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetStringArrayAttribute"), VitruvioComponent, [&Name, &Values, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetStringArrayAttribute(Name, Values, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetBoolArrayAttribute(UVitruvioComponent* VitruvioComponent, const FString& Name,
																						const TArray<bool>& Values, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetBoolArrayAttribute"), VitruvioComponent, [&Name, &Values, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetBoolArrayAttribute(Name, Values, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetAttributes(UVitruvioComponent* VitruvioComponent,
																				const TMap<FString, FString>& NewAttributes, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetAttributes"), VitruvioComponent, [&NewAttributes, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetAttributes(NewAttributes, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetMeshInitialShape(UVitruvioComponent* VitruvioComponent, UStaticMesh* StaticMesh,
																					  bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetMeshInitialShape"), VitruvioComponent, [StaticMesh, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetMeshInitialShape(StaticMesh, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetSplineInitialShape(UVitruvioComponent* VitruvioComponent,
																						const TArray<FSplinePoint>& SplinePoints, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetSplineInitialShape"), VitruvioComponent, [&SplinePoints, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UVitruvioComponent* VitruvioComponent)
	{
		VitruvioComponent->SetSplineInitialShape(SplinePoints, bGenerateModel, Proxy);
	});
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
