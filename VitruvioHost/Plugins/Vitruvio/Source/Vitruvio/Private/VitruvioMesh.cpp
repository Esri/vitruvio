#include "VitruvioMesh.h"
#include "VitruvioModule.h"
#include "MaterialConversion.h"
#include "StaticMeshAttributes.h"

UMaterialInstanceDynamic* CacheMaterial(UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent,
                                        TMap<FString, Vitruvio::FTextureData>& TextureCache,
                                        TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
                                        const Vitruvio::FMaterialAttributeContainer& MaterialAttributes, const FName& Name, UObject* Outer)
{
	check(IsInGameThread());

	const auto Result = MaterialCache.Find(MaterialAttributes);
	if (Result)
	{
		return *Result;
	}
	
	const FName UniqueMaterialName(Name);
	UMaterialInstanceDynamic* Material = Vitruvio::GameThread_CreateMaterialInstance(Outer, UniqueMaterialName, OpaqueParent, MaskedParent,
																					 TranslucentParent, MaterialAttributes, TextureCache);
	MaterialCache.Add(MaterialAttributes, Material);
	return Material;
}

FVitruvioMesh::~FVitruvioMesh()
{
	VitruvioModule* VitruvioModule = VitruvioModule::GetUnchecked();
	if (StaticMesh && VitruvioModule)
	{
		VitruvioModule->UnregisterMesh(StaticMesh);
	}
}

void FVitruvioMesh::Build(const FString& Name, TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
						  TMap<FString, Vitruvio::FTextureData>& TextureCache, UMaterial* OpaqueParent, UMaterial* MaskedParent,
						  UMaterial* TranslucentParent)
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

	TArray<FVector> Vertices;
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
		const FName MaterialName = MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId];
		UMaterialInstanceDynamic* Material = CacheMaterial(OpaqueParent, MaskedParent, TranslucentParent, TextureCache, MaterialCache,
														   Materials[MaterialIndex], MaterialName, StaticMesh);

		if (MaterialSlots.Contains(Material))
		{
			MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = MaterialSlots[Material];
		}
		else
		{
			const FName SlotName = StaticMesh->AddMaterial(Material);
			MeshAttributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = SlotName;
			MaterialSlots.Add(Material, SlotName);
		}

		++MaterialIndex;

		// cache collision data
		for (FPolygonID PolygonID : MeshDescription.GetPolygonGroupPolygons(PolygonGroupId))
		{
			for (FTriangleID TriangleID : MeshDescription.GetPolygonTriangleIDs(PolygonID))
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

	StaticMesh->BuildFromMeshDescriptions(MeshDescriptionPtrs);
	CollisionData = {Indices, Vertices};
}
