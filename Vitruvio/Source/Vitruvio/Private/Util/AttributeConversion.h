// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "PRTTypes.h"
#include "RuleAttributes.h"

#include "CoreUObject.h"

namespace Vitruvio
{
TMap<FString, URuleAttribute*> ConvertAttributeMap(const AttributeMapUPtr& AttributeMap, const RuleFileInfoUPtr& RuleInfo, UObject* Outer);

AttributeMapUPtr CreateAttributeMap(const TMap<FString, URuleAttribute*>& Attributes);
} // namespace Vitruvio
