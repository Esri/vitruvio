// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "VitruvioComponent.h"

#include "AttributeConversion.h"
#include "MaterialConversion.h"
#include "VitruvioModule.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMeshActor.h"
#include "ObjectEditorUtils.h"
#include "PolygonWindings.h"
#include "StaticMeshAttributes.h"
#include "UnrealCallbacks.h"
#include "VitruvioTypes.h"

#if WITH_EDITOR
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SSpinBox.h"
#endif

namespace
{

FVector GetCentroid(const TArray<FVector>& Vertices)
{
	FVector Centroid = FVector::ZeroVector;
	for (const FVector& Vertex : Vertices)
	{
		Centroid += Vertex;
	}
	Centroid /= FMath::Max(1, Vertices.Num());

	return Centroid;
}

#if WITH_EDITOR
bool IsRelevantObject(UVitruvioComponent* VitruvioComponent, UObject* Object)
{
	if (VitruvioComponent == Object || VitruvioComponent->InitialShape == Object)
	{
		return true;
	}
	AActor* Owner = VitruvioComponent->GetOwner();
	if (!Owner)
	{
		return false;
	}

	const TSet<UActorComponent*>& Components = Owner->GetComponents();
	for (UActorComponent* ChildComponent : Components)
	{
		if (ChildComponent == Object)
		{
			return true;
		}
	}

	return false;
}
#endif

} // namespace

UVitruvioComponent::UVitruvioComponent()
{

	static ConstructorHelpers::FObjectFinder<UMaterial> Opaque(TEXT("Material'/Vitruvio/Materials/M_OpaqueParent.M_OpaqueParent'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Masked(TEXT("Material'/Vitruvio/Materials/M_MaskedParent.M_MaskedParent'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Translucent(TEXT("Material'/Vitruvio/Materials/M_TranslucentParent.M_TranslucentParent'"));
	OpaqueParent = Opaque.Object;
	MaskedParent = Masked.Object;
	TranslucentParent = Translucent.Object;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
}

void UVitruvioComponent::CalculateRandomSeed()
{
	if (!bValidRandomSeed && InitialShape && InitialShape->IsValid())
	{
		const FVector Centroid = GetCentroid(InitialShape->GetVertices());
		RandomSeed = GetTypeHash(GetOwner()->GetActorTransform().TransformPosition(Centroid));
		bValidRandomSeed = true;
	}
}

bool UVitruvioComponent::HasValidInputData() const
{
	return InitialShape && InitialShape->IsValid() && Rpk;
}

bool UVitruvioComponent::IsReadyToGenerate() const
{
	return HasValidInputData() && bAttributesReady;
}

void UVitruvioComponent::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (!PropertyChangeDelegate.IsValid())
	{
		PropertyChangeDelegate = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UVitruvioComponent::OnPropertyChanged);
	}
#endif

	CalculateRandomSeed();

	// Check if we can load the attributes and then generate (eg during play)
	if (InitialShape && InitialShape->IsValid() && Rpk && bAttributesReady)
	{
		Generate();
	}
}

void UVitruvioComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	// Detect initial initial shape type (eg if there has already been a StaticMeshComponent assigned to the actor)
	check(GetInitialShapesClasses().Num() > 0);
	for (const auto& InitialShapeClasses : GetInitialShapesClasses())
	{
		UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(this->GetOwner()))
		{
			InitialShape = NewObject<UInitialShape>(GetOwner(), DefaultInitialShape->GetClass());
		}
	}

	if (!InitialShape)
	{
		InitialShape = NewObject<UInitialShape>(GetOwner(), GetInitialShapesClasses()[0]);
	}

	InitialShape->Initialize(this);

	CalculateRandomSeed();

	// If everything is ready we can generate (used for example for copy paste to regenerate the model)
	if (bAttributesReady)
	{
		Generate();
	}

#if WITH_EDITOR
	if (!PropertyChangeDelegate.IsValid())
	{
		PropertyChangeDelegate = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UVitruvioComponent::OnPropertyChanged);
	}
#endif
}

void UVitruvioComponent::ProcessGenerateQueue()
{
	if (!GenerateQueue.IsEmpty())
	{
		// Get from queue and build meshes
		FGenerateResultDescription Result;
		GenerateQueue.Dequeue(Result);

		FConvertedGenerateResult ConvertedResult =
			BuildResult(Result, VitruvioModule::Get().GetMaterialCache(), VitruvioModule::Get().GetTextureCache());

		RemoveGeneratedMeshes();

		// Create actors for generated meshes
		QUICK_SCOPE_CYCLE_COUNTER(STAT_VitruvioActor_CreateActors);
		FActorSpawnParameters Parameters;
		Parameters.Owner = GetOwner();
		AStaticMeshActor* StaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(Parameters);
		StaticMeshActor->SetMobility(EComponentMobility::Movable);
		StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(ConvertedResult.ShapeMesh);
		StaticMeshActor->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepRelativeTransform);

		for (const FInstance& Instance : ConvertedResult.Instances)
		{
			auto InstancedComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(StaticMeshActor);
			const TArray<FTransform>& Transforms = Instance.Transforms;
			InstancedComponent->SetStaticMesh(Instance.Mesh);

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

			InstancedComponent->AttachToComponent(StaticMeshActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			InstancedComponent->RegisterComponent();
		}
	}
}

void UVitruvioComponent::ProcessLoadAttributesQueue()
{
	if (!LoadAttributesQueue.IsEmpty())
	{
		FLoadAttributes LoadAttributes;
		LoadAttributesQueue.Dequeue(LoadAttributes);
		if (LoadAttributes.bKeepOldAttributes)
		{
			TMap<FString, URuleAttribute*> OldAttributes = Attributes;
			Attributes = LoadAttributes.AttributeMap->ConvertToUnrealAttributeMap(this);

			for (auto Attribute : Attributes)
			{
				if (OldAttributes.Contains(Attribute.Key))
				{
					Attribute.Value->CopyValue(OldAttributes[Attribute.Key]);
				}
			}
		}
		else
		{
			Attributes = LoadAttributes.AttributeMap->ConvertToUnrealAttributeMap(this);
		}

		bAttributesReady = true;

		NotifyAttributesChanged();

		if (GenerateAutomatically)
		{
			Generate();
		}
	}
}

void UVitruvioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	ProcessGenerateQueue();
	ProcessLoadAttributesQueue();
}

void UVitruvioComponent::NotifyAttributesChanged()
{
#if WITH_EDITOR
	// Notify possible listeners (eg. Details panel) about changes to the Attributes
	FPropertyChangedEvent PropertyEvent(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Attributes)));
	FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(this, PropertyEvent);
#endif // WITH_EDITOR
}

void UVitruvioComponent::RemoveGeneratedMeshes()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	TArray<AActor*> GeneratedMeshes;
	Owner->GetAttachedActors(GeneratedMeshes);
	for (const auto& Child : GeneratedMeshes)
	{
		Child->Destroy();
	}
}

FConvertedGenerateResult UVitruvioComponent::BuildResult(FGenerateResultDescription& GenerateResult,
														 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
														 TMap<FString, Vitruvio::FTextureData>& TextureCache)
{
	TMap<int32, UStaticMesh*> MeshMap;

	auto CachedMaterial = [this, &MaterialCache, &TextureCache](const Vitruvio::FMaterialAttributeContainer& MaterialAttributes, const FName& Name,
																UObject* Outer) {
		if (MaterialCache.Contains(MaterialAttributes))
		{
			return MaterialCache[MaterialAttributes];
		}
		else
		{
			UMaterialInstanceDynamic* Material = Vitruvio::GameThread_CreateMaterialInstance(Outer, Name, OpaqueParent, MaskedParent,
																							 TranslucentParent, MaterialAttributes, TextureCache);
			MaterialCache.Add(MaterialAttributes, Material);
			return Material;
		}
	};

	// convert all meshes
	for (auto& IdAndMesh : GenerateResult.MeshDescriptions)
	{
		UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
		TMap<UMaterialInstanceDynamic*, FName> MaterialSlots;

		const TArray<Vitruvio::FMaterialAttributeContainer>& MeshMaterials = GenerateResult.Materials[IdAndMesh.Key];
		FStaticMeshAttributes MeshAttributes(IdAndMesh.Value);
		const auto PolygonGroups = IdAndMesh.Value.PolygonGroups();
		size_t MaterialIndex = 0;
		for (const auto& PolygonId : PolygonGroups.GetElementIDs())
		{
			const FName MaterialName = MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonId];
			UMaterialInstanceDynamic* Material = CachedMaterial(MeshMaterials[MaterialIndex], MaterialName, StaticMesh);

			if (MaterialSlots.Contains(Material))
			{
				MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonId] = MaterialSlots[Material];
			}
			else
			{
				const FName SlotName = StaticMesh->AddMaterial(Material);
				MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonId] = SlotName;
				MaterialSlots.Add(Material, SlotName);
			}

			++MaterialIndex;
		}

		TArray<const FMeshDescription*> MeshDescriptionPtrs;
		MeshDescriptionPtrs.Emplace(&IdAndMesh.Value);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptionPtrs);
		MeshMap.Add(IdAndMesh.Key, StaticMesh);
	}

	// convert materials
	TArray<FInstance> Instances;
	for (const auto& Instance : GenerateResult.Instances)
	{
		UStaticMesh* Mesh = MeshMap[Instance.Key.PrototypeId];
		TArray<UMaterialInstanceDynamic*> OverrideMaterials;
		for (size_t MaterialIndex = 0; MaterialIndex < Instance.Key.MaterialOverrides.Num(); ++MaterialIndex)
		{
			const Vitruvio::FMaterialAttributeContainer& MaterialContainer = Instance.Key.MaterialOverrides[MaterialIndex];
			FName MaterialName = FName(MaterialContainer.Name);
			OverrideMaterials.Add(CachedMaterial(MaterialContainer, MaterialName, GetTransientPackage()));
		}

		Instances.Add({Mesh, OverrideMaterials, Instance.Value});
	}

	UStaticMesh* ShapeMesh = MeshMap.Contains(UnrealCallbacks::NO_PROTOTYPE_INDEX) ? MeshMap[UnrealCallbacks::NO_PROTOTYPE_INDEX] : nullptr;
	return {ShapeMesh, Instances};
}

void UVitruvioComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (GenerateToken)
	{
		GenerateToken->Invalidate();
	}

	if (LoadAttributesInvalidationToken)
	{
		LoadAttributesInvalidationToken->Invalidate();
	}

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangeDelegate);
	PropertyChangeDelegate.Reset();
#endif
}

void UVitruvioComponent::Generate()
{
	// If the initial shape and RPK are valid but we have not yet loaded the attributes we load the attributes
	// and regenerate afterwards
	if (HasValidInputData() && !bAttributesReady)
	{
		LoadDefaultAttributes(false, true);
		return;
	}

	// If either the RPK, initial shape or attributes are not ready we can not generate
	if (!HasValidInputData())
	{
		RemoveGeneratedMeshes();
		return;
	}

	// Since we can not abort an ongoing generate call from PRT, we invalidate the result and regenerate after the current generate call has
	// completed.
	if (GenerateToken)
	{
		GenerateToken->RequestRegenerate();

		return;
	}

	if (InitialShape)
	{
		FGenerateResult GenerateResult =
			VitruvioModule::Get().GenerateAsync(InitialShape->GetInitialShapeData(), OpaqueParent, MaskedParent, TranslucentParent, Rpk,
												Vitruvio::CreateAttributeMap(Attributes), RandomSeed);

		GenerateToken = GenerateResult.Token;

		// clang-format off
		GenerateResult.Result.Next([this](const FGenerateResult::ResultType& Result)
		{
			FScopeLock Lock(&Result.Token->Lock);

			if (Result.Token->IsInvalid()) {
				return;
			}

			GenerateToken.Reset();
			if (Result.Token->IsRegenerateRequested())
			{
				Generate();
			}
			else
			{
				GenerateQueue.Enqueue(Result.Value);
			}
		});
		// clang-format on
	}
}

#if WITH_EDITOR

void UVitruvioComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OnPropertyChanged(this, PropertyChangedEvent);
}

void UVitruvioComponent::OnPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	// Happens for example during import from copy paste
	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	bool bComponentPropertyChanged = false;
	if (Object == this && PropertyChangedEvent.Property)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Rpk))
		{
			Attributes.Empty();
			bAttributesReady = false;
			bComponentPropertyChanged = true;

			NotifyAttributesChanged();
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, RandomSeed))
		{
			bValidRandomSeed = true;
			bComponentPropertyChanged = true;
		}
	}

	const bool bRelevantProperty = InitialShape && InitialShape->IsRelevantProperty(Object, PropertyChangedEvent);
	const bool bRecreateInitialShape = IsRelevantObject(this, Object) && bRelevantProperty;

	// If a property has changed which is used for creating the initial shape we have to recreate it
	if (bRecreateInitialShape)
	{
		InitialShape = DuplicateObject(InitialShape, GetOwner());
		InitialShape->Initialize(this);

		CalculateRandomSeed();
	}

	if (bAttributesReady && GenerateAutomatically && (bRecreateInitialShape || bComponentPropertyChanged))
	{
		Generate();
	}

	if (!HasValidInputData())
	{
		RemoveGeneratedMeshes();
	}

	if (HasValidInputData() && !bAttributesReady)
	{
		LoadDefaultAttributes();
	}
}

void UVitruvioComponent::SetInitialShapeType(const TSubclassOf<UInitialShape>& Type)
{
	if (InitialShape)
	{
		InitialShape->Uninitialize();
	}

	InitialShape = DuplicateObject(Type.GetDefaultObject(), GetOwner());
	InitialShape->Initialize(this);

	bAttributesReady = false;
	RemoveGeneratedMeshes();
}

#endif // WITH_EDITOR

void UVitruvioComponent::LoadDefaultAttributes(const bool KeepOldAttributeValues, bool ForceRegenerate)
{
	check(Rpk);
	check(InitialShape);

	if (LoadingAttributes)
	{
		return;
	}

	bAttributesReady = false;
	LoadingAttributes = true;

	FAttributeMapResult AttributesResult = VitruvioModule::Get().LoadDefaultRuleAttributesAsync(InitialShape->GetInitialShapeData(), Rpk, RandomSeed);

	LoadAttributesInvalidationToken = AttributesResult.Token;

	AttributesResult.Result.Next([this, ForceRegenerate, KeepOldAttributeValues](const FAttributeMapResult::ResultType& Result) {
		FScopeLock(&Result.Token->Lock);

		if (Result.Token->IsInvalid())
		{
			return;
		}

		LoadingAttributes = false;

		LoadAttributesQueue.Enqueue({Result.Value, KeepOldAttributeValues, ForceRegenerate});
	});
}

TArray<TSubclassOf<UInitialShape>> UVitruvioComponent::GetInitialShapesClasses()
{
	return {UStaticMeshInitialShape::StaticClass(), USplineInitialShape::StaticClass()};
}
