#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"
#include "RuleAttributes.generated.h"

using FAttributeGroups = TArray<FString>;

UENUM()
enum EFilesystemMode { File, Directory };

UCLASS()
class VITRUVIO_API UAttributeAnnotation : public UObject
{
	GENERATED_BODY()
};

UCLASS()
class VITRUVIO_API UColorAnnotation : public UAttributeAnnotation
{
	GENERATED_BODY()
};

UCLASS()
class VITRUVIO_API UFilesystemAnnotation : public UAttributeAnnotation
{
	GENERATED_BODY()
	
public:
	EFilesystemMode Mode;
    TArray<FString> Extensions;
};

UCLASS()
class VITRUVIO_API URangeAnnotation : public UAttributeAnnotation
{
	GENERATED_BODY()
	
public:
	float Min;
	float Max;
	bool Restricted;
};

UCLASS()
class VITRUVIO_API UEnumAnnotation : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	TArray<bool> BoolValues;
	TArray<FString> StringValues;
	TArray<float> FloatValues;
	bool Restricted;
};

UCLASS()
class VITRUVIO_API UAttributeMetadata : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UAttributeAnnotation* Annotation;
	
	FString Description;
	FAttributeGroups Groups;
	int Order;
	int GroupOrder;

	bool Hidden;
};

UCLASS()
class VITRUVIO_API URuleAttribute : public UObject
{
	GENERATED_BODY()

public:
	FString Name;

	UPROPERTY()
	UAttributeMetadata* Metadata;
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