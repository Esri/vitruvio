// Copyright © 2017-2022 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Algo/AllOf.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Object.h"
#include "Materials/MaterialInstance.h"

#include "MaterialReplacement.generated.h"

USTRUCT(BlueprintType)
struct VITRUVIO_API FMaterialDescription
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TMap<FString, FString> TextureProperties;
	UPROPERTY(VisibleAnywhere)
	TMap<FString, FLinearColor> ColorProperties;
	UPROPERTY(VisibleAnywhere)
	TMap<FString, double> ScalarProperties;

	EBlendMode BlendMode = BLEND_Opaque;

	FMaterialDescription()
	{
	}

	explicit FMaterialDescription(UMaterialInstance* MaterialInterface)
	{
		for (const auto& TextureParameterValue : MaterialInterface->TextureParameterValues)
		{
			const FString Value = TextureParameterValue.ParameterValue ? TextureParameterValue.ParameterValue->GetName() : FString {};
			TextureProperties.Add(TextureParameterValue.ParameterInfo.Name.ToString(), Value);
		}
		for (const auto& VectorParameterValue : MaterialInterface->VectorParameterValues)
		{
			ColorProperties.Add(VectorParameterValue.ParameterInfo.Name.ToString(), VectorParameterValue.ParameterValue);
		}
		for (const auto& ScalarParameterValue : MaterialInterface->ScalarParameterValues)
		{
			ScalarProperties.Add(ScalarParameterValue.ParameterInfo.Name.ToString(), ScalarParameterValue.ParameterValue);
		}

		BlendMode = MaterialInterface->GetBlendMode();
	}

	friend bool operator==(const FMaterialDescription& Lhs, const FMaterialDescription& Rhs)
	{
		// clang-format off
		return Lhs.TextureProperties.OrderIndependentCompareEqual(Rhs.TextureProperties) &&
			   Lhs.ColorProperties.OrderIndependentCompareEqual(Rhs.ColorProperties) &&
			   Lhs.ScalarProperties.OrderIndependentCompareEqual(Rhs.ScalarProperties) &&
			   Lhs.BlendMode == Rhs.BlendMode;
		// clang-format on
	}

	friend bool operator!=(const FMaterialDescription& Lhs, const FMaterialDescription& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

uint32 GetTypeHash(const FMaterialDescription& Object);

USTRUCT(BlueprintType)
struct FMaterialReplacementData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName SourceMaterialSlotName;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* ReplacementMaterial;

	FMaterialReplacementData()
		: ReplacementMaterial(nullptr)
	{
	}

	bool HasReplacement() const
	{
		return ReplacementMaterial != nullptr;
	}

	friend bool operator==(const FMaterialReplacementData& Lhs, const FMaterialReplacementData& Rhs)
	{
		return Lhs.SourceMaterialSlotName == Rhs.SourceMaterialSlotName && Lhs.ReplacementMaterial == Rhs.ReplacementMaterial;
	}

	friend bool operator!=(const FMaterialReplacementData& Lhs, const FMaterialReplacementData& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

UCLASS(BlueprintType)
class VITRUVIO_API UMaterialReplacementAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FMaterialReplacementData> Replacements;

	bool IsValid() const
	{
		return Algo::AllOf(Replacements, [](const FMaterialReplacementData& ReplacementData) { return ReplacementData.HasReplacement(); });
	}
};
