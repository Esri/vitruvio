/* Copyright 2021 Esri
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

#include "InitialShape.h"
#include "PolygonWindings.h"
#include "VitruvioComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

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

FMeshDescription CreateMeshDescription(const TArray<FInitialShapeFace>& InFaces)
{
	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();

	// Need at least 1 uv set (can be empty) otherwise will crash when building the mesh
	const auto VertexUVs = Attributes.GetVertexInstanceUVs();
	VertexUVs.SetNumIndices(1);

	const auto VertexPositions = Attributes.GetVertexPositions();
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
		if (PolygonVertexInstances.Num() >= 3)
		{
			Description.CreatePolygon(PolygonGroupId, PolygonVertexInstances);
		}
	}

	return Description;
}

// Returns false if all faces are degenerate and true otherwise
bool HasValidGeometry(const TArray<FInitialShapeFace>& InFaces)
{
	// 1. Construct MeshDescription from Faces
	FMeshDescription Description = CreateMeshDescription(InFaces);

	// 2. Triangulate as the input initial shape is in non triangulated form
	Description.TriangulateMesh();

	// 3. Check if any of the triangles is NOT degenerate (which means the polygon has valid geometry) if all
	// faces are degenerate we return false (meaning non valid geometry)
	const float ComparisonThreshold = 0.0001;
	const float AdjustedComparisonThreshold = FMath::Max(ComparisonThreshold, MIN_flt);

	FStaticMeshAttributes Attributes(Description);
	const auto VertexPositions = Attributes.GetVertexPositions();
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

void UInitialShape::SetFaces(const TArray<FInitialShapeFace>& InFaces)
{
	Faces = InFaces;
	bIsValid = HasValidGeometry(InFaces);
}

bool UInitialShape::CanDestroy()
{
	return !InitialShapeSceneComponent || InitialShapeSceneComponent->CreationMethod == EComponentCreationMethod::Instance;
}

void UInitialShape::Uninitialize()
{
	if (InitialShapeSceneComponent)
	{
		// Similarly to Unreal Ed component deletion. See ComponentEditorUtils#DeleteComponents
#if WITH_EDITOR
		InitialShapeSceneComponent->Modify();
#endif
		// Note that promote to children of DestroyComponent only checks for attached children not actual child components
		// therefore we have to destroy them manually here
		TArray<USceneComponent*> Children;
		InitialShapeSceneComponent->GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
		{
			Child->DestroyComponent(true);
		}

		AActor* Owner = InitialShapeSceneComponent->GetOwner();

		InitialShapeSceneComponent->DestroyComponent(true);
#if WITH_EDITOR
		Owner->RerunConstructionScripts();
#endif

		InitialShapeSceneComponent = nullptr;
		VitruvioComponent = nullptr;
	}
}

void UStaticMeshInitialShape::Initialize(UVitruvioComponent* Component)
{
	Super::Initialize(Component);

	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return;
	}

	UStaticMeshComponent* StaticMeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
	if (!StaticMeshComponent)
	{
		StaticMeshComponent = AttachComponent<UStaticMeshComponent>(Owner, TEXT("InitialShapeStaticMesh"));
	}
	InitialShapeSceneComponent = StaticMeshComponent;

	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();

	if (StaticMesh == nullptr)
	{
		bIsValid = false;
		return;
	}

#if WITH_EDITORONLY_DATA
	InitialShapeMesh = StaticMesh;
#endif

#if WITH_EDITOR
	if (!StaticMesh->bAllowCPUAccess)
	{
		StaticMesh->Modify(true);
		StaticMesh->bAllowCPUAccess = true;
		StaticMesh->PostEditChange();
	}
#else
	if (!ensure(StaticMesh->bAllowCPUAccess))
	{
		bIsValid = false;
		return;
	}
#endif

	TArray<FVector> MeshVertices;
	TArray<int32> RemappedIndices;
	TArray<int32> MeshIndices;

	if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.IsValidIndex(0))
	{
		const FStaticMeshLODResources& LOD = StaticMesh->GetRenderData()->LODResources[0];

		for (auto SectionIndex = 0; SectionIndex < LOD.Sections.Num(); ++SectionIndex)
		{
			for (uint32 VertexIndex = 0; VertexIndex < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++VertexIndex)
			{
				FVector Vertex = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);

				auto VertexFindPredicate = [Vertex](const FVector& In) {
					return Vertex.Equals(In);
				};
				int32 MappedVertexIndex = MeshVertices.IndexOfByPredicate(VertexFindPredicate);

				if (MappedVertexIndex == INDEX_NONE)
				{
					RemappedIndices.Add(MeshVertices.Num());
					MeshVertices.Add(Vertex);
				}
				else
				{
					RemappedIndices.Add(MappedVertexIndex);
				}
			}

			const FStaticMeshSection& Section = LOD.Sections[SectionIndex];
			FIndexArrayView IndicesView = LOD.IndexBuffer.GetArrayView();

			for (uint32 Triangle = 0; Triangle < Section.NumTriangles; ++Triangle)
			{
				for (uint32 TriangleVertexIndex = 0; TriangleVertexIndex < 3; ++TriangleVertexIndex)
				{
					const uint32 OriginalMeshIndex = IndicesView[Section.FirstIndex + Triangle * 3 + TriangleVertexIndex];
					const uint32 MeshVertIndex = RemappedIndices[OriginalMeshIndex];
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
	SetFaces(InitialShapeFaces);
}

void UStaticMeshInitialShape::Initialize(UVitruvioComponent* Component, const TArray<FInitialShapeFace>& InitialFaces)
{
	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return;
	}

	FMeshDescription MeshDescription = CreateMeshDescription(InitialFaces);
	MeshDescription.TriangulateMesh();

	TArray<const FMeshDescription*> MeshDescriptions;
	MeshDescriptions.Emplace(&MeshDescription);

	UStaticMesh* StaticMesh;
#if WITH_EDITOR
	const FString InitialShapeName = TEXT("InitialShape");
	const FString PackageName = TEXT("/Game/Vitruvio/") + InitialShapeName;
	UPackage* Package = CreatePackage(*PackageName);
	const FName StaticMeshName = MakeUniqueObjectName(Package, UStaticMesh::StaticClass(), FName(InitialShapeName));

	StaticMesh = NewObject<UStaticMesh>(Package, StaticMeshName, RF_Public | RF_Standalone);
#else
	StaticMesh = NewObject<UStaticMesh>();
#endif

	StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);

	UStaticMeshComponent* AttachedStaticMeshComponent = AttachComponent<UStaticMeshComponent>(Owner, TEXT("InitialShapeStaticMesh"));
	AttachedStaticMeshComponent->SetStaticMesh(StaticMesh);

	Initialize(Component);
}

bool UStaticMeshInitialShape::CanConstructFrom(AActor* Owner) const
{
	if (Owner)
	{
		UStaticMeshComponent* StaticMeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
		return StaticMeshComponent != nullptr && StaticMeshComponent->GetStaticMesh() != nullptr;
	}
	return false;
}

void UStaticMeshInitialShape::SetHidden(bool bHidden)
{
	InitialShapeSceneComponent->SetVisibility(!bHidden, false);
	InitialShapeSceneComponent->SetHiddenInGame(bHidden);
}

bool USplineInitialShape::CanConstructFrom(AActor* Owner) const
{
	if (Owner)
	{
		USplineComponent* SplineComponent = Owner->FindComponentByClass<USplineComponent>();
		return SplineComponent != nullptr && SplineComponent->GetNumberOfSplinePoints() > 0;
	}
	return false;
}
#if WITH_EDITOR

bool USplineInitialShape::IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object)
	{
		FProperty* Property = PropertyChangedEvent.Property;
		return Property && (Property->GetFName() == TEXT("SplineCurves") || (Property->GetFName() == TEXT("SplineApproximationPoints") &&
																			 PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet));
	}
	return false;
}

bool UStaticMeshInitialShape::IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object)
	{
		FProperty* Property = PropertyChangedEvent.Property;
		return Property && (Property->GetFName() == TEXT("StaticMesh") || Property->GetFName() == TEXT("StaticMeshComponent"));
	}
	return false;
}

void UStaticMeshInitialShape::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
#if WITH_EDITORONLY_DATA
	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticMeshInitialShape, InitialShapeMesh))
	{
		UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(InitialShapeSceneComponent);
		StaticMeshComponent->SetStaticMesh(InitialShapeMesh);

		// We need to fire the property changed event manually
		for (TFieldIterator<FProperty> PropIt(StaticMeshComponent->GetClass()); PropIt; ++PropIt)
		{
			if (PropIt->GetFName() == TEXT("StaticMesh"))
			{
				FProperty* Property = *PropIt;
				FPropertyChangedEvent StaticMeshPropertyChangedEvent(Property);
				VitruvioComponent->OnPropertyChanged(VitruvioComponent, StaticMeshPropertyChangedEvent);
				break;
			}
		}
	}
#endif
}
#endif

void USplineInitialShape::Initialize(UVitruvioComponent* Component)
{
	Super::Initialize(Component);

	AActor* Owner = Component->GetOwner();
	USplineComponent* SplineComponent = Owner->FindComponentByClass<USplineComponent>();
	if (!SplineComponent)
	{
		SplineComponent = AttachComponent<USplineComponent>(Owner, TEXT("InitialShapeSpline"));
	}

	SplineComponent->SetClosedLoop(true);

	InitialShapeSceneComponent = SplineComponent;

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

	SetFaces({FInitialShapeFace{Vertices}});
}

void USplineInitialShape::Initialize(UVitruvioComponent* Component, const TArray<FInitialShapeFace>& InitialFaces)
{

	AActor* Owner = Component->GetOwner();

	for (const FInitialShapeFace& Face : InitialFaces)
	{
		auto UniqueName = MakeUniqueObjectName(Owner, USplineComponent::StaticClass(), TEXT("InitialShapeSpline"));
		USplineComponent* Spline = AttachComponent<USplineComponent>(Owner, UniqueName.ToString());
		Spline->ClearSplinePoints(true);

		int32 PointIndex = 0;
		for (const FVector& Position : Face.Vertices)
		{
			Spline->AddSplineLocalPoint(Position);
			Spline->SetSplinePointType(PointIndex, ESplinePointType::Linear, true);
			PointIndex++;
		}
	}

	Initialize(Component);
}
