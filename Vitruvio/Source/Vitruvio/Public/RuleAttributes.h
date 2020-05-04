#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"
#include "RuleAttributes.generated.h"

using FAttributeGroups = TArray<FString>;

enum FilesystemMode { File, Directory, None };

class AttributeAnnotation
{
public:
	virtual ~AttributeAnnotation() = default;
};

class FilesystemAnnotation : public AttributeAnnotation
{
public:
	FilesystemMode Mode = None;
    FString Extensions;
};

class RangeAnnotation : public AttributeAnnotation
{
public:
	TOptional<double> Min;
	TOptional<double> Max;
	double StepSize = 0.1;
	bool Restricted = true;
};

template <typename T>
class EnumAnnotation : public AttributeAnnotation
{
public:
	TArray<T> Values;
	bool Restricted = true;
};

UCLASS()
class VITRUVIO_API URuleAttribute : public UObject
{
	GENERATED_BODY()
	
protected:
	std::shared_ptr<AttributeAnnotation> Annotation;
	
public:
	FString Name;
	FString DisplayName;

	FString Description;
	FAttributeGroups Groups;
	int Order;
	int GroupOrder;

	bool Hidden;

	void SetAnnotation(std::shared_ptr<AttributeAnnotation> InAnnotation)
	{
		this->Annotation = std::move(InAnnotation);
	}
};

UCLASS()
class VITRUVIO_API UStringAttribute : public URuleAttribute
{
	GENERATED_BODY()

public:
	FString Value;

	std::shared_ptr<EnumAnnotation<FString>> GetEnumAnnotation() const
	{
		return std::dynamic_pointer_cast<EnumAnnotation<FString>>(Annotation);
	}
};

UCLASS()
class VITRUVIO_API UFloatAttribute : public URuleAttribute
{
	GENERATED_BODY()

public:
	double Value;

	std::shared_ptr<EnumAnnotation<double>> GetEnumAnnotation() const
	{
		return std::dynamic_pointer_cast<EnumAnnotation<double>>(Annotation);
	}

	std::shared_ptr<RangeAnnotation> GetRangeAnnotation() const
	{
		return std::dynamic_pointer_cast<RangeAnnotation>(Annotation);
	}
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