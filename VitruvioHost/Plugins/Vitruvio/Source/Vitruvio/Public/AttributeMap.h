// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "RuleAttributes.h"

#include "CoreUObject.h"
#include "PRTTypes.h"

class VITRUVIO_API FAttributeMap
{
public:
	FAttributeMap() {}

	FAttributeMap(AttributeMapUPtr AttributeMap, RuleFileInfoUPtr RuleInfo) : AttributeMap(std::move(AttributeMap)), RuleInfo(std::move(RuleInfo)) {}

	TMap<FString, URuleAttribute*> ConvertToUnrealAttributeMap(UObject* const Outer);

private:
	const AttributeMapUPtr AttributeMap;
	const RuleFileInfoUPtr RuleInfo;
};

using FAttributeMapPtr = TSharedPtr<FAttributeMap>;
