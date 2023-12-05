#include "VitruvioComponentTypes.h"

FConvertedGenerateResult BuildResult(const FGenerateResultDescription& GenerateResult,
									 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
									 TMap<FString, Vitruvio::FTextureData>& TextureCache,
									 TMap<UMaterialInterface*, FString>& MaterialIdentifiers,
									 TMap<FString, int32>& UniqueMaterialIdentifiers,
									 UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent)
{
	MaterialIdentifiers.Empty();
	UniqueMaterialIdentifiers.Empty();

	// Build all meshes
	if (GenerateResult.GeneratedModel)
	{
		GenerateResult.GeneratedModel->Build(TEXT("GeneratedModel"), MaterialCache, TextureCache, MaterialIdentifiers, UniqueMaterialIdentifiers, OpaqueParent, MaskedParent, TranslucentParent);
	}

	for (const auto& IdAndMesh : GenerateResult.InstanceMeshes)
	{
		FString Name = GenerateResult.InstanceNames[IdAndMesh.Key];
		IdAndMesh.Value->Build(Name, MaterialCache, TextureCache, MaterialIdentifiers, UniqueMaterialIdentifiers, OpaqueParent, MaskedParent,
							   TranslucentParent);
	}

	// Convert instances
	TArray<FInstance> Instances;
	for (const auto& Instance : GenerateResult.Instances)
	{
		auto VitruvioMesh = GenerateResult.InstanceMeshes[Instance.Key.PrototypeId];
		FString MeshName = GenerateResult.InstanceNames[Instance.Key.PrototypeId];
		TArray<UMaterialInstanceDynamic*> OverrideMaterials;

		for (size_t MaterialIndex = 0; MaterialIndex < Instance.Key.MaterialOverrides.Num(); ++MaterialIndex)
		{
			const Vitruvio::FMaterialAttributeContainer& MaterialContainer = Instance.Key.MaterialOverrides[MaterialIndex];
			OverrideMaterials.Add(CacheMaterial(OpaqueParent, MaskedParent, TranslucentParent, TextureCache, MaterialCache, MaterialContainer,
												UniqueMaterialIdentifiers, MaterialIdentifiers, VitruvioMesh->GetStaticMesh()));
		}

		Instances.Add({MeshName, VitruvioMesh, OverrideMaterials, Instance.Value});
	}

	return {GenerateResult.GeneratedModel, Instances, GenerateResult.Reports};
}

FString UniqueComponentName(const FString& Name, TMap<FString, int32>& UsedNames)
{
	FString CurrentName = Name;
	while (UsedNames.Contains(CurrentName))
	{
		const int32 Count = UsedNames[CurrentName]++;
		CurrentName = Name + FString::FromInt(Count);
	}
	UsedNames.Add(CurrentName, 0);
	return CurrentName;
}
