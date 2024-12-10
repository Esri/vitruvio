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

#include "CityEngineBatchActor.h"

#include "AttributeConversion.h"
#include "Materials/Material.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "GenerateCompletedCallbackProxy.h"
#include "PhysicsEngine/BodySetup.h"

void UTile::MarkForGenerate(UCityEngineComponent* CityEngineComponent, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	bMarkedForGenerate = true;
	if (CallbackProxy)
	{
		CallbackProxies.Add(CityEngineComponent, CallbackProxy);
	}
}

void UTile::UnmarkForGenerate()
{
	bMarkedForGenerate = false;
}

void UTile::Add(UCityEngineComponent* CityEngineComponent)
{
	CityEngineComponents.Add(CityEngineComponent);
}

void UTile::Remove(UCityEngineComponent* CityEngineComponent)
{
	CityEngineComponents.Remove(CityEngineComponent);
}

bool UTile::Contains(UCityEngineComponent* CityEngineComponent) const
{
	return CityEngineComponents.Contains(CityEngineComponent);
}

TTuple<TArray<FInitialShape>, TArray<UCityEngineComponent*>> UTile::GetInitialShapes()
{
	TArray<FInitialShape> InitialShapes;
	TArray<UCityEngineComponent*> ValidCityEngineComponents;
	
	for (UCityEngineComponent* CityEngineComponent : CityEngineComponents)
	{
		if (!CityEngineComponent->GetRpk())
		{
			continue;
		}

		ValidCityEngineComponents.Add(CityEngineComponent);
		
		FInitialShape InitialShape;
		InitialShape.Offset = CityEngineComponent->GetOwner()->GetTransform().GetLocation();
		InitialShape.Polygon = CityEngineComponent->InitialShape->GetPolygon();
		InitialShape.Attributes = CityEngine::CreateAttributeMap(CityEngineComponent->GetAttributes());
		InitialShape.RandomSeed = CityEngineComponent->GetRandomSeed();
		InitialShape.RulePackage = CityEngineComponent->GetRpk();

		InitialShapes.Emplace(MoveTemp(InitialShape));
	}

	return MakeTuple(MoveTemp(InitialShapes), ValidCityEngineComponents);
}

void FGrid::MarkForGenerate(UCityEngineComponent* CityEngineComponent, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	if (UTile** FoundTile = TilesByComponent.Find(CityEngineComponent))
	{
		UTile* Tile = *FoundTile;
		Tile->MarkForGenerate(CityEngineComponent, CallbackProxy);
	}
}

void FGrid::MarkAllForGenerate()
{
	for (const auto& [Component, Tile] : TilesByComponent)
	{
		Tile->MarkForGenerate(Component);
	}
}

void FGrid::RegisterAll(const TSet<UCityEngineComponent*>& CityEngineComponents, ACityEngineBatchActor* CityEngineBatchActor)
{
	for (UCityEngineComponent* CityEngineComponent : CityEngineComponents)
	{
		Register(CityEngineComponent, CityEngineBatchActor);
	} 
}

void FGrid::Register(UCityEngineComponent* CityEngineComponent, ACityEngineBatchActor* CityEngineBatchActor)
{
	const FIntPoint Position = CityEngineBatchActor->GetPosition(CityEngineComponent);
	
	UTile* Tile;
	if (UTile** TileResult = Tiles.Find(Position))
	{
		Tile = *TileResult;
	}
	else
	{
		Tile = NewObject<UTile>();
		Tile->Location = Position;
		Tiles.Add(Position, Tile);
	}

	if (!Tile->Contains(CityEngineComponent))
	{
		Tile->Add(CityEngineComponent);
		Tile->MarkForGenerate(CityEngineComponent);
		TilesByComponent.Add(CityEngineComponent, Tile);
	}
}

void FGrid::Unregister(UCityEngineComponent* CityEngineComponent)
{
	if (UTile** FoundTile = TilesByComponent.Find(CityEngineComponent))
	{
		UTile* Tile = *FoundTile;

		if (Tile->GenerateToken)
		{
			Tile->GenerateToken->Invalidate();
			Tile->GenerateToken.Reset();
		}
		
		Tile->Remove(CityEngineComponent);
		Tile->MarkForGenerate(CityEngineComponent);
	}
}

void FGrid::Clear()
{
	for (auto& [CityEngineComponent, Tile] : TilesByComponent)
	{
		if (Tile->GeneratedModelComponent && IsValid(Tile->GeneratedModelComponent))
		{
			TArray<USceneComponent*> InstanceSceneComponents;
			Tile->GeneratedModelComponent->GetChildrenComponents(true, InstanceSceneComponents);
			for (USceneComponent* InstanceComponent : InstanceSceneComponents)
			{
				InstanceComponent->DestroyComponent(true);
			}
			
			Tile->GeneratedModelComponent->DestroyComponent(true);
		}
	}

	TilesByComponent.Reset();
	Tiles.Reset();
}

TArray<UTile*> FGrid::GetTilesMarkedForGenerate() const
{
	TArray<UTile*> TilesToGenerate;
	for (auto& [Point, Tile] : Tiles)
	{
		if (Tile->bMarkedForGenerate)
		{
			TilesToGenerate.Add(Tile);
		}
	}

	return TilesToGenerate;
}

void FGrid::UnmarkForGenerate()
{
	for (auto& [Point, Tile] : Tiles)
	{
		Tile->UnmarkForGenerate();
	}
}

ACityEngineBatchActor::ACityEngineBatchActor()
{
	SetTickGroup(TG_LastDemotable);
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UMaterial> Opaque(TEXT("Material'/CityEnginePlugin/Materials/M_OpaqueParent.M_OpaqueParent'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Masked(TEXT("Material'/CityEnginePlugin/Materials/M_MaskedParent.M_MaskedParent'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Translucent(TEXT("Material'/CityEnginePlugin/Materials/M_TranslucentParent.M_TranslucentParent'"));
	OpaqueParent = Opaque.Object;
	MaskedParent = Masked.Object;
	TranslucentParent = Translucent.Object;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	bLockLocation = true;
	bActorLabelEditable = false;
#endif // WITH_EDITORONLY_DATA
}

FIntPoint ACityEngineBatchActor::GetPosition(const UCityEngineComponent* CityEngineComponent) const
{
	const FVector Position = CityEngineComponent->GetOwner()->GetTransform().GetLocation();
	const int PositionX = static_cast<int>(FMath::Floor(Position.X / GridDimension.X));
	const int PositionY = static_cast<int>(FMath::Floor(Position.Y / GridDimension.Y));
	return FIntPoint {PositionX, PositionY};
}

void ACityEngineBatchActor::ProcessTiles()
{
	for (UTile* Tile : Grid.GetTilesMarkedForGenerate())
	{
		// Initialize and cleanup the model component
		UGeneratedModelStaticMeshComponent* VitruvioModelComponent = Tile->GeneratedModelComponent;
		if (VitruvioModelComponent)
		{
			VitruvioModelComponent->SetStaticMesh(nullptr);

			// Cleanup old hierarchical instances
			TArray<USceneComponent*> InstanceSceneComponents;
			VitruvioModelComponent->GetChildrenComponents(true, InstanceSceneComponents);
			for (USceneComponent* InstanceComponent : InstanceSceneComponents)
			{
				InstanceComponent->DestroyComponent(true);
			}
		}
		else
		{
			const FString TileName = FString::FromInt(NumModelComponents++);
			VitruvioModelComponent = NewObject<UGeneratedModelStaticMeshComponent>(RootComponent, FName(TEXT("GeneratedModel") + TileName),
																			   RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
			VitruvioModelComponent->CreationMethod = EComponentCreationMethod::Instance;
			RootComponent->GetOwner()->AddOwnedComponent(VitruvioModelComponent);
			VitruvioModelComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			VitruvioModelComponent->OnComponentCreated();
			VitruvioModelComponent->RegisterComponent();

			Tile->GeneratedModelComponent = VitruvioModelComponent;
		}

		// Generate model
		auto [InitialShapes, InitialShapeCityEngineComponents] = Tile->GetInitialShapes();
		if (!InitialShapes.IsEmpty())
		{
			if (Tile->GenerateToken)
			{
				Tile->GenerateToken->Invalidate();
			}
			
			FBatchGenerateResult GenerateResult = FCityEngineModule::Get().BatchGenerateAsync(MoveTemp(InitialShapes));
			
			Tile->GenerateToken = GenerateResult.Token;
			Tile->bIsGenerating = true;
		
			// clang-format off
			GenerateResult.Result.Next([this, Tile, InitialShapeCityEngineComponents](const FBatchGenerateResult::ResultType& Result)
			{
				FScopeLock Lock(&Result.Token->Lock);

				if (Result.Token->IsInvalid())
				{
					return;
				}

				Tile->GenerateToken.Reset();

				FScopeLock GenerateQueueLock(&ProcessQueueCriticalSection);
				GenerateQueue.Enqueue({Result.Value, Tile, InitialShapeCityEngineComponents});
			});
			// clang-format on
		}
	}

	Grid.UnmarkForGenerate();
}

void ACityEngineBatchActor::ProcessGenerateQueue()
{
	ProcessQueueCriticalSection.Lock();
	
	if (!GenerateQueue.IsEmpty())
	{
		FBatchGenerateQueueItem Item;
		GenerateQueue.Dequeue(Item);

		ProcessQueueCriticalSection.Unlock();

		for (int ComponentIndex = 0; ComponentIndex < Item.CityEngineComponents.Num(); ++ComponentIndex)
		{
			UCityEngineComponent* CityEngineComponent = Item.CityEngineComponents[ComponentIndex];
			Item.GenerateResultDescription.EvaluatedAttributes[ComponentIndex]->UpdateUnrealAttributeMap(CityEngineComponent->Attributes, CityEngineComponent);
			CityEngineComponent->NotifyAttributesChanged();
		}

		UGeneratedModelStaticMeshComponent* VitruvioModelComponent = Item.Tile->GeneratedModelComponent;

		const FConvertedGenerateResult ConvertedResult = BuildGenerateResult(Item.GenerateResultDescription,
	FCityEngineModule::Get().GetMaterialCache(), FCityEngineModule::Get().GetTextureCache(),
				MaterialIdentifiers, UniqueMaterialIdentifiers, OpaqueParent, MaskedParent, TranslucentParent, GetWorld());

		if (ConvertedResult.ShapeMesh)
		{
			VitruvioModelComponent->SetStaticMesh(ConvertedResult.ShapeMesh->GetStaticMesh());
			
			// Reset Material replacements
			for (int32 MaterialIndex = 0; MaterialIndex < VitruvioModelComponent->GetNumMaterials(); ++MaterialIndex)
			{
				VitruvioModelComponent->SetMaterial(MaterialIndex, VitruvioModelComponent->GetStaticMesh()->GetMaterial(MaterialIndex));
			}

			ApplyMaterialReplacements(VitruvioModelComponent, MaterialIdentifiers, MaterialReplacement);
		}

		// Cleanup old hierarchical instances
		TArray<USceneComponent*> ChildInstanceComponents;
		VitruvioModelComponent->GetChildrenComponents(true, ChildInstanceComponents);
		for (USceneComponent* InstanceComponent : ChildInstanceComponents)
		{
			InstanceComponent->DestroyComponent(true);
		}

		TMap<FString, int32> NameMap;
		TSet<FInstance> Replaced = ApplyInstanceReplacements(VitruvioModelComponent, ConvertedResult.Instances, InstanceReplacement, NameMap);
		for (const FInstance& Instance : ConvertedResult.Instances)
		{
			if (Replaced.Contains(Instance))
			{
				continue;
			}

			FString UniqueName = UniqueComponentName(Instance.Name, NameMap);
			auto InstancedComponent = NewObject<UGeneratedModelHISMComponent>(VitruvioModelComponent, FName(UniqueName),
																			  RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
			const TArray<FTransform>& Transforms = Instance.Transforms;
			InstancedComponent->SetStaticMesh(Instance.InstanceMesh->GetStaticMesh());
			InstancedComponent->SetMeshIdentifier(Instance.InstanceMesh->GetIdentifier());
			
			// Add all instance transforms
			for (const FTransform& Transform : Transforms)
			{
				InstancedComponent->AddInstance(Transform);
			}

			// Apply override materials
			for (int32 MaterialIndex = 0; MaterialIndex < Instance.OverrideMaterials.Num(); ++MaterialIndex)
			{
				InstancedComponent->SetMaterial(MaterialIndex, Instance.OverrideMaterials[MaterialIndex]);
			}

			// Attach and register instance component
			InstancedComponent->AttachToComponent(VitruvioModelComponent, FAttachmentTransformRules::KeepRelativeTransform);
			InstancedComponent->CreationMethod = EComponentCreationMethod::Instance;
			RootComponent->GetOwner()->AddOwnedComponent(InstancedComponent);
			InstancedComponent->OnComponentCreated();
			InstancedComponent->RegisterComponent();
		}

		for (auto& [CityEngineComponent, CallbackProxy] : Item.Tile->CallbackProxies)
		{
			CallbackProxy->OnGenerateCompletedBlueprint.Broadcast();
			CallbackProxy->OnGenerateCompleted.Broadcast();
			CallbackProxy->SetReadyToDestroy();
		}

		Item.Tile->CallbackProxies.Empty();
		Item.Tile->bIsGenerating = false;
	}
	else
	{
		ProcessQueueCriticalSection.Unlock();
	}

	if (GenerateAllCallbackProxy)
	{
		TArray<UTile*> Tiles;
		Grid.Tiles.GenerateValueArray(Tiles);
		bool bAllGenerated = Algo::NoneOf(Tiles, [](const UTile* Tile) { return Tile->bIsGenerating; });
		if (bAllGenerated)
		{
			GenerateAllCallbackProxy->OnGenerateCompleted.Broadcast();
			GenerateAllCallbackProxy = nullptr;
		}
	}
}

void ACityEngineBatchActor::Tick(float DeltaSeconds)
{
	ProcessTiles();
	ProcessGenerateQueue();
}

void ACityEngineBatchActor::RegisterCityEngineComponent(UCityEngineComponent* CityEngineComponent)
{
	CityEngineComponents.Add(CityEngineComponent);
	Grid.Register(CityEngineComponent, this);
}

void ACityEngineBatchActor::UnregisterCityEngineComponent(UCityEngineComponent* CityEngineComponent)
{
	CityEngineComponents.Remove(CityEngineComponent);
	Grid.Unregister(CityEngineComponent);
}

void ACityEngineBatchActor::UnregisterAllCityEngineComponents()
{
	Grid.Clear();
	CityEngineComponents.Empty();
}

TSet<UCityEngineComponent*> ACityEngineBatchActor::GetCityEngineComponents()
{
	return CityEngineComponents;
}

void ACityEngineBatchActor::Generate(UCityEngineComponent* CityEngineComponent, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	Grid.MarkForGenerate(CityEngineComponent, CallbackProxy);
}

void ACityEngineBatchActor::GenerateAll(UGenerateCompletedCallbackProxy* CallbackProxy)
{
	GenerateAllCallbackProxy = CallbackProxy;
	Grid.MarkAllForGenerate();
}

bool ACityEngineBatchActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ACityEngineBatchActor::SetMaterialReplacementAsset(UMaterialReplacementAsset* MaterialReplacementAsset)
{
	MaterialReplacement = MaterialReplacementAsset;
	GenerateAll();
}

void ACityEngineBatchActor::SetInstanceReplacementAsset(UInstanceReplacementAsset* InstanceReplacementAsset)
{
	InstanceReplacement = InstanceReplacementAsset;
	GenerateAll();
}

#if WITH_EDITOR
bool ACityEngineBatchActor::CanDeleteSelectedActor(FText& OutReason) const
{
	return false;
}

void ACityEngineBatchActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty &&
		PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ACityEngineBatchActor, GridDimension))
	{
		Grid.Clear();
		Grid.RegisterAll(CityEngineComponents, this);
	}

	if (!PropertyChangedEvent.Property)
	{
		return;
	}
	
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UCityEngineComponent, MaterialReplacement) ||
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UCityEngineComponent, InstanceReplacement))
	{
		GenerateAll();
	}
}
#endif
