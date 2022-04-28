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
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "VitruvioReplacements.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UReplacementInterface : public UInterface
{
	GENERATED_BODY()
};

class IReplacementInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Vitruvio")
	void OnConstructed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Vitruvio")
	void OnInstancesAdded();
};

UENUM()
enum class FilterType
{
	Any,
	All,
};

UENUM()
enum class FilterOperator
{
	StartsWith,
	Equals,
	Contains
};

UENUM()
enum class ReplacementType
{
	HierarchicalInstances,
	Actor,
};

USTRUCT()
struct FReplacementOption
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ReplacementActor;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UHierarchicalInstancedStaticMeshComponent> ReplacementInstances;

	UPROPERTY(EditAnywhere)
	float Probability = 1.0f;

	UPROPERTY(EditAnywhere)
	FString ReplacementName;

	friend bool operator==(const FReplacementOption& Lhs, const FReplacementOption& Rhs)
	{
		return Lhs.ReplacementActor == Rhs.ReplacementActor && Lhs.ReplacementInstances == Rhs.ReplacementInstances &&
			   Lhs.Probability == Rhs.Probability && Lhs.ReplacementName == Rhs.ReplacementName;
	}

	friend bool operator!=(const FReplacementOption& Lhs, const FReplacementOption& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

USTRUCT()
struct FReplacementFilter
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FilterOperator Operator;

	UPROPERTY(EditAnywhere)
	FString Value;

	bool Matches(const FString& Input) const
	{
		switch (Operator)
		{
		case FilterOperator::StartsWith:
			return Input.StartsWith(Value);
		case FilterOperator::Equals:
			return Input.Equals(Value);
		case FilterOperator::Contains:
			return Input.Contains(Value);
		default:
			return false;
		}
	}

	FString ToString() const
	{
		FString Result;

		switch (Operator)
		{
		case FilterOperator::StartsWith:
			Result = "Starts With ";
			break;
		case FilterOperator::Equals:
			Result = "Equals ";
			break;
		case FilterOperator::Contains:
			Result = "Contains ";
			break;
		default:
			return "";
		}

		Result += Value;
		return Result;
	}

	friend bool operator==(const FReplacementFilter& Lhs, const FReplacementFilter& Rhs)
	{
		return Lhs.Operator == Rhs.Operator && Lhs.Value == Rhs.Value;
	}

	friend bool operator!=(const FReplacementFilter& Lhs, const FReplacementFilter& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

USTRUCT()
struct FReplacementFilters
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FilterType Type;

	UPROPERTY(EditAnywhere)
	TArray<FReplacementFilter> Filters;

	bool Matches(const FString& Input) const
	{
		switch (Type)
		{
		case FilterType::Any:
			return Algo::AnyOf(Filters, [Input](const auto& Filter) { return Filter.Matches(Input); });
		case FilterType::All:
			return Algo::AllOf(Filters, [Input](const auto& Filter) { return Filter.Matches(Input); });
		default:
			return false;
		}
	}

	friend bool operator==(const FReplacementFilters& Lhs, const FReplacementFilters& Rhs)
	{
		return Lhs.Type == Rhs.Type && Lhs.Filters == Rhs.Filters;
	}

	friend bool operator!=(const FReplacementFilters& Lhs, const FReplacementFilters& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

USTRUCT()
struct FReplacement
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FReplacementFilters Filters;

	UPROPERTY(EditAnywhere)
	ReplacementType ReplacementType;

	UPROPERTY(EditAnywhere)
	TArray<FReplacementOption> ReplacementOptions;

	bool IsValid() const
	{
		if (Filters.Filters.IsEmpty())
		{
			return false;
		}

		for (const FReplacementFilter& Filter : Filters.Filters)
		{
			if (Filter.Value.IsEmpty())
			{
				return false;
			}
		}

		if (ReplacementOptions.IsEmpty())
		{
			return false;
		}

		for (const FReplacementOption& Option : ReplacementOptions)
		{
			if (Option.ReplacementActor.Get() == nullptr && Option.ReplacementInstances.Get() == nullptr)
			{
				return false;
			}

			if (ReplacementType == ReplacementType::Actor &&
				(Option.ReplacementInstances.Get() != nullptr || Option.ReplacementActor.Get() == nullptr))
			{
				return false;
			}

			if (ReplacementType == ReplacementType::HierarchicalInstances &&
				(Option.ReplacementInstances.Get() == nullptr || Option.ReplacementActor.Get() != nullptr))
			{
				return false;
			}
		}

		return true;
	}

	bool Matches(const FString& Input) const
	{
		return Filters.Matches(Input);
	}

	friend bool operator==(const FReplacement& Lhs, const FReplacement& Rhs)
	{
		return Lhs.Filters == Rhs.Filters && Lhs.ReplacementType == Rhs.ReplacementType && Lhs.ReplacementOptions == Rhs.ReplacementOptions;
	}

	friend bool operator!=(const FReplacement& Lhs, const FReplacement& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

UCLASS()
class VITRUVIO_API UVitruvioReplacements : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FReplacement> Replacements;
};