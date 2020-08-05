// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"

#include "RuleAttributes.generated.h"

using FAttributeGroups = TArray<FString>;

enum class FilesystemMode
{
	File,
	Directory,
	None
};

enum class AnnotationType
{
	FileSystem,
	Range,
	Enum,
	Color
};

class AttributeAnnotation
{
public:
	virtual ~AttributeAnnotation() = default;
	virtual AnnotationType GetAnnotationType() = 0;
};

class ColorAnnotation final : public AttributeAnnotation
{
	AnnotationType GetAnnotationType() override { return AnnotationType::Color; }
};

class FilesystemAnnotation final : public AttributeAnnotation
{
public:
	FilesystemMode Mode = FilesystemMode::None;
	FString Extensions;

	AnnotationType GetAnnotationType() override { return AnnotationType::FileSystem; }
};

class RangeAnnotation final : public AttributeAnnotation
{
public:
	TOptional<double> Min;
	TOptional<double> Max;
	double StepSize = 0.1;
	bool Restricted = true;

	AnnotationType GetAnnotationType() override { return AnnotationType::Range; }
};

template <typename T>
class EnumAnnotation final : public AttributeAnnotation
{
public:
	TArray<T> Values;
	bool Restricted = true;

	AnnotationType GetAnnotationType() override { return AnnotationType::Enum; }
};

UCLASS(Abstract)
class VITRUVIO_API URuleAttribute : public UObject
{
	GENERATED_BODY()

protected:
	TSharedPtr<AttributeAnnotation> Annotation;

public:
	FString Name;
	FString DisplayName;

	FString Description;
	FAttributeGroups Groups;
	int Order;
	int GroupOrder;

	bool Hidden;

	void SetAnnotation(TSharedPtr<AttributeAnnotation> InAnnotation) { this->Annotation = MoveTemp(InAnnotation); }

	virtual void CopyValue(const URuleAttribute* FromAttribute){};
};

UCLASS()
class VITRUVIO_API UStringAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString Value;

	TSharedPtr<EnumAnnotation<FString>> GetEnumAnnotation() const
	{
		return Annotation && Annotation->GetAnnotationType() == AnnotationType::Enum ? StaticCastSharedPtr<EnumAnnotation<FString>>(Annotation)
																					 : TSharedPtr<EnumAnnotation<FString>>();
	}

	TSharedPtr<ColorAnnotation> GetColorAnnotation() const
	{
		return Annotation && Annotation->GetAnnotationType() == AnnotationType::Color ? StaticCastSharedPtr<ColorAnnotation>(Annotation)
																					  : TSharedPtr<ColorAnnotation>();
	}

	void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UStringAttribute* FromStringAttribute = Cast<UStringAttribute>(FromAttribute);
		if (FromStringAttribute)
		{
			Value = FromStringAttribute->Value;
		};
	};
};

UCLASS()
class VITRUVIO_API UFloatAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY()
	double Value;

	TSharedPtr<EnumAnnotation<double>> GetEnumAnnotation() const
	{
		return Annotation && Annotation->GetAnnotationType() == AnnotationType::Enum ? StaticCastSharedPtr<EnumAnnotation<double>>(Annotation)
																					 : TSharedPtr<EnumAnnotation<double>>();
	}

	TSharedPtr<RangeAnnotation> GetRangeAnnotation() const
	{
		return Annotation && Annotation->GetAnnotationType() == AnnotationType::Range ? StaticCastSharedPtr<RangeAnnotation>(Annotation)
																					  : TSharedPtr<RangeAnnotation>();
	}

	void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UFloatAttribute* FromFloatAttribute = Cast<UFloatAttribute>(FromAttribute);
		if (FromFloatAttribute)
		{
			Value = FromFloatAttribute->Value;
		};
	};
};

UCLASS()
class VITRUVIO_API UBoolAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool Value;

	void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UBoolAttribute* FromBoolAttribute = Cast<UBoolAttribute>(FromAttribute);
		if (FromBoolAttribute)
		{
			Value = FromBoolAttribute->Value;
		};
	};
};
