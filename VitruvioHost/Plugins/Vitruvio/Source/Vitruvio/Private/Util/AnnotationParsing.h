// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "PRTTypes.h"
#include "RuleAttributes.h"

namespace Vitruvio
{
void ParseAttributeAnnotations(const prt::RuleFileInfo::Entry* AttributeInfo, URuleAttribute& InAttribute, UObject* const Outer);
}
