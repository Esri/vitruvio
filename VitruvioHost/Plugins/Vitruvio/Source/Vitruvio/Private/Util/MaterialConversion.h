// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "prt/AttributeMap.h"

#include "CoreUObject.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "VitruvioTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMaterialConversion, Log, All);

namespace Vitruvio
{
UMaterialInstanceDynamic* GameThread_CreateMaterialInstance(UObject* Outer, const FName& Name, UMaterialInterface* OpaqueParent,
															UMaterialInterface* MaskedParent, UMaterialInterface* TranslucentParent,
															const FMaterialAttributeContainer& MaterialAttributes,
															TMap<FTextureCacheKey, FTextureAndChannels>& TextureCache);
}
