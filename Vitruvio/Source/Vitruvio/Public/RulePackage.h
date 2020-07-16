// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"

#include "RulePackage.generated.h"

UCLASS(BlueprintType, HideCategories = (Object))
class VITRUVIO_API URulePackage final : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<uint8> Data;

	void PreSave(const ITargetPlatform* TargetPlatform) override
	{
		Super::PreSave(TargetPlatform);

		// Create a unique ID for this object which can be used by FLazyObjectPtr to reference loaded/unloaded objects
		// The ID would be automatically generated the first time a FLazyObjectPtr of this object is created,
		// but it will mark the object as dirty and require a save.
		FUniqueObjectGuid::GetOrCreateIDForObject(this);
	}
};
