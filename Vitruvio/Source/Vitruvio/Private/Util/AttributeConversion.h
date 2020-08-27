// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "PRTTypes.h"
#include "RuleAttributes.h"

#include "CoreUObject.h"

namespace Vitruvio
{
TMap<FString, URuleAttribute*> ConvertAttributeMap(const AttributeMapUPtr& AttributeMap, const RuleFileInfoUPtr& RuleInfo, UObject* Outer);

AttributeMapUPtr CreateAttributeMap(const TMap<FString, URuleAttribute*>& Attributes);
} // namespace Vitruvio
