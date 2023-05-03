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

#include "DynamicMesh/DynamicMesh3.h"
#include "VitruvioTypes.h"

UMaterialInstanceDynamic* CacheMaterial(UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent,
										TMap<FString, Vitruvio::FTextureData>& TextureCache,
										TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
										const Vitruvio::FMaterialAttributeContainer& MaterialAttributes, UObject* Outer);

class FVitruvioMesh
{
	FString Uri;

	UE::Geometry::FDynamicMesh3* DynamicMesh;
	TArray<Vitruvio::FMaterialAttributeContainer> Materials;

public:
	FVitruvioMesh(const FString& Uri, UE::Geometry::FDynamicMesh3* DynamicMesh, const TArray<Vitruvio::FMaterialAttributeContainer>& Materials)
		: Uri(Uri), DynamicMesh(DynamicMesh), Materials(Materials)
	{
	}

	FString GetUri() const
	{
		return Uri;
	}

	const TArray<Vitruvio::FMaterialAttributeContainer>& GetMaterials() const
	{
		return Materials;
	}

	UE::Geometry::FDynamicMesh3* GetDynamicMesh() const
	{
		return DynamicMesh;
	}

	void Build(const FString& Name, TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
			   TMap<FString, Vitruvio::FTextureData>& TextureCache, UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent);
};
