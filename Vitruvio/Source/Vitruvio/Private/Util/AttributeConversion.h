// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "PRTTypes.h"
#include "RuleAttributes.h"
#include "Vitruvio/Public/AttributeMap.h"

#include "CoreUObject.h"

namespace Vitruvio
{
namespace AttributeConversion
{
	FAttributeMap ConvertAttributeMap(const AttributeMapUPtr& AttributeMap, const RuleFileInfoUPtr& RuleInfo);

	AttributeMapUPtr CreateAttributeMap(const TMap<FString, URuleAttribute*>& Attributes);
} // namespace AttributeConversion
} // namespace Vitruvio
