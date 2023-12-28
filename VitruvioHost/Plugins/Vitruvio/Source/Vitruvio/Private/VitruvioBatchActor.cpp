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

#include "VitruvioBatchActor.h"

#include "AttributeConversion.h"
#include "GenerateCompletedCallbackProxy.h"

void UTile::MarkDirty(UVitruvioComponent* VitruvioComponent, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	bDirty = true;
	if (CallbackProxy)
	{
		CallbackProxies.Add(VitruvioComponent, CallbackProxy);
	}
}

void UTile::UnmarkDirty()
{
	bDirty = false;
}

void UTile::Add(UVitruvioComponent* VitruvioComponent)
{
	VitruvioComponents.Add(VitruvioComponent);
}

void UTile::Remove(UVitruvioComponent* VitruvioComponent)
{
	VitruvioComponents.Remove(VitruvioComponent);
}

bool UTile::Contains(UVitruvioComponent* VitruvioComponent) const
{
	return VitruvioComponents.Contains(VitruvioComponent);
}

TArray<FInitialShape> UTile::GetInitialShapes()
{
	TArray<FInitialShape> InitialShapes;
	
	for (UVitruvioComponent* VitruvioComponent : VitruvioComponents)
	{
		FInitialShape InitialShape;
		InitialShape.Offset = VitruvioComponent->GetOwner()->GetTransform().GetLocation();
		InitialShape.Polygon = VitruvioComponent->InitialShape->GetPolygon();
		InitialShape.Attributes = Vitruvio::CreateAttributeMap(VitruvioComponent->GetAttributes());
		InitialShape.RandomSeed = VitruvioComponent->GetRandomSeed();
		InitialShape.RulePackage = VitruvioComponent->GetRpk();

		InitialShapes.Emplace(MoveTemp(InitialShape));
	}

	return InitialShapes;
}

const TSet<UVitruvioComponent*>& UTile::GetVitruvioComponents()
{
	return VitruvioComponents;
}

void FGrid::MarkDirty(UVitruvioComponent* VitruvioComponent, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	if (UTile** FoundTile = TilesByComponent.Find(VitruvioComponent))
	{
		UTile* Tile = *FoundTile;
		Tile->MarkDirty(VitruvioComponent, CallbackProxy);
	}
}

void FGrid::RegisterAll(const TSet<UVitruvioComponent*>& VitruvioComponents, AVitruvioBatchActor* VitruvioBatchActor)
{
	for (UVitruvioComponent* VitruvioComponent : VitruvioComponents)
	{
		Register(VitruvioComponent, VitruvioBatchActor);
	} 
}

void FGrid::Register(UVitruvioComponent* VitruvioComponent, AVitruvioBatchActor* VitruvioBatchActor)
{
	const FIntPoint Position = VitruvioBatchActor->GetPosition(VitruvioComponent);
	
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

	if (!Tile->Contains(VitruvioComponent))
	{
		Tile->Add(VitruvioComponent);
		Tile->MarkDirty(VitruvioComponent);
		TilesByComponent.Add(VitruvioComponent, Tile);
	}
}

void FGrid::Unregister(UVitruvioComponent* VitruvioComponent)
{
	if (UTile** FoundTile = TilesByComponent.Find(VitruvioComponent))
	{
		UTile* Tile = *FoundTile;
		Tile->Remove(VitruvioComponent);
		Tile->MarkDirty(VitruvioComponent);
	}
}

void FGrid::Clear()
{
	for (auto& [VitruvioComponent, Tile] : TilesByComponent)
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

TArray<UTile*> FGrid::GetDirtyTiles() const
{
	TArray<UTile*> DirtyTiles;
	for (auto& [Point, Tile] : Tiles)
	{
		if (Tile->bDirty)
		{
			DirtyTiles.Add(Tile);
		}
	}

	return DirtyTiles;
}

void FGrid::UnmarkDirty()
{
	for (auto& [Point, Tile] : Tiles)
	{
		Tile->UnmarkDirty();
	}
}

AVitruvioBatchActor::AVitruvioBatchActor()
{
	SetTickGroup(TG_LastDemotable);
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UMaterial> Opaque(TEXT("Material'/Vitruvio/Materials/M_OpaqueParent.M_OpaqueParent'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Masked(TEXT("Material'/Vitruvio/Materials/M_MaskedParent.M_MaskedParent'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Translucent(TEXT("Material'/Vitruvio/Materials/M_TranslucentParent.M_TranslucentParent'"));
	OpaqueParent = Opaque.Object;
	MaskedParent = Masked.Object;
	TranslucentParent = Translucent.Object;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	bLockLocation = true;
	bActorLabelEditable = false;
#endif // WITH_EDITORONLY_DATA
}

FIntPoint AVitruvioBatchActor::GetPosition(const UVitruvioComponent* VitruvioComponent) const
{
	const FVector Position = VitruvioComponent->GetOwner()->GetTransform().GetLocation();
	const int PositionX = static_cast<int>(FMath::Floor(Position.X / GridDimension.X));
	const int PositionY = static_cast<int>(FMath::Floor(Position.Y / GridDimension.Y));
	return FIntPoint {PositionX, PositionY};
}

void AVitruvioBatchActor::ProcessTiles()
{
	for (UTile* Tile : Grid.GetDirtyTiles())
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
		const TArray<FInitialShape>& InitialShapes = Tile->GetInitialShapes();
		if (!InitialShapes.IsEmpty())
		{
			FBatchGenerateResult GenerateResult = VitruvioModule::Get().BatchGenerateAsync(Tile->GetInitialShapes());

			if (Tile->GenerateToken)
			{
				Tile->GenerateToken->Invalidate();
			}
			
			Tile->GenerateToken = GenerateResult.Token;
		
			// clang-format off
			GenerateResult.Result.Next([this, Tile](const FBatchGenerateResult::ResultType& Result)
			{
				FScopeLock Lock(&Result.Token->Lock);

				if (Result.Token->IsInvalid())
				{
					return;
				}

				Tile->GenerateToken.Reset();

				FScopeLock GenerateQueueLock(&ProcessQueueCriticalSection);
				GenerateQueue.Enqueue({Result.Value, Tile});
			});
			// clang-format on
		}
	}

	Grid.UnmarkDirty();
}

void AVitruvioBatchActor::ProcessGenerateQueue()
{
	ProcessQueueCriticalSection.Lock();
	
	if (!GenerateQueue.IsEmpty())
	{
		FBatchGenerateQueueItem Item;
		GenerateQueue.Dequeue(Item);

		ProcessQueueCriticalSection.Unlock();

		UGeneratedModelStaticMeshComponent* VitruvioModelComponent = Item.Tile->GeneratedModelComponent;

		const FConvertedGenerateResult ConvertedResult =
			BuildResult(Item.GenerateResultDescription, VitruvioModule::Get().GetMaterialCache(), VitruvioModule::Get().GetTextureCache(),
				MaterialIdentifiers, UniqueMaterialIdentifiers, OpaqueParent, MaskedParent, TranslucentParent);

		if (ConvertedResult.ShapeMesh)
		{
			VitruvioModelComponent->SetStaticMesh(ConvertedResult.ShapeMesh->GetStaticMesh());
			VitruvioModelComponent->SetCollisionData(ConvertedResult.ShapeMesh->GetCollisionData());
		}

		// Cleanup old hierarchical instances
		TArray<USceneComponent*> ChildInstanceComponents;
		VitruvioModelComponent->GetChildrenComponents(true, ChildInstanceComponents);
		for (USceneComponent* InstanceComponent : ChildInstanceComponents)
		{
			InstanceComponent->DestroyComponent(true);
		}

		TMap<FString, int32> NameMap;
		TSet<FInstance> Replaced;

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
			InstancedComponent->SetCollisionData(Instance.InstanceMesh->GetCollisionData());
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

		for (auto& [VitruvioComponent, CallbackProxy] : Item.Tile->CallbackProxies)
		{
			CallbackProxy->OnGenerateCompletedBlueprint.Broadcast();
			CallbackProxy->OnGenerateCompleted.Broadcast();
			CallbackProxy->SetReadyToDestroy();
		}

		Item.Tile->CallbackProxies.Empty();
	}
	else
	{
		ProcessQueueCriticalSection.Unlock();
	}
}

void AVitruvioBatchActor::Tick(float DeltaSeconds)
{
	ProcessTiles();
	ProcessGenerateQueue();
}

void AVitruvioBatchActor::RegisterVitruvioComponent(UVitruvioComponent* VitruvioComponent)
{
	VitruvioComponents.Add(VitruvioComponent);
	Grid.Register(VitruvioComponent, this);
}

void AVitruvioBatchActor::UnregisterVitruvioComponent(UVitruvioComponent* VitruvioComponent)
{
	VitruvioComponents.Remove(VitruvioComponent);
	Grid.Unregister(VitruvioComponent);
}

void AVitruvioBatchActor::MarkDirty(UVitruvioComponent* VitruvioComponent, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	Grid.MarkDirty(VitruvioComponent, CallbackProxy);
}

bool AVitruvioBatchActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

#if WITH_EDITOR
void AVitruvioBatchActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty &&
		PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AVitruvioBatchActor, GridDimension))
	{
		Grid.Clear();
		Grid.RegisterAll(VitruvioComponents, this);
	}
}
#endif
