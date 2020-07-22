#include "VitruvioTypes.h"

namespace
{
enum class EMaterialPropertyType
{
	TEXTURE,
	LINEAR_COLOR,
	SCALAR
};

const TMap<FString, EMaterialPropertyType> KeyToTypeMap = {
	{TEXT("diffuseMap"), EMaterialPropertyType::TEXTURE},		 {TEXT("opacityMap"), EMaterialPropertyType::TEXTURE},
	{TEXT("emissiveMap"), EMaterialPropertyType::TEXTURE},		 {TEXT("metallicMap"), EMaterialPropertyType::TEXTURE},
	{TEXT("roughnessMap"), EMaterialPropertyType::TEXTURE},		 {TEXT("normalMap"), EMaterialPropertyType::TEXTURE},

	{TEXT("diffuseColor"), EMaterialPropertyType::LINEAR_COLOR}, {TEXT("emissiveColor"), EMaterialPropertyType::LINEAR_COLOR},

	{TEXT("metallic"), EMaterialPropertyType::SCALAR},			 {TEXT("opacity"), EMaterialPropertyType::SCALAR},
	{TEXT("roughness"), EMaterialPropertyType::SCALAR},
};

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

namespace Vitruvio
{
FMaterialContainer::FMaterialContainer(const prt::AttributeMap* AttributeMap)
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
		case EMaterialPropertyType::TEXTURE: TextureProperties.Add(KeyString, FirstValidTextureUri(AttributeMap, Key)); break;
		case EMaterialPropertyType::LINEAR_COLOR: ColorProperties.Add(KeyString, GetLinearColor(AttributeMap, Key)); break;
		case EMaterialPropertyType::SCALAR: ScalarProperties.Add(KeyString, AttributeMap->getFloat(Key)); break;
		default:;
		}
	}

	if (AttributeMap->hasKey(L"opacityMap.mode"))
	{
		BlendMode = AttributeMap->getString(L"opacityMap.mode");
	}
}
} // namespace Vitruvio
