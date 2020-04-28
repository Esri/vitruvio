#pragma once

#include "Containers/Array.h"
#include "Misc/TVariant.h"
#include "UObject/Object.h"
#include "RuleAttributes.generated.h"

using FAttributeGroups = TArray<FString>;

UCLASS()
class VITRUVIO_API UAttributeAnnotation : public UObject
{
	GENERATED_BODY()
	
};

UCLASS()
class VITRUVIO_API URangeAnnotation : public UAttributeAnnotation
{
	GENERATED_BODY()
	
public:
	float Min;
	float Max; 
};

UCLASS()
class UAnnotationArgument : public UObject
{
	GENERATED_BODY()

	TVariant<FString, bool, double> Value;
public:
	
	float GetFloat() const
	{
		return Value.Get<double>();
	}
	
	FString GetString() const
	{
		return Value.Get<FString>();
	}

	bool GetBool() const
	{
		return Value.Get<bool>();
	}
};

UCLASS()
class VITRUVIO_API UEnumAnnotation : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	TArray<UAnnotationArgument> Arguments;
};

UCLASS()
class VITRUVIO_API URuleAttribute : public UObject
{
	GENERATED_BODY()

public:
	FString Name;

	UPROPERTY()
	UAttributeAnnotation* Annotation;
	FAttributeGroups Groups;
};

UCLASS()
class VITRUVIO_API UStringAttribute : public URuleAttribute
{
	GENERATED_BODY()

public:
	FString Value;
};

UCLASS()
class VITRUVIO_API UFloatAttribute : public URuleAttribute
{
	GENERATED_BODY()

public:
	float Value;

};

UCLASS()
class VITRUVIO_API UBoolAttribute : public URuleAttribute
{
	GENERATED_BODY()

public:
	bool Value;
};

UCLASS()
class VITRUVIO_API UColorAttribute : public URuleAttribute
{
	GENERATED_BODY()

public:
    FColor Color;
};