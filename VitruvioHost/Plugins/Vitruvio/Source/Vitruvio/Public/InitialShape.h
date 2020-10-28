// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Components/SplineComponent.h"
#include "CoreUObject.h"

#include "StaticMeshAttributes.h"
#include "StaticMeshDescription.h"

#include "InitialShape.generated.h"

USTRUCT()
struct VITRUVIO_API FInitialShapeFace
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> Vertices;
};

UCLASS(Abstract)
class VITRUVIO_API UInitialShape : public UObject
{
	GENERATED_BODY()

	// Non triangulated vertices per face
	UPROPERTY()
	TArray<FInitialShapeFace> Faces;

protected:
	UPROPERTY()
	bool bIsValid;

	UPROPERTY()
	USceneComponent* Component;

public:
	const TArray<FInitialShapeFace>& GetFaces() const
	{
		return Faces;
	}

	TArray<FVector> GetVertices() const;

	bool IsValid() const
	{
		return bIsValid;
	}

	void SetFaces(const TArray<FInitialShapeFace>& InFaces);

	virtual void Initialize(UActorComponent* OwnerComponent, const TArray<FInitialShapeFace>& InitialFaces)
	{
		unimplemented();
	}

	virtual void Initialize(UActorComponent* OwnerComponent)
	{
		unimplemented();
	}

	virtual bool CanConstructFrom(AActor* Owner) const
	{
		unimplemented();
		return false;
	}

	virtual bool CanDestroy();
	virtual void Uninitialize();

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent)
	{
		unimplemented();
		return false;
	}

#endif
};

UCLASS(meta = (DisplayName = "Static Mesh"))
class VITRUVIO_API UStaticMeshInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	virtual void Initialize(UActorComponent* OwnerComponent) override;
	virtual void Initialize(UActorComponent* OwnerComponent, const TArray<FInitialShapeFace>& InitialFaces) override;
	virtual bool CanConstructFrom(AActor* Owner) const override;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

UCLASS(meta = (DisplayName = "Spline"))
class VITRUVIO_API USplineInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Vitruvio", Meta = (UIMin = 5, UIMax = 50))
	int32 SplineApproximationPoints = 15;

	virtual void Initialize(UActorComponent* OwnerComponent) override;
	virtual void Initialize(UActorComponent* OwnerComponent, const TArray<FInitialShapeFace>& InitialFaces) override;
	virtual bool CanConstructFrom(AActor* Owner) const override;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
