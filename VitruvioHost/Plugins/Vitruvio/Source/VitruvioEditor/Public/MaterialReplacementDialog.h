// Copyright © 2017-2022 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MaterialReplacement.h"

#include "MaterialReplacementDialog.generated.h"

UCLASS(BlueprintType)
class UMaterialReplacement : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<UStaticMeshComponent*> Components;
	
	UPROPERTY()
	FName SourceMaterialSlot;
	
	UPROPERTY()
	UMaterialInterface* ReplacementMaterial;
};

USTRUCT()
struct FMaterialKey
{
	GENERATED_BODY()

	UPROPERTY()
	UMaterialInterface* Material;

	UPROPERTY()
	FName SourceMaterialSlot;

	friend bool operator==(const FMaterialKey& Lhs, const FMaterialKey& RHS)
	{
		return Lhs.Material == RHS.Material && Lhs.SourceMaterialSlot == RHS.SourceMaterialSlot;
	}

	friend bool operator!=(const FMaterialKey& Lhs, const FMaterialKey& RHS)
	{
		return !(Lhs == RHS);
	}

	friend uint32 GetTypeHash(const FMaterialKey& Object)
	{
		return HashCombine(GetTypeHash(Object.SourceMaterialSlot), GetTypeHash(Object.Material));
	}
};

UCLASS(meta = (DisplayName = "Options"))
class UMaterialReplacementDialogOptions : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UMaterialReplacementAsset* TargetReplacementAsset;

	UPROPERTY(EditAnywhere)
	TMap<FMaterialKey, UMaterialReplacement*> MaterialReplacements;
};

class FMaterialReplacementDialog
{
public:
	static void OpenDialog(class UVitruvioComponent* VitruvioComponent);
};
