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

// Returns false if all faces are degenerate and true otherwise
bool HasValidGeometry(const TArray<FInitialShapeFace>& InFaces)
{
	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	const auto VertexPositions = Attributes.GetVertexPositions();

	// 1. Construct MeshDescription from Faces
	const FPolygonGroupID PolygonGroupId = Description.CreatePolygonGroup();
	for (const FInitialShapeFace& Face : InFaces)
	{
		int32 VertexIndex = 0;
		TArray<FVertexInstanceID> PolygonVertexInstances;
		for (const FVector& Vertex : Face.Vertices)
		{
			const FVertexID VertexID = Description.CreateVertex();
			VertexPositions[VertexID] = Vertex;
			FVertexInstanceID InstanceId = Description.CreateVertexInstance(FVertexID(VertexIndex++));
			PolygonVertexInstances.Add(InstanceId);
		}

		Description.CreatePolygon(PolygonGroupId, PolygonVertexInstances);
	}

	// 2. Triangulate as the input initial shape is in non triangulated form
	Description.TriangulateMesh();

	// 3. Check if any of the triangles is NOT degenerate (which means the polygon has valid geometry) if all
	// faces are degenerate we return false (meaning non valid geometry)
	const float ComparisonThreshold = 0.0001;
	const float AdjustedComparisonThreshold = FMath::Max(ComparisonThreshold, MIN_flt);

	for (const FPolygonID& PolygonID : Description.Polygons().GetElementIDs())
	{
		for (const FTriangleID TriangleID : Description.GetPolygonTriangleIDs(PolygonID))
		{
			TArrayView<const FVertexInstanceID> TriangleVertexInstances = Description.GetTriangleVertexInstances(TriangleID);
			const FVertexID VertexID0 = Description.GetVertexInstanceVertex(TriangleVertexInstances[0]);
			const FVertexID VertexID1 = Description.GetVertexInstanceVertex(TriangleVertexInstances[1]);
			const FVertexID VertexID2 = Description.GetVertexInstanceVertex(TriangleVertexInstances[2]);

			const FVector Position0 = VertexPositions[VertexID0];
			const FVector DPosition1 = VertexPositions[VertexID1] - Position0;
			const FVector DPosition2 = VertexPositions[VertexID2] - Position0;

			FVector TmpNormal = FVector::CrossProduct(DPosition2, DPosition1).GetSafeNormal(AdjustedComparisonThreshold);
			if (!TmpNormal.IsNearlyZero(ComparisonThreshold))
			{
				return true;
			}
		}
	}

	return false;
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

void UInitialShape::SetInitialShapeData(const TArray<FInitialShapeFace>& InFaces)
{
	Faces = InFaces;
	bIsValid = HasValidGeometry(InFaces);
}

bool UInitialShape::CanDestroy()
{
	return !Component || Component->CreationMethod == EComponentCreationMethod::Instance;
}

void UInitialShape::Uninitialize()
{
	if (Component)
	{
		// Similarly to Unreal Ed component deletion. See ComponentEditorUtils#DeleteComponents
#if WITH_EDITOR
		Component->Modify();
#endif
		Component->DestroyComponent(true);
#if WITH_EDITOR
		AActor* Owner = Component->GetOwner();
		Owner->RerunConstructionScripts();
#endif
	}
}

void UStaticMeshInitialShape::Initialize(UActorComponent* OwnerComponent)
{
	AActor* Owner = OwnerComponent->GetOwner();
	UStaticMeshComponent* StaticMeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
	if (!StaticMeshComponent)
	{
		StaticMeshComponent = AttachComponent<UStaticMeshComponent>(Owner, TEXT("InitialShapeStaticMesh"));
	}
	Component = StaticMeshComponent;

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

bool UStaticMeshInitialShape::CanConstructFrom(UActorComponent* OwnerComponent)
{
	AActor* Owner = OwnerComponent->GetOwner();
	if (Owner)
	{
		UStaticMeshComponent* StaticMeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
		return StaticMeshComponent != nullptr && StaticMeshComponent->GetStaticMesh() != nullptr;
	}
	return false;
}

bool USplineInitialShape::CanConstructFrom(UActorComponent* OwnerComponent)
{
	AActor* Owner = OwnerComponent->GetOwner();
	if (Owner)
	{
		USplineComponent* SplineComponent = Owner->FindComponentByClass<USplineComponent>();
		return SplineComponent != nullptr && SplineComponent->GetNumberOfSplinePoints() > 0;
	}
	return false;
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
	USplineComponent* SplineComponent = Owner->FindComponentByClass<USplineComponent>();
	if (!SplineComponent)
	{
		SplineComponent = AttachComponent<USplineComponent>(Owner, TEXT("InitialShapeSpline"));
	}

	SplineComponent->SetClosedLoop(true);

	Component = SplineComponent;

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
