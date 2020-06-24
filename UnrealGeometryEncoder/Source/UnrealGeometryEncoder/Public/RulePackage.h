// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"

#include "RulePackage.generated.h"

UCLASS(BlueprintType, HideCategories = (Object))
class UNREALGEOMETRYENCODER_API URulePackage final : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<uint8> Data;

	void PreSave(const class ITargetPlatform* TargetPlatform) override;
};
