#pragma once

#include "CoreUObject.h"

#include "InitialShape.generated.h"

struct FInitialShapeData
{
public:
	FInitialShapeData() : FaceVertices({}) {}
	explicit FInitialShapeData(const TArray<TArray<FVector>>& FaceVertices) : FaceVertices(FaceVertices) {}

	TArray<FVector> GetVertices() const;

	const TArray<TArray<FVector>>& GetFaceVertices() const { return FaceVertices; }

private:
	// Non triangulated vertices per face
	TArray<TArray<FVector>> FaceVertices;
};

UCLASS()
class UInitialShape : public UObject
{
	GENERATED_BODY()

private:
	FInitialShapeData Data = {};

public:
	const FInitialShapeData& GetInitialShapeData() const { return Data; }

	void SetInitialShapeData(const FInitialShapeData& InitialShapeData) { Data = InitialShapeData; }
};

UCLASS()
class USplineInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Meta = (UIMin = 5, UIMax = 50))
	int32 SplineApproximationPoints = 15;
};
