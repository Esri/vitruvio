// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "RulePackage.h"

void URulePackage::PreSave(const ITargetPlatform* TargetPlatform)
{
	Super::PreSave(TargetPlatform);

	// Create a unique ID for this object which can be used by FLazyObjectPtr to reference loaded/unloaded objects
	// The ID would be automatically generated the first time a FLazyObjectPtr of this object is created,
	// but it will mark the object as dirty and require a save.
	FUniqueObjectGuid::GetOrCreateIDForObject(this);
}