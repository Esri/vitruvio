// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "AttributeConversion.h"

#include "AnnotationParsing.h"

#include "PRTTypes.h"
#include "PRTUtils.h"
#include "RuleAttributes.h"

namespace
{
const FString DEFAULT_STYLE = TEXT("Default");

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
	case prt::AAT_UNKNOWN:
	case prt::AAT_VOID:
	case prt::AAT_BOOL_ARRAY:
	case prt::AAT_FLOAT_ARRAY:
	case prt::AAT_STR_ARRAY:
	default: return nullptr;
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

			if (!Attribute->Hidden)
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
	}

	return AttributeMapUPtr(AttributeMapBuilder->createAttributeMap(), PRTDestroyer());
}
} // namespace Vitruvio
