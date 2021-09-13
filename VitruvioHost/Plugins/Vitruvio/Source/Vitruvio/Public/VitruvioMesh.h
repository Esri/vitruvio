#pragma once

#include "VitruvioTypes.h"

UMaterialInstanceDynamic* CacheMaterial(UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent,
										TMap<FString, Vitruvio::FTextureData>& TextureCache,
										TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
										const Vitruvio::FMaterialAttributeContainer& MaterialAttributes, const FName& Name, UObject* Outer);

struct FCollisionData
{
	TArray<FTriIndices> Indices;
	TArray<FVector> Vertices;

	bool IsValid() const
	{
		return Indices.Num() > 0 && Vertices.Num() > 0;
	}
};

class FVitruvioMesh : public FGCObject
{
	FString Uri;

	FMeshDescription MeshDescription;
	TArray<Vitruvio::FMaterialAttributeContainer> Materials;

	UStaticMesh* StaticMesh;
	FCollisionData CollisionData;

public:
	FVitruvioMesh(const FString& Uri, const FMeshDescription& MeshDescription,
				  const TArray<Vitruvio::FMaterialAttributeContainer>& Materials)
		: Uri(Uri), MeshDescription(MeshDescription), Materials(Materials), StaticMesh(nullptr)
	{
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	FString GetUri() const
	{
		return Uri;
	}

	const TArray<Vitruvio::FMaterialAttributeContainer>& GetMaterials() const
	{
		return Materials;
	}

	UStaticMesh* GetStaticMesh() const
	{
		return StaticMesh;
	}

	const FCollisionData& GetCollisionData() const
	{
		return CollisionData;
	}

	void Invalidate();

	void Build(const FString& Name, TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
			   TMap<FString, Vitruvio::FTextureData>& TextureCache, UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent);
};
