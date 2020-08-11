// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "VitruvioComponent.h"

#include "VitruvioModule.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMeshActor.h"
#include "ObjectEditorUtils.h"
#include "PolygonWindings.h"
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
		UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass()));
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
		InitialShape->SetInitialShapeData(FInitialShapeData(Windings));
		return InitialShape;
	}

	virtual bool CanCreateFrom(UVitruvioComponent* Component) const override
	{
		AActor* Owner = Component->GetOwner();
		if (Owner)
		{
			UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass()));
			return StaticMeshComponent != nullptr;
		}
		return false;
	}

	virtual bool IsRelevantProperty(UObject* Object, FProperty* Property) override
	{
		if (Object && Object->IsA(UStaticMeshComponent::StaticClass()))
		{
			return Property && Property->GetFName() == TEXT("StaticMesh");
		}
		return false;
	}
};

class FSplineInitialShapeFactory : public FInitialShapeFactory
{
	// Adapted from https://stackoverflow.com/a/6989383
	static void OrderClockwise(TArray<FVector>& Vertices)
	{
		const FVector Center = GetCentroid(Vertices);
		Vertices.Sort([Center](const FVector& A, const FVector& B) {
			if (A.X - Center.X >= 0 && B.X - Center.X < 0)
			{
				return true;
			}

			if (A.X - Center.X < 0 && B.X - Center.X >= 0)
			{
				return false;
			}

			if (A.X - Center.X == 0 && B.X - Center.X == 0)
			{
				if (A.Y - Center.Y >= 0 || B.Y - Center.Y >= 0)
				{
					return A.Y > B.Y;
				}

				return B.Y > A.Y;
			}

			// compute the cross product of vectors (center -> a) x (center -> b)
			const int Det = (A.X - Center.X) * (B.Y - Center.Y) - (B.X - Center.X) * (A.Y - Center.Y);
			if (Det < 0)
			{
				return true;
			}

			if (Det > 0)
			{
				return false;
			}

			// points a and b are on the same line from the center
			// check which point is closer to the center
			const int D1 = (A.X - Center.X) * (A.X - Center.X) + (A.Y - Center.Y) * (A.Y - Center.Y);
			const int D2 = (B.X - Center.X) * (B.X - Center.X) + (B.Y - Center.Y) * (B.Y - Center.Y);
			return D1 > D2;
		});
	}

	virtual USplineInitialShape* CreateInitialShape(UVitruvioComponent* Component, UInitialShape* OldInitialShape) const override
	{
		AActor* Owner = Component->GetOwner();
		USplineComponent* SplineComponent = Cast<USplineComponent>(Owner->GetComponentByClass(USplineComponent::StaticClass()));
		USplineInitialShape* InitialShape = NewObject<USplineInitialShape>(Owner);

		if (USplineInitialShape* OldSplineInitialShape = Cast<USplineInitialShape>(OldInitialShape))
		{
			InitialShape->SplineApproximationPoints = OldSplineInitialShape->SplineApproximationPoints;
		}
		else
		{
			InitialShape->SplineApproximationPoints = 15;
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

		OrderClockwise(Vertices);
		InitialShape->SetInitialShapeData(FInitialShapeData({Vertices}));
		return InitialShape;
	}

	virtual bool CanCreateFrom(UVitruvioComponent* Component) const override
	{
		AActor* Owner = Component->GetOwner();
		if (Owner)
		{
			USplineComponent* SplineComponent = Cast<USplineComponent>(Owner->GetComponentByClass(USplineComponent::StaticClass()));
			return SplineComponent != nullptr;
		}
		return false;
	}

	virtual bool IsRelevantProperty(UObject* Object, FProperty* Property) override
	{
		if (Object && Object->IsA(USplineComponent::StaticClass()))
		{
			return Property && Property->GetFName() == TEXT("SplineCurves");
		}
		return false;
	}
};

TArray<FInitialShapeFactory*> GInitialShapeFactories = {new FStaticMeshInitialShapeFactory, new FSplineInitialShapeFactory};

int32 CalculateRandomSeed(const FTransform Transform, const UInitialShape* InitialShape)
{
	const FVector Centroid = GetCentroid(InitialShape->GetInitialShapeData().GetVertices());
	return GetTypeHash(Transform.TransformPosition(Centroid));
}
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

void UVitruvioComponent::OnRegister()
{
	Super::OnRegister();

	// Try to create the initial shape on first register
	for (FInitialShapeFactory* Factory : GInitialShapeFactories)
	{
		if (Factory->CanCreateFrom(this))
		{
			InitialShape = Factory->CreateInitialShape(this, InitialShape);
			InitialShapeFactory = Factory;
			break;
		}
	}

	if (InitialShape && Rpk)
	{
		LoadDefaultAttributes(true);
	}
#if WITH_EDITOR
	PropertyChangeDelegate = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UVitruvioComponent::OnPropertyChanged);
#endif
}

void UVitruvioComponent::OnUnregister()
{
	Super::OnUnregister();

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangeDelegate);
#endif
}

void UVitruvioComponent::Generate()
{
	if (!Rpk || !AttributesReady || !InitialShape)
	{
		return;
	}

	// Since we can not abort an ongoing generate call from PRT, we just regenerate after the current generate call has completed.
	if (bIsGenerating)
	{
		bNeedsRegenerate = true;
		return;
	}

	bIsGenerating = true;

	if (InitialShape)
	{
		TFuture<FGenerateResult> GenerateFuture = VitruvioModule::Get().GenerateAsync(InitialShape->GetInitialShapeData(), OpaqueParent, MaskedParent,
																					  TranslucentParent, Rpk, Attributes, RandomSeed);

		// clang-format off
		GenerateFuture.Next([=](const FGenerateResult& Result)
		{
			const FGraphEventRef CreateMeshTask = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &Result]()
			{
				bIsGenerating = false;

				AActor* Owner = GetOwner();
				if (Owner == nullptr)
				{
					return;
				}
				
				// If we need a regenerate (eg there has been a generate request while there 
				if (bNeedsRegenerate)
				{
					bNeedsRegenerate = false;
					Generate();
				}
				else
				{
					// Remove previously generated actors
					TArray<AActor*> GeneratedMeshes;
					Owner->GetAttachedActors(GeneratedMeshes);
					for (const auto& Child : GeneratedMeshes)
					{
						Child->Destroy();
					}

					// Create actors for generated meshes
					QUICK_SCOPE_CYCLE_COUNTER(STAT_VitruvioActor_CreateActors);
					FActorSpawnParameters Parameters;
					Parameters.Owner = Owner;
					AStaticMeshActor* StaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(Parameters);
					StaticMeshActor->SetMobility(EComponentMobility::Movable);
					StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(Result.ShapeMesh);
					StaticMeshActor->AttachToActor(Owner, FAttachmentTransformRules::KeepRelativeTransform);

					for (const TTuple<Vitruvio::FInstanceCacheKey, TArray<FTransform>> & MeshAndInstance : Result.Instances)
					{
						auto InstancedComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(StaticMeshActor);
						const TArray<FTransform>& Instances = MeshAndInstance.Value;
						const Vitruvio::FInstanceCacheKey& CacheKey = MeshAndInstance.Key;
						InstancedComponent->SetStaticMesh(CacheKey.Mesh);

						// Add all instance transforms
						for (const FTransform& Transform : Instances)
						{
							InstancedComponent->AddInstance(Transform);
						}

						// Apply override materials
						for (int32 MaterialIndex = 0; MaterialIndex < CacheKey.MaterialOverrides.Num(); ++MaterialIndex)
						{
							InstancedComponent->SetMaterial(MaterialIndex, CacheKey.MaterialOverrides[MaterialIndex]);
						}
						
						InstancedComponent->AttachToComponent(StaticMeshActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
						StaticMeshActor->AddInstanceComponent(InstancedComponent);
					}
					StaticMeshActor->RegisterAllComponents();

					bNeedsRegenerate = false;
				}
				
			},
			TStatId(), nullptr, ENamedThreads::GameThread);

			FTaskGraphInterface::Get().WaitUntilTaskCompletes(CreateMeshTask);
		});
		// clang-format on
	}
}

#if WITH_EDITOR

FInitialShapeFactory* UVitruvioComponent::FindFactory(UObject* Object, FProperty* Property)
{
	for (FInitialShapeFactory* Factory : GInitialShapeFactories)
	{
		if (Factory->IsRelevantProperty(Object, Property))
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
	if (Object == this && PropertyChangedEvent.Property)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Rpk))
		{
			Attributes.Empty();
			AttributesReady = false;
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
		InitialShapeFactory = FindFactory(Object, PropertyChangedEvent.Property);
		InitialShape = nullptr;
		bRecreateInitialShape = InitialShapeFactory && InitialShapeFactory->CanCreateFrom(this);
	}
	// If a property has changed which is used for creating the initial shape we have to recreate it
	else if (InitialShapeFactory && InitialShapeFactory->IsRelevantProperty(Object, PropertyChangedEvent.Property))
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

		if (AttributesReady)
		{
			Generate();
		}
	}

	if (InitialShape && Rpk && !AttributesReady)
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

	AttributesReady = false;
	LoadingAttributes = true;

	TFuture<FAttributeMapPtr> AttributesFuture =
		VitruvioModule::Get().LoadDefaultRuleAttributesAsync(InitialShape->GetInitialShapeData(), Rpk, RandomSeed);
	AttributesFuture.Next([this, KeepOldAttributeValues](const FAttributeMapPtr& Result) {
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[this, Result, KeepOldAttributeValues]() {
				LoadingAttributes = false;
				if (KeepOldAttributeValues)
				{
					TMap<FString, URuleAttribute*> OldAttributes = Attributes;
					Attributes = Result->ConvertToUnrealAttributeMap(this);

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
					Attributes = Result->ConvertToUnrealAttributeMap(this);
				}

				AttributesReady = true;

#if WITH_EDITOR
				// Notify possible listeners (eg. Details panel) about changes to the Attributes
				FPropertyChangedEvent PropertyEvent(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Attributes)));
				FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(this, PropertyEvent);
#endif // WITH_EDITOR

				if (GenerateAutomatically)
				{
					Generate();
				}
			},
			TStatId(), nullptr, ENamedThreads::GameThread);
	});
}
