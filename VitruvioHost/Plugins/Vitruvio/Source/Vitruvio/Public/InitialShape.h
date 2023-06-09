/* Copyright 2023 Esri
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

#pragma once

#include "Components/SplineComponent.h"

#include "InitialShape.generated.h"

class UVitruvioComponent;

USTRUCT()
struct VITRUVIO_API FTextureCoordinateSet
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2f> TextureCoordinates;

	friend bool operator==(const FTextureCoordinateSet& Lhs, const FTextureCoordinateSet& RHS)
	{
		return Lhs.TextureCoordinates == RHS.TextureCoordinates;
	}

	friend bool operator!=(const FTextureCoordinateSet& Lhs, const FTextureCoordinateSet& RHS)
	{
		return !(Lhs == RHS);
	}
};

USTRUCT()
struct VITRUVIO_API FInitialShapeHole
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> Indices;

	friend bool operator==(const FInitialShapeHole& Lhs, const FInitialShapeHole& RHS)
	{
		return Lhs.Indices == RHS.Indices;
	}

	friend bool operator!=(const FInitialShapeHole& Lhs, const FInitialShapeHole& RHS)
	{
		return !(Lhs == RHS);
	}
};

USTRUCT()
struct VITRUVIO_API FInitialShapeFace
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> Indices;

	UPROPERTY()
	TArray<FInitialShapeHole> Holes;

	friend bool operator==(const FInitialShapeFace& Lhs, const FInitialShapeFace& RHS)
	{
		return Lhs.Indices == RHS.Indices && Lhs.Holes == RHS.Holes;
	}

	friend bool operator!=(const FInitialShapeFace& Lhs, const FInitialShapeFace& RHS)
	{
		return !(Lhs == RHS);
	}
};

USTRUCT()
struct VITRUVIO_API FInitialShapePolygon
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInitialShapeFace> Faces;

	UPROPERTY()
	TArray<FVector3f> Vertices;

	UPROPERTY()
	TArray<FTextureCoordinateSet> TextureCoordinateSets;

	void FixOrientation();

	friend bool operator==(const FInitialShapePolygon& Lhs, const FInitialShapePolygon& RHS)
	{
		return Lhs.Faces == RHS.Faces && Lhs.Vertices == RHS.Vertices && Lhs.TextureCoordinateSets == RHS.TextureCoordinateSets;
	}

	friend bool operator!=(const FInitialShapePolygon& Lhs, const FInitialShapePolygon& RHS)
	{
		return !(Lhs == RHS);
	}
};

UCLASS(Abstract)
class VITRUVIO_API UInitialShape : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
	FInitialShapePolygon Polygon;

	UPROPERTY()
	bool bIsPolygonValid = false;

public:
	const FInitialShapePolygon& GetPolygon() const
	{
		return Polygon;
	}

	void SetPolygon(const FInitialShapePolygon& NewPolygon);

	const TArray<FVector3f>& GetVertices() const;
	bool IsValid() const;
	void Initialize();

	virtual USceneComponent* CreateInitialShapeComponent(UVitruvioComponent* Component)
	{
		unimplemented();
		return nullptr;
	}

	virtual void UpdatePolygon(UVitruvioComponent* Component)
	{
		unimplemented();
	}

	virtual void UpdateSceneComponent(UVitruvioComponent* Component)
	{
		unimplemented();
	}

	virtual bool CanConstructFrom(AActor* Owner) const
	{
		unimplemented();
		return false;
	}

	virtual USceneComponent* CopySceneComponent(AActor* OldActor, AActor* NewActor) const
	{
		unimplemented();
		return nullptr;
	}

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent)
	{
		unimplemented();
		return false;
	}

	virtual bool ShouldConvert(const FInitialShapePolygon& InitialShapePolygon)
	{
		return true;
	}

#endif
};

UCLASS(meta = (DisplayName = "Static Mesh"))
class VITRUVIO_API UStaticMeshInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	virtual USceneComponent* CreateInitialShapeComponent(UVitruvioComponent* Component) override;
	USceneComponent* CreateInitialShapeComponent(UVitruvioComponent* Component, UStaticMesh* StaticMesh);
	virtual void UpdatePolygon(UVitruvioComponent* Component) override;
	void UpdateSceneComponent(UVitruvioComponent* Component) override;
	virtual bool CanConstructFrom(AActor* Owner) const override;
	virtual USceneComponent* CopySceneComponent(AActor* OldActor, AActor* NewActor) const override;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	TSoftObjectPtr<UStaticMesh> InitialShapeMesh;
#endif
};

UCLASS(meta = (DisplayName = "Spline"))
class VITRUVIO_API USplineInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Vitruvio", Meta = (UIMin = 5, UIMax = 50))
	int32 SplineApproximationPoints = 15;

	virtual USceneComponent* CreateInitialShapeComponent(UVitruvioComponent* Component) override;
	USceneComponent* CreateInitialShapeComponent(UVitruvioComponent* Component, const TArray<FSplinePoint>& SplinePoints);
	virtual void UpdatePolygon(UVitruvioComponent* Component) override;
	virtual void UpdateSceneComponent(UVitruvioComponent* Component) override;
	virtual bool CanConstructFrom(AActor* Owner) const override;
	virtual USceneComponent* CopySceneComponent(AActor* OldActor, AActor* NewActor) const override;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldConvert(const FInitialShapePolygon& InitialShapePolygon) override;
#endif
};
