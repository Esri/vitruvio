#pragma once

#include "CoreUObject.h"

#include "InitialShape.generated.h"

UCLASS()
class UInitialShape : public UObject
{
public:
	GENERATED_BODY()

	// Non triangulated vertices per face
	TArray<TArray<FVector>> FaceVertices;

	TArray<FVector> GetVertices() const;
};

UCLASS()
class USplineInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Meta = (UIMin = 5, UIMax = 50))
	int32 SplineApproximationPoints;
};
