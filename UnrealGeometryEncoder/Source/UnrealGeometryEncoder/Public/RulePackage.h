#pragma once

#include "Array.h"
#include "UObject/Object.h"
#include "RulePackage.generated.h"

UCLASS(BlueprintType, hidecategories = (Object))
class UNREALGEOMETRYENCODER_API URulePackage : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<uint8> Data;
};