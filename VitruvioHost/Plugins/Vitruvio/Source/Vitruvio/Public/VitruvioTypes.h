/* Copyright 2021 Esri
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

#include "CoreUObject.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "PhysicsCore/Public/Interface_CollisionDataProviderCore.h"

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

struct FTextureData
{
	FTextureData() = default;

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

} // namespace Vitruvio
