/* Copyright 2022 Esri
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

#include "CompGeom/PolygonTriangulation.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/SavePackage.h"

namespace
{
template <typename T>
T* AttachComponent(AActor* Owner, const FString& Name, bool bAttachToRoot = true)
{
	T* Component = NewObject<T>(Owner, *Name, RF_DuplicateTransient | RF_Transient | RF_TextExportTransient);
	Component->Mobility = EComponentMobility::Movable;
	Owner->AddOwnedComponent(Component);
	Component->CreationMethod = EComponentCreationMethod::Instance;
	if (bAttachToRoot)
	{
		Component->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}
	Component->OnComponentCreated();
	Component->RegisterComponent();
	return Component;
}

// Create a mesh description from an initial shape polygon. Ignores holes for now.
FMeshDescription CreateMeshDescription(const FInitialShapePolygon& InPolygon)
{
	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();

	// Need at least 1 uv set (can be empty) otherwise will crash when building the mesh
	const auto VertexUVs = Attributes.GetVertexInstanceUVs();
	VertexUVs.SetNumChannels(1);

	const auto VertexPositions = Attributes.GetVertexPositions();
	const FPolygonGroupID PolygonGroupId = Description.CreatePolygonGroup();

	for (const FVector3f& Vertex : InPolygon.Vertices)
	{
		const FVertexID VertexID = Description.CreateVertex();
		VertexPositions[VertexID] = Vertex;
	}

	for (const FInitialShapeFace& Face : InPolygon.Faces)
	{
		TArray<FVertexInstanceID> PolygonVertexInstances;
		for (const int32& VertexIndex : Face.Indices)
		{
			FVertexInstanceID InstanceId = Description.CreateVertexInstance(FVertexID(VertexIndex));
			PolygonVertexInstances.Add(InstanceId);
		}

		if (PolygonVertexInstances.Num() >= 3)
		{
			Description.CreatePolygon(PolygonGroupId, PolygonVertexInstances);
		}
	}

	return Description;
}

// Returns false if all faces are degenerate or true otherwise
bool HasValidGeometry(const FInitialShapePolygon& Polygon)
{
	// 1. Construct MeshDescription from Polygon
	FMeshDescription Description = CreateMeshDescription(Polygon);

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
		for (const FTriangleID TriangleID : Description.GetPolygonTriangles(PolygonID))
		{
			TArrayView<const FVertexInstanceID> TriangleVertexInstances = Description.GetTriangleVertexInstances(TriangleID);
			const FVertexID VertexID0 = Description.GetVertexInstanceVertex(TriangleVertexInstances[0]);
			const FVertexID VertexID1 = Description.GetVertexInstanceVertex(TriangleVertexInstances[1]);
			const FVertexID VertexID2 = Description.GetVertexInstanceVertex(TriangleVertexInstances[2]);

			const FVector3f Position0 = VertexPositions[VertexID0];
			const FVector3f DPosition1 = VertexPositions[VertexID1] - Position0;
			const FVector3f DPosition2 = VertexPositions[VertexID2] - Position0;

			FVector3f TmpNormal = FVector3f::CrossProduct(DPosition2, DPosition1).GetSafeNormal(AdjustedComparisonThreshold);
			if (!TmpNormal.IsNearlyZero(ComparisonThreshold))
			{
				return true;
			}
		}
	}

	return false;
}

FInitialShapePolygon CreateInitialPolygonFromStaticMesh(const UStaticMesh* StaticMesh)
{
	TArray<FVector3f> MeshVertices;
	TArray<int32> MeshIndices;
	TArray<FTextureCoordinateSet> MeshTextureCoordinateSets;
	MeshTextureCoordinateSets.AddDefaulted(8);

	if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.IsValidIndex(0))
	{
		TArray<int32> RemappedIndices;
		const FStaticMeshLODResources& LOD = StaticMesh->GetRenderData()->LODResources[0];

		for (auto SectionIndex = 0; SectionIndex < LOD.Sections.Num(); ++SectionIndex)
		{
			for (uint32 VertexIndex = 0; VertexIndex < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++VertexIndex)
			{
				FVector3f Vertex = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);

				auto VertexFindPredicate = [Vertex](const FVector3f& In) {
					return Vertex.Equals(In);
				};
				int32 MappedVertexIndex = MeshVertices.IndexOfByPredicate(VertexFindPredicate);

				if (MappedVertexIndex == INDEX_NONE)
				{
					RemappedIndices.Add(MeshVertices.Num());
					MeshVertices.Add(Vertex);
					for (uint32 TextCoordIndex = 0; TextCoordIndex < LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords(); ++TextCoordIndex)
					{
						FVector2f TexCoord = LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndex, TextCoordIndex);
						MeshTextureCoordinateSets[TextCoordIndex].TextureCoordinates.Add(TexCoord);
					}
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

	FInitialShapePolygon InitialShapePolygon = Vitruvio::GetPolygon(MeshVertices, MeshIndices);
	InitialShapePolygon.FixOrientation();

	return InitialShapePolygon;
}

FInitialShapePolygon CreateInitialShapePolygonFromSpline(const USplineComponent* SplineComponent, const uint32 SplineApproximationPoints)
{
	TArray<FVector3f> Vertices;
	TArray<int> Indices;
	const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
	for (int32 SplinePointIndex = 0; SplinePointIndex < NumPoints; ++SplinePointIndex)
	{
		const ESplinePointType::Type SplineType = SplineComponent->GetSplinePointType(SplinePointIndex);
		if (SplineType == ESplinePointType::Linear)
		{
			Indices.Add(Vertices.Num());
			Vertices.Add(FVector3f(SplineComponent->GetLocationAtSplinePoint(SplinePointIndex, ESplineCoordinateSpace::Local)));
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
				Indices.Add(Vertices.Num());
				Vertices.Add(FVector3f(SplineComponent->GetLocationAtDistanceAlongSpline(Position, ESplineCoordinateSpace::Local)));
				Position += Distance;
			}
		}
	}

	FInitialShapeFace Face = {Indices};
	return {{Face}, Vertices};
}

FInitialShapePolygon CreateDefaultInitialShapePolygon()
{
	FInitialShapePolygon Polygon;
	Polygon.Vertices = {FVector3f(1000, -1000, 0), FVector3f(-1000, -1000, 0), FVector3f(-1000, 1000, 0), FVector3f(1000, 1000, 0)};
	FInitialShapeFace InitialShapeFace;
	InitialShapeFace.Indices = {0, 1, 2, 3};
	Polygon.Faces.Add(InitialShapeFace);

	return Polygon;
}

bool IsDefaultInitialShape(const FInitialShapePolygon& InitialShapePolygon)
{
	const FInitialShapePolygon DefaultInitialShapePolygon = CreateDefaultInitialShapePolygon();
	check(DefaultInitialShapePolygon.Faces.Num() == 1);
	const TArray<FVector3f>& DefaultVertices = DefaultInitialShapePolygon.Vertices;
	const TArray<int32>& DefaultIndices = DefaultInitialShapePolygon.Faces[0].Indices;
	check(DefaultVertices.Num() == 4);

	if (InitialShapePolygon.Faces.Num() != DefaultInitialShapePolygon.Faces.Num())
	{
		return false;
	}
	const TArray<FVector3f>& Vertices = InitialShapePolygon.Vertices;
	const TArray<int32>& Indices = InitialShapePolygon.Faces[0].Indices;

	if (Vertices.Num() != DefaultVertices.Num() || Indices.Num() != DefaultIndices.Num())
	{
		return false;
	}

	const FVector3f FirstVertex = DefaultVertices[Indices[0]];
	int32 InitialIndexOffset;
	if (Vertices.Find(FirstVertex, InitialIndexOffset))
	{
		for (int32 CurrentIndex = 0; CurrentIndex < DefaultVertices.Num(); CurrentIndex++)
		{
			const int32 VertexIndex = Indices[(InitialIndexOffset + CurrentIndex) % Vertices.Num()];
			if (!Vertices[VertexIndex].Equals(DefaultVertices[DefaultIndices[CurrentIndex]]))
			{
				return false;
			}
		}
		return true;
	}

	return false;
}

UStaticMesh* CreateDefaultStaticMesh()
{
	UStaticMesh* StaticMesh;

#if WITH_EDITOR
	const FString InitialShapeName = TEXT("DefaultInitialShape");
	const FName StaticMeshName = FName(InitialShapeName);
	const FString PackageName = TEXT("/Game/Vitruvio/") + InitialShapeName;

	UPackage* Package = LoadPackage(nullptr, ToCStr(PackageName), LOAD_None);

	if (Package != nullptr)
	{
		StaticMesh = FindObjectFast<UStaticMesh>(Package, StaticMeshName);
		if (StaticMesh != nullptr)
		{
			return StaticMesh;
		}
	}
#endif

	const FInitialShapePolygon InitialShapePolygon = CreateDefaultInitialShapePolygon();
	FMeshDescription MeshDescription = CreateMeshDescription(InitialShapePolygon);
	MeshDescription.TriangulateMesh();

	TArray<const FMeshDescription*> MeshDescriptions;
	MeshDescriptions.Emplace(&MeshDescription);

#if WITH_EDITOR
	Package = CreatePackage(*PackageName);
	StaticMesh = NewObject<UStaticMesh>(Package, StaticMeshName, RF_Public | RF_Standalone | RF_Transactional);
#else
	StaticMesh = NewObject<UStaticMesh>();
#endif

	StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);

#if WITH_EDITOR
	const FString PackageFileName = InitialShapeName + FPackageName::GetAssetPackageExtension();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, StaticMesh, *PackageFileName, Args);
#endif

	return StaticMesh;
}

UStaticMesh* CreateStaticMeshFromInitialShapePolygon(const FInitialShapePolygon& InitialShapePolygon)
{
	if (IsDefaultInitialShape(InitialShapePolygon) || InitialShapePolygon.Faces.Num() == 0)
	{
		return CreateDefaultStaticMesh();
	}

	FMeshDescription MeshDescription = CreateMeshDescription(InitialShapePolygon);
	MeshDescription.TriangulateMesh();

	TArray<const FMeshDescription*> MeshDescriptions;
	MeshDescriptions.Emplace(&MeshDescription);

	UStaticMesh* StaticMesh;
#if WITH_EDITOR
	const FString InitialShapeName = TEXT("InitialShape");
	const FString PackageName = TEXT("/Game/Vitruvio/") + InitialShapeName;
	UPackage* Package = CreatePackage(*PackageName);
	const FName StaticMeshName = MakeUniqueObjectName(Package, UStaticMesh::StaticClass(), FName(InitialShapeName));

	StaticMesh = NewObject<UStaticMesh>(Package, StaticMeshName, RF_Public | RF_Standalone | RF_Transactional);
#else
	StaticMesh = NewObject<UStaticMesh>();
#endif

	StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
	return StaticMesh;
}

TArray<FSplinePoint> CreateSplinePointsFromInitialShapePolygon(const FInitialShapePolygon& InitialShapePolygon)
{
	// create small default square footprint, if there is no startMesh
	FInitialShapePolygon CurrInitialShapePolygon = InitialShapePolygon;
	if (CurrInitialShapePolygon.Faces.Num() == 0)
	{
		CurrInitialShapePolygon = CreateDefaultInitialShapePolygon();
	}

	TArray<FSplinePoint> SplinePoints;
	int32 PointIndex = 0;
	for (const int32& Index : CurrInitialShapePolygon.Faces[0].Indices)
	{
		FSplinePoint SplinePoint;
		SplinePoint.Position = FVector(CurrInitialShapePolygon.Vertices[Index]);
		SplinePoint.Type = ESplinePointType::Linear;
		SplinePoint.InputKey = PointIndex;
		SplinePoints.Add(SplinePoint);
		PointIndex++;
	}
	return SplinePoints;
}

} // namespace

void FInitialShapePolygon::FixOrientation()
{
	for (FInitialShapeFace& Face : Faces)
	{
		if (Face.Indices.Num() < 3)
		{
			continue;
		}

		// Reverse all indices if plane normal points down
		TArray<FVector3f> VertexPositions;
		VertexPositions.Reserve(Vertices.Num());
		for (const auto& Index : Face.Indices)
		{
			VertexPositions.Add(FVector3f(Vertices[Index].X, Vertices[Index].Y, Vertices[Index].Z));
		}
		FVector3f PlanePointOut;
		FVector3f PlaneNormalOut;
		PolygonTriangulation::ComputePolygonPlane(VertexPositions, PlaneNormalOut, PlanePointOut);

		const FVector PlaneNormal(PlaneNormalOut.X, PlaneNormalOut.Y, PlaneNormalOut.Z);
		const float Dot = FVector::DotProduct(FVector::UpVector, PlaneNormal);
		if (Dot < 0)
		{
			Algo::Reverse(Face.Indices);
			for (FInitialShapeHole& Hole : Face.Holes)
			{
				Algo::Reverse(Hole.Indices);
			}
		}
	}
}

void UInitialShape::SetPolygon(const FInitialShapePolygon& NewPolygon)
{
	Polygon = NewPolygon;
	bIsPolygonValid = HasValidGeometry(Polygon);
}

const TArray<FVector3f>& UInitialShape::GetVertices() const
{
	return Polygon.Vertices;
}

bool UInitialShape::IsValid() const
{
	return bIsPolygonValid;
}

void UInitialShape::Initialize(UVitruvioComponent* Component)
{
	UpdatePolygon(Component);
}

USceneComponent* UStaticMeshInitialShape::CreateInitialShapeComponent(UVitruvioComponent* Component)
{
	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UStaticMeshComponent* StaticMeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>())
	{
#if WITH_EDITOR
		InitialShapeMesh = StaticMeshComponent->GetStaticMesh();
#endif
		return StaticMeshComponent;
	}

	return CreateInitialShapeComponent(Component, CreateDefaultStaticMesh());
}

USceneComponent* UStaticMeshInitialShape::CreateInitialShapeComponent(UVitruvioComponent* Component, const FInitialShapePolygon& InitialShapePolygon)
{
	return CreateInitialShapeComponent(Component, CreateStaticMeshFromInitialShapePolygon(InitialShapePolygon));
}

USceneComponent* UStaticMeshInitialShape::CreateInitialShapeComponent(UVitruvioComponent* Component, UStaticMesh* NewStaticMesh)
{
	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	UStaticMeshComponent* StaticMeshComponent = AttachComponent<UStaticMeshComponent>(Owner, TEXT("InitialShapeStaticMesh"));

	StaticMeshComponent->SetStaticMesh(NewStaticMesh);
#if WITH_EDITORONLY_DATA
	InitialShapeMesh = NewStaticMesh;
#endif

	return StaticMeshComponent;
}

void UStaticMeshInitialShape::UpdatePolygon(UVitruvioComponent* Component)
{
#if WITH_EDITOR
	Modify();
#endif

	UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component->InitialShapeSceneComponent);
	SetPolygon(StaticMeshComponent && StaticMeshComponent->GetStaticMesh() ? CreateInitialPolygonFromStaticMesh(StaticMeshComponent->GetStaticMesh())
																		   : CreateDefaultInitialShapePolygon());
}

void UStaticMeshInitialShape::UpdateSceneComponent(UVitruvioComponent* Component)
{
	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component->InitialShapeSceneComponent))
	{
		FInitialShapePolygon OldPolygon = {};
		if (StaticMeshComponent->GetStaticMesh())
		{
			OldPolygon = CreateInitialPolygonFromStaticMesh(StaticMeshComponent->GetStaticMesh());
		}

		if (OldPolygon != GetPolygon())
		{
			UStaticMesh* NewStaticMesh = CreateStaticMeshFromInitialShapePolygon(GetPolygon());
			StaticMeshComponent->SetStaticMesh(NewStaticMesh);

#if WITH_EDITOR
			InitialShapeMesh = NewStaticMesh;
#endif
		}
	}
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

USceneComponent* UStaticMeshInitialShape::CopySceneComponent(AActor* OldActor, AActor* NewActor) const
{
	const UStaticMeshComponent* OldStaticMeshComponent = OldActor->FindComponentByClass<UStaticMeshComponent>();
	USceneComponent* RootComponent = AttachComponent<USceneComponent>(NewActor, TEXT("RootComponent"), false);
	NewActor->SetRootComponent(RootComponent);

	UStaticMeshComponent* NewStaticMeshComponent = AttachComponent<UStaticMeshComponent>(NewActor, TEXT("InitialShapeStaticMesh"), true);
	if (OldStaticMeshComponent)
	{
		NewStaticMeshComponent->SetStaticMesh(OldStaticMeshComponent->GetStaticMesh());
		RootComponent->SetWorldTransform(OldStaticMeshComponent->GetComponentTransform());
	}

	return NewStaticMeshComponent;
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

USceneComponent* USplineInitialShape::CopySceneComponent(AActor* OldActor, AActor* NewActor) const
{
	const USplineComponent* OldSplineComponent = OldActor->FindComponentByClass<USplineComponent>();
	USceneComponent* RootComponent = AttachComponent<USceneComponent>(NewActor, TEXT("RootComponent"), false);
	NewActor->SetRootComponent(RootComponent);

	USplineComponent* NewSplineComponent = AttachComponent<USplineComponent>(NewActor, TEXT("InitialShapeSpline"), true);
	NewSplineComponent->SetClosedLoop(true);
	if (OldSplineComponent)
	{
		NewSplineComponent->SplineCurves = OldSplineComponent->SplineCurves;
		RootComponent->SetWorldTransform(OldSplineComponent->GetComponentTransform());
	}
	return NewSplineComponent;
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

bool USplineInitialShape::ShouldConvert(const FInitialShapePolygon& InitialShapePolygon)
{
	if (InitialShapePolygon.Faces.Num() > 1 || (InitialShapePolygon.Faces.Num() > 0 && InitialShapePolygon.Faces[0].Holes.Num() > 0))
	{
		auto Result = FMessageDialog::Open(EAppMsgType::OkCancel,
										   FText::FromString(TEXT("The initial shape contains multiple faces or faces with holes which spline "
																  "initial shapes do not support. Continuing will remove them.")));
		if (Result == EAppReturnType::Cancel)
		{
			return false;
		}
	}
	return true;
}

void UStaticMeshInitialShape::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
#if WITH_EDITORONLY_DATA
	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticMeshInitialShape, InitialShapeMesh))
	{
		if (AActor* Owner = Cast<AActor>(GetOuter()))
		{
			if (UVitruvioComponent* VitruvioComponent = Owner->FindComponentByClass<UVitruvioComponent>())
			{
				UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(VitruvioComponent->InitialShapeSceneComponent);
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
		}
	}
#endif
}
#endif

USceneComponent* USplineInitialShape::CreateInitialShapeComponent(UVitruvioComponent* Component)
{
	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (USplineComponent* SplineComponent = Owner->FindComponentByClass<USplineComponent>())
	{
		return SplineComponent;
	}

	return CreateInitialShapeComponent(Component, CreateSplinePointsFromInitialShapePolygon(CreateDefaultInitialShapePolygon()));
}

USceneComponent* USplineInitialShape::CreateInitialShapeComponent(UVitruvioComponent* Component, const FInitialShapePolygon& InitialShapePolygon)
{
	return CreateInitialShapeComponent(Component, CreateSplinePointsFromInitialShapePolygon(InitialShapePolygon));
}

USceneComponent* USplineInitialShape::CreateInitialShapeComponent(UVitruvioComponent* Component, const TArray<FSplinePoint>& SplinePoints)
{
	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	const auto UniqueName = MakeUniqueObjectName(Owner, USplineComponent::StaticClass(), TEXT("InitialShapeSpline"));
	USplineComponent* SplineComponent = AttachComponent<USplineComponent>(Owner, UniqueName.ToString());
	SplineComponent->ClearSplinePoints(true);
	for (const auto& Point : SplinePoints)
	{
		SplineComponent->AddPoint(Point, true);
	}
	SplineComponent->SetClosedLoop(true);

	return SplineComponent;
}

void USplineInitialShape::UpdatePolygon(UVitruvioComponent* Component)
{
#if WITH_EDITOR
	Modify();
#endif

	USplineComponent* SplineComponent = Cast<USplineComponent>(Component->InitialShapeSceneComponent);
	SetPolygon(SplineComponent ? CreateInitialShapePolygonFromSpline(SplineComponent, SplineApproximationPoints)
							   : CreateDefaultInitialShapePolygon());
}

void USplineInitialShape::UpdateSceneComponent(UVitruvioComponent* Component)
{
	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (USplineComponent* SplineComponent = Cast<USplineComponent>(Component->InitialShapeSceneComponent))
	{
		FInitialShapePolygon OldPolygon = CreateInitialShapePolygonFromSpline(SplineComponent, SplineApproximationPoints);

		if (OldPolygon != GetPolygon())
		{
			SplineComponent->ClearSplinePoints(true);

			TArray<FSplinePoint> SplinePoints = CreateSplinePointsFromInitialShapePolygon(GetPolygon());
			for (const auto& Point : SplinePoints)
			{
				SplineComponent->AddPoint(Point, true);
			}
			SplineComponent->SetClosedLoop(true);
		}
	}
}
