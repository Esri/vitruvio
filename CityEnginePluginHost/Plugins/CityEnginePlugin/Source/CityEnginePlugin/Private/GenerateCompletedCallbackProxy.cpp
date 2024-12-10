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

#include "GenerateCompletedCallbackProxy.h"

#include "CityEngineActor.h"
#include "CityEngineBatchSubsystem.h"
#include "Algo/Count.h"
#include "Engine/World.h"
#include "CityEngineBlueprintLibrary.h"

namespace
{
void CopyInitialShapeSceneComponent(AActor* OldActor, AActor* NewActor)
{
	for (const auto& InitialShapeClasses : UCityEngineComponent::GetInitialShapesClasses())
	{
		UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(OldActor))
		{
			DefaultInitialShape->CopySceneComponent(OldActor, NewActor);
		}

		break;
	}
}

template <typename TFun>
UGenerateCompletedCallbackProxy* ExecuteIfComponentValid(const FString& FunctionName, UCityEngineComponent* CityEngineComponent, TFun&& Function)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	if (CityEngineComponent)
	{
		Proxy->RegisterWithGameInstance(CityEngineComponent);
		Function(Proxy, CityEngineComponent);
	}
	else
	{
		UE_LOG(LogCityEngineComponent, Error, TEXT("Cannot execute \"%s\" without valid CityEngineComponent argument."), *FunctionName)
	}

	return Proxy;
}

} // namespace

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetRpk(UCityEngineComponent* CityEngineComponent, URulePackage* RulePackage,
																		 bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetRpk"), CityEngineComponent, [RulePackage, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetRpk(RulePackage, true, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetRandomSeed(UCityEngineComponent* CityEngineComponent, int32 NewRandomSeed,
																				bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetRandomSeed"), CityEngineComponent, [NewRandomSeed, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetRandomSeed(NewRandomSeed, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::Generate(UCityEngineComponent* CityEngineComponent, FGenerateOptions GenerateOptions)
{
	return ExecuteIfComponentValid(TEXT("Generate"), CityEngineComponent, [GenerateOptions](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->Generate(Proxy, GenerateOptions);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetFloatAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																					float Value, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetFloatAttribute"), CityEngineComponent, [&Name, &Value, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetFloatAttribute(Name, Value, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetStringAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																					 const FString& Value, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetStringAttribute"), CityEngineComponent, [&Name, &Value, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetStringAttribute(Name, Value, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetBoolAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																				   bool Value, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetBoolAttribute"), CityEngineComponent, [&Name, &Value, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetBoolAttribute(Name, Value, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetFloatArrayAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																						 const TArray<double>& Values, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetFloatArrayAttribute"), CityEngineComponent, [&Name, &Values, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetFloatArrayAttribute(Name, Values, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetStringArrayAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																						  const TArray<FString>& Values, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetStringArrayAttribute"), CityEngineComponent, [&Name, &Values, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetStringArrayAttribute(Name, Values, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetBoolArrayAttribute(UCityEngineComponent* CityEngineComponent, const FString& Name,
																						const TArray<bool>& Values, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetBoolArrayAttribute"), CityEngineComponent, [&Name, &Values, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetBoolArrayAttribute(Name, Values, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetAttributes(UCityEngineComponent* CityEngineComponent,
																				const TMap<FString, FString>& NewAttributes, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetAttributes"), CityEngineComponent, [&NewAttributes, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetAttributes(NewAttributes, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetMeshInitialShape(UCityEngineComponent* CityEngineComponent, UStaticMesh* StaticMesh,
																					  bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetMeshInitialShape"), CityEngineComponent, [StaticMesh, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetMeshInitialShape(StaticMesh, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::SetSplineInitialShape(UCityEngineComponent* CityEngineComponent,
																						const TArray<FSplinePoint>& SplinePoints, bool bGenerateModel)
{
	return ExecuteIfComponentValid(TEXT("SetSplineInitialShape"), CityEngineComponent, [&SplinePoints, bGenerateModel](UGenerateCompletedCallbackProxy* Proxy, UCityEngineComponent* CityEngineComponent)
	{
		CityEngineComponent->SetSplineInitialShape(SplinePoints, bGenerateModel, Proxy);
	});
}

UGenerateCompletedCallbackProxy* UGenerateCompletedCallbackProxy::ConvertToCityEngine(UObject* WorldContextObject, const TArray<AActor*>& Actors,
																						 TArray<ACityEngineActor*>& OutCityEngineActors,
																						 URulePackage* Rpk, bool bGenerateModels, bool bBatchGeneration)
{
	UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
	Proxy->RegisterWithGameInstance(WorldContextObject);

	UGenerateCompletedCallbackProxy* NonBatchedProxy = nullptr;
	if (!bBatchGeneration)
	{
		NonBatchedProxy = NewObject<UGenerateCompletedCallbackProxy>();
		const int32 TotalActors = Algo::CountIf(Actors, [](AActor* Actor) { return UCityEngineBlueprintLibrary::CanConvertToCityEngineActor(Actor); });
		NonBatchedProxy->RegisterWithGameInstance(WorldContextObject);
		NonBatchedProxy->OnGenerateCompleted.AddLambda(FExecuteAfterCountdown(TotalActors, [Proxy]() {
			Proxy->OnGenerateCompletedBlueprint.Broadcast();
			Proxy->OnGenerateCompleted.Broadcast();
		}));
		NonBatchedProxy->OnAttributesEvaluated.AddLambda(FExecuteAfterCountdown(TotalActors, [Proxy]() {
			Proxy->OnAttributesEvaluatedBlueprint.Broadcast();
			Proxy->OnAttributesEvaluated.Broadcast();
		}));
	}

	for (AActor* Actor : Actors)
	{
		AActor* OldAttachParent = Actor->GetAttachParentActor();
		if (UCityEngineBlueprintLibrary::CanConvertToCityEngineActor(Actor))
		{
			ACityEngineActor* CityEngineActor = Actor->GetWorld()->SpawnActor<ACityEngineActor>(Actor->GetActorLocation(), Actor->GetActorRotation());

			CopyInitialShapeSceneComponent(Actor, CityEngineActor);

			UCityEngineComponent* CityEngineComponent = CityEngineActor->CityEngineComponent;
			CityEngineComponent->SetBatchGenerated(bBatchGeneration);

			CityEngineComponent->SetRpk(Rpk, !bBatchGeneration, bGenerateModels, NonBatchedProxy);

			if (OldAttachParent)
			{
				CityEngineActor->AttachToActor(OldAttachParent, FAttachmentTransformRules::KeepWorldTransform);
			}

			Actor->Destroy();

			OutCityEngineActors.Add(CityEngineActor);
		}
	}

	if (bBatchGeneration)
	{
		UCityEngineBatchSubsystem* VitruvioBatchSubsystem = WorldContextObject->GetWorld()->GetSubsystem<UCityEngineBatchSubsystem>();
		VitruvioBatchSubsystem->GenerateAll(Proxy);
	}
	
	return Proxy;
}
