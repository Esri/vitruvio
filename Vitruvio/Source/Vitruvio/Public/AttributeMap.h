// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "RuleAttributes.h"

#include "CoreUObject.h"

class VITRUVIO_API FAttributeMap final : public FGCObject
{
public:
	TMap<FString, URuleAttribute*> Attributes;

	void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		TArray<URuleAttribute*> ReferencedObjects;
		Attributes.GenerateValueArray(ReferencedObjects);
		Collector.AddReferencedObjects(ReferencedObjects);
	}

	// This function has to be defined explicitly, otherwise it generates a compiler error because the parent operator= is not accessible.
	FAttributeMap& operator=(const FAttributeMap& Other)
	{
		if (this != &Other)
		{
			this->Attributes = Other.Attributes;
		}
		return *this;
	}
};
