// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "InitialShape.h"

#include "PolygonWindings.h"

namespace
{
template <typename T>
T* AttachComponent(AActor* Owner, const FString& Name)
{
	T* Component = NewObject<T>(Owner, *Name);
	Component->Mobility = EComponentMobility::Movable;
	Owner->AddInstanceComponent(Component);
	Component->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	Component->OnComponentCreated();
	Component->RegisterComponent();
	return Component;
}

void DetachComponent(USceneComponent* SceneComponent)
{
#if WITH_EDITOR
	SceneComponent->Modify();
#endif
	SceneComponent->DestroyComponent(true);
#if WITH_EDITOR
	AActor* Owner = SceneComponent->GetOwner();
	Owner->RerunConstructionScripts();
#endif
}

} // namespace

TArray<FVector> UInitialShape::GetVertices() const
{
	TArray<FVector> AllVertices;
	for (const FInitialShapeFace& Face : Faces)
	{
		AllVertices.Append(Face.Vertices);
	}
	return AllVertices;
}

void UStaticMeshInitialShape::Initialize(UActorComponent* OwnerComponent)
{
	AActor* Owner = OwnerComponent->GetOwner();
	StaticMeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
	if (!StaticMeshComponent)
	{
		StaticMeshComponent = AttachComponent<UStaticMeshComponent>(Owner, TEXT("InitialShapeStaticMesh"));
	}

	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();

	if (StaticMesh == nullptr)
	{
		bIsValid = false;
		return;
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

	TArray<FInitialShapeFace> InitialShapeFaces;
	for (const TArray<FVector>& FaceVertices : Windings)
	{
		InitialShapeFaces.Push(FInitialShapeFace{FaceVertices});
	}
	SetInitialShapeData(InitialShapeFaces);
}

void UStaticMeshInitialShape::Uninitialize()
{
	Super::Uninitialize();

	if (StaticMeshComponent)
	{
		DetachComponent(StaticMeshComponent);
	}
}

bool UStaticMeshInitialShape::CanConstructFrom(UActorComponent* OwnerComponent)
{
	AActor* Owner = OwnerComponent->GetOwner();
	if (Owner)
	{
		UStaticMeshComponent* Component = Owner->FindComponentByClass<UStaticMeshComponent>();
		return Component != nullptr && Component->GetStaticMesh() != nullptr;
	}
	return false;
}

bool UStaticMeshInitialShape::CanDestroy()
{
	return !StaticMeshComponent || StaticMeshComponent->CreationMethod == EComponentCreationMethod::Instance;
}

bool USplineInitialShape::CanConstructFrom(UActorComponent* OwnerComponent)
{
	AActor* Owner = OwnerComponent->GetOwner();
	if (Owner)
	{
		USplineComponent* Component = Owner->FindComponentByClass<USplineComponent>();
		return Component != nullptr && Component->GetNumberOfSplinePoints() > 0;
	}
	return false;
}

bool USplineInitialShape::CanDestroy()
{
	return !SplineComponent || SplineComponent->CreationMethod == EComponentCreationMethod::Instance;
}

#if WITH_EDITOR

bool USplineInitialShape::IsRelevantProperty(UObject* Object, FProperty* Property)
{
	if (Object)
	{
		return Property && (Property->GetFName() == TEXT("SplineCurves") || Property->GetFName() == TEXT("SplineApproximationPoints"));
	}
	return false;
}

bool UStaticMeshInitialShape::IsRelevantProperty(UObject* Object, FProperty* Property)
{
	if (Object)
	{
		return Property && (Property->GetFName() == TEXT("StaticMesh") || Property->GetFName() == TEXT("StaticMeshComponent"));
	}
	return false;
}
#endif

void USplineInitialShape::Initialize(UActorComponent* OwnerComponent)
{
	AActor* Owner = OwnerComponent->GetOwner();
	SplineComponent = Owner->FindComponentByClass<USplineComponent>();
	if (!SplineComponent)
	{
		SplineComponent = AttachComponent<USplineComponent>(Owner, TEXT("InitialShapeSpline"));
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
			const float Distance = SplineComponent->GetSplineLength() / SplineApproximationPoints;
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

	SetInitialShapeData({FInitialShapeFace{Vertices}});
}

void USplineInitialShape::Uninitialize()
{
	Super::Uninitialize();

	if (SplineComponent)
	{
		DetachComponent(SplineComponent);
	}
}
