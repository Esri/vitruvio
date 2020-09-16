// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Components/SplineComponent.h"
#include "CoreUObject.h"

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

public:
	const TArray<FInitialShapeFace>& GetInitialShapeData() const
	{
		return Faces;
	}

	TArray<FVector> GetVertices() const;

	bool IsValid() const
	{
		return bIsValid;
	}

	void SetInitialShapeData(const TArray<FInitialShapeFace>& InFaces)
	{
		Faces = InFaces;
		bIsValid = true;
	}

	virtual void Initialize(UActorComponent* OwnerComponent)
	{
		unimplemented()
	}

	virtual bool CanConstructFrom(UActorComponent* OwnerComponent)
	{
		unimplemented() return false;
	}

	virtual bool CanDestroy()
	{
		return true;
	}

	virtual void Uninitialize()
	{
		bIsValid = false;
	}

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, FProperty* Property)
	{
		unimplemented() return false;
	}

#endif
};

UCLASS(meta = (DisplayName = "Static Mesh"))
class VITRUVIO_API UStaticMeshInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	virtual void Initialize(UActorComponent* OwnerComponent) override;
	virtual void Uninitialize() override;
	virtual bool CanConstructFrom(UActorComponent* OwnerComponent) override;
	virtual bool CanDestroy() override;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, FProperty* Property) override;
#endif
private:
	UStaticMeshComponent* StaticMeshComponent;
};

UCLASS(meta = (DisplayName = "Spline"))
class VITRUVIO_API USplineInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Meta = (UIMin = 5, UIMax = 50))
	int32 SplineApproximationPoints = 15;

	virtual void Initialize(UActorComponent* OwnerComponent) override;
	virtual void Uninitialize() override;
	virtual bool CanConstructFrom(UActorComponent* OwnerComponent) override;
	virtual bool CanDestroy() override;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, FProperty* Property) override;
#endif
private:
	USplineComponent* SplineComponent;
};
