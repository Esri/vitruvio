/* Copyright 2022 Esri
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
	Texture,
	LinearColor,
	Scalar,
	String
};

// clang-format off
const TMap<FString, EMaterialPropertyType> KeyToTypeMap = {
	{TEXT("diffuseMap"), EMaterialPropertyType::Texture},
	{TEXT("opacityMap"), EMaterialPropertyType::Texture},
	{TEXT("emissiveMap"), EMaterialPropertyType::Texture},
	{TEXT("metallicMap"), EMaterialPropertyType::Texture},
	{TEXT("roughnessMap"), EMaterialPropertyType::Texture},
	{TEXT("normalMap"), EMaterialPropertyType::Texture},
	
	{TEXT("diffuseColor"), EMaterialPropertyType::LinearColor},
	{TEXT("emissiveColor"), EMaterialPropertyType::LinearColor},

	{TEXT("metallic"), EMaterialPropertyType::Scalar},
	{TEXT("opacity"), EMaterialPropertyType::Scalar},
	{TEXT("roughness"), EMaterialPropertyType::Scalar},

	{TEXT("shader"), EMaterialPropertyType::String},
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

FString GetTextureUriFromIdx(const prt::AttributeMap* MaterialAttributes, wchar_t const* Key, size_t Index)
{
	size_t ValuesCount = 0;
	wchar_t const* const* Values = MaterialAttributes->getStringArray(Key, &ValuesCount);
	if (Index < ValuesCount)
	{
		FString TextureUri(Values[Index]);

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
		case EMaterialPropertyType::Texture:
			if (KeyString.Equals(TEXT("diffuseMap")))
			{
				FString ColorMapUri = GetTextureUriFromIdx(AttributeMap, Key, 0);
				FString DirtMapUri = GetTextureUriFromIdx(AttributeMap, Key, 1);

				if (ColorMapUri.Len() > 0)
				{
					TextureProperties.Add(TEXT("colorMap"), ColorMapUri);
				}
				if (DirtMapUri.Len() > 0)
				{
					TextureProperties.Add(TEXT("dirtMap"), DirtMapUri);
				}
			}
			else
			{
				FString MapUri = FirstValidTextureUri(AttributeMap, Key);

				if (MapUri.Len() > 0)
				{
					TextureProperties.Add(KeyString, MapUri);
				}
			}
			break;
		case EMaterialPropertyType::LinearColor:
			ColorProperties.Add(KeyString, GetLinearColor(AttributeMap, Key));
			break;
		case EMaterialPropertyType::Scalar:
			ScalarProperties.Add(KeyString, AttributeMap->getFloat(Key));
			break;
		case EMaterialPropertyType::String:
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
