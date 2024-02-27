#pragma once

#include "VitruvioMesh.h"

#include "Interfaces/Interface_CollisionDataProvider.h"

class VITRUVIO_API FCustomCollisionDataProvider : public IInterface_CollisionDataProvider
{
	virtual bool GetPhysicsTriMeshData(FTriMeshCollisionData* TriCollisionData, bool InUseAllTriData) override
	{
		if (!CollisionData.IsValid())
		{
			return false;
		}

		TriCollisionData->Indices = CollisionData.Indices;
		TArray<uint16> MaterialIndices;
		MaterialIndices.SetNumZeroed(CollisionData.Indices.Num());
		TriCollisionData->MaterialIndices = MaterialIndices;
		TriCollisionData->Vertices = CollisionData.Vertices;
		TriCollisionData->bFlipNormals = true;
		return true;
	}

	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override
	{
		return CollisionData.IsValid();
	}

public:
	void SetCollisionData(const FCollisionData& InCollisionData)
	{
		CollisionData = InCollisionData;
	}

private:
	FCollisionData CollisionData;
};