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

#include "Components/StaticMeshComponent.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "VitruvioMesh.h"

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