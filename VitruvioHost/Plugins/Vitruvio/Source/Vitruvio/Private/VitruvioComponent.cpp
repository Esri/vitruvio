// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "VitruvioComponent.h"

#include "AttributeConversion.h"
#include "GeneratedModelHISMComponent.h"
#include "GeneratedModelStaticMeshComponent.h"
#include "MaterialConversion.h"
#include "UnrealCallbacks.h"
#include "VitruvioModule.h"
#include "VitruvioTypes.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMeshActor.h"
#include "ObjectEditorUtils.h"
#include "PhysicsEngine/BodySetup.h"
#include "PolygonWindings.h"
#include "StaticMeshAttributes.h"

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

template <typename A, typename T>
bool GetAttribute(const TMap<FString, URuleAttribute*>& Attributes, const FString& Name, T& OutValue)
{
	URuleAttribute const* const* FoundAttribute = Attributes.Find(Name);
	if (!FoundAttribute)
	{
		return false;
	}

	URuleAttribute const* Attribute = *FoundAttribute;
	A const* TAttribute = Cast<A>(Attribute);
	if (!TAttribute)
	{
		return false;
	}

	OutValue = TAttribute->Value;

	return true;
}

template <typename A, typename T>
bool SetAttribute(UVitruvioComponent* VitruvioComponent, TMap<FString, URuleAttribute*>& Attributes, const FString& Name, const T& Value)
{
	URuleAttribute** FoundAttribute = Attributes.Find(Name);
	if (!FoundAttribute)
	{
		return false;
	}

	URuleAttribute* Attribute = *FoundAttribute;
	A* TAttribute = Cast<A>(Attribute);
	if (!TAttribute)
	{
		return false;
	}

	TAttribute->Value = Value;
	if (VitruvioComponent->GenerateAutomatically && VitruvioComponent->IsReadyToGenerate())
	{
		VitruvioComponent->Generate();
	}

	return true;
}

bool IsOuterOf(UObject* Inner, UObject* Outer)
{
	if (!Outer)
	{
		return false;
	}

	UObject* Current = Inner->GetOuter();

	while (Current != nullptr)
	{
		if (Current == Outer)
		{
			return true;
		}

		Current = Current->GetOuter();
	}

	return false;
}

void InitializeBodySetup(UBodySetup* BodySetup, bool GenerateComplexCollision)
{
	BodySetup->DefaultInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BodySetup->CollisionTraceFlag =
		GenerateComplexCollision ? ECollisionTraceFlag::CTF_UseComplexAsSimple : ECollisionTraceFlag::CTF_UseSimpleAsComplex;
	BodySetup->bDoubleSidedGeometry = true;
	BodySetup->bMeshCollideAll = true;
	BodySetup->InvalidatePhysicsData();
	BodySetup->CreatePhysicsMeshes();
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

UVitruvioComponent::FOnHierarchyChanged UVitruvioComponent::OnHierarchyChanged;

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

void UVitruvioComponent::SetRpk(URulePackage* RulePackage)
{
	if (this->Rpk == RulePackage)
	{
		return;
	}

	this->Rpk = RulePackage;

	Attributes.Empty();
	bAttributesReady = false;
	bNotifyAttributeChange = true;
}

bool UVitruvioComponent::SetStringAttribute(const FString& Name, const FString& Value)
{
	return SetAttribute<UStringAttribute, FString>(this, this->Attributes, Name, Value);
}

bool UVitruvioComponent::GetStringAttribute(const FString& Name, FString& OutValue) const
{
	return GetAttribute<UStringAttribute, FString>(this->Attributes, Name, OutValue);
}

bool UVitruvioComponent::SetBoolAttribute(const FString& Name, bool Value)
{
	return SetAttribute<UBoolAttribute, bool>(this, this->Attributes, Name, Value);
}

bool UVitruvioComponent::GetBoolAttribute(const FString& Name, bool& OutValue) const
{
	return GetAttribute<UBoolAttribute, bool>(this->Attributes, Name, OutValue);
}

bool UVitruvioComponent::SetFloatAttribute(const FString& Name, float Value)
{
	return SetAttribute<UFloatAttribute, float>(this, this->Attributes, Name, Value);
}

bool UVitruvioComponent::GetFloatAttribute(const FString& Name, float& OutValue) const
{
	return GetAttribute<UFloatAttribute, float>(this->Attributes, Name, OutValue);
}

const TMap<FString, URuleAttribute*>& UVitruvioComponent::GetAttributes() const
{
	return Attributes;
}

URulePackage* UVitruvioComponent::GetRpk() const
{
	return Rpk;
}

void UVitruvioComponent::SetRandomSeed(int32 NewRandomSeed)
{
	RandomSeed = NewRandomSeed;
	bValidRandomSeed = true;

	if (GenerateAutomatically && IsReadyToGenerate())
	{
		Generate();
	}
}

void UVitruvioComponent::LoadInitialShape()
{
	// Do nothing if an initial shape has already been loaded
	if (InitialShape)
	{
		return;
	}

	// Detect initial initial shape type (eg if there has already been a StaticMeshComponent assigned to the actor)
	check(GetInitialShapesClasses().Num() > 0);
	for (const auto& InitialShapeClasses : GetInitialShapesClasses())
	{
		UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(this->GetOwner()))
		{
			InitialShape = NewObject<UInitialShape>(GetOwner(), DefaultInitialShape->GetClass(), NAME_None, RF_Transient | RF_TextExportTransient);
		}
	}

	if (!InitialShape)
	{
		InitialShape = NewObject<UInitialShape>(GetOwner(), GetInitialShapesClasses()[0], NAME_None, RF_Transient | RF_TextExportTransient);
	}

	InitialShape->Initialize(this);
}

void UVitruvioComponent::Initialize()
{
	// During cooking we don't need to do initialize anything as we don't wont to generate during the cooking process
	if (GIsCookerLoadingPackage)
	{
		return;
	}

#if WITH_EDITOR
	if (!PropertyChangeDelegate.IsValid())
	{
		PropertyChangeDelegate = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UVitruvioComponent::OnPropertyChanged);
	}
#endif

	LoadInitialShape();

	OnHierarchyChanged.Broadcast(this);

	CalculateRandomSeed();

	// Check if we can load the attributes and then generate (eg during play)
	if (IsReadyToGenerate())
	{
		Generate();
	}
}

void UVitruvioComponent::PostLoad()
{
	Super::PostLoad();

	// Only initialize if the Component is instance (eg via details panel to any Actor)
	if (CreationMethod == EComponentCreationMethod::Instance)
	{
		Initialize();
	}
}

void UVitruvioComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	// Only initialize if the Component is instance (eg via details panel to any Actor)
	if (CreationMethod == EComponentCreationMethod::Instance)
	{
		Initialize();
	}
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

		QUICK_SCOPE_CYCLE_COUNTER(STAT_VitruvioActor_CreateModelActors);

		USceneComponent* InitialShapeComponent = InitialShape->GetComponent();
		UGeneratedModelStaticMeshComponent* VitruvioModelComponent = nullptr;

		TArray<USceneComponent*> InitialShapeChildComponents;
		InitialShapeComponent->GetChildrenComponents(false, InitialShapeChildComponents);
		for (USceneComponent* Component : InitialShapeChildComponents)
		{
			if (Component->IsA(UGeneratedModelStaticMeshComponent::StaticClass()))
			{
				VitruvioModelComponent = Cast<UGeneratedModelStaticMeshComponent>(Component);

				VitruvioModelComponent->SetStaticMesh(nullptr);

				// Cleanup old hierarchical instances
				TArray<USceneComponent*> InstanceComponents;
				VitruvioModelComponent->GetChildrenComponents(true, InstanceComponents);
				for (USceneComponent* InstanceComponent : InstanceComponents)
				{
					InstanceComponent->DestroyComponent(true);
				}

				break;
			}
		}

		if (!VitruvioModelComponent)
		{
			VitruvioModelComponent = NewObject<UGeneratedModelStaticMeshComponent>(InitialShape->GetComponent(), FName(TEXT("GeneratedModel")),
																				   RF_Transient | RF_DuplicateTransient);
			VitruvioModelComponent->AttachToComponent(InitialShapeComponent, FAttachmentTransformRules::KeepRelativeTransform);
			InitialShapeComponent->GetOwner()->AddInstanceComponent(VitruvioModelComponent);
			VitruvioModelComponent->OnComponentCreated();
			VitruvioModelComponent->RegisterComponent();
		}

		VitruvioModelComponent->SetStaticMesh(ConvertedResult.ShapeMesh);
		VitruvioModelComponent->SetCollisionData(ConvertedResult.CollisionData);

		// StaticMesh Collision
		UStaticMesh* ShapeMesh = ConvertedResult.ShapeMesh;
		UBodySetup* BodySetup = NewObject<UBodySetup>(VitruvioModelComponent);
		InitializeBodySetup(BodySetup, GenerateCollision);
		ShapeMesh->BodySetup = BodySetup;
		VitruvioModelComponent->RecreatePhysicsState();

		for (const FInstance& Instance : ConvertedResult.Instances)
		{
			auto InstancedComponent =
				NewObject<UGeneratedModelHISMComponent>(VitruvioModelComponent, NAME_None, RF_Transient | RF_DuplicateTransient);
			const TArray<FTransform>& Transforms = Instance.Transforms;
			InstancedComponent->SetStaticMesh(Instance.Mesh);
			InstancedComponent->SetCollisionData(Instance.CollisionData);

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

			// Instanced component collision
			UStaticMesh* InstanceMesh = Instance.Mesh;
			UBodySetup* InstanceBodySetup = NewObject<UBodySetup>(InstancedComponent);
			InitializeBodySetup(InstanceBodySetup, GenerateCollision);
			InstanceMesh->BodySetup = InstanceBodySetup;
			InstancedComponent->RecreatePhysicsState();

			// Attach and register instance component
			InstancedComponent->AttachToComponent(VitruvioModelComponent, FAttachmentTransformRules::KeepRelativeTransform);
			InitialShapeComponent->GetOwner()->AddInstanceComponent(InstancedComponent);
			InstancedComponent->OnComponentCreated();
			InstancedComponent->RegisterComponent();
		}

		OnHierarchyChanged.Broadcast(this);

		HasGeneratedMesh = true;

		InitialShape->SetHidden(HideAfterGeneration);
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

		bNotifyAttributeChange = true;

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

	if (bNotifyAttributeChange)
	{
		NotifyAttributesChanged();
		bNotifyAttributeChange = false;
	}
}

void UVitruvioComponent::NotifyAttributesChanged()
{
#if WITH_EDITOR
	// Notify possible listeners (eg. Details panel) about changes to the Attributes
	FPropertyChangedEvent PropertyEvent(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Attributes)));
	FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(this, PropertyEvent);
#endif
}

void UVitruvioComponent::RemoveGeneratedMeshes()
{
	if (!InitialShape)
	{
		return;
	}

	USceneComponent* InitialShapeComponent = InitialShape->GetComponent();

	TArray<USceneComponent*> Children;
	InitialShapeComponent->GetChildrenComponents(true, Children);
	for (USceneComponent* Child : Children)
	{
		Child->DestroyComponent(true);
	}

	HasGeneratedMesh = false;
	InitialShape->SetHidden(false);
}

FConvertedGenerateResult UVitruvioComponent::BuildResult(FGenerateResultDescription& GenerateResult,
														 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
														 TMap<FString, Vitruvio::FTextureData>& TextureCache)
{
	TMap<int32, TTuple<UStaticMesh*, Vitruvio::FCollisionData>> MeshMap;

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
		UStaticMesh* StaticMesh = NewObject<UStaticMesh>(GetTransientPackage(), NAME_None, RF_Transient);
		TMap<UMaterialInstanceDynamic*, FName> MaterialSlots;

		const TArray<Vitruvio::FMaterialAttributeContainer>& MeshMaterials = GenerateResult.Materials[IdAndMesh.Key];
		FMeshDescription& MeshDescription = IdAndMesh.Value;
		FStaticMeshAttributes MeshAttributes(MeshDescription);

		TArray<FVector> Vertices;
		auto VertexPositions = MeshAttributes.GetVertexPositions();
		for (int32 VertexIndex = 0; VertexIndex < VertexPositions.GetNumElements(); ++VertexIndex)
		{
			Vertices.Add(VertexPositions[FVertexID(VertexIndex)]);
		}

		TArray<FTriIndices> Indices;
		const auto PolygonGroups = MeshDescription.PolygonGroups();
		size_t MaterialIndex = 0;
		for (const auto& PolygonGroupId : PolygonGroups.GetElementIDs())
		{
			const FName MaterialName = MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId];
			UMaterialInstanceDynamic* Material = CachedMaterial(MeshMaterials[MaterialIndex], MaterialName, StaticMesh);

			if (MaterialSlots.Contains(Material))
			{
				MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = MaterialSlots[Material];
			}
			else
			{
				const FName SlotName = StaticMesh->AddMaterial(Material);
				MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = SlotName;
				MaterialSlots.Add(Material, SlotName);
			}

			++MaterialIndex;

			// cache collision data
			for (FPolygonID PolygonID : MeshDescription.GetPolygonGroupPolygons(PolygonGroupId))
			{
				for (FTriangleID TriangleID : MeshDescription.GetPolygonTriangleIDs(PolygonID))
				{
					auto TriangleVertexInstances = MeshDescription.GetTriangleVertexInstances(TriangleID);

					auto VertexID0 = MeshDescription.GetVertexInstanceVertex(TriangleVertexInstances[0]);
					auto VertexID1 = MeshDescription.GetVertexInstanceVertex(TriangleVertexInstances[1]);
					auto VertexID2 = MeshDescription.GetVertexInstanceVertex(TriangleVertexInstances[2]);

					FTriIndices TriIndex;
					TriIndex.v0 = VertexID0.GetValue();
					TriIndex.v1 = VertexID1.GetValue();
					TriIndex.v2 = VertexID2.GetValue();
					Indices.Add(TriIndex);
				}
			}
		}

		TArray<const FMeshDescription*> MeshDescriptionPtrs;
		MeshDescriptionPtrs.Emplace(&IdAndMesh.Value);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptionPtrs);
		MeshMap.Add(IdAndMesh.Key, MakeTuple(StaticMesh, Vitruvio::FCollisionData{Indices, Vertices}));
	}

	// convert materials
	TArray<FInstance> Instances;
	for (const auto& Instance : GenerateResult.Instances)
	{
		UStaticMesh* Mesh = MeshMap[Instance.Key.PrototypeId].Key;
		Vitruvio::FCollisionData& CollisionData = MeshMap[Instance.Key.PrototypeId].Value;
		TArray<UMaterialInstanceDynamic*> OverrideMaterials;
		for (size_t MaterialIndex = 0; MaterialIndex < Instance.Key.MaterialOverrides.Num(); ++MaterialIndex)
		{
			const Vitruvio::FMaterialAttributeContainer& MaterialContainer = Instance.Key.MaterialOverrides[MaterialIndex];
			FName MaterialName = FName(MaterialContainer.Name);
			OverrideMaterials.Add(CachedMaterial(MaterialContainer, MaterialName, GetTransientPackage()));
		}

		Instances.Add({Mesh, CollisionData, OverrideMaterials, Instance.Value});
	}

	if (MeshMap.Contains(UnrealCallbacks::NO_PROTOTYPE_INDEX))
	{
		return {MeshMap[UnrealCallbacks::NO_PROTOTYPE_INDEX].Key, MeshMap[UnrealCallbacks::NO_PROTOTYPE_INDEX].Value, Instances};
	}
	return {nullptr, Vitruvio::FCollisionData{}, Instances};
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
		FGenerateResult GenerateResult = VitruvioModule::Get().GenerateAsync(InitialShape->GetFaces(), OpaqueParent, MaskedParent, TranslucentParent,
																			 Rpk, Vitruvio::CreateAttributeMap(Attributes), RandomSeed);

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
	if (Object != this && !IsOuterOf(Object, this->GetOwner()))
	{
		return;
	}

	// Happens for example during import from copy paste
	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	bool bComponentPropertyChanged = false;

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Rpk))
	{
		Attributes.Empty();
		bAttributesReady = false;
		bComponentPropertyChanged = true;
		bNotifyAttributeChange = true;
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, RandomSeed))
	{
		bValidRandomSeed = true;
		bComponentPropertyChanged = true;
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, GenerateCollision))
	{
		bComponentPropertyChanged = true;
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, HideAfterGeneration))
	{
		InitialShape->SetHidden(HideAfterGeneration && HasGeneratedMesh);
	}

	const bool bRelevantProperty = InitialShape && InitialShape->IsRelevantProperty(Object, PropertyChangedEvent);
	const bool bRecreateInitialShape = IsRelevantObject(this, Object) && bRelevantProperty;

	// If a property has changed which is used for creating the initial shape we have to recreate it
	if (bRecreateInitialShape)
	{
		UInitialShape* DuplicatedInitialShape = DuplicateObject(InitialShape, GetOwner());
		InitialShape->Rename(nullptr, GetTransientPackage()); // Remove from Owner
		InitialShape = DuplicatedInitialShape;
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
	UInitialShape* NewInitialShape = NewObject<UInitialShape>(GetOwner(), Type, NAME_None, RF_Transient | RF_TextExportTransient);

	if (InitialShape)
	{
		const TArray<FInitialShapeFace> Faces = InitialShape->GetFaces();
		InitialShape->Uninitialize();

		NewInitialShape->Initialize(this, Faces);
	}
	else
	{
		NewInitialShape->Initialize(this);
	}

	InitialShape->Rename(nullptr, GetTransientPackage()); // Remove from Owner
	InitialShape = NewInitialShape;

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

	FAttributeMapResult AttributesResult = VitruvioModule::Get().LoadDefaultRuleAttributesAsync(InitialShape->GetFaces(), Rpk, RandomSeed);

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
