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

#include "VitruvioMesh.h"
#include "MaterialConversion.h"
#include "Materials/Material.h"
#include "StaticMeshAttributes.h"
#include "VitruvioModule.h"

namespace
{
FString MakeUniqueMaterialName(FString Name, TMap<FString, int32>& UniqueMaterialNames)
{
	if (UniqueMaterialNames.Contains(Name))
	{
		const int32 Index = UniqueMaterialNames[Name]++;
		Name += TEXT("_") + FString::FromInt(Index);
	}
	else
	{
		UniqueMaterialNames.Add(Name, 1);
	}

	return Name;
}

} // namespace

UMaterialInstanceDynamic* CacheMaterial(UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent,
										TMap<FString, Vitruvio::FTextureData>& TextureCache,
										TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
										const Vitruvio::FMaterialAttributeContainer& MaterialAttributes, TMap<FString, int32>& UniqueMaterialNames,
										TMap<UMaterialInterface*, FString>& MaterialIdentifiers, UObject* Outer)
{
	check(IsInGameThread());

	const FString MaterialIdentifier = MaterialAttributes.GetMaterialName();

	if (UMaterialInstanceDynamic** Result = MaterialCache.Find(MaterialAttributes))
	{
		UMaterialInstanceDynamic* Material = *Result;
		MaterialIdentifiers.Add(Material, MaterialIdentifier);
		return Material;
	}

	const FString UniqueMaterialIdentifier = MakeUniqueMaterialName(MaterialIdentifier, UniqueMaterialNames);
	UMaterialInstanceDynamic* Material = GameThread_CreateMaterialInstance(Outer, UniqueMaterialIdentifier, OpaqueParent, MaskedParent,
																		   TranslucentParent, MaterialAttributes, TextureCache);

	MaterialCache.Add(MaterialAttributes, Material);
	MaterialIdentifiers.Add(Material, MaterialIdentifier);

	return Material;
}

FVitruvioMesh::~FVitruvioMesh()
{
	if (IsEngineExitRequested())
	{
		return;
	}
	VitruvioModule* VitruvioModule = VitruvioModule::GetUnchecked();
	if (StaticMesh && VitruvioModule)
	{
		VitruvioModule->UnregisterMesh(StaticMesh);
	}
}

void FVitruvioMesh::Build(const FString& Name, TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
						  TMap<FString, Vitruvio::FTextureData>& TextureCache, TMap<UMaterialInterface*, FString>& UniqueMaterialIdentifiers,
						  TMap<FString, int32>& UniqueMaterialNames, UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent)
{
	check(IsInGameThread());

	if (StaticMesh)
	{
		return;
	}

	FString MeshName = Name.Replace(TEXT("."), TEXT(""));
	const FName StaticMeshName = MakeUniqueObjectName(nullptr, UStaticMesh::StaticClass(), FName(MeshName));
	StaticMesh = NewObject<UStaticMesh>(GetTransientPackage(), StaticMeshName, RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
	VitruvioModule::Get().RegisterMesh(StaticMesh);

	TMap<UMaterialInstanceDynamic*, FName> MaterialSlots;

	FStaticMeshAttributes MeshAttributes(MeshDescription);

	TArray<FVector3f> Vertices;
	auto VertexPositions = MeshAttributes.GetVertexPositions();
	for (int32 VertexIndex = 0; VertexIndex < VertexPositions.GetNumElements(); ++VertexIndex)
	{
		Vertices.Add(VertexPositions[FVertexID(VertexIndex)]);
	}

	TArray<FTriIndices> Indices;
	const auto PolygonGroups = MeshDescription.PolygonGroups();
	size_t MaterialIndex = 0;

	for (const auto& PolygonGroupId : PolygonGroups.GetElementIDs())
	{
		UMaterialInstanceDynamic* Material = CacheMaterial(OpaqueParent, MaskedParent, TranslucentParent, TextureCache, MaterialCache,
														   Materials[MaterialIndex], UniqueMaterialNames, UniqueMaterialIdentifiers, StaticMesh);

		const FName SlotName = StaticMesh->AddMaterial(Material);
		MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = SlotName;
		MaterialSlots.Add(Material, SlotName);

		++MaterialIndex;

		// cache collision data
		for (FPolygonID PolygonID : MeshDescription.GetPolygonGroupPolygonIDs(PolygonGroupId))
		{
			for (FTriangleID TriangleID : MeshDescription.GetPolygonTriangles(PolygonID))
			{
				auto TriangleVertexInstances = MeshDescription.GetTriangleVertexInstances(TriangleID);

				auto VertexID0 = MeshDescription.GetVertexInstanceVertex(TriangleVertexInstances[0]);
				auto VertexID1 = MeshDescription.GetVertexInstanceVertex(TriangleVertexInstances[1]);
				auto VertexID2 = MeshDescription.GetVertexInstanceVertex(TriangleVertexInstances[2]);

				FTriIndices TriIndex;
				TriIndex.v0 = VertexID0.GetValue();
				TriIndex.v1 = VertexID1.GetValue();
				TriIndex.v2 = VertexID2.GetValue();
				Indices.Add(TriIndex);
			}
		}
	}

	TArray<const FMeshDescription*> MeshDescriptionPtrs;
	MeshDescriptionPtrs.Emplace(&MeshDescription);

	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;
	StaticMesh->BuildFromMeshDescriptions(MeshDescriptionPtrs, Params);
	CollisionData = {Indices, Vertices};
}
