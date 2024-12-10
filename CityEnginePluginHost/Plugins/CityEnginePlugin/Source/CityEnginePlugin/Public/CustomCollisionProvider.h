#pragma once

#include "CityEngineTypes.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "CustomCollisionProvider.generated.h"

UCLASS()
class CITYENGINEPLUGIN_API UCustomCollisionDataProvider : public UObject, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

protected:
	CityEngine::FCollisionData CollisionData;
	
	bool UpdateTrieMeshCollisionData(FTriMeshCollisionData* TriCollisionData) const
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
	
public:

	void SetCollisionData(const CityEngine::FCollisionData& InCollisionData)
	{
		CollisionData = InCollisionData;
	}

	void ClearCollisionData()
	{
		CollisionData = {};
	}

	virtual bool GetPhysicsTriMeshData(FTriMeshCollisionData* TriCollisionData, bool InUseAllTriData) override
	{
		return UpdateTrieMeshCollisionData(TriCollisionData);
	}

	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override
	{
		return CollisionData.IsValid();
	}
};