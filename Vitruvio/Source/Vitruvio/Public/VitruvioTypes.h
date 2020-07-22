#pragma once

#include "CoreUObject.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "prt/AttributeMap.h"

namespace Vitruvio
{

struct FInstance
{
	FTransform Transform;
	TArray<UMaterialInstanceDynamic*> MaterialOverrides;
};

struct FMaterialContainer
{
	TMap<FString, FString> TextureProperties;
	TMap<FString, FLinearColor> ColorProperties;
	TMap<FString, double> ScalarProperties;

	FString BlendMode;

	explicit FMaterialContainer(const prt::AttributeMap* AttributeMap);

	friend bool operator==(const FMaterialContainer& Lhs, const FMaterialContainer& RHS)
	{
		return Lhs.TextureProperties.OrderIndependentCompareEqual(RHS.TextureProperties) &&
			   Lhs.ColorProperties.OrderIndependentCompareEqual(RHS.ColorProperties) &&
			   Lhs.ScalarProperties.OrderIndependentCompareEqual(RHS.ScalarProperties) && Lhs.BlendMode == RHS.BlendMode;
	}

	friend bool operator!=(const FMaterialContainer& Lhs, const FMaterialContainer& RHS) { return !(Lhs == RHS); }

	friend uint32 GetTypeHash(const FMaterialContainer& Object);
};

} // namespace Vitruvio
