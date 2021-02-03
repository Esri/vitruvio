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

#include "VitruvioTypes.h"

#include "Core/Public/Containers/UnrealString.h"
#include "Core/Public/Templates/TypeHash.h"

namespace
{
enum class EMaterialPropertyType
{
	TEXTURE,
	LINEAR_COLOR,
	SCALAR,
	STRING
};

// clang-format off
const TMap<FString, EMaterialPropertyType> KeyToTypeMap = {
	{TEXT("diffuseMap"), EMaterialPropertyType::TEXTURE},
	{TEXT("opacityMap"), EMaterialPropertyType::TEXTURE},
	{TEXT("emissiveMap"), EMaterialPropertyType::TEXTURE},
	{TEXT("metallicMap"), EMaterialPropertyType::TEXTURE},
	{TEXT("roughnessMap"), EMaterialPropertyType::TEXTURE},
	{TEXT("normalMap"), EMaterialPropertyType::TEXTURE},
	
	{TEXT("diffuseColor"), EMaterialPropertyType::LINEAR_COLOR},
	{TEXT("emissiveColor"), EMaterialPropertyType::LINEAR_COLOR},

	{TEXT("metallic"), EMaterialPropertyType::SCALAR},
	{TEXT("opacity"), EMaterialPropertyType::SCALAR},
	{TEXT("roughness"), EMaterialPropertyType::SCALAR},

	{TEXT("shader"), EMaterialPropertyType::STRING},
};
// clang-format on

FString FirstValidTextureUri(const prt::AttributeMap* MaterialAttributes, wchar_t const* Key)
{
	size_t ValuesCount = 0;
	wchar_t const* const* Values = MaterialAttributes->getStringArray(Key, &ValuesCount);
	for (int ValueIndex = 0; ValueIndex < ValuesCount; ++ValueIndex)
	{
		FString TextureUri(Values[ValueIndex]);
		if (TextureUri.Len() > 0)
		{
			return TextureUri;
		}
	}
	return TEXT("");
}

FLinearColor GetLinearColor(const prt::AttributeMap* MaterialAttributes, wchar_t const* Key)
{
	size_t count;
	const double* values = MaterialAttributes->getFloatArray(Key, &count);
	if (count < 3)
	{
		return FLinearColor();
	}
	const FColor Color(values[0] * 255.0, values[1] * 255.0, values[2] * 255.0);
	return FLinearColor(Color);
}

} // namespace

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
FMaterialAttributeContainer::FMaterialAttributeContainer(const prt::AttributeMap* AttributeMap)
{
	size_t KeyCount = 0;
	wchar_t const* const* Keys = AttributeMap->getKeys(&KeyCount);
	for (size_t KeyIndex = 0; KeyIndex < KeyCount; KeyIndex++)
	{
		const wchar_t* Key = Keys[KeyIndex];
		const FString KeyString(Key);

		if (!KeyToTypeMap.Contains(KeyString))
		{
			continue;
		}
		const EMaterialPropertyType Type = KeyToTypeMap[KeyString];
		switch (Type)
		{
		case EMaterialPropertyType::TEXTURE:
			TextureProperties.Add(KeyString, FirstValidTextureUri(AttributeMap, Key));
			break;
		case EMaterialPropertyType::LINEAR_COLOR:
			ColorProperties.Add(KeyString, GetLinearColor(AttributeMap, Key));
			break;
		case EMaterialPropertyType::SCALAR:
			ScalarProperties.Add(KeyString, AttributeMap->getFloat(Key));
			break;
		case EMaterialPropertyType::STRING:
			StringProperties.Add(KeyString, AttributeMap->getString(Key));
			break;
		default:;
		}
	}

	if (AttributeMap->hasKey(L"opacityMap.mode"))
	{
		BlendMode = AttributeMap->getString(L"opacityMap.mode");
	}

	if (AttributeMap->hasKey(L"name"))
	{
		Name = AttributeMap->getString(L"name");
	}
}

uint32 GetTypeHash(const FMaterialAttributeContainer& Object)
{
	uint32 Hash = 0x274110C5;
	Hash = HashCombine(Hash, GetMapHash<FString, FString>(Object.TextureProperties));
	Hash = HashCombine(Hash, GetMapHash<FString, FLinearColor>(Object.ColorProperties));
	Hash = HashCombine(Hash, GetMapHash<FString, double>(Object.ScalarProperties));
	Hash = HashCombine(Hash, GetMapHash<FString, FString>(Object.StringProperties));
	Hash = HashCombine(Hash, GetTypeHash(Object.BlendMode));
	return Hash;
}

uint32 GetTypeHash(const FInstanceCacheKey& Object)
{
	return HashCombine(Object.PrototypeId, GetArrayHash(Object.MaterialOverrides));
}

} // namespace Vitruvio
