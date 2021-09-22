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

#include "AttributeConversion.h"

#include "AnnotationParsing.h"

#include "PRTTypes.h"
#include "PRTUtils.h"
#include "RuleAttributes.h"

namespace
{
const FString DEFAULT_STYLE = TEXT("Default");

std::vector<const wchar_t*> ToPtrVector(const TArray<FString>& Input)
{
	std::vector<const wchar_t*> PtrVec(Input.Num());
	for (size_t i = 0; i < Input.Num(); i++)
		PtrVec[i] = *Input[i];
	return PtrVec;
}

URuleAttribute* CreateAttribute(const AttributeMapUPtr& AttributeMap, const prt::RuleFileInfo::Entry* AttrInfo, UObject* const Outer)
{
	const std::wstring Name(AttrInfo->getName());
	switch (AttrInfo->getReturnType())
	{
	case prt::AAT_BOOL:
	{
		UBoolAttribute* BoolAttribute = NewObject<UBoolAttribute>(Outer);
		BoolAttribute->Value = AttributeMap->getBool(Name.c_str());
		return BoolAttribute;
	}
	case prt::AAT_INT:
	case prt::AAT_FLOAT:
	{
		UFloatAttribute* FloatAttribute = NewObject<UFloatAttribute>(Outer);
		FloatAttribute->Value = AttributeMap->getFloat(Name.c_str());
		return FloatAttribute;
	}
	case prt::AAT_STR:
	{
		UStringAttribute* StringAttribute = NewObject<UStringAttribute>(Outer);
		StringAttribute->Value = WCHAR_TO_TCHAR(AttributeMap->getString(Name.c_str()));
		return StringAttribute;
	}
	case prt::AAT_STR_ARRAY:
	{
		UStringArrayAttribute* StringArrayAttribute = NewObject<UStringArrayAttribute>(Outer);
		size_t Count = 0;
		const wchar_t* const* Arr = AttributeMap->getStringArray(Name.c_str(), &Count);
		for (size_t Index = 0; Index < Count; Index++)
		{
			StringArrayAttribute->Values.Add(Arr[Index]);
		}
		return StringArrayAttribute;
	}
	case prt::AAT_BOOL_ARRAY:
	{
		UBoolArrayAttribute* BoolArrayAttribute = NewObject<UBoolArrayAttribute>(Outer);
		size_t Count = 0;
		const bool* Arr = AttributeMap->getBoolArray(Name.c_str(), &Count);
		for (size_t Index = 0; Index < Count; Index++)
		{
			BoolArrayAttribute->Values.Add(Arr[Index]);
		}
		return BoolArrayAttribute;
	}
	case prt::AAT_FLOAT_ARRAY:
	{
		UFloatArrayAttribute* FloatArrayAttribute = NewObject<UFloatArrayAttribute>(Outer);
		size_t Count = 0;
		const double* Arr = AttributeMap->getFloatArray(Name.c_str(), &Count);
		for (size_t Index = 0; Index < Count; Index++)
		{
			FloatArrayAttribute->Values.Add(Arr[Index]);
		}
		return FloatArrayAttribute;
	}
	case prt::AAT_UNKNOWN:
	case prt::AAT_VOID:

	default:
		return nullptr;
	}
}
} // namespace

namespace Vitruvio
{
TMap<FString, URuleAttribute*> ConvertAttributeMap(const AttributeMapUPtr& AttributeMap, const RuleFileInfoUPtr& RuleInfo, UObject* const Outer)
{
	TMap<FString, URuleAttribute*> UnrealAttributeMap;
	for (size_t AttributeIndex = 0; AttributeIndex < RuleInfo->getNumAttributes(); AttributeIndex++)
	{
		const prt::RuleFileInfo::Entry* AttrInfo = RuleInfo->getAttribute(AttributeIndex);
		if (AttrInfo->getNumParameters() != 0)
		{
			continue;
		}

		// We only support the default style for the moment
		FString Style(WCHAR_TO_TCHAR(prtu::getStyle(AttrInfo->getName()).c_str()));
		if (Style != DEFAULT_STYLE)
		{
			continue;
		}

		const std::wstring Name(AttrInfo->getName());
		if (UnrealAttributeMap.Contains(WCHAR_TO_TCHAR(Name.c_str())))
		{
			continue;
		}

		URuleAttribute* Attribute = CreateAttribute(AttributeMap, AttrInfo, Outer);

		if (Attribute)
		{
			const FString AttributeName = WCHAR_TO_TCHAR(Name.c_str());
			const FString DisplayName = WCHAR_TO_TCHAR(prtu::removeImport(prtu::removeStyle(Name.c_str())).c_str());
			Attribute->Name = AttributeName;
			Attribute->DisplayName = DisplayName;

			ParseAttributeAnnotations(AttrInfo, *Attribute, Outer);

			if (!Attribute->bHidden)
			{
				UnrealAttributeMap.Add(AttributeName, Attribute);
			}
		}
	}
	return UnrealAttributeMap;
}

AttributeMapUPtr CreateAttributeMap(const TMap<FString, URuleAttribute*>& Attributes)
{
	AttributeMapBuilderUPtr AttributeMapBuilder(prt::AttributeMapBuilder::create());

	for (const TPair<FString, URuleAttribute*>& AttributeEntry : Attributes)
	{
		const URuleAttribute* Attribute = AttributeEntry.Value;

		if (!Attribute->bUserSet) continue;

		if (const UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
		{
			AttributeMapBuilder->setFloat(TCHAR_TO_WCHAR(*Attribute->Name), FloatAttribute->Value);
		}
		else if (const UStringAttribute* StringAttribute = Cast<UStringAttribute>(Attribute))
		{
			AttributeMapBuilder->setString(TCHAR_TO_WCHAR(*Attribute->Name), TCHAR_TO_WCHAR(*StringAttribute->Value));
		}
		else if (const UBoolAttribute* BoolAttribute = Cast<UBoolAttribute>(Attribute))
		{
			AttributeMapBuilder->setBool(TCHAR_TO_WCHAR(*Attribute->Name), BoolAttribute->Value);
		}
		else if (const UStringArrayAttribute* StringArrayAttribute = Cast<UStringArrayAttribute>(Attribute))
		{
			std::vector<const wchar_t*> PtrVec = ToPtrVector(StringArrayAttribute->Values);
			AttributeMapBuilder->setStringArray(TCHAR_TO_WCHAR(*Attribute->Name), PtrVec.data(), StringArrayAttribute->Values.Num());
		}
		else if (const UBoolArrayAttribute* BoolArrayAttribute = Cast<UBoolArrayAttribute>(Attribute))
		{
			AttributeMapBuilder->setBoolArray(TCHAR_TO_WCHAR(*Attribute->Name), BoolArrayAttribute->Values.GetData(), BoolArrayAttribute->Values.Num());
		}
		else if (const UFloatArrayAttribute* FloatArrayAttribute = Cast<UFloatArrayAttribute>(Attribute))
		{
			AttributeMapBuilder->setFloatArray(TCHAR_TO_WCHAR(*Attribute->Name), FloatArrayAttribute->Values.GetData(), FloatArrayAttribute->Values.Num());
		}
	}

	return AttributeMapUPtr(AttributeMapBuilder->createAttributeMap(), PRTDestroyer());
}
} // namespace Vitruvio
