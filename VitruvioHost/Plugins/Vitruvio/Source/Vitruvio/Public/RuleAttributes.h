/* Copyright 2024 Esri
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

UENUM(BlueprintType)
enum class EFilesystemMode : uint8
{
	File,
	Directory,
	None
};

UCLASS(BlueprintType)
class UAttributeAnnotation : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UAttributeAnnotation() = default;
};

UCLASS(BlueprintType)
class UColorAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()
};

UCLASS(BlueprintType)
class UFilesystemAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	EFilesystemMode Mode = EFilesystemMode::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	FString Extensions;
};

UCLASS(BlueprintType)
class URangeAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	bool HasMin = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	bool HasMax = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	double Min = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	double Max = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	double StepSize = 0.1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	bool Restricted = true;
};

UCLASS(BlueprintType)
class UStringEnumAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	TArray<FString> Values;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	bool Restricted = true;
};

UCLASS(BlueprintType)
class UFloatEnumAnnotation final : public UAttributeAnnotation
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	TArray<double> Values;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	bool Restricted = true;
};

UCLASS(Abstract)
class VITRUVIO_API URuleAttribute : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	UAttributeAnnotation* Annotation;
	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		SetFlags(RF_Transactional);
	}

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	FString Name;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	FString DisplayName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	FString ImportPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	FString Description;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	TArray<FString> Groups;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	int Order = INT32_MAX;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	int GroupOrder = INT32_MAX;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	int ImportOrder = INT32_MAX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	bool bHidden;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitruvio")
	bool bUserSet;

	void SetAnnotation(UAttributeAnnotation* InAnnotation)
	{
		Annotation = InAnnotation;
	}

	virtual void CopyValue(const URuleAttribute* FromAttribute) {}

	/**
	 * Returns value of this Attribute as a String.
	 * 
	 * Note to get the actual value you need to cast this object to its correct type (eg. UFloatAttribute).
	 * @return the value of this Attribute as a String.
	 */
	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	virtual FString GetValueAsString()
	{
		return TEXT("");
	}

	/**
	 * Returns the annotation associated with this attribute. Needs to be casted to its actual type to access relevant
	 * fields (eg. to UStringEnumAnnotation). More specific sub types of URuleAttribute contain helper methods to access appropriate
	 * annotations (eg. UFloatAttribute#GetRangeAnnotation).
	 * 
	 * @return the annotation associated with this attribute.
	 */
	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	virtual UAttributeAnnotation* GetAnnotation()
	{
		return Annotation;
	}
};

UCLASS(Abstract)
class VITRUVIO_API UArrayAttribute : public URuleAttribute
{
	GENERATED_BODY()

public:
	virtual void InitializeDefaultArrayValue(int32 Index) {}
};

UCLASS(BlueprintType)
class VITRUVIO_API UStringAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	FString Value;

	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	UStringEnumAnnotation* GetEnumAnnotation() const
	{
		return Cast<UStringEnumAnnotation>(Annotation);
	}

	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	UColorAnnotation* GetColorAnnotation() const
	{
		return Cast<UColorAnnotation>(Annotation);
	}

	virtual void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UStringAttribute* FromStringAttribute = Cast<UStringAttribute>(FromAttribute);
		if (FromStringAttribute)
		{
			Value = FromStringAttribute->Value;
		}
	}

	virtual FString GetValueAsString() override
	{
		return Value;
	}

	/**
	 * Returns the value of this Attribute.
	 * 
	 * Note to set the value, you need to use UVitruvioComponent#SetStringAttribute.
	 * @return the value of this Attribute.
	 */
	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	const FString& GetValue() const
	{
		return Value;
	}
};

UCLASS(BlueprintType)
class VITRUVIO_API UStringArrayAttribute final : public UArrayAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	TArray<FString> Values;

	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	UStringEnumAnnotation* GetEnumAnnotation() const
	{
		return Cast<UStringEnumAnnotation>(Annotation);
	}

	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	UColorAnnotation* GetColorAnnotation() const
	{
		return Cast<UColorAnnotation>(Annotation);
	}

	virtual void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UStringArrayAttribute* FromStringAttribute = Cast<UStringArrayAttribute>(FromAttribute);
		if (FromStringAttribute)
		{
			Values = FromStringAttribute->Values;
		}
	}

	virtual void InitializeDefaultArrayValue(int32 Index) override
	{
		if (Index == INDEX_NONE || Index >= Values.Num())
		{
			return;
		}

		if (UStringEnumAnnotation* EnumAnnotation = GetEnumAnnotation())
		{
			if (EnumAnnotation->Values.Num() > 0)
			{
				Values[Index] = EnumAnnotation->Values[0];
			}
		}
		else if (GetColorAnnotation())
		{
			Values[Index] = TEXT("#") + FLinearColor::White.ToFColor(true).ToHex();
		}
	}

	virtual FString GetValueAsString() override
	{
		return FString::Join(Values, TEXT(", "));
	}

	/**
	 * Returns the value of this Attribute. 
	 * 
	 * Note to set the value, you need to use UVitruvioComponent#SetStringArrayAttribute.
	 * @return the value of this Attribute.
	 */
	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	const TArray<FString>& GetValue() const
	{
		return Values;
	}
};

UCLASS(BlueprintType)
class VITRUVIO_API UFloatAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	double Value;

	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	UFloatEnumAnnotation* GetEnumAnnotation() const
	{
		return Cast<UFloatEnumAnnotation>(Annotation);
	}

	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	URangeAnnotation* GetRangeAnnotation() const
	{
		return Cast<URangeAnnotation>(Annotation);
	}

	virtual void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UFloatAttribute* FromFloatAttribute = Cast<UFloatAttribute>(FromAttribute);
		if (FromFloatAttribute)
		{
			Value = FromFloatAttribute->Value;
		}
	}

	virtual FString GetValueAsString() override
	{
		return FString::SanitizeFloat(Value);
	}

	/**
	 * Returns the value of this Attribute. 
	 * 
	 * Note to set the value, you need to use UVitruvioComponent#SetFloatAttribute.
	 * @return the value of this Attribute.
	 */
	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	double GetValue() const
	{
		return Value;
	}
};

UCLASS(BlueprintType)
class VITRUVIO_API UFloatArrayAttribute final : public UArrayAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	TArray<double> Values;

	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	UFloatEnumAnnotation* GetEnumAnnotation() const
	{
		return Cast<UFloatEnumAnnotation>(Annotation);
	}

	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	URangeAnnotation* GetRangeAnnotation() const
	{
		return Cast<URangeAnnotation>(Annotation);
	}

	virtual void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UFloatArrayAttribute* FromFloatArrayAttribute = Cast<UFloatArrayAttribute>(FromAttribute);
		if (FromFloatArrayAttribute)
		{
			Values = FromFloatArrayAttribute->Values;
		}
	}

	virtual void InitializeDefaultArrayValue(int32 Index) override
	{
		if (Index == INDEX_NONE || Index >= Values.Num())
		{
			return;
		}

		if (UFloatEnumAnnotation* FloatEnumAnnotation = GetEnumAnnotation())
		{
			if (FloatEnumAnnotation->Values.Num() > 0)
			{
				Values[Index] = FloatEnumAnnotation->Values[0];
			}
		}
		else if (URangeAnnotation* RangeAnnotation = GetRangeAnnotation())
		{
			if (RangeAnnotation->HasMin)
			{
				Values[Index] = RangeAnnotation->Min;
			}
		}
	}

	virtual FString GetValueAsString() override
	{
		return FString::JoinBy(Values, TEXT(", "), [](double Item) { return FString::SanitizeFloat(Item); });
	}

	/**
	 * Returns the value of this Attribute. 
	 * 
	 * Note to set the value, you need to use UVitruvioComponent#SetFloatArrayAttribute.
	 * @return the value of this Attribute.
	 */
	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	const TArray<double>& GetValue() const
	{
		return Values;
	}
};

UCLASS(BlueprintType)
class VITRUVIO_API UBoolAttribute final : public URuleAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	bool Value;

	virtual void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UBoolAttribute* FromBoolAttribute = Cast<UBoolAttribute>(FromAttribute);
		if (FromBoolAttribute)
		{
			Value = FromBoolAttribute->Value;
		}
	}

	virtual FString GetValueAsString() override
	{
		return Value ? TEXT("True") : TEXT("False");
	}

	/**
	 * Returns the value of this Attribute. 
	 * 
	 * Note to set the value, you need to use UVitruvioComponent#SetBoolAttribute.
	 * @return the value of this Attribute.
	 */
	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	bool GetValue() const
	{
		return Value;
	}
};

UCLASS(BlueprintType)
class VITRUVIO_API UBoolArrayAttribute final : public UArrayAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	TArray<bool> Values;

	virtual void CopyValue(const URuleAttribute* FromAttribute) override
	{
		const UBoolArrayAttribute* FromBoolArrayAttribute = Cast<UBoolArrayAttribute>(FromAttribute);
		if (FromBoolArrayAttribute)
		{
			Values = FromBoolArrayAttribute->Values;
		}
	}

	virtual FString GetValueAsString() override
	{
		return FString::JoinBy(Values, TEXT(", "), [](bool Item) { return Item ? TEXT("True") : TEXT("False"); });
	}

	/**
	 * Returns the value of this Attribute. 
	 * 
	 * Note to set the value, you need to use UVitruvioComponent#SetBoolArrayAttribute.
	 * @return the value of this Attribute.
	 */
	UFUNCTION(BlueprintPure, Category = "Vitruvio")
	const TArray<bool>& GetValue() const
	{
		return Values;
	}
};
