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

#include "CityEngineActorCooker.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Dialogs/DlgPickPath.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "GeneratedModelHISMComponent.h"
#include "GeneratedModelStaticMeshComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "CityEngineBatchActor.h"

#include "Misc/ScopedSlowTask.h"
#include "CityEngineComponent.h"
#include "CityEngineEditorModule.h"
#include "CityEngineModule.h"

namespace
{

using FMaterialCache = TMap<UMaterialInstance*, UMaterialInstanceConstant*>;
using FTextureCache = TMap<UTexture*, UTexture2D*>;
using FStaticMeshCache = TMap<UStaticMesh*, UStaticMesh*>;

std::atomic<bool> IsCooking;

template <typename T>
T* AttachMeshComponent(AActor* Parent, USceneComponent* AttachParent, UStaticMesh* Mesh, const FName& Name, const FTransform& Transform)
{
	T* NewStaticMeshComponent = NewObject<T>(Parent, Name);
	NewStaticMeshComponent->Mobility = EComponentMobility::Movable;
	NewStaticMeshComponent->SetStaticMesh(Mesh);
	NewStaticMeshComponent->SetWorldTransform(Transform);
	Parent->AddInstanceComponent(NewStaticMeshComponent);
	NewStaticMeshComponent->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepWorldTransform);
	NewStaticMeshComponent->OnComponentCreated();
	NewStaticMeshComponent->RegisterComponent();
	return NewStaticMeshComponent;
}

template <typename T>
T* AttachMeshComponent(AActor* Parent, UStaticMesh* Mesh, const FName& Name, const FTransform& Transform)
{
	return AttachMeshComponent<T>(Parent, Parent->GetRootComponent(), Mesh, Name, Transform);
}

UPackage* CreateUniquePackage(const FString& InputName, FString& OutAssetName)
{
	FString PackageName;
	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(InputName, TEXT(""), PackageName, OutAssetName);
	UPackage* Package = CreatePackage(*PackageName);
	return Package;
}

void BlockUntilCookCompleted()
{
	FScopedSlowTask PRTGenerateCallsTasks(0, FText::FromString("Finishing previous CityEngineActor cooking..."));
	while (IsCooking.load())
	{
		FPlatformProcess::Sleep(0); // SwitchToThread
		int32 CurrentNumGenerateCalls = FCityEngineModule::Get().GetNumGenerateCalls();
		PRTGenerateCallsTasks.EnterProgressFrame(0);
	}
}

ETextureSourceFormat GetTextureFormatFromPixelFormat(const EPixelFormat PixelFormat)
{
	switch (PixelFormat)
	{
	case PF_B8G8R8A8:
	{
		return ETextureSourceFormat::TSF_BGRA8;
	}
	case PF_A16B16G16R16:
	{
		return ETextureSourceFormat::TSF_RGBA16;
	}
	case PF_FloatRGBA:
	{
		return ETextureSourceFormat::TSF_RGBA16F;
	}
	default:;
		return ETextureSourceFormat::TSF_Invalid;
	}
}

UTexture2D* SaveTexture(UTexture2D* Original, const FString& Path, FTextureCache& TextureCache)
{
	if (TextureCache.Contains(Original))
	{
		return TextureCache[Original];
	}
	FString AssetName;
	UPackage* TexturePackage = CreateUniquePackage(FPaths::Combine(Path, TEXT("Textures"), Original->GetName()), AssetName);
	UTexture2D* NewTexture = NewObject<UTexture2D>(TexturePackage, *AssetName, RF_Public | RF_Standalone);
	NewTexture->CompressionSettings = Original->CompressionSettings;
	NewTexture->SRGB = Original->SRGB;

	FTexturePlatformData* PlatformData = new FTexturePlatformData();
	const FTexturePlatformData* OriginalPlatformData = Original->GetPlatformData();
	PlatformData->SizeX = OriginalPlatformData->SizeX;
	PlatformData->SizeY = OriginalPlatformData->SizeY;
	PlatformData->PixelFormat = OriginalPlatformData->PixelFormat;

	// Allocate first mipmap and upload the pixel data
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	const FTexture2DMipMap& OriginalMip = OriginalPlatformData->Mips[0];

	PlatformData->Mips.Add(Mip);

	Mip->SizeX = OriginalMip.SizeX;
	Mip->SizeY = OriginalMip.SizeY;

	NewTexture->SetPlatformData(PlatformData);

	const uint8* SourcePixels = static_cast<const uint8*>(OriginalMip.BulkData.LockReadOnly());
	Mip->BulkData.Lock(LOCK_READ_WRITE);

	void* TextureData = Mip->BulkData.Realloc(OriginalMip.BulkData.GetBulkDataSize());
	FMemory::Memcpy(TextureData, SourcePixels, OriginalMip.BulkData.GetBulkDataSize());
	Mip->BulkData.Unlock();

	const ETextureSourceFormat SourceFormat = GetTextureFormatFromPixelFormat(OriginalPlatformData->PixelFormat);
	NewTexture->Source.Init(OriginalPlatformData->SizeX, OriginalPlatformData->SizeY, 1, 1, SourceFormat, SourcePixels);
	OriginalMip.BulkData.Unlock();

	NewTexture->PostEditChange();
	TexturePackage->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	TextureCache.Add(Original, NewTexture);

	return NewTexture;
}

UMaterialInstanceConstant* SaveMaterial(UMaterialInstance* Material, const FString& Path, FMaterialCache& MaterialCache,
										FTextureCache& TextureCache)
{
	if (MaterialCache.Contains(Material))
	{
		return MaterialCache[Material];
	}

	FString AssetName;
	UPackage* MaterialPackage = CreateUniquePackage(FPaths::Combine(Path, TEXT("Materials"), Material->GetName()), AssetName);

	UMaterialInstanceConstantFactoryNew* MaterialFactory = NewObject<UMaterialInstanceConstantFactoryNew>();
	MaterialFactory->InitialParent = Material->Parent;

	UMaterialInstanceConstant* NewMaterial = Cast<UMaterialInstanceConstant>(MaterialFactory->FactoryCreateNew(
		UMaterialInstanceConstant::StaticClass(), MaterialPackage, *AssetName, RF_Public | RF_Standalone, nullptr, GWarn));
	FAssetRegistryModule::AssetCreated(NewMaterial);

	TArray<FMaterialParameterInfo> ParameterInfos;
	TArray<FGuid> ParameterIds;

	Material->GetAllScalarParameterInfo(ParameterInfos, ParameterIds);
	for (const FMaterialParameterInfo& Info : ParameterInfos)
	{
		float Value;
		Material->GetScalarParameterValue(Info, Value);
		NewMaterial->SetScalarParameterValueEditorOnly(Info, Value);
	}

	Material->GetAllTextureParameterInfo(ParameterInfos, ParameterIds);
	for (const FMaterialParameterInfo& Info : ParameterInfos)
	{
		UTexture* Value;
		Material->GetTextureParameterValue(Info, Value);
		if (Value)
		{
			if (Value->HasAnyFlags(RF_Transient))
			{
				UTexture2D* PersistedTexture = SaveTexture(Cast<UTexture2D>(Value), Path, TextureCache);
				NewMaterial->SetTextureParameterValueEditorOnly(Info, PersistedTexture);
			}
			else
			{
				NewMaterial->SetTextureParameterValueEditorOnly(Info, Value);
			}
		}
	}

	Material->GetAllVectorParameterInfo(ParameterInfos, ParameterIds);
	for (const FMaterialParameterInfo& Info : ParameterInfos)
	{
		FLinearColor Value;
		Material->GetVectorParameterValue(Info, Value);
		NewMaterial->SetVectorParameterValueEditorOnly(Info, Value);
	}

	MaterialCache.Add(Material, NewMaterial);

	NewMaterial->PostEditChange();
	MaterialPackage->MarkPackageDirty();

	return NewMaterial;
}

UStaticMesh* SaveStaticMesh(UStaticMesh* Mesh, const FString& Path, FStaticMeshCache& MeshCache, FMaterialCache& MaterialCache,
							FTextureCache& TextureCache)
{
	if (MeshCache.Contains(Mesh))
	{
		return MeshCache[Mesh];
	}

	// Create new StaticMesh Asset
	FString AssetName;
	UPackage* MeshPackage = CreateUniquePackage(FPaths::Combine(Path, TEXT("Geometry"), Mesh->GetName()), AssetName);
	UStaticMesh* PersistedMesh = NewObject<UStaticMesh>(MeshPackage, *AssetName, RF_Public | RF_Standalone);
	PersistedMesh->InitResources();

	FMeshDescription* OriginalMeshDescription = Mesh->GetMeshDescription(0);
	FMeshDescription NewMeshDescription(*OriginalMeshDescription);
	FStaticMeshAttributes MeshAttributes(NewMeshDescription);

	// Copy Materials
	TMap<UMaterialInterface*, FName> MaterialSlots;

	const auto PolygonGroups = NewMeshDescription.PolygonGroups();
	for (const auto& PolygonGroupId : PolygonGroups.GetElementIDs())
	{
		const FName MaterialName = MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId];
		int32 Index = Mesh->GetMaterialIndex(MaterialName);

		if (Index != INDEX_NONE)
		{
			UMaterialInterface* Material = Mesh->GetMaterial(Index);
			if (UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material))
			{
				Material = SaveMaterial(MaterialInstance, Path, MaterialCache, TextureCache);
			}
			
			const auto MaterialResult = MaterialSlots.Find(Material);
			if (MaterialResult)
			{
				MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = *MaterialResult;
			}
			else
			{
				FName NewSlot = PersistedMesh->AddMaterial(Material);
				MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = NewSlot;
				MaterialSlots.Add(Material, NewSlot);
			}
		}
	}

	// Build the Static Mesh
	TArray<const FMeshDescription*> MeshDescriptions;
	MeshDescriptions.Add(&NewMeshDescription);
	PersistedMesh->BuildFromMeshDescriptions(MeshDescriptions);

	check(PersistedMesh->GetNumSourceModels() == 1);
	FStaticMeshSourceModel& SrcModel = PersistedMesh->GetSourceModel(0);
	SrcModel.BuildSettings.bRecomputeNormals = false;
	SrcModel.BuildSettings.bRecomputeTangents = false;
	SrcModel.BuildSettings.bRemoveDegenerates = true;
	PersistedMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;

	PersistedMesh->PostEditChange();
	PersistedMesh->MarkPackageDirty();

	// Notify asset registry of new asset
	FAssetRegistryModule::AssetCreated(PersistedMesh);

	MeshCache.Add(Mesh, PersistedMesh);
	return PersistedMesh;
}

void CookActors(const TArray<AActor*>& Actors, const FString& CookPath)
{
	FScopedSlowTask CookTask(Actors.Num(), FText::FromString("Cooking models..."));
	CookTask.MakeDialog();

	FMaterialCache MaterialCache;
	FTextureCache TextureCache;
	FStaticMeshCache MeshCache;

	for (AActor* Actor : Actors)
	{
		CookTask.EnterProgressFrame(1);

		UCityEngineComponent* CityEngineComponent = Actor->FindComponentByClass<UCityEngineComponent>();
		ACityEngineBatchActor* CityEngineBatchActor = Cast<ACityEngineBatchActor>(Actor);
		if (!CityEngineComponent && !CityEngineBatchActor)
		{
			continue;
		}
		
		if (CityEngineComponent && CityEngineComponent->IsBatchGenerated())
		{
			continue;
		}

		AActor* OldAttachParent = Actor->GetAttachParentActor();

		// Spawn new Actor with persisted geometry
		AActor* CookedActor = Actor->GetWorld()->SpawnActor<AActor>(Actor->GetActorLocation(), Actor->GetActorRotation());

		USceneComponent* RootComponent = NewObject<USceneComponent>(CookedActor, "Root");
		RootComponent->SetMobility(EComponentMobility::Movable);
		CookedActor->SetRootComponent(RootComponent);
		CookedActor->AddOwnedComponent(RootComponent);

		RootComponent->SetWorldRotation(Actor->GetActorRotation());
		RootComponent->SetWorldLocation(Actor->GetActorLocation());
		RootComponent->OnComponentCreated();
		RootComponent->RegisterComponent();

		if (OldAttachParent)
		{
			CookedActor->AttachToActor(OldAttachParent, FAttachmentTransformRules::KeepWorldTransform);
		}

		// Persist Generated Models
		TArray<UGeneratedModelStaticMeshComponent*> GeneratedModelComponents;
		Actor->GetComponents<UGeneratedModelStaticMeshComponent>(GeneratedModelComponents);
		for (UGeneratedModelStaticMeshComponent* GeneratedModelStaticMeshComponent : GeneratedModelComponents)
		{
			if (!GeneratedModelStaticMeshComponent || !GeneratedModelStaticMeshComponent->GetStaticMesh())
			{
				continue;
			}

			auto CookOverrideMaterials = [CookPath, &MaterialCache, &TextureCache](UStaticMeshComponent* OriginalMeshComponent, UStaticMeshComponent* CookedMeshComponent)
			{
				for (int32 MaterialIndex = 0; MaterialIndex < OriginalMeshComponent->GetNumOverrideMaterials(); ++MaterialIndex)
				{
					UMaterialInterface* Material = OriginalMeshComponent->OverrideMaterials[MaterialIndex];
					if (UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material))
					{
						Material = SaveMaterial(MaterialInstance, CookPath, MaterialCache, TextureCache);
					}
					CookedMeshComponent->SetMaterial(MaterialIndex, Material);
				}
			};
			UStaticMesh* PersistedMesh = SaveStaticMesh(GeneratedModelStaticMeshComponent->GetStaticMesh(), CookPath, MeshCache, MaterialCache, TextureCache);
			UStaticMeshComponent* CookedMeshComponent = AttachMeshComponent<UStaticMeshComponent>(CookedActor, PersistedMesh, GeneratedModelStaticMeshComponent->GetFName(), GeneratedModelStaticMeshComponent->GetComponentTransform());

			CookOverrideMaterials(GeneratedModelStaticMeshComponent, CookedMeshComponent);
			
			// Persist instanced Components
			TArray<TObjectPtr<USceneComponent>> AttachedComponents = GeneratedModelStaticMeshComponent->GetAttachChildren();
			for (UActorComponent* ActorComponent : AttachedComponents)
			{
				UGeneratedModelHISMComponent* GeneratedModelHismComponent = Cast<UGeneratedModelHISMComponent>(ActorComponent);

				if (!GeneratedModelHismComponent)
				{
					continue;
				}

				if (UStaticMesh* InstanceMesh = GeneratedModelHismComponent->GetStaticMesh())
				{
					UHierarchicalInstancedStaticMeshComponent* InstancedStaticMeshComponent;
					
					if (InstanceMesh->GetOutermost() == GetTransientPackage())
					{
						UStaticMesh* PersistedInstanceMesh = SaveStaticMesh(InstanceMesh, CookPath, MeshCache, MaterialCache, TextureCache);

						FName Name = MakeUniqueObjectName(CookedActor, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *PersistedInstanceMesh->GetName());
						InstancedStaticMeshComponent = AttachMeshComponent<UHierarchicalInstancedStaticMeshComponent>(CookedActor, CookedMeshComponent, PersistedInstanceMesh, Name, GeneratedModelHismComponent->GetComponentTransform());

						CookOverrideMaterials(GeneratedModelHismComponent, InstancedStaticMeshComponent);
					}
					else
					{
						FName Name = MakeUniqueObjectName(CookedActor, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *InstanceMesh->GetName());
						InstancedStaticMeshComponent = AttachMeshComponent<UHierarchicalInstancedStaticMeshComponent>(CookedActor, CookedMeshComponent, InstanceMesh, Name, GeneratedModelHismComponent->GetComponentTransform());
					}
					
					for (int32 InstanceIndex = 0; InstanceIndex < GeneratedModelHismComponent->GetInstanceCount(); ++InstanceIndex)
					{
						FTransform Transform;
						GeneratedModelHismComponent->GetInstanceTransform(InstanceIndex, Transform);
						InstancedStaticMeshComponent->AddInstance(Transform);
					}
				}
			}
		}
		
		FString OldActorLabel = Actor->GetActorLabel();

		// Destroy the old procedural CityEngine Actors
		if (CityEngineBatchActor)
		{
			TSet<UCityEngineComponent*> BatchedCityEngineComponents = CityEngineBatchActor->GetCityEngineComponents();
			CityEngineBatchActor->UnregisterAllCityEngineComponents();
			for (UCityEngineComponent* BatchedCityEngineComponent : BatchedCityEngineComponents)
			{
				BatchedCityEngineComponent->GetOwner()->Destroy();
			}
		}
		else
		{
			Actor->Destroy();
		}
		

		CookedActor->SetActorLabel(OldActorLabel);

		GEditor->SelectActor(CookedActor, true, false);
	}
}

} // namespace

void CookCityEngineActors(TArray<AActor*> Actors)
{
	// If there is a previous cooking already ongoing we have to wait until it has completed. This could happen because the last part
	// of the cooking process is asynchronous.
	BlockUntilCookCompleted();

	IsCooking = true;

	// Wait until all ongoing generate calls to PRT have finished (might happen if we try to cook before all models of a scene
	// have been generated).
	FCityEngineEditorModule::Get().BlockUntilGenerated();

	// Regenerate the selected Actors to make sure we have a model to cook.
	TArray<AActor*> ActorsToGenerate = Actors.FilterByPredicate([](const AActor* Actor)
	{
		return Actor->FindComponentByClass<UCityEngineComponent>() || Cast<ACityEngineBatchActor>(Actor);
	});

	// Cook actors after all models have been generated and their meshes constructed
	UGenerateCompletedCallbackProxy* CallbackProxy = NewObject<UGenerateCompletedCallbackProxy>();
	CallbackProxy->RegisterWithGameInstance(ActorsToGenerate[0]);
	CallbackProxy->OnGenerateCompleted.AddLambda(FExecuteAfterCountdown(ActorsToGenerate.Num(), [ActorsToGenerate]() {

		const TSharedRef<SDlgPickPath> PickContentPathDlg = SNew(SDlgPickPath).Title(FText::FromString("Choose location for cooked models."));

		if (PickContentPathDlg->ShowModal() == EAppReturnType::Cancel)
		{
			IsCooking = false;
			return;
		}
		const FString CookPath = PickContentPathDlg->GetPath().ToString();

		CookActors(ActorsToGenerate, CookPath);

		IsCooking = false;
	}));

	for (AActor* Actor : ActorsToGenerate)
	{
		if (UCityEngineComponent* CityEngineComponent = Actor->FindComponentByClass<UCityEngineComponent>())
		{
			CityEngineComponent->Generate(CallbackProxy);
		}
		else if (ACityEngineBatchActor* CityEngineBatchActor = Cast<ACityEngineBatchActor>(Actor))
		{
			CityEngineBatchActor->GenerateAll(CallbackProxy);
		}
	}
}