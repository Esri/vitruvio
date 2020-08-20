#pragma once

#include "CoreUObject.h"

#include "InitialShape.generated.h"

USTRUCT()
struct FInitialShapeFace
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> Vertices;
};

UCLASS()
class UInitialShape : public UObject
{
	GENERATED_BODY()

	// Non triangulated vertices per face
	UPROPERTY()
	TArray<FInitialShapeFace> Faces;

public:
	const TArray<FInitialShapeFace>& GetInitialShapeData() const { return Faces; }

	TArray<FVector> GetVertices() const;

	void SetInitialShapeData(const TArray<FInitialShapeFace>& InFaces) { Faces = InFaces; }
};

UCLASS()
class USplineInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Meta = (UIMin = 5, UIMax = 50))
	int32 SplineApproximationPoints = 15;
};
