// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/Object.h"

#include "InstanceReplacement.generated.h"

USTRUCT()
struct FReplacementOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere)
	double Frequency = 1;

	UPROPERTY(EditAnywhere)
	bool bRandomScale = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bRandomScale", EditConditionHides))
	FVector MinScale = {1, 1, 1};

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bRandomScale", EditConditionHides))
	FVector MaxScale = {1, 1, 1};

	UPROPERTY(EditAnywhere)
	bool bRandomRotation = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bRandomRotation", EditConditionHides))
	FVector MinRotation = {0, 0, 0};

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bRandomRotation", EditConditionHides))
	FVector MaxRotation = {0, 0, 0};

	friend bool operator==(const FReplacementOption& Lhs, const FReplacementOption& Rhs)
	{
		return Lhs.Mesh == Rhs.Mesh && Lhs.Frequency == Rhs.Frequency && Lhs.bRandomScale == Rhs.bRandomScale && Lhs.MinScale == Rhs.MinScale &&
			   Lhs.MaxScale == Rhs.MaxScale && Lhs.bRandomRotation == Rhs.bRandomRotation && Lhs.MaxRotation == Rhs.MaxRotation;
	}

	friend bool operator!=(const FReplacementOption& Lhs, const FReplacementOption& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

USTRUCT(BlueprintType)
struct FInstanceReplacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString SourceMeshIdentifier;

	UPROPERTY(EditAnywhere)
	TArray<FReplacementOption> Replacements;

	FInstanceReplacement() {}

	bool HasReplacement() const
	{
		return Algo::AnyOf(Replacements, [](const FReplacementOption& Replacement) { return Replacement.Mesh != nullptr; });
	}

	friend bool operator==(const FInstanceReplacement& Lhs, const FInstanceReplacement& Rhs)
	{
		return Lhs.SourceMeshIdentifier == Rhs.SourceMeshIdentifier && Lhs.Replacements == Rhs.Replacements;
	}

	friend bool operator!=(const FInstanceReplacement& Lhs, const FInstanceReplacement& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

UCLASS(BlueprintType)
class VITRUVIO_API UInstanceReplacementAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FInstanceReplacement> Replacements;

	bool IsValid() const
	{
		return Algo::AllOf(Replacements, [](const FInstanceReplacement& ReplacementData) { return ReplacementData.HasReplacement(); });
	}
};
