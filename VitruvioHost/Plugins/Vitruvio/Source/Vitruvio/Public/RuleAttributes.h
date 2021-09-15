/* Copyright 2021 Esri
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
	bool HasMin = false;
	UPROPERTY()
	bool HasMax = false;
	UPROPERTY()
	double Min = 0;
	UPROPERTY()
	double Max = 0;
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
class VITRUVIO_API UStringArrayAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FString> Values;

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
		const UStringArrayAttribute* FromStringAttribute = Cast<UStringArrayAttribute>(FromAttribute);
		if (FromStringAttribute)
		{
			Values = FromStringAttribute->Values;
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
class VITRUVIO_API UFloatArrayAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<double> Values;

	UFloatEnumAnnotation* GetEnumAnnotation() const
	{
		return Cast<UFloatEnumAnnotation>(Annotation);
	}

	UColorAnnotation* GetColorAnnotation() const
	{
		return Cast<UColorAnnotation>(Annotation);
	}

	void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UFloatArrayAttribute* FromFloatArrayAttribute = Cast<UFloatArrayAttribute>(FromAttribute);
		if (FromFloatArrayAttribute)
		{
			Values = FromFloatArrayAttribute->Values;
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

UCLASS()
class VITRUVIO_API UBoolArrayAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<bool> Values;

	UColorAnnotation* GetColorAnnotation() const
	{
		return Cast<UColorAnnotation>(Annotation);
	}

	void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UBoolArrayAttribute* FromBoolArrayAttribute = Cast<UBoolArrayAttribute>(FromAttribute);
		if (FromBoolArrayAttribute)
		{
			Values = FromBoolArrayAttribute->Values;
		}
	}
};
