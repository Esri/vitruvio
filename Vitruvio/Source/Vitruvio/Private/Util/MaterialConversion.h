// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "prt/AttributeMap.h"

#include "CoreUObject.h"
#include "Materials/MaterialInstanceDynamic.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMaterialConversion, Log, All);

UMaterialInstanceDynamic* CreateMaterialInstance(UObject* Outer, UMaterialInterface* OpaqueParent, UMaterialInterface* MaskedParent, UMaterialInterface* TranslucentParent,
												 const prt::AttributeMap* MaterialAttributes);
