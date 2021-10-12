#include "VitruvioCooker.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Dialogs/DlgPickPath.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "GeneratedModelHISMComponent.h"
#include "GeneratedModelStaticMeshComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "VitruvioComponent.h"
#include "VitruvioModule.h"

namespace
{

using FMaterialCache = TMap<UMaterialInstanceDynamic*, UMaterialInstanceConstant*>;
using FTextureCache = TMap<UTexture*, UTexture2D*>;
using FStaticMeshCache = TMap<UStaticMesh*, UStaticMesh*>;

FDelegateHandle ModelsGeneratedHandle;
std::atomic<bool> IsCooking;

template <typename T>
T* AttachMeshComponent(AActor* Parent, UStaticMesh* Mesh, const FName& Name, const FTransform& Transform)
{
	T* NewStaticMeshComponent = NewObject<T>(Parent, Name);
	NewStaticMeshComponent->Mobility = EComponentMobility::Movable;
	NewStaticMeshComponent->SetStaticMesh(Mesh);
	NewStaticMeshComponent->SetWorldTransform(Transform);
	Parent->AddInstanceComponent(NewStaticMeshComponent);
	NewStaticMeshComponent->AttachToComponent(Parent->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	NewStaticMeshComponent->OnComponentCreated();
	NewStaticMeshComponent->RegisterComponent();
	return NewStaticMeshComponent;
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
	FScopedSlowTask PRTGenerateCallsTasks(0, FText::FromString("Finishing previous Vitruvio cooking..."));
	while (IsCooking.load())
	{
		FPlatformProcess::Sleep(0); // SwitchToThread
		int32 CurrentNumGenerateCalls = VitruvioModule::Get().GetNumGenerateCalls();
		PRTGenerateCallsTasks.EnterProgressFrame(0);
	}
}

void BlockUntilGenerated()
{
	// Wait until all async generate calls to PRT are finished. We want to block the UI and show a modal progress bar.
	int32 TotalGenerateCalls = VitruvioModule::Get().GetNumGenerateCalls();
	FScopedSlowTask PRTGenerateCallsTasks(TotalGenerateCalls, FText::FromString("Generating models..."));
	PRTGenerateCallsTasks.MakeDialog();
	while (VitruvioModule::Get().IsGenerating() || VitruvioModule::Get().IsLoadingRpks())
	{
		FPlatformProcess::Sleep(0); // SwitchToThread
		int32 CurrentNumGenerateCalls = VitruvioModule::Get().GetNumGenerateCalls();
		PRTGenerateCallsTasks.EnterProgressFrame(TotalGenerateCalls - CurrentNumGenerateCalls);
		TotalGenerateCalls = CurrentNumGenerateCalls;
	}
}

UTexture2D* SaveTexture(UTexture2D* Original, const FString& Path, FTextureCache& TextureCache)
{
	if (TextureCache.Contains(Original))
	{
		return TextureCache[Original];
	}
	FString AssetName;
	UPackage* TexturePackage = CreateUniquePackage(FPaths::Combine(Path, TEXT("Textures")), AssetName);
	UTexture2D* NewTexture = NewObject<UTexture2D>(TexturePackage, *AssetName, RF_Public | RF_Standalone);

	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = Original->PlatformData->SizeX;
	NewTexture->PlatformData->SizeY = Original->PlatformData->SizeY;
	NewTexture->PlatformData->PixelFormat = Original->PlatformData->PixelFormat;
	NewTexture->CompressionSettings = Original->CompressionSettings;
	NewTexture->SRGB = Original->SRGB;

	// Allocate first mipmap and upload the pixel data
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	FTexture2DMipMap& OriginalMip = Original->PlatformData->Mips[0];

	NewTexture->PlatformData->Mips.Add(Mip);

	Mip->SizeX = OriginalMip.SizeX;
	Mip->SizeY = OriginalMip.SizeY;

	const uint8* SourcePixels = static_cast<const uint8*>(OriginalMip.BulkData.LockReadOnly());
	Mip->BulkData.Lock(LOCK_READ_WRITE);

	void* TextureData = Mip->BulkData.Realloc(OriginalMip.BulkData.GetBulkDataSize());
	FMemory::Memcpy(TextureData, SourcePixels, OriginalMip.BulkData.GetBulkDataSize());
	Mip->BulkData.Unlock();

	NewTexture->Source.Init(Original->PlatformData->SizeX, Original->PlatformData->SizeY, 1, 1, ETextureSourceFormat::TSF_BGRA8, SourcePixels);
	OriginalMip.BulkData.Unlock();

	NewTexture->PostEditChange();
	TexturePackage->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	TextureCache.Add(Original, NewTexture);

	return NewTexture;
}

UMaterialInstanceConstant* SaveMaterial(UMaterialInstanceDynamic* Material, const FString& Path, FMaterialCache& MaterialCache,
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
			UTexture2D* PersistedTexture = SaveTexture(Cast<UTexture2D>(Value), Path, TextureCache);
			NewMaterial->SetTextureParameterValueEditorOnly(Info, PersistedTexture);
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
	TMap<UMaterialInstanceConstant*, FName> MaterialSlots;

	const auto PolygonGroups = NewMeshDescription.PolygonGroups();
	for (const auto& PolygonGroupId : PolygonGroups.GetElementIDs())
	{
		const FName MaterialName = MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId];
		int32 Index = Mesh->GetMaterialIndex(MaterialName);

		if (Index != INDEX_NONE)
		{
			UMaterialInterface* Material = Mesh->GetMaterial(Index);

			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);
			check(DynamicMaterial);
			UMaterialInstanceConstant* NewMaterial = SaveMaterial(DynamicMaterial, Path, MaterialCache, TextureCache);

			const auto MaterialResult = MaterialSlots.Find(NewMaterial);
			if (MaterialResult)
			{
				MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = *MaterialResult;
			}
			else
			{
				FName NewSlot = PersistedMesh->AddMaterial(NewMaterial);
				MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = NewSlot;
				MaterialSlots.Add(NewMaterial, NewSlot);
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
} // namespace

void CookVitruvioActors(TArray<AActor*> Actors)
{
	// If there is a previous cooking already ongoing we have to wait until it has completed. This could happen because the last part
	// of the cooking process is asynchronous.
	BlockUntilCookCompleted();

	IsCooking = true;

	// Wait until all ongoing generate calls to PRT have finished (might happen if we try to cook before all models of a scene
	// have been generated).
	BlockUntilGenerated();

	// Notify on generate completed for VitruvioComponents. Unlike the busy waiting for PRT generate calls which happen async we need to use
	// a callback here as creating the StaticMeshes from generation also happens on the game thread.
	ModelsGeneratedHandle = VitruvioModule::Get().OnAllGenerateCompleted.AddLambda([Actors](int Warnings, int Errors) {
		TSharedRef<SDlgPickPath> PickContentPathDlg = SNew(SDlgPickPath).Title(FText::FromString("Choose location for cooked models."));

		if (PickContentPathDlg->ShowModal() == EAppReturnType::Cancel)
		{
			IsCooking = false;
			VitruvioModule::Get().OnAllGenerateCompleted.Remove(ModelsGeneratedHandle);
			ModelsGeneratedHandle.Reset();
			return;
		}
		FString CookPath = PickContentPathDlg->GetPath().ToString();

		// Cook actors after all models have been generated and their meshes constructed
		FScopedSlowTask CookTask(Actors.Num(), FText::FromString("Cooking models..."));
		CookTask.MakeDialog();
		
		FMaterialCache MaterialCache;
		FTextureCache TextureCache;
		FStaticMeshCache MeshCache;

		for (AActor* Actor : Actors)
		{
			CookTask.EnterProgressFrame(1);
			
			UVitruvioComponent* VitruvioComponent = Actor->FindComponentByClass<UVitruvioComponent>();
			if (!VitruvioComponent)
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

			// Persist Mesh
			UGeneratedModelStaticMeshComponent* StaticMeshComponent = Actor->FindComponentByClass<UGeneratedModelStaticMeshComponent>();
			if (StaticMeshComponent)
			{
				check(StaticMeshComponent->GetStaticMesh());

				UStaticMesh* GeneratedMesh = StaticMeshComponent->GetStaticMesh();

				UStaticMesh* PersistedMesh = SaveStaticMesh(GeneratedMesh, CookPath, MeshCache, MaterialCache, TextureCache);
				AttachMeshComponent<UStaticMeshComponent>(CookedActor, PersistedMesh, TEXT("Model"),
														  StaticMeshComponent->GetComponentTransform());
			}

			// Persist instanced Component
			TArray<UActorComponent*> HismComponents;
			Actor->GetComponents(UGeneratedModelHISMComponent::StaticClass(), HismComponents);
			for (UActorComponent* HismComponent : HismComponents)
			{
				UGeneratedModelHISMComponent* GeneratedModelHismComponent = Cast<UGeneratedModelHISMComponent>(HismComponent);
				UStaticMesh* GeneratedMesh = GeneratedModelHismComponent->GetStaticMesh();
				if (GeneratedMesh)
				{
					UStaticMesh* PersistedMesh = SaveStaticMesh(GeneratedMesh, CookPath, MeshCache, MaterialCache, TextureCache);

					FName Name =
						MakeUniqueObjectName(CookedActor, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *PersistedMesh->GetName());
					auto* InstancedStaticMeshComponent = AttachMeshComponent<UHierarchicalInstancedStaticMeshComponent>(
						CookedActor, PersistedMesh, Name, GeneratedModelHismComponent->GetComponentTransform());

					for (int32 MaterialIndex = 0; MaterialIndex < GeneratedModelHismComponent->GetNumOverrideMaterials(); ++MaterialIndex)
					{
						UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(GeneratedModelHismComponent->OverrideMaterials[MaterialIndex]);
						check(DynamicMaterial);
						UMaterialInstanceConstant* NewMaterial = SaveMaterial(DynamicMaterial, CookPath, MaterialCache, TextureCache);
						
						InstancedStaticMeshComponent->SetMaterial(MaterialIndex, NewMaterial);
					}
					
					for (int32 InstanceIndex = 0; InstanceIndex < GeneratedModelHismComponent->GetInstanceCount(); ++InstanceIndex)
					{
						FTransform Transform;
						GeneratedModelHismComponent->GetInstanceTransform(InstanceIndex, Transform);
						InstancedStaticMeshComponent->AddInstance(Transform);
					}
				}
			}

			FString OldActorLabel = Actor->GetActorLabel();

			// Destroy the old procedural Vitruvio Actor
			Actor->Destroy();

			CookedActor->SetActorLabel(OldActorLabel);

			GEditor->SelectActor(CookedActor, true, false);
		}

		IsCooking = false;
		VitruvioModule::Get().OnAllGenerateCompleted.Remove(ModelsGeneratedHandle);
		ModelsGeneratedHandle.Reset();
	});

	// Regenerate the selected Actors to make sure we have a model to cook.
	for (AActor* Actor : Actors)
	{
		UVitruvioComponent* VitruvioComponent = Actor->FindComponentByClass<UVitruvioComponent>();
		if (!VitruvioComponent)
		{
			continue;
		}

		VitruvioComponent->Generate();
	}
}