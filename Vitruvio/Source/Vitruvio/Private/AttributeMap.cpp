// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "AttributeMap.h"

#include "Util/AttributeConversion.h"

TMap<FString, URuleAttribute*> FAttributeMap::ConvertToUnrealAttributeMap(UObject* const Outer)
{
	return Vitruvio::ConvertAttributeMap(AttributeMap, RuleInfo, Outer);
}
