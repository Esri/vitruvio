// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Algo/AllOf.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/Object.h"

#include "InstanceReplacement.generated.h"

USTRUCT(BlueprintType)
struct FInstanceReplacementData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString SourceMeshIdentifier;

	UPROPERTY(EditAnywhere)
	UStaticMesh* ReplacementMesh;

	FInstanceReplacementData() : ReplacementMesh(nullptr) {}

	bool HasReplacement() const
	{
		return ReplacementMesh != nullptr;
	}

	friend bool operator==(const FInstanceReplacementData& Lhs, const FInstanceReplacementData& Rhs)
	{
		return Lhs.SourceMeshIdentifier == Rhs.SourceMeshIdentifier && Lhs.ReplacementMesh == Rhs.ReplacementMesh;
	}

	friend bool operator!=(const FInstanceReplacementData& Lhs, const FInstanceReplacementData& Rhs)
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
	TArray<FInstanceReplacementData> Replacements;

	bool IsValid() const
	{
		return Algo::AllOf(Replacements, [](const FInstanceReplacementData& ReplacementData) { return ReplacementData.HasReplacement(); });
	}
};
