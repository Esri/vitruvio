#pragma once

#include "CoreUObject.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "prt/AttributeMap.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMaterialConversion, Log, All);

UMaterialInstanceDynamic* CreateMaterialInstance(UObject* Outer, UMaterialInterface* OpaqueParent, UMaterialInterface* MaskedParent, UMaterialInterface* TranslucentParent, 
	const prt::AttributeMap* MaterialAttributes);
