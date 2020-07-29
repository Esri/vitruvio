#pragma once

#include "CoreUObject.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "prt/AttributeMap.h"

namespace Vitruvio
{

struct FMaterialAttributeContainer
{
	TMap<FString, FString> TextureProperties;
	TMap<FString, FLinearColor> ColorProperties;
	TMap<FString, double> ScalarProperties;

	FString BlendMode;

	explicit FMaterialAttributeContainer(const prt::AttributeMap* AttributeMap);

	friend bool operator==(const FMaterialAttributeContainer& Lhs, const FMaterialAttributeContainer& RHS)
	{
		return Lhs.TextureProperties.OrderIndependentCompareEqual(RHS.TextureProperties) &&
			   Lhs.ColorProperties.OrderIndependentCompareEqual(RHS.ColorProperties) &&
			   Lhs.ScalarProperties.OrderIndependentCompareEqual(RHS.ScalarProperties) && Lhs.BlendMode == RHS.BlendMode;
	}

	friend bool operator!=(const FMaterialAttributeContainer& Lhs, const FMaterialAttributeContainer& RHS) { return !(Lhs == RHS); }

	friend uint32 GetTypeHash(const FMaterialAttributeContainer& Object);
};

struct FInstanceCacheKey
{
	UStaticMesh* Mesh;
	TArray<UMaterialInstanceDynamic*> MaterialOverrides;

	friend uint32 GetTypeHash(const FInstanceCacheKey& Object);

	friend bool operator==(const FInstanceCacheKey& Lhs, const FInstanceCacheKey& RHS)
	{
		return Lhs.Mesh == RHS.Mesh && Lhs.MaterialOverrides == RHS.MaterialOverrides;
	}

	friend bool operator!=(const FInstanceCacheKey& Lhs, const FInstanceCacheKey& RHS) { return !(Lhs == RHS); }
};
using FInstanceMap = TMap<FInstanceCacheKey, TArray<FTransform>>;

} // namespace Vitruvio
