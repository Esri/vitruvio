// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "VitruvioComponent.h"

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

class FStaticMeshInitialShapeFactory : public FInitialShapeFactory
{
	virtual UInitialShape* CreateInitialShape(UVitruvioComponent* Component, UInitialShape* OldInitialShape) const override
	{
		AActor* Owner = Component->GetOwner();
		UStaticMeshComponent* StaticMeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
		if (StaticMesh == nullptr)
		{
			return nullptr;
		}

		StaticMesh->bAllowCPUAccess = true;

		TArray<FVector> MeshVertices;
		TArray<int32> MeshIndices;

		if (StaticMesh->RenderData != nullptr && StaticMesh->RenderData->LODResources.IsValidIndex(0))
		{
			const FStaticMeshLODResources& LOD = StaticMesh->RenderData->LODResources[0];

			for (auto SectionIndex = 0; SectionIndex < LOD.Sections.Num(); ++SectionIndex)
			{
				for (uint32 VertexIndex = 0; VertexIndex < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++VertexIndex)
				{
					FVector Vertex = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);
					MeshVertices.Add(Vertex);
				}

				const FStaticMeshSection& Section = LOD.Sections[SectionIndex];
				FIndexArrayView IndicesView = LOD.IndexBuffer.GetArrayView();

				for (uint32 Triangle = 0; Triangle < Section.NumTriangles; ++Triangle)
				{
					for (uint32 TriangleVertexIndex = 0; TriangleVertexIndex < 3; ++TriangleVertexIndex)
					{
						const uint32 MeshVertIndex = IndicesView[Section.FirstIndex + Triangle * 3 + TriangleVertexIndex];
						MeshIndices.Add(MeshVertIndex);
					}
				}
			}
		}

		const TArray<TArray<FVector>> Windings = Vitruvio::GetOutsideWindings(MeshVertices, MeshIndices);
		UInitialShape* InitialShape = NewObject<UInitialShape>(Owner);

		TArray<FInitialShapeFace> InitialShapeFaces;
		for (const TArray<FVector>& FaceVertices : Windings)
		{
			InitialShapeFaces.Push(FInitialShapeFace{FaceVertices});
		}
		InitialShape->SetInitialShapeData(InitialShapeFaces);
		return InitialShape;
	}

	virtual bool CanCreateFrom(UVitruvioComponent* Component) const override
	{
		AActor* Owner = Component->GetOwner();
		if (Owner)
		{
			UStaticMeshComponent* StaticMeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
			return StaticMeshComponent != nullptr && StaticMeshComponent->GetStaticMesh() != nullptr;
		}
		return false;
	}

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, FProperty* Property) override
	{
		if (Object)
		{
			return Property && (Property->GetFName() == TEXT("StaticMesh") || Property->GetFName() == TEXT("StaticMeshComponent"));
		}
		return false;
	}
#endif
};

class FSplineInitialShapeFactory : public FInitialShapeFactory
{

	virtual USplineInitialShape* CreateInitialShape(UVitruvioComponent* Component, UInitialShape* OldInitialShape) const override
	{
		AActor* Owner = Component->GetOwner();
		USplineComponent* SplineComponent = Owner->FindComponentByClass<USplineComponent>();
		USplineInitialShape* InitialShape = NewObject<USplineInitialShape>(Owner);

		if (USplineInitialShape* OldSplineInitialShape = Cast<USplineInitialShape>(OldInitialShape))
		{
			InitialShape->SplineApproximationPoints = OldSplineInitialShape->SplineApproximationPoints;
		}

		TArray<FVector> Vertices;
		const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
		for (int32 SplinePointIndex = 0; SplinePointIndex < NumPoints; ++SplinePointIndex)
		{
			const ESplinePointType::Type SplineType = SplineComponent->GetSplinePointType(SplinePointIndex);
			if (SplineType == ESplinePointType::Linear)
			{
				Vertices.Add(SplineComponent->GetLocationAtSplinePoint(SplinePointIndex, ESplineCoordinateSpace::Local));
			}
			else
			{
				const int32 NextPointIndex = SplinePointIndex + 1;
				float Position = SplineComponent->GetDistanceAlongSplineAtSplinePoint(SplinePointIndex);
				const float EndDistance = NextPointIndex < NumPoints ? SplineComponent->GetDistanceAlongSplineAtSplinePoint(NextPointIndex)
																	 : SplineComponent->GetSplineLength();
				const float Distance = SplineComponent->GetSplineLength() / InitialShape->SplineApproximationPoints;
				while (Position < EndDistance)
				{
					Vertices.Add(SplineComponent->GetLocationAtDistanceAlongSpline(Position, ESplineCoordinateSpace::Local));
					Position += Distance;
				}
			}
		}

		// Reverse vertices if first three vertices are counter clockwise
		if (Vertices.Num() >= 3)
		{
			const FVector V1 = Vertices[1] - Vertices[0];
			const FVector V2 = Vertices[2] - Vertices[0];
			const FVector Normal = FVector::CrossProduct(V1, V2);
			const float Dot = FVector::DotProduct(FVector::UpVector, Normal);

			if (Dot > 0)
			{
				Algo::Reverse(Vertices);
			}
		}

		InitialShape->SetInitialShapeData({FInitialShapeFace{Vertices}});

		return InitialShape;
	}

	virtual bool CanCreateFrom(UVitruvioComponent* Component) const override
	{
		AActor* Owner = Component->GetOwner();
		if (Owner)
		{
			USplineComponent* SplineComponent = Owner->FindComponentByClass<USplineComponent>();
			return SplineComponent != nullptr && SplineComponent->GetNumberOfSplinePoints() > 0;
		}
		return false;
	}

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, FProperty* Property) override
	{
		if (Object)
		{
			return Property && (Property->GetFName() == TEXT("SplineCurves") || Property->GetFName() == TEXT("SplineApproximationPoints"));
		}
		return false;
	}
#endif
};

TArray<FInitialShapeFactory*> GInitialShapeFactories = {new FStaticMeshInitialShapeFactory, new FSplineInitialShapeFactory};

int32 CalculateRandomSeed(const FTransform Transform, const UInitialShape* InitialShape)
{
	const FVector Centroid = GetCentroid(InitialShape->GetVertices());
	return GetTypeHash(Transform.TransformPosition(Centroid));
}

} // namespace

#if WITH_EDITOR
bool FInitialShapeFactory::IsRelevantObject(UVitruvioComponent* VitruvioComponent, UObject* Object)
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

void UVitruvioComponent::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (!PropertyChangeDelegate.IsValid())
	{
		PropertyChangeDelegate = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UVitruvioComponent::OnPropertyChanged);
	}
#endif

	// Check if we can load the attributes and then generate (eg during play)
	if (InitialShape && Rpk && bAttributesReady)
	{
		Generate();
	}
}

void UVitruvioComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	for (FInitialShapeFactory* Factory : GInitialShapeFactories)
	{
		if (Factory->CanCreateFrom(this))
		{
			InitialShape = Factory->CreateInitialShape(this, InitialShape);
			InitialShapeFactory = Factory;
			break;
		}
	}

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

		FConvertedGenerateResult ConvertedResult = BuildResult(Result, VitruvioModule::Get().GetMaterialCache());

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
														 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& Cache)
{
	TMap<int32, UStaticMesh*> MeshMap;

	auto CachedMaterial = [this, &Cache](const Vitruvio::FMaterialAttributeContainer& MaterialAttributes, const FName& Name, UObject* Outer) {
		if (Cache.Contains(MaterialAttributes))
		{
			return Cache[MaterialAttributes];
		}
		else
		{
			UMaterialInstanceDynamic* Material =
				Vitruvio::GameThread_CreateMaterialInstance(Outer, Name, OpaqueParent, MaskedParent, TranslucentParent, MaterialAttributes);
			Cache.Add(MaterialAttributes, Material);
			return Material;
		}
	};

	// convert all meshes
	for (auto& IdAndMesh : GenerateResult.MeshDescriptions)
	{
		UStaticMesh* StaticMesh = NewObject<UStaticMesh>();

		const TArray<Vitruvio::FMaterialAttributeContainer>& MeshMaterials = GenerateResult.Materials[IdAndMesh.Key];
		FStaticMeshAttributes MeshAttributes(IdAndMesh.Value);
		const auto PolygonGroups = IdAndMesh.Value.PolygonGroups();
		size_t MaterialIndex = 0;
		for (const auto& PolygonId : PolygonGroups.GetElementIDs())
		{
			const FName MaterialName = MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonId];
			const FName SlotName = StaticMesh->AddMaterial(CachedMaterial(MeshMaterials[MaterialIndex], MaterialName, StaticMesh));
			MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonId] = SlotName;

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
			FName MaterialName = FName(MaterialContainer.StringProperties["name"]);
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
	if (!Rpk || !bAttributesReady || !InitialShape)
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
		FGenerateResult GenerateResult = VitruvioModule::Get().GenerateAsync(InitialShape->GetInitialShapeData(), OpaqueParent, MaskedParent,
																			 TranslucentParent, Rpk, Attributes, RandomSeed);

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

FInitialShapeFactory* UVitruvioComponent::FindFactory(UVitruvioComponent* VitruvioComponent)
{
	for (FInitialShapeFactory* Factory : GInitialShapeFactories)
	{
		if (Factory->CanCreateFrom(VitruvioComponent))
		{
			return Factory;
		}
	}

	return nullptr;
}

void UVitruvioComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OnPropertyChanged(this, PropertyChangedEvent);
}

void UVitruvioComponent::OnPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	// Happens during import eg from copy paste
	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	if (Object == this && PropertyChangedEvent.Property)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Rpk))
		{
			Attributes.Empty();
			bAttributesReady = false;

			NotifyAttributesChanged();
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, RandomSeed))
		{
			bValidRandomSeed = true;
		}
	}

	bool bRecreateInitialShape = false;
	// If we have not create the initial shape or we can not create it anymore (eg a required component has been deleted)
	// we need to reinitialize the factory and recreate the initial shape
	if (!InitialShapeFactory || !InitialShapeFactory->CanCreateFrom(this))
	{
		InitialShapeFactory = FindFactory(this);
		InitialShape = nullptr;
		bRecreateInitialShape = InitialShapeFactory != nullptr;
	}
	// If a property has changed which is used for creating the initial shape we have to recreate it
	else if (InitialShapeFactory && InitialShapeFactory->IsRelevantObject(this, Object) &&
			 InitialShapeFactory->IsRelevantProperty(Object, PropertyChangedEvent.Property))
	{
		bRecreateInitialShape = true;
	}

	if (bRecreateInitialShape)
	{
		InitialShape = InitialShapeFactory->CreateInitialShape(this, InitialShape);

		if (!bValidRandomSeed && InitialShape)
		{
			RandomSeed = CalculateRandomSeed(GetOwner()->GetActorTransform(), InitialShape);
			bValidRandomSeed = true;
		}

		if (bAttributesReady)
		{
			Generate();
		}
	}

	if (!InitialShape || !Rpk)
	{
		RemoveGeneratedMeshes();
	}

	if (InitialShape && Rpk && !bAttributesReady)
	{
		LoadDefaultAttributes();
	}
}

#endif // WITH_EDITOR

void UVitruvioComponent::LoadDefaultAttributes(const bool KeepOldAttributeValues)
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

	AttributesResult.Result.Next([this, KeepOldAttributeValues](const FAttributeMapResult::ResultType& Result) {
		FScopeLock(&Result.Token->Lock);

		if (Result.Token->IsInvalid())
		{
			return;
		}

		LoadingAttributes = false;

		LoadAttributesQueue.Enqueue({Result.Value, KeepOldAttributeValues});
	});
}
