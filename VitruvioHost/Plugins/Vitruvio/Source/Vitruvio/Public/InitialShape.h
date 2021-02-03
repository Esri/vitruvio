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

#pragma once

#include "Components/SplineComponent.h"
#include "CoreUObject.h"

#include "StaticMeshAttributes.h"
#include "StaticMeshDescription.h"

#include "InitialShape.generated.h"

class UVitruvioComponent;

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
	USceneComponent* InitialShapeSceneComponent;

	UPROPERTY()
	UVitruvioComponent* VitruvioComponent;

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

	USceneComponent* GetComponent() const
	{
		return InitialShapeSceneComponent;
	}

	void SetFaces(const TArray<FInitialShapeFace>& InFaces);

	virtual void Initialize(UVitruvioComponent* Component, const TArray<FInitialShapeFace>& InitialFaces)
	{
		unimplemented();
	}

	virtual void Initialize(UVitruvioComponent* Component)
	{
		VitruvioComponent = Component;
	}

	virtual bool CanConstructFrom(AActor* Owner) const
	{
		unimplemented();
		return false;
	}

	virtual void SetHidden(bool bHidden) {}

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

	virtual void Initialize(UVitruvioComponent* Component) override;
	virtual void Initialize(UVitruvioComponent* Component, const TArray<FInitialShapeFace>& InitialFaces) override;
	virtual bool CanConstructFrom(AActor* Owner) const override;
	virtual void SetHidden(bool bHidden) override;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Vitruvio", TextExportTransient, Transient, DuplicateTransient)
	UStaticMesh* InitialShapeMesh;
#endif
};

UCLASS(meta = (DisplayName = "Spline"))
class VITRUVIO_API USplineInitialShape : public UInitialShape
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Vitruvio", Meta = (UIMin = 5, UIMax = 50))
	int32 SplineApproximationPoints = 15;

	virtual void Initialize(UVitruvioComponent* Component) override;
	virtual void Initialize(UVitruvioComponent* Component, const TArray<FInitialShapeFace>& InitialFaces) override;
	virtual bool CanConstructFrom(AActor* Owner) const override;

#if WITH_EDITOR
	virtual bool IsRelevantProperty(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
