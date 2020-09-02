// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"

#include "RuleAttributes.generated.h"

UENUM()
enum class EFilesystemMode
{
	File,
	Directory,
	None
};

UCLASS()
class UAttributeAnnotation : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UAttributeAnnotation() = default;
};

UCLASS()
class UColorAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()
};

UCLASS()
class UFilesystemAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EFilesystemMode Mode = EFilesystemMode::None;
	UPROPERTY()
	FString Extensions;
};

UCLASS()
class URangeAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	double Min = NAN;
	UPROPERTY()
	double Max = NAN;
	UPROPERTY()
	double StepSize = 0.1;
	UPROPERTY()
	bool Restricted = true;
};

UCLASS()
class UStringEnumAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FString> Values;
	UPROPERTY()
	bool Restricted = true;
};

UCLASS()
class UFloatEnumAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<double> Values;
	UPROPERTY()
	bool Restricted = true;
};

UCLASS(Abstract)
class VITRUVIO_API URuleAttribute : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	UAttributeAnnotation* Annotation;

public:
	UPROPERTY()
	FString Name;
	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	FString Description;
	UPROPERTY()
	TArray<FString> Groups;
	UPROPERTY()
	int Order;
	UPROPERTY()
	int GroupOrder;

	UPROPERTY()
	bool Hidden;

	void SetAnnotation(UAttributeAnnotation* InAnnotation)
	{
		Annotation = InAnnotation;
	}

	virtual void CopyValue(const URuleAttribute* FromAttribute){};
};

UCLASS()
class VITRUVIO_API UStringAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString Value;

	UStringEnumAnnotation* GetEnumAnnotation() const
	{
		return Cast<UStringEnumAnnotation>(Annotation);
	}

	UColorAnnotation* GetColorAnnotation() const
	{
		return Cast<UColorAnnotation>(Annotation);
	}

	void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UStringAttribute* FromStringAttribute = Cast<UStringAttribute>(FromAttribute);
		if (FromStringAttribute)
		{
			Value = FromStringAttribute->Value;
		}
	}
};

UCLASS()
class VITRUVIO_API UFloatAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY()
	double Value;

	UFloatEnumAnnotation* GetEnumAnnotation() const
	{
		return Cast<UFloatEnumAnnotation>(Annotation);
	}

	URangeAnnotation* GetRangeAnnotation() const
	{
		return Cast<URangeAnnotation>(Annotation);
	}

	void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UFloatAttribute* FromFloatAttribute = Cast<UFloatAttribute>(FromAttribute);
		if (FromFloatAttribute)
		{
			Value = FromFloatAttribute->Value;
		}
	}
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
		}
	}
};
