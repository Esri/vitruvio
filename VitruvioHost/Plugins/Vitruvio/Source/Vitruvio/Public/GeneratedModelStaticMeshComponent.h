// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Components/StaticMeshComponent.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "VitruvioTypes.h"

#include "GeneratedModelStaticMeshComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VITRUVIO_API UGeneratedModelStaticMeshComponent : public UStaticMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

	virtual bool GetPhysicsTriMeshData(FTriMeshCollisionData* TriCollisionData, bool InUseAllTriData) override
	{
		if (!CollisionData.IsValid())
		{
			return false;
		}

		TriCollisionData->Indices = CollisionData.Indices;
		TriCollisionData->Vertices = CollisionData.Vertices;
		TriCollisionData->bFlipNormals = true;
		return true;
	}

	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override
	{
		return CollisionData.IsValid();
	}

public:
	void SetCollisionData(const Vitruvio::FCollisionData& InCollisionData)
	{
		CollisionData = InCollisionData;
	}

private:
	Vitruvio::FCollisionData CollisionData;
};