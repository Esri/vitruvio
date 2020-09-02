// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "AttributeMap.h"

#include "Util/AttributeConversion.h"

TMap<FString, URuleAttribute*> FAttributeMap::ConvertToUnrealAttributeMap(UObject* const Outer)
{
	return Vitruvio::ConvertAttributeMap(AttributeMap, RuleInfo, Outer);
}
