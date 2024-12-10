// Copyright © 2017-2022 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Algo/AllOf.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Object.h"

#include "MaterialReplacement.generated.h"

USTRUCT(BlueprintType)
struct FMaterialReplacementData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	FString MaterialIdentifier;

	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	UMaterialInterface* ReplacementMaterial;

	FMaterialReplacementData() : ReplacementMaterial(nullptr) {}

	bool HasReplacement() const
	{
		return ReplacementMaterial != nullptr;
	}

	friend bool operator==(const FMaterialReplacementData& Lhs, const FMaterialReplacementData& Rhs)
	{
		return Lhs.MaterialIdentifier == Rhs.MaterialIdentifier && Lhs.ReplacementMaterial == Rhs.ReplacementMaterial;
	}

	friend bool operator!=(const FMaterialReplacementData& Lhs, const FMaterialReplacementData& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

UCLASS(BlueprintType)
class CITYENGINEPLUGIN_API UMaterialReplacementAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	TArray<FMaterialReplacementData> Replacements;

	bool IsValid() const
	{
		return Algo::AllOf(Replacements, [](const FMaterialReplacementData& ReplacementData) { return ReplacementData.HasReplacement(); });
	}
};
