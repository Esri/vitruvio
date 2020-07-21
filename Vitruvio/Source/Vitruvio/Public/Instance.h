#pragma once

#include "CoreUObject.h"
#include "Materials/MaterialInstanceDynamic.h"

struct FInstance
{
	FTransform Transform;
	TArray<UMaterialInstanceDynamic*> MaterialOverrides;
};
