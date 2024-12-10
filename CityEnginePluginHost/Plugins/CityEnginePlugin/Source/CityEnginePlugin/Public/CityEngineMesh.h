/* Copyright 2024 Esri
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

#include "CustomCollisionProvider.h"
#include "MeshDescription.h"
#include "CityEngineTypes.h"
#include "Runtime/PhysicsCore/Public/Interface_CollisionDataProviderCore.h"


UMaterialInstanceDynamic* CacheMaterial(UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent,
										TMap<FString, CityEngine::FTextureData>& TextureCache,
										TMap<CityEngine::FMaterialAttributeContainer, TObjectPtr<UMaterialInstanceDynamic>>& MaterialCache,
										const CityEngine::FMaterialAttributeContainer& MaterialAttributes, TMap<FString, int32>& UniqueMaterialNames,
										TMap<UMaterialInterface*, FString>& MaterialIdentifiers, UObject* Outer);

class FCityEngineMesh
{
	FString Identifier;

	FMeshDescription MeshDescription;
	TArray<CityEngine::FMaterialAttributeContainer> Materials;

	UStaticMesh* StaticMesh;
	UCustomCollisionDataProvider* CollisionDataProvider;

public:
	FCityEngineMesh(const FString& Identifier, const FMeshDescription& MeshDescription,
				  const TArray<CityEngine::FMaterialAttributeContainer>& Materials)
		: Identifier(Identifier), MeshDescription(MeshDescription), Materials(Materials), StaticMesh(nullptr), CollisionDataProvider(nullptr)
	{
	}

	~FCityEngineMesh();

	FString GetIdentifier() const
	{
		return Identifier;
	}

	const TArray<CityEngine::FMaterialAttributeContainer>& GetMaterials() const
	{
		return Materials;
	}

	UStaticMesh* GetStaticMesh() const
	{
		return StaticMesh;
	}

	void Build(const FString& Name, TMap<CityEngine::FMaterialAttributeContainer, TObjectPtr<UMaterialInstanceDynamic>>& MaterialCache,
			   TMap<FString, CityEngine::FTextureData>& TextureCache, TMap<UMaterialInterface*, FString>& MaterialIdentifiers,
			   TMap<FString, int32>& UniqueMaterialNames, UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent,
			   UWorld* World);
};
