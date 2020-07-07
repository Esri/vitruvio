// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "PRTTypes.h"
#include "RuleAttributes.h"

namespace Vitruvio
{
void ParseAttributeAnnotations(const prt::RuleFileInfo::Entry* AttributeInfo, URuleAttribute& InAttribute);
}
