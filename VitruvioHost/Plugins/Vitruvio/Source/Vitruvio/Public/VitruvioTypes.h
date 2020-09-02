// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

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
	TMap<FString, FString> StringProperties;

	FString BlendMode;

	explicit FMaterialAttributeContainer(const prt::AttributeMap* AttributeMap);

	friend bool operator==(const FMaterialAttributeContainer& Lhs, const FMaterialAttributeContainer& RHS)
	{
		// clang-format off
		return Lhs.TextureProperties.OrderIndependentCompareEqual(RHS.TextureProperties) &&
			   Lhs.ColorProperties.OrderIndependentCompareEqual(RHS.ColorProperties) &&
			   Lhs.ScalarProperties.OrderIndependentCompareEqual(RHS.ScalarProperties) &&
			   Lhs.StringProperties.OrderIndependentCompareEqual(RHS.StringProperties) && 
			   Lhs.BlendMode == RHS.BlendMode;
		// clang-format on
	}

	friend bool operator!=(const FMaterialAttributeContainer& Lhs, const FMaterialAttributeContainer& RHS)
	{
		return !(Lhs == RHS);
	}

	friend uint32 GetTypeHash(const FMaterialAttributeContainer& Object);
};

struct FInstanceCacheKey
{
	int32 PrototypeId;
	TArray<Vitruvio::FMaterialAttributeContainer> MaterialOverrides;

	friend uint32 GetTypeHash(const FInstanceCacheKey& Object);

	friend bool operator==(const FInstanceCacheKey& Lhs, const FInstanceCacheKey& RHS)
	{
		return Lhs.PrototypeId == RHS.PrototypeId && Lhs.MaterialOverrides == RHS.MaterialOverrides;
	}

	friend bool operator!=(const FInstanceCacheKey& Lhs, const FInstanceCacheKey& RHS)
	{
		return !(Lhs == RHS);
	}
};
using FInstanceMap = TMap<FInstanceCacheKey, TArray<FTransform>>;

struct FTextureAndChannels
{
	FTextureAndChannels() = default;

	UTexture2D* Texture = nullptr;
	uint32 NumChannels = 0;

	friend bool operator==(const FTextureAndChannels& Lhs, const FTextureAndChannels& Rhs)
	{
		return Lhs.Texture == Rhs.Texture && Lhs.NumChannels == Rhs.NumChannels;
	}

	friend bool operator!=(const FTextureAndChannels& Lhs, const FTextureAndChannels& RHS)
	{
		return !(Lhs == RHS);
	}
};

struct FTextureCacheKey
{
	FString Path;
	FDateTime TimeStamp;

	friend uint32 GetTypeHash(const FTextureCacheKey& Object);

	friend bool operator==(const FTextureCacheKey& Lhs, const FTextureCacheKey& Rhs)
	{
		return Lhs.Path == Rhs.Path;
	}
};

} // namespace Vitruvio
