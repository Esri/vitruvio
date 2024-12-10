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

#include "CityEngineBatchSubsystem.h"

#include "EngineUtils.h"

void UCityEngineBatchSubsystem::RegisterCityEngineComponent(UCityEngineComponent* CityEngineComponent)
{
	RegisteredComponents.Add(CityEngineComponent);
	GetBatchActor()->RegisterCityEngineComponent(CityEngineComponent);

	OnComponentRegistered.Broadcast();
}

void UCityEngineBatchSubsystem::UnregisterCityEngineComponent(UCityEngineComponent* CityEngineComponent)
{
	RegisteredComponents.Remove(CityEngineComponent);
	GetBatchActor()->UnregisterCityEngineComponent(CityEngineComponent);

	OnComponentDeregistered.Broadcast();
}

void UCityEngineBatchSubsystem::Generate(UCityEngineComponent* CityEngineComponent, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	GetBatchActor()->Generate(CityEngineComponent, CallbackProxy);
}

void UCityEngineBatchSubsystem::GenerateAll(UGenerateCompletedCallbackProxy* CallbackProxy)
{
	GetBatchActor()->GenerateAll(CallbackProxy);
}

ACityEngineBatchActor* UCityEngineBatchSubsystem::GetBatchActor()
{
	if (!CityEngineBatchActor)
	{
		const TActorIterator<ACityEngineBatchActor> ActorIterator(GetWorld());

		if (ActorIterator)
		{
			CityEngineBatchActor = *ActorIterator;
		}
		else
		{
			FActorSpawnParameters ActorSpawnParameters;
			ActorSpawnParameters.Name = FName(TEXT("CityEngineBatchActor"));
			CityEngineBatchActor = GetWorld()->SpawnActor<ACityEngineBatchActor>(ActorSpawnParameters);
		}

		for (UCityEngineComponent* CityEngineComponent : RegisteredComponents)
		{
			CityEngineBatchActor->RegisterCityEngineComponent(CityEngineComponent);
		}
	}

	return CityEngineBatchActor;
}

bool UCityEngineBatchSubsystem::HasRegisteredCityEngineComponents() const
{
	return RegisteredComponents.Num() > 0;
}

void UCityEngineBatchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorldSubsystem::Initialize(Collection);

#if WITH_EDITOR
	auto ActorMoved = [this](const AActor* Actor)
	{
		if (UCityEngineComponent* CityEngineComponent = Actor->FindComponentByClass<UCityEngineComponent>())
		{
			if (CityEngineComponent->IsBatchGenerated())
			{
				UnregisterCityEngineComponent(CityEngineComponent);
				RegisterCityEngineComponent(CityEngineComponent);
			}
		}
	};
	
	OnActorMoved = GEngine->OnActorMoved().AddLambda([ActorMoved](AActor* Actor)
	{
		ActorMoved(Actor);
	});

	OnActorsMoved = GEngine->OnActorsMoved().AddLambda([ActorMoved](const TArray<AActor*>& Actors)
	{
		for (const AActor* Actor : Actors)
		{
			ActorMoved(Actor);
		}
	});

	OnActorDeleted = GEngine->OnLevelActorDeleted().AddLambda([this](AActor* Actor)
	{
		if (UCityEngineComponent* CityEngineComponent = Actor->FindComponentByClass<UCityEngineComponent>())
		{
			if (CityEngineComponent->IsBatchGenerated())
			{
				UnregisterCityEngineComponent(CityEngineComponent);
			}
		}
	});
#endif

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;

		if (UCityEngineComponent* CityEngineComponent = Actor->FindComponentByClass<UCityEngineComponent>())
		{
			if (CityEngineComponent->IsBatchGenerated())
			{
				RegisterCityEngineComponent(CityEngineComponent);
			}
		}
	}
}

void UCityEngineBatchSubsystem::Deinitialize()
{
#if WITH_EDITOR
	GEngine->OnActorMoved().Remove(OnActorMoved);
	GEngine->OnActorsMoved().Remove(OnActorsMoved);
	GEngine->OnLevelActorDeleted().Remove(OnActorDeleted);
#endif
	
	UWorldSubsystem::Deinitialize();
}
