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

#include "Interface_CollisionDataProviderCore.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"

#include "prt/AttributeMap.h"

/**
 * Hash function for TMap. Requires that the Key K and Value V support GetTypeHash.
 */
template <typename K, typename V>
uint32 GetMapHash(const TMap<K, V>& In)
{
	uint32 CombinedHash = 0;
	for (const auto& Entry : In)
	{
		const uint32 EntryHash = HashCombine(GetTypeHash(Entry.Key), GetTypeHash(Entry.Value));
		CombinedHash += EntryHash;
	}
	return CombinedHash;
}

/**
 * Hash function for TArray. Requires that the Value V supports GetTypeHash.
 */
template <typename V>
uint32 GetArrayHash(const TArray<V>& In)
{
	uint32 CombinedHash = 0;
	for (const auto& Entry : In)
	{
		CombinedHash += GetTypeHash(Entry);
	}
	return CombinedHash;
}

namespace Vitruvio
{

enum class EUnrealUvSetType : int32
{
	None = -1,
	ColorMap = 0, // skip 1 because unreal engine saves lightmaps per default in uv set 1
	DirtMap = 2,
	OpacityMap = 3,
	NormalMap = 4,
	EmissiveMap = 5,
	RoughnessMap = 6,
	MetallicMap = 7
};

enum class EPrtUvSetType : int32
{
	None = -1,
	ColorMap = 0,
	BumpMap = 1,
	DirtMap = 2,
	SpecularMap = 3,
	OpacityMap = 4,
	NormalMap = 5,
	EmissiveMap = 6,
	OcclusionMap = 7,
	RoughnessMap = 8,
	MetallicMap = 9
};

const FString CityEngineDefaultMaterialName("CityEngineMaterial");

struct FMaterialAttributeContainer
{
	TMap<FString, FString> TextureProperties;
	TMap<FString, FLinearColor> ColorProperties;
	TMap<FString, double> ScalarProperties;
	TMap<FString, FString> StringProperties;

	FString BlendMode;
	FString Name; // ignored on purpose for hash and equality

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

	FString GetMaterialName() const
	{
		if (Name.StartsWith(CityEngineDefaultMaterialName))
		{
			if (const FString* ColorMapKey = TextureProperties.Find("colorMap"))
			{
				return FPaths::GetBaseFilename(*ColorMapKey);
			}
		}

		return Name;
	}
};

struct FInstanceCacheKey
{
	FString MeshId;
	TArray<Vitruvio::FMaterialAttributeContainer> MaterialOverrides;

	friend uint32 GetTypeHash(const FInstanceCacheKey& Object);

	friend bool operator==(const FInstanceCacheKey& Lhs, const FInstanceCacheKey& RHS)
	{
		return Lhs.MeshId == RHS.MeshId && Lhs.MaterialOverrides == RHS.MaterialOverrides;
	}

	friend bool operator!=(const FInstanceCacheKey& Lhs, const FInstanceCacheKey& RHS)
	{
		return !(Lhs == RHS);
	}
};
using FInstanceMap = TMap<FInstanceCacheKey, TArray<FTransform>>;

struct FTextureData
{
	UTexture2D* Texture = nullptr;
	uint32 NumChannels = 0;
	FDateTime LoadTime;

	friend bool operator==(const FTextureData& Lhs, const FTextureData& Rhs)
	{
		return Lhs.Texture == Rhs.Texture && Lhs.NumChannels == Rhs.NumChannels;
	}

	friend bool operator!=(const FTextureData& Lhs, const FTextureData& RHS)
	{
		return !(Lhs == RHS);
	}
};

struct FCollisionData
{
	TArray<FTriIndices> Indices;
	TArray<FVector3f> Vertices;

	bool IsValid() const
	{
		return Indices.Num() > 0 && Vertices.Num() > 0;
	}
};

} // namespace Vitruvio
