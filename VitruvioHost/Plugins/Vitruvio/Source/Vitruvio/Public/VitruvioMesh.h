#pragma once
#include "VitruvioTypes.h"

struct FCollisionData
{
	TArray<FTriIndices> Indices;
	TArray<FVector> Vertices;

	bool IsValid() const
	{
		return Indices.Num() > 0 && Vertices.Num() > 0;
	}
};

class FVitruvioMesh
{
	FString Name;
	FString Uri;

	FMeshDescription MeshDescription;
	TArray<Vitruvio::FMaterialAttributeContainer> Materials;

	UStaticMesh* StaticMesh;
	FCollisionData CollisionData;

public:
	FVitruvioMesh(const FString& Name, const FString& Uri, const FMeshDescription& MeshDescription,
		const TArray<Vitruvio::FMaterialAttributeContainer>& Materials)
		: Name(Name),
		  Uri(Uri),
		  MeshDescription(MeshDescription),
		  Materials(Materials),
		  StaticMesh(nullptr)
	{
	}

	FString GetName() const
	{
		return Name;
	}

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

	void Build(TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache, TMap<FString, Vitruvio::FTextureData>& TextureCache,
		UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent);
};
