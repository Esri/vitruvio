// Copyright © 2017-2022 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VitruvioTypes.h"
#include "UObject/Object.h"
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Engine/DataAsset.h"
#include "Materials/MaterialInterface.h"

#include "MaterialReplacement.generated.h"

UCLASS()
class UMaterialDescription : public UObject
{
	GENERATED_BODY()

public:
	TMap<FString, FString> TextureProperties;
	TMap<FString, FLinearColor> ColorProperties;
	TMap<FString, double> ScalarProperties;
	TMap<FString, FString> StringProperties;

	FString BlendMode;
	FString Name; // ignored on purpose for hash and equality

	friend bool operator==(const UMaterialDescription& Lhs, const UMaterialDescription& RHS)
	{
		// clang-format off
		return Lhs.TextureProperties.OrderIndependentCompareEqual(RHS.TextureProperties) &&
			   Lhs.ColorProperties.OrderIndependentCompareEqual(RHS.ColorProperties) &&
			   Lhs.ScalarProperties.OrderIndependentCompareEqual(RHS.ScalarProperties) &&
			   Lhs.StringProperties.OrderIndependentCompareEqual(RHS.StringProperties) && 
			   Lhs.BlendMode == RHS.BlendMode;
		// clang-format on
	}

	friend bool operator!=(const UMaterialDescription& Lhs, const UMaterialDescription& RHS)
	{
		return !(Lhs == RHS);
	}
};

uint32 GetTypeHash(const UMaterialDescription& Object);

inline bool MaterialEquals(UMaterialInterface* Lhs, UMaterialInterface* Rhs);

USTRUCT(BlueprintType)
struct FMaterialReplacementData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UMaterialDescription* Source;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Replacement;
	
	FMaterialReplacementData() : Source(nullptr), Replacement(nullptr)
	{
	}

	bool HasReplacement() const
	{
		return Replacement != nullptr;
	}
	
	friend bool operator==(const FMaterialReplacementData& Lhs, const FMaterialReplacementData& Rhs)
	{
		return Lhs.Source == Rhs.Source
			&& MaterialEquals(Lhs.Replacement, Rhs.Replacement);
	}

	friend bool operator!=(const FMaterialReplacementData& Lhs, const FMaterialReplacementData& RHS)
	{
		return !(Lhs == RHS);
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

